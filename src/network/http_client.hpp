// network/http_client.hpp

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include "network/tls.hpp"
#include "utils/log.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <future>
#include <chrono>

class HTTPClient {
public:
    HTTPClient(const std::string& uri,
               const std::string& auth_id,
               const std::string& auth_pwd,
               const std::string& table,
               const std::string& token);
    ~HTTPClient();

    // Initialize and connect to the server
    bool connect();

    // Check if the client is connected to the server
    bool isConnected() const { return m_connected; }

    // Send a JSON message
    bool sendJSON(const std::string& json_message);

    // Disconnect from the server
    void disconnect();

private:
    std::string m_uri;
    std::string m_auth_id;
    std::string m_auth_pwd;
    std::string m_table;
    std::string m_token;

    // Networking components
    TLSConnection m_tls_conn;
    bool m_connected;

    // Threading for asynchronous sending
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_cond_var;
    std::thread m_send_thread;
    bool m_stop_thread;

    // Helper methods
    std::string constructURI();
    std::string constructAuthHeader();
    void sendLoop();
};

#endif // HTTP_CLIENT_HPP
