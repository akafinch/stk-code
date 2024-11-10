#include "network/server_analytics.hpp"
#include "network/http_client.hpp"
#include "network/server_config.hpp"


ServerAnalytics::ServerAnalytics(const std::string& endpoint_uri,
                               const std::string& auth_id,
                               const std::string& auth_pwd)
    : m_http_client(new HTTPClient(endpoint_uri, 
                                 auth_id, 
                                 auth_pwd, 
                                 ServerConfig::m_tpk_table.c_str(),  // Add table
                                 ServerConfig::m_tpk_token.c_str())), // Add token
      m_stop_thread(false)
{
    m_send_thread = std::thread(&ServerAnalytics::sendLoop, this);
}

ServerAnalytics::~ServerAnalytics()
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

bool ServerAnalytics::isConnected() const 
{ 
    return m_http_client->isConnected(); 
}

bool ServerAnalytics::connect()
{
    return m_http_client->connect();
}

void ServerAnalytics::disconnect()
{
    m_http_client->disconnect();
}

bool ServerAnalytics::sendAnalytics(const std::string& json_data)
{
if (!m_http_client->isConnected()) {
        Log::warn("ServerAnalytics", "Attempted to send analytics while not connected");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_message_queue.push(json_data);
    }
    m_cond_var.notify_one();
    return true;
}

void ServerAnalytics::sendLoop()
{
    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_cond_var.wait(lock, [this] { 
            return !m_message_queue.empty() || m_stop_thread; 
        });

        if (m_stop_thread && m_message_queue.empty()) {
            break;
        }

        while (!m_message_queue.empty()) {
            std::string message = m_message_queue.front();
            m_message_queue.pop();
            lock.unlock();

            m_http_client->sendJSON(message);

            lock.lock();
        }
    }
}

