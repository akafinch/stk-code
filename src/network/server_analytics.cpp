#include "network/server_analytics.hpp"
#include "network/http_client.hpp"
#include "network/server_config.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/controller.hpp"  
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "tracks/track.hpp"
#include "utils/time.hpp"
#include "utils/log.hpp"

ServerAnalytics::ServerAnalytics(const std::string& endpoint_uri,
                               const std::string& auth_id,
                               const std::string& auth_pwd)
    : m_http_client(new HTTPClient(endpoint_uri, 
                                 auth_id, 
                                 auth_pwd, 
                                 ServerConfig::m_tpk_table.c_str(),
                                 ServerConfig::m_tpk_token.c_str())),
      m_stop_thread(false),
      m_race_in_progress(false),
      m_last_send_time(0)
{
    // Don't block on connection - just log and continue
    Log::info("ServerAnalytics", "Initializing analytics in background");

    // Start the send thread
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
    Log::debug("ServerAnalytics", "Attempting to connect to analytics server");
    // Try to connect but don't block
    try {
        bool success = m_http_client->connect();
        Log::debug("ServerAnalytics", "Connection attempt result: %d", success ? 1 : 0);
        return success;
    } catch (...) {
        Log::warn("ServerAnalytics", "Failed to connect to analytics server - continuing without analytics");
        return false;
    }
}

void ServerAnalytics::disconnect()
{
    m_http_client->disconnect();
}

bool ServerAnalytics::sendAnalytics(const std::string& json_data)
{
    /*if (!m_http_client->isConnected()) {
        Log::warn("ServerAnalytics", "Attempted to send analytics while not connected");
        return false;
    }*/

    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (!m_race_in_progress) {
            Log::debug("ServerAnalytics", "Ignoring analytics data - race not in progress");
            return false;
        }
        m_message_queue.push(json_data);
    }
    
    // Only notify if we've reached the batch size
    if (m_message_queue.size() >= MAX_QUEUE_SIZE)
        m_cond_var.notify_one();
        
    return true;
}

void ServerAnalytics::startRace()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    Log::info("ServerAnalytics", "Starting race with analytics - Connected: %d", 
            isConnected() ? 1 : 0);
    m_race_in_progress = true;
    m_last_send_time = StkTime::getMonoTimeMs();
}

void ServerAnalytics::endRace()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_race_in_progress = false;
    // Trigger one final send of remaining data
    m_cond_var.notify_one();
}

/*void ServerAnalytics::sendLoop()
{

    Log::info("ServerAnalytics", "Analytics send thread started");

    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);

        // Log initial state
        uint64_t current_time = StkTime::getMonoTimeMs();
        Log::info("ServerAnalytics", "Initial state - Last send time: %lu, Current time: %lu", 
                 m_last_send_time, current_time);

        // Log before wait
        Log::info("ServerAnalytics", "About to wait - Queue size: %d, Race in progress: %d", 
                 m_message_queue.size(), m_race_in_progress ? 1 : 0);

        // Wait until we should send data
        m_cond_var.wait(lock, [this] {
            uint64_t current_time = StkTime::getMonoTimeMs();
            bool time_to_send = (current_time - m_last_send_time) >= SEND_INTERVAL;
            bool enough_data = m_message_queue.size() >= MAX_QUEUE_SIZE;
            
            // Allow time-based sending even with empty queue
            bool should_send = time_to_send || 
                            (!m_message_queue.empty() && enough_data) || 
                            (m_stop_thread && !m_message_queue.empty()) ||
                            (!m_race_in_progress && !m_message_queue.empty());
            
            Log::debug("ServerAnalytics", "Wait condition check - Time since last: %lu, Queue size: %d, Race in progress: %d, Should send: %d",
                    current_time - m_last_send_time, m_message_queue.size(), 
                    m_race_in_progress ? 1 : 0, should_send ? 1 : 0);
            
            return should_send;
        });

        // Log after wait
        Log::info("ServerAnalytics", "Wait condition satisfied - preparing to send batch");

        if (m_stop_thread && m_message_queue.empty()) {
            break;
        }

        // Prepare batch of messages
        std::string batch_message = "[";
        bool first = true;
        while (!m_message_queue.empty()) {
            if (!first) batch_message += ",";
            batch_message += m_message_queue.front();
            m_message_queue.pop();
            first = false;
        }
        batch_message += "]";
        
        m_last_send_time = StkTime::getMonoTimeMs();
        lock.unlock();

        // Send batch
        if (!batch_message.empty()) {
            Log::info("ServerAnalytics", "Sending batch of %d analytics events to endpoint",
              m_message_queue.size());
            m_http_client->sendJSON(batch_message);
        }

        lock.lock();
    }
}
*/

void ServerAnalytics::sendLoop()
{
    Log::info("ServerAnalytics", "Analytics send thread started");

    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        
        // Log initial state
        uint64_t current_time = StkTime::getMonoTimeMs();
        Log::info("ServerAnalytics", "Initial state - Last send time: %lu, Current time: %lu", 
                 m_last_send_time, current_time);

        // Log before wait
        Log::info("ServerAnalytics", "About to wait - Queue size: %d, Race in progress: %d", 
                 m_message_queue.size(), m_race_in_progress ? 1 : 0);
        
        // Wait for either timeout or condition
        bool predicate_satisfied = m_cond_var.wait_for(lock, 
            std::chrono::milliseconds(SEND_INTERVAL),
            [this] {
                bool enough_data = m_message_queue.size() >= MAX_QUEUE_SIZE;
                bool should_send = (!m_message_queue.empty() && enough_data) || 
                                 (m_stop_thread && !m_message_queue.empty()) ||
                                 (!m_race_in_progress && !m_message_queue.empty());
                
                Log::debug("ServerAnalytics", "Wait condition check - Queue size: %d, Race in progress: %d, Should send: %d",
                          m_message_queue.size(), m_race_in_progress ? 1 : 0, should_send ? 1 : 0);
                
                return should_send;
            });

        // Log after wait - either timeout occurred or condition was met
        Log::info("ServerAnalytics", "Wait ended - predicate satisfied: %d", 
                 predicate_satisfied ? 1 : 0);

        // Process queue if not empty
        if (!m_message_queue.empty()) {
            std::string batch_message = "[";
            bool first = true;
            while (!m_message_queue.empty()) {
                if (!first) batch_message += ",";
                batch_message += m_message_queue.front();
                m_message_queue.pop();
                first = false;
            }
            batch_message += "]";
            
            if (!batch_message.empty()) {
                Log::info("ServerAnalytics", "Sending batch of analytics events");
                Log::info("ServerAnalytics", "Preparing to send batch: %s", batch_message.c_str());
                Log::info("ServerAnalytics", "HTTP Client connected: %d", m_http_client->isConnected() ? 1 : 0);

                bool send_result = m_http_client->sendJSON(batch_message);
                Log::info("ServerAnalytics", "Send result: %d", send_result ? 1 : 0);

                if (!send_result) {
                    Log::warn("ServerAnalytics", "Failed to send analytics batch");
                }

            }
        }
        
        // Update last send time
        m_last_send_time = StkTime::getMonoTimeMs();
    }
}

std::string AnalyticsEvent::toJson() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"player-id\":\"" << player_id << "\",";
    ss << "\"match-id\":\"" << match_id << "\",";
    ss << "\"track\":" << track << ",";
    ss << "\"kart\":" << kart << ",";
    ss << "\"timestamp\":\"" << timestamp << "\",";
    ss << "\"loc-x\":" << loc_x << ",";
    ss << "\"loc-y\":" << loc_y << ",";
    ss << "\"loc-z\":" << loc_z << ",";
    ss << "\"face-x\":" << face_x << ",";
    ss << "\"face-y\":" << face_y << ",";
    ss << "\"face-z\":" << face_z << ",";
    ss << "\"speed\":" << speed << ",";
    ss << "\"gas\":" << (gas ? "true" : "false") << ",";
    ss << "\"brake\":" << (brake ? "true" : "false") << ",";
    ss << "\"nitro\":" << (nitro ? "true" : "false") << ",";
    ss << "\"skid\":" << (skid ? "true" : "false") << ",";
    ss << "\"back\":" << (back ? "true" : "false") << ",";
    ss << "\"event\":" << event;
    if (!metadata.empty())
        ss << ",\"metadata\":\"" << metadata << "\"";
    ss << "}";
    return ss.str();
}

void ServerAnalytics::queueAnalyticsEvent(const std::string& player_id, 
                                        uint16_t event_id,
                                        int kart_id,
                                        const std::string& metadata)
{

        // Add debug logging for analytics state
    Log::debug("ServerAnalytics", "Analytics state - Race in progress: %d, Connected: %d",
               m_race_in_progress ? 1 : 0, isConnected() ? 1 : 0);

    if (!m_race_in_progress) 
    {
        Log::warn("ServerAnalytics", "Skipping event - race not in progress or not connected");
        return;
    }

    // Queue event even if disconnected
    if (!isConnected()) {
        Log::info("ServerAnalytics", "HTTP client disconnected but queueing event anyway");
    }

    Log::info("ServerAnalytics", "Queueing analytics event %d for player %s (kart %d): %s",
              event_id, player_id.c_str(), kart_id, metadata.c_str());
    
    World* world = World::getWorld();
    if (!world) return;

    AbstractKart* kart = world->getKart(kart_id);
    if (!kart) return;

    AnalyticsEvent event;
    event.player_id = player_id;
    event.match_id = StringUtils::toString(world->getTicksSinceStart());
    
    event.track = (uint16_t)RaceManager::get()->getTrackName().length();
    event.kart = (uint16_t)kart->getIdent().length();
    
    // Get current time
    time_t now = std::time(nullptr);
    struct tm* tm_info = std::localtime(&now);  // Use localtime instead of gmtime
    char buffer[32];
    strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm_info);

    // Add milliseconds from STK time
    uint64_t ms = StkTime::getMonoTimeMs() % 1000;
        event.timestamp = StringUtils::toString(buffer) + "." + 
        (ms < 10 ? "00" : ms < 100 ? "0" : "") + StringUtils::toString(ms);


    /*uint64_t now = StkTime::getTimeSinceEpoch();
    uint64_t ms = now % 1000;
    time_t seconds = now / 1000;
    struct tm* tm_info = gmtime(&seconds);
    char buffer[32];
    strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm_info);
    event.timestamp = StringUtils::toString(buffer) + "." + 
        StringUtils::toString(ms).substr(0,3); */

    const Vec3& pos = kart->getXYZ();
    event.loc_x = pos.getX();
    event.loc_y = pos.getY();
    event.loc_z = pos.getZ();

    const btTransform& trans = kart->getSmoothedTrans();
    const btQuaternion& q = trans.getRotation();
    btVector3 fwd = btVector3(0, 0, 1).rotate(q.getAxis(), q.getAngle());
    event.face_x = fwd.x();
    event.face_y = fwd.y();
    event.face_z = fwd.z();

    event.speed = kart->getSpeed();
    
    Controller* controller = kart->getController();
    if (controller)
    {
        const KartControl* controls = controller->getControls();
        event.gas = controls->getAccel() > 0;
        event.brake = controls->getBrake();
        event.nitro = controls->getNitro();
        event.skid = controls->getSkidControl() != KartControl::SC_NONE;
        event.back = controls->getLookBack();
    }
    else
    {
        event.gas = false;
        event.brake = false;
        event.nitro = false;
        event.skid = false;
        event.back = false;
    }
    
    event.event = event_id;
    event.metadata = metadata;

    std::string json = event.toJson();
    sendAnalytics(json);
}
