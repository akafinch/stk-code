// network/http_client.cpp

#include "network/http_client.hpp"
#include "utils/string_utils.hpp"
#include "utils/base64.hpp"
#include "utils/time.hpp"
#include <sstream>
#include <iostream>
#include <cstring>
// #include <base64.h> // Assuming base64.h is part of existing utils

HTTPClient::HTTPClient(const std::string& uri,
                       const std::string& auth_id,
                       const std::string& auth_pwd,
                       const std::string& table,
                       const std::string& token)
    : m_uri(uri),
      m_auth_id(auth_id),
      m_auth_pwd(auth_pwd),
      m_table(table),
      m_token(token),
      m_connected(false),
      m_stop_thread(false)
{
    // Connection and thread start will be done in connect()
}

HTTPClient::~HTTPClient()
{
    disconnect();

    // Stop the send thread
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_stop_thread = true;
    }
    m_cond_var.notify_all();
    if (m_send_thread.joinable()) {
        m_send_thread.join();
    }
}

bool HTTPClient::connect()
{
    Log::debug("HTTPClient", "Starting connection to URI: %s", m_uri.c_str());
    
    // Parse the URI to extract host and path
    std::string protocol, host, path;
    unsigned short port = 443; // Default HTTPS port

    try {
        // Simple URI parsing
        size_t pos = m_uri.find("://");
        if (pos == std::string::npos) {
            Log::error("HTTPClient", "Invalid URI format: %s", m_uri.c_str());
            return false;
        }
        protocol = m_uri.substr(0, pos);
        size_t host_start = pos + 3;
        size_t path_start = m_uri.find('/', host_start);
        if (path_start == std::string::npos) {
            host = m_uri.substr(host_start);
            path = "/";
        } else {
            host = m_uri.substr(host_start, path_start - host_start);
            path = m_uri.substr(path_start);
        }

        // Check for port in host
        size_t port_pos = host.find(':');
        if (port_pos != std::string::npos) {
            port = static_cast<unsigned short>(std::stoi(host.substr(port_pos + 1)));
            host = host.substr(0, port_pos);
        }

        SocketAddress server_addr(host, port);
        if (server_addr.isUnset()) {
            Log::warn("HTTPClient", "Failed to resolve address for %s - continuing without analytics", host.c_str());
            return false;
        }

        // Try TLS connection but don't block
        if (!m_tls_conn.connect(server_addr)) {
            Log::warn("HTTPClient", "Failed to connect to %s:%d - continuing without analytics", host.c_str(), port);
            return false;
        }

        m_connected = true;
        return true;

    } catch (const std::exception& e) {
        Log::warn("HTTPClient", "Exception during connect: %s - continuing without analytics", e.what());
        return false;
    } catch (...) {
        Log::warn("HTTPClient", "Unknown error during connect - continuing without analytics");
        return false;
    }
}

bool HTTPClient::sendJSON(const std::string& json_message)
{
    if (!m_connected) {
        Log::warn("HTTPClient", "Attempted to send JSON while not connected.");
        return false;
    }

    // Enqueue the message
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_message_queue.push(json_message);
    }
    m_cond_var.notify_one();
    return true;
}

void HTTPClient::disconnect()
{
    if (m_connected) {
        // Send a simple HTTP/1.1 connection close
        std::string close_request = "QUIT\r\n";
        m_tls_conn.sendData(close_request);
        m_tls_conn.disconnect();
        m_connected = false;
        Log::info("HTTPClient", "Disconnected from %s", m_uri.c_str());
    }
}

std::string HTTPClient::constructAuthHeader()
{
    std::string credentials = m_auth_id + ":" + m_auth_pwd;
    // Base64 encode the credentials
    return base64_encode(reinterpret_cast<const unsigned char*>(credentials.c_str()), credentials.length());
}

void HTTPClient::sendLoop()
{
    while (true) {
        std::vector<std::string> messages_to_send;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_cond_var.wait(lock, [this] { return !m_message_queue.empty() || m_stop_thread; });

            if (m_stop_thread && m_message_queue.empty()) {
                break;
            }

            // Batch up to 10 messages at a time
            while (!m_message_queue.empty() && messages_to_send.size() < 10) {
                messages_to_send.push_back(m_message_queue.front());
                m_message_queue.pop();
            }
        } // unlock here

        for (const std::string& message : messages_to_send) {
            // Construct HTTP POST request
            std::ostringstream post_stream;
            // Extract path and query from URI
            std::string protocol, host, path;
            size_t pos = m_uri.find("://");
            if (pos == std::string::npos) {
                Log::error("HTTPClient", "Invalid URI: %s", m_uri.c_str());
                continue;
            }
            protocol = m_uri.substr(0, pos);
            size_t host_start = pos + 3;
            size_t path_start = m_uri.find('/', host_start);
            if (path_start == std::string::npos) {
                host = m_uri.substr(host_start);
                path = "/";
            } else {
                host = m_uri.substr(host_start, path_start - host_start);
                path = m_uri.substr(path_start);
            }

            // Handle query parameters if any
            std::ostringstream full_path;
            full_path << path;
            if (path.find('?') == std::string::npos) {
                full_path << "?table=" << m_table << "&token=" << m_token;
            } else {
                full_path << "&table=" << m_table << "&token=" << m_token;
            }

            post_stream << "POST " << full_path.str() << " HTTP/1.1\r\n";
            post_stream << "Host: " << host << "\r\n";
            post_stream << "Authorization: Basic " << constructAuthHeader() << "\r\n";
            post_stream << "Content-Type: application/json\r\n";
            post_stream << "Content-Length: " << message.length() << "\r\n";
            post_stream << "Accept: application/json\r\n";
            post_stream << "Connection: keep-alive\r\n\r\n";
            post_stream << message;

            std::string post_request = post_stream.str();

            try {
                // Send the POST request with a short timeout
                if (!m_tls_conn.sendData(post_request)) {
                    Log::warn("HTTPClient", "Failed to send analytics data");
                    continue;
                }

                // Read response with a short timeout
                std::string response;
                if (!m_tls_conn.receiveData(response, 4096)) {
                    Log::debug("HTTPClient", "No response received");
                }
            }
            catch (const std::exception& e) {
                Log::warn("HTTPClient", "Error in analytics: %s", e.what());
            }
            catch (...) {
                Log::warn("HTTPClient", "Unknown error in analytics");
            }
        }
        
        // Sleep outside the critical section
        StkTime::sleep(1);
    }
}