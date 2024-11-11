// network/http_client.cpp

#include "network/http_client.hpp"
#include "utils/string_utils.hpp"
#include "utils/base64.hpp"
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
    // Start the send thread
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
    Log::debug("HTTPClient", "Starting connection to URI: %s", m_uri.c_str());
    
    // Parse the URI to extract host and path
    std::string protocol, host, path;
    unsigned short port = 443; // Default HTTPS port

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

    Log::debug("HTTPClient", "Parsed connection details - Host: %s, Port: %d, Path: %s",
               host.c_str(), port, path.c_str());

    SocketAddress server_addr(host, port);
    if (server_addr.isUnset()) {
        Log::error("HTTPClient", "Failed to resolve address for %s", host.c_str());
        return false;
    }
    
    Log::debug("HTTPClient", "Resolved address: %s", server_addr.toString().c_str());

    // Construct and log the complete URL with parameters
    std::string complete_url = protocol + "://" + host + path;
    if (path.find('?') == std::string::npos) {
        complete_url += "?table=" + m_table + "&token=" + m_token;
    } else {
        complete_url += "&table=" + m_table + "&token=" + m_token;
    }
    Log::info("HTTPClient", "Connecting to URL: %s", complete_url.c_str());

    // Check for port in host
    size_t port_pos = host.find(':');
    if (port_pos != std::string::npos) {
        port = static_cast<unsigned short>(std::stoi(host.substr(port_pos + 1)));
        host = host.substr(0, port_pos);
    }

    // Establish TLS connection
    if (!m_tls_conn.connect(server_addr)) {
        Log::error("HTTPClient", "Failed to connect to %s:%d", host.c_str(), port);
        return false;
    }

    // Construct initial HTTP request headers
    std::ostringstream request_stream;
    request_stream << "GET " << path << "?" << "table=" << m_table << "&token=" << m_token << " HTTP/1.1\r\n";
    request_stream << "Host: " << host << "\r\n";
    request_stream << "Authorization: Basic " << constructAuthHeader() << "\r\n";
    request_stream << "Accept: application/json\r\n";
    request_stream << "Content-Type: application/json\r\n";
    request_stream << "Connection: keep-alive\r\n\r\n";

    std::string request = request_stream.str();

    // Send the GET request to initiate the connection
    if (!m_tls_conn.sendData(request)) {
        Log::error("HTTPClient", "Failed to send initial GET request.");
        return false;
    }

    // Simple response check (not robust)
    std::string response;
    if (!m_tls_conn.receiveData(response, 1024)) {
        Log::error("HTTPClient", "Failed to receive response from server.");
        return false;
    }

    if (response.find("200 OK") == std::string::npos) {
        Log::error("HTTPClient", "Server responded with an error: %s", response.c_str());
        return false;
    }

    m_connected = true;
    Log::info("HTTPClient", "Successfully connected to %s", m_uri.c_str());
    return true;
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
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_cond_var.wait(lock, [this] { return !m_message_queue.empty() || m_stop_thread; });

        if (m_stop_thread && m_message_queue.empty()) {
            break;
        }

        while (!m_message_queue.empty()) {
            std::string message = m_message_queue.front();
            m_message_queue.pop();
            lock.unlock();

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

            // Send the POST request
            if (!m_tls_conn.sendData(post_request)) {
                Log::error("HTTPClient", "Failed to send POST request.");
                disconnect();
                break;
            }

            // Optionally, you can handle the response here
            // For simplicity, we'll skip it
            Log::info("HTTPClient", "Sent JSON message: %s", message.c_str());

            lock.lock();
        }
    }
}