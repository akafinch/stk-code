#include "network/protocols/game_events_protocol.hpp"

#include "karts/abstract_kart.hpp"
#include "items/attachment.hpp"
#include "items/item.hpp"
#include "items/item_manager.hpp"
#include "items/powerup.hpp"
#include "modes/world.hpp"
#include "network/event.hpp"
#include "network/game_setup.hpp"
#include "network/network_player_profile.hpp"
#include "network/protocol_manager.hpp"
#include "network/stk_host.hpp"
#include "network/stk_peer.hpp"

#include <stdint.h>

GameEventsProtocol::GameEventsProtocol() : Protocol(PROTOCOL_GAME_EVENTS)
{
}   // GameEventsProtocol

// ----------------------------------------------------------------------------
GameEventsProtocol::~GameEventsProtocol()
{
}   // ~GameEventsProtocol

// ----------------------------------------------------------------------------
bool GameEventsProtocol::notifyEvent(Event* event)
{
    if (event->getType() != EVENT_TYPE_MESSAGE)
        return true;
    NetworkString &data = event->data();
    if (data.size() < 5) // for token and type
    {
        Log::warn("GameEventsProtocol", "Too short message.");
        return true;
    }
    if ( event->getPeer()->getClientServerToken() != data.gui32())
    {
        Log::warn("GameEventsProtocol", "Bad token.");
        return true;
    }
    int8_t type = data.gui8(4);
    data.removeFront(5);
    switch (type)
    {
        case GE_ITEM_COLLECTED:
        {
            if (data.size() < 6)
            {
                Log::warn("GameEventsProtocol", "Too short message.");
                return true;
            }
            uint32_t item_id = data.gui32();
            uint8_t powerup_type = data.gui8(4);
            uint8_t player_id = data.gui8(5);
            // now set the kart powerup
            AbstractKart* kart = World::getWorld()->getKart(
                      STKHost::get()->getGameSetup()
                                    ->getProfile(player_id)->getWorldKartID());
            ItemManager::get()->collectedItem(
                                          ItemManager::get()->getItem(item_id),
                                          kart,
                                          powerup_type);
            Log::info("GameEventsProtocol", "Item %d picked by a player.",
                       powerup_type);
        }   break;
        case GE_KART_FINISHED_RACE:
            kartFinishedRace(data);
            break;
        default:
            Log::warn("GameEventsProtocol", "Unkown message type.");
            break;
    }
    return true;
}   // notifyEvent

// ----------------------------------------------------------------------------
void GameEventsProtocol::setup()
{
}   // setup

// ----------------------------------------------------------------------------
void GameEventsProtocol::update()
{
}   // update

// ----------------------------------------------------------------------------
void GameEventsProtocol::collectedItem(Item* item, AbstractKart* kart)
{
    GameSetup* setup = STKHost::get()->getGameSetup();
    assert(setup);
    // use kart name
    const NetworkPlayerProfile* player_profile =
                                           setup->getProfile(kart->getIdent());

    const std::vector<STKPeer*> &peers = STKHost::get()->getPeers();
    for (unsigned int i = 0; i < peers.size(); i++)
    {
        NetworkString ns(11);
        ns.ai32(peers[i]->getClientServerToken());
        // 0x01 : item picked : send item id, powerup type and kart race id
        uint8_t powerup = 0;
        if (item->getType() == Item::ITEM_BANANA)
            powerup = (int)(kart->getAttachment()->getType());
        else if (item->getType() == Item::ITEM_BONUS_BOX)
            powerup = (((int)(kart->getPowerup()->getType()) << 4) & 0xf0) 
                           + (kart->getPowerup()->getNum()         & 0x0f);

        ns.ai8(GE_ITEM_COLLECTED).ai32(item->getItemId()).ai8(powerup)
          .ai8(player_profile->getGlobalPlayerId());
        ProtocolManager::getInstance()->sendMessage(this, peers[i], ns,
                                                    /*reliable*/true,
                                                    /*synchronous*/true);
        Log::info("GameEventsProtocol",
                  "Notified a peer that a kart collected item %d.",
                  (int)(kart->getPowerup()->getType()));
    }
}   // collectedItem

// ----------------------------------------------------------------------------
/** This function is called from the server when a kart finishes a race. It
 *  sends a notification to all clients about this event.
 *  \param kart The kart that finished the race.
 *  \param time The time at which the kart finished.
 */
void GameEventsProtocol::kartFinishedRace(AbstractKart *kart, float time)
{
    NetworkString ns(20);

    const std::vector<STKPeer*> &peers = STKHost::get()->getPeers();
    for (unsigned int i = 0; i < peers.size(); i++)
    {
        ns.addUInt32(peers[i]->getClientServerToken())
          .addUInt8(GE_KART_FINISHED_RACE)
          .addUInt8(kart->getWorldKartId()).addFloat(time);

        ProtocolManager::getInstance()->sendMessage(this, peers[i], ns,
                                                    /*reliable*/true,
                                                    /*synchronous*/true);
    }   // for i in peers
}   // kartFinishedRace

// ----------------------------------------------------------------------------
/** This function is called on a client when it receives a kartFinishedRace
 *  event from the server. It updates the game with this information.
 *  \param ns The message from the server.
 */
void GameEventsProtocol::kartFinishedRace(const NetworkString &ns)
{
    uint8_t kart_id = ns.getUInt8(0);
    float time      = ns.getFloat(1);
    World::getWorld()->getKart(kart_id)->finishedRace(time,
                                                      /*from_server*/true);
}   // kartFinishedRace
