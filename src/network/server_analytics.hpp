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
class AbstractKart;  // Forward declaration
class World;  // Forward declaration
class Controller;  // Forward declaration

struct AnalyticsEvent {
    std::string player_id;
    std::string match_id;
    uint16_t track;
    uint16_t kart;
    std::string timestamp;
    float loc_x;
    float loc_y;
    float loc_z;
    float face_x;
    float face_y;
    float face_z;
    float speed;
    bool gas;
    bool brake;
    bool nitro;
    bool skid;
    bool back;
    uint16_t event;
    std::string metadata;

    std::string toJson() const;
};

class ServerAnalytics {
private:
    std::unique_ptr<HTTPClient> m_http_client;
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_cond_var;
    std::thread m_send_thread;
    bool m_stop_thread;
    
    // New variables for batching
    bool m_race_in_progress;
    uint64_t m_last_send_time;
    static constexpr uint64_t SEND_INTERVAL = 5000; // 5 seconds in ms
    static constexpr size_t MAX_QUEUE_SIZE = 100;
    
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
    
    // Add new methods
    void startRace();
    void endRace();
    void queueAnalyticsEvent(const std::string& player_id, 
                            uint16_t event_id,
                            int kart_id,
                            const std::string& metadata = "");
};

static const uint16_t ANALYTICS_EVENT_RACE_START = 1;
static const uint16_t ANALYTICS_EVENT_LAP_COMPLETE = 2;
static const uint16_t ANALYTICS_EVENT_RACE_END = 3;
static const uint16_t ANALYTICS_EVENT_PLAYER_UPRANKED = 4;
static const uint16_t ANALYTICS_EVENT_PLAYER_DOWNRANKED = 5;
static const uint16_t ANALYTICS_EVENT_PLAYER_COLLISION = 6;
static const uint16_t ANALYTICS_EVENT_PLAYER_CRASHED = 7;
static const uint16_t ANALYTICS_EVENT_PLAYER_USED_ITEM = 8;

#endif