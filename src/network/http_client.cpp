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
    // Start the send thread immediately
    m_send_thread = std::thread(&HTTPClient::sendLoop, this);
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
    if (m_connected) {
        Log::debug("HTTPClient", "Already connected, skipping connect()");
        return true;
    }

    Log::info("HTTPClient", "Starting connection to URI: %s", m_uri.c_str());
    
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
            Log::warn("HTTPClient", "Failed to connect to %s:%d", host.c_str(), port);
            m_connected = false;
            return false;
        }

        m_connected = true;
        Log::info("HTTPClient", "Successfully connected to %s:%d - m_connected now true", host.c_str(), port);
        return true;

    } catch (const std::exception& e) {
        Log::warn("HTTPClient", "Exception during connect: %s", e.what());
        m_connected = false;
        return false;
    }
}

bool HTTPClient::sendJSON(const std::string& json_message)
{
    Log::info("HTTPClient", "sendJSON called with message length: %d, connected: %d", 
              (int)json_message.length(), m_connected ? 1 : 0);

    // Enqueue the message
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_message_queue.push(json_message);
        Log::info("HTTPClient", "Message queued. Queue size now: %d", (int)m_message_queue.size());
    }
    m_cond_var.notify_one();
    return true;
}

void HTTPClient::disconnect()
{
    if (m_connected) {
        Log::info("HTTPClient", "Disconnecting from %s - setting m_connected to false", m_uri.c_str());
        m_tls_conn.disconnect();
        m_connected = false;
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

    Log::info("HTTPClient", "Send loop starting");

    while (true) {
        Log::info("HTTPClient", "Send loop iteration starting");

        std::vector<std::string> messages_to_send;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            Log::info("HTTPClient", "Waiting for messages. Queue size: %d, Stop thread: %d", 
                (int)m_message_queue.size(), m_stop_thread ? 1 : 0);

            m_cond_var.wait(lock, [this] { return !m_message_queue.empty() || m_stop_thread; });

            Log::info("HTTPClient", "Wait completed. Queue size: %d, Stop thread: %d",
                (int)m_message_queue.size(), m_stop_thread ? 1 : 0);

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
            // Check connection state before sending
            if (!m_connected) {
                Log::info("HTTPClient", "Connection needed - waiting 1 second before retry");
                StkTime::sleep(1);  // Add a delay before reconnect
                if (!connect()) {
                    Log::warn("HTTPClient", "Connection attempt failed - will try again later");
                    break;  // Exit the message loop and try again next iteration
                }
            }

            // Construct HTTP POST request
            std::ostringstream post_stream;
            // Extract path and query from URI
            std::string protocol, host, path;
            size_t pos = m_uri.find("://");
            if (pos == std::string::npos) {
                Log::error("HTTPClient", "Invalid URI: %s", m_uri.c_str());
                continue;
            }

            // Log the URI and message
            Log::info("HTTPClient", "Preparing to send to URI: %s", m_uri.c_str());
            Log::info("HTTPClient", "Message payload: %s", message.c_str());

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

            // Log the full request (be careful with auth headers in production!)
            Log::info("HTTPClient", "Sending HTTP request:\n%s", post_request.c_str());


            try {
                // Send the POST request with a short timeout
                if (!m_tls_conn.sendData(post_request)) {
                    Log::warn("HTTPClient", "Failed to send analytics data");
                    continue;
                }

                // Read response with a short timeout
                std::string response;
                if (!m_tls_conn.receiveData(response, 4096)) {
                    Log::info("HTTPClient", "No response received - marking as disconnected");
                    m_connected = false;
                    // Don't try to reconnect immediately - wait for next send
                    continue;
                } else {
                    Log::info("HTTPClient", "Received response:\n%s", response.c_str());

                    // If server sent Connection: close, just mark as disconnected
                    if (response.find("Connection: close") != std::string::npos) {
                        Log::info("HTTPClient", "Server requested connection close - marking as disconnected");
                        m_connected = false;
                        m_tls_conn.disconnect();  // Ensure we clean up the connection
                    }
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