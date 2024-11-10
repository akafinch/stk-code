#ifndef SERVER_ANALYTICS_HPP
#define SERVER_ANALYTICS_HPP

#include "utils/string_utils.hpp"
#include "utils/log.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <memory>

class HTTPClient;  // Forward declaration

class ServerAnalytics {
private:
    std::unique_ptr<HTTPClient> m_http_client;
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_cond_var;
    std::thread m_send_thread;
    bool m_stop_thread;
    
    void sendLoop();

public:
    ServerAnalytics(const std::string& endpoint_uri,
                   const std::string& auth_id = "",
                   const std::string& auth_pwd = "");
    ~ServerAnalytics();
    
    bool connect();
    void disconnect();
    bool sendAnalytics(const std::string& json_data);
    bool isConnected() const;  // Declaration only
};

#endif