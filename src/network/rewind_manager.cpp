//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "network/rewind_manager.hpp"

#include "graphics/irr_driver.hpp"
#include "modes/world.hpp"
#include "network/network_config.hpp"
#include "network/network_string.hpp"
#include "network/protocols/game_protocol.hpp"
#include "network/rewinder.hpp"
#include "network/rewind_info.hpp"
#include "network/time_step_info.hpp"
#include "physics/physics.hpp"
#include "race/history.hpp"
#include "utils/log.hpp"
#include "utils/profiler.hpp"

#include <algorithm>

RewindManager* RewindManager::m_rewind_manager        = NULL;
bool           RewindManager::m_enable_rewind_manager = false;

/** Creates the singleton. */
RewindManager *RewindManager::create()
{
    assert(!m_rewind_manager);
    m_rewind_manager = new RewindManager();
    return m_rewind_manager;
}   // create

// ----------------------------------------------------------------------------
/** Destroys the singleton. */
void RewindManager::destroy()
{
    assert(m_rewind_manager);
    delete m_rewind_manager;
    m_rewind_manager = NULL;
}   // destroy

// ============================================================================
/** The constructor.
 */
RewindManager::RewindManager()
{
    reset();
}   // RewindManager

// ----------------------------------------------------------------------------
/** Frees all saved state information. Note that the Rewinder data must be
 *  freed elsewhere.
 */
RewindManager::~RewindManager()
{
}   // ~RewindManager

// ----------------------------------------------------------------------------
/** Frees all saved state information and all destroyable rewinder.
 */
void RewindManager::reset()
{
    m_is_rewinding         = false;
    m_not_rewound_time     = 0;
    m_overall_state_size   = 0;
    m_state_frequency      = 1.0f / stk_config->m_network_state_frequeny;
    m_last_saved_state     = -9999.9f;  // forces initial state save

    if(!m_enable_rewind_manager) return;

    AllRewinder::iterator r = m_all_rewinder.begin();
    while(r!=m_all_rewinder.end())
    {
        if(!(*r)->canBeDestroyed())
        {
            r++;
            continue;
        }
        Rewinder *rewinder = *r;
        r = m_all_rewinder.erase(r);
        delete rewinder;
    }

    m_rewind_queue.reset();
}   // reset

// ----------------------------------------------------------------------------
/** Adds a new TimeStep entry. Only exception is time=0 (which happens during
 *  all of 'ready, set, go') - for which only one entry is created.
 */
void RewindManager::addNextTimeStep(float time, float dt)
{
    // Add a timestep entry each timestep, except at 'ready, set, go'
    // at which time is 0 - we add only one entry there
    if (  ( time>0 || m_rewind_queue.isEmpty()                           ) &&
          World::getWorld()->getPhase() != WorldStatus::IN_GAME_MENU_PHASE    )
    {
        m_rewind_queue.addNewTimeStep(time, dt);
    }
}   // addNextTimeStep

// ----------------------------------------------------------------------------    
/** Adds an event to the rewind data. The data to be stored must be allocated
 *  and not freed by the caller!
 *  \param time Time at which the event was recorded. If time is not specified
 *          (or set to -1), the current world time is used.
 *  \param buffer Pointer to the event data. 
 */
void RewindManager::addEvent(EventRewinder *event_rewinder,
                             BareNetworkString *buffer, bool confirmed,
                             float time                                   )
{
    if(m_is_rewinding) 
    {
        delete buffer;
        Log::error("RewindManager", "Adding event when rewinding");
        return;
    }

    if (time < 0)
        time = World::getWorld()->getTime();
    m_rewind_queue.addLocalEvent(event_rewinder, buffer, confirmed, time);
}   // addEvent

// ----------------------------------------------------------------------------
/** Adds an event to the list of network rewind data. This function is
 *  threadsafe so can be called by the network thread. The data is synched
 *  to m_rewind_info by the main thread. The data to be stored must be
 *  allocated and not freed by the caller!
 *  \param time Time at which the event was recorded.
 *  \param buffer Pointer to the event data.
 */
void RewindManager::addNetworkEvent(EventRewinder *event_rewinder,
                                    BareNetworkString *buffer, float time)
{
    m_rewind_queue.addNetworkEvent(event_rewinder, buffer, time);
}   // addNetworkEvent

// ----------------------------------------------------------------------------
/** Adds a state to the list of network rewind data. This function is
 *  threadsafe so can be called by the network thread. The data is synched
 *  to m_rewind_info by the main thread. The data to be stored must be
 *  allocated and not freed by the caller!
 *  \param time Time at which the event was recorded.
 *  \param buffer Pointer to the event data.
 */
void RewindManager::addNetworkState(int rewinder_index, BareNetworkString *buffer,
                                    float time)
{
    assert(NetworkConfig::get()->isClient());
    // On a client dt from a state is never used, it maintains
    // its own dt information (using TimeEvents).
    m_rewind_queue.addNetworkState(m_all_rewinder[rewinder_index], buffer,
                                   time, -99);
}   // addNetworkState

// ----------------------------------------------------------------------------
/** Determines if a new state snapshot should be taken, and if so calls all
 *  rewinder to do so.
 *  \param dt Time step size.
 */
void RewindManager::update(float dt)
{
    if(!m_enable_rewind_manager  || 
        m_all_rewinder.size()==0 ||
        m_is_rewinding              )  return;

    float time = World::getWorld()->getTime();
    m_not_rewound_time = time;

    // Clients don't save state, so they just exit.
    if ( NetworkConfig::get()->isClient() || 
         time - m_last_saved_state < m_state_frequency  )
    {
        return;
    }
    PROFILER_PUSH_CPU_MARKER("RewindManager - save state", 0x20, 0x7F, 0x20);

    // Save state
    GameProtocol::getInstance()->startNewState();
    AllRewinder::const_iterator rewinder;
    for(rewinder=m_all_rewinder.begin(); rewinder!=m_all_rewinder.end(); ++rewinder)
    {
        BareNetworkString *buffer = (*rewinder)->saveState();
        if(buffer && buffer->size()>=0)
        {
            m_overall_state_size += buffer->size();
            // Add to the previously created container
            m_rewind_queue.addLocalState(*rewinder, buffer, /*confirmed*/true,
                                         World::getWorld()->getTime());
            GameProtocol::getInstance()->addState(buffer);
        }   // size >= 0
        else
            delete buffer;   // NULL or 0 byte buffer
    }
    PROFILER_POP_CPU_MARKER();
    PROFILER_PUSH_CPU_MARKER("RewindManager - send state", 0x20, 0x7F, 0x40);
    GameProtocol::getInstance()->sendState();
    PROFILER_POP_CPU_MARKER();

    m_last_saved_state = time;
}   // update

// ----------------------------------------------------------------------------
/** Replays all events from the last event played till the specified time.
 *  \param time Up to (and inclusive) which time events will be replayed.
 *  \param dt Time step size. This might get adjusted if a new state has
 *         been received.
 */
void RewindManager::playEventsTill(float time, float *dt)
{    
    bool needs_rewind;
    float rewind_time;

    // Merge in all network events that have happened since the last
    // merge and that have happened before the current time (which will
    // be getTime()+dt - world time has not been updated yet).
    m_rewind_queue.mergeNetworkData(World::getWorld()->getTime(), *dt,
                                    &needs_rewind, &rewind_time);

    if (needs_rewind)
    {
        Log::setPrefix("Rewind");
        PROFILER_PUSH_CPU_MARKER("Rewind", 128, 128, 128);
        rewindTo(rewind_time);
        PROFILER_POP_CPU_MARKER();
        Log::setPrefix("");
        TimeStepInfo *tsi = m_rewind_queue.getCurrent();
        World::getWorld()->setTime(tsi->getTime());
    }

    // This is necessary to avoid that rewinding an event will store the 
    // event again as a seemingly new event.
    assert(!m_is_rewinding);
    m_is_rewinding = true;

    // Now play all events between time and time + dt, i.e. all events
    // stored at the last TimeStep info in the rewind queue.
    //assert(m_rewind_queue.getLast() == m_rewind_queue.getCurrent());

    TimeStepInfo *tsi = m_rewind_queue.getLast();

   // ++m_rewind_queue;   // Point to end of queue now
    tsi->replayAllEvents();

    if (tsi->hasConfirmedState() && NetworkConfig::get()->isClient())
    {
        Log::warn("RewindManager",
            "Client has received state in the future: at %f state %f",
            World::getWorld()->getTime(), tsi->getTime());
    }
    m_is_rewinding = false;
}   // playEventsTill

// ----------------------------------------------------------------------------
/** Rewinds to the specified time, then goes forward till the current
 *  World::getTime() is reached again: it will replay everything before
 *  World::getTime(), but not the events at World::getTime() (or later)/
 *  \param rewind_time Time to rewind to.
 */
void RewindManager::rewindTo(float rewind_time)
{
    assert(!m_is_rewinding);
    history->doReplayHistory(History::HISTORY_NONE);

    // First save all current transforms so that the error
    // can be computed between the transforms before and after
    // the rewind.
    AllRewinder::iterator rewinder;
    for (rewinder = m_all_rewinder.begin(); 
         rewinder != m_all_rewinder.end(); ++rewinder)
    {
        (*rewinder)->saveTransform();
    }

    // Then undo the rewind infos going backwards in time
    // --------------------------------------------------
    m_is_rewinding = true;
    m_rewind_queue.undoUntil(rewind_time);

    // Rewind the required state(s)
    // ----------------------------
    World *world = World::getWorld();
    float current_time = world->getTime();

    // Get the (first) full state to which we have to rewind
    TimeStepInfo *current = m_rewind_queue.getCurrent();

    // Store the time to which we have to replay to,
    // which can be earlier than rewind_time
    float exact_rewind_time = current->getTime();

    // Now start the rewind with the full state:
    world->setTime(exact_rewind_time);
    float local_physics_time = current->getLocalPhysicsTime();
    Physics::getInstance()->getPhysicsWorld()->setLocalTime(local_physics_time);

    float dt = -1.0f;

    // Need to exit loop if in-game menu is open, since world clock
    // will not be increased while the game is paused
    if (World::getWorld()->getPhase() == WorldStatus::IN_GAME_MENU_PHASE)
    {
        m_is_rewinding = false;
        return;
    }

    // Restore state from the current time
    current->replayAllStates();

    // Now go forward through the list of rewind infos. A new timestep
    // info for the current time has already been added previously, so
    // we rewind till we have reached the last timestep entry (which is
    // the current time step).
    while (current !=m_rewind_queue.getLast())
    {
        // Now handle all events(!) at the current time (i.e. between 
        // World::getTime() and World::getTime()+dt) before updating
        // the world:
        current->replayAllEvents();
        dt = current->getDT();
        world->updateWorld(dt);
#undef SHOW_ROLLBACK
#ifdef SHOW_ROLLBACK
        irr_driver->update(dt);
#endif
        world->updateTime(dt);

        ++m_rewind_queue;
        current = m_rewind_queue.getCurrent();
        world->setTime(current->getTime());
    }   // while (world->getTime() < current_time)

    // Now compute the errors which need to be visually smoothed
    for (rewinder = m_all_rewinder.begin();
        rewinder != m_all_rewinder.end(); ++rewinder)
    {
        (*rewinder)->computeError();
    }


    m_is_rewinding = false;
}   // rewindTo

// ----------------------------------------------------------------------------