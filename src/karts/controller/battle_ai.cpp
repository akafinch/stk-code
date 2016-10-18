//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2005 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006-2007 Eduardo Hernandez Munoz
//  Copyright (C) 2008-2015 Joerg Henrichs
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

#include "karts/controller/battle_ai.hpp"

#include "items/attachment.hpp"
#include "items/powerup.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/kart_control.hpp"
#include "karts/controller/spare_tire_ai.hpp"
#include "modes/three_strikes_battle.hpp"
#include "tracks/arena_graph.hpp"

#ifdef AI_DEBUG
#include "irrlicht.h"
#endif

BattleAI::BattleAI(AbstractKart *kart)
         : ArenaAI(kart)
{

    reset();

#ifdef AI_DEBUG
    video::SColor col_debug(128, 128, 0, 0);
    video::SColor col_debug_next(128, 0, 128, 128);
    m_debug_sphere = irr_driver->addSphere(1.0f, col_debug);
    m_debug_sphere->setVisible(true);
    m_debug_sphere_next = irr_driver->addSphere(1.0f, col_debug_next);
    m_debug_sphere_next->setVisible(true);
#endif
    m_world = dynamic_cast<ThreeStrikesBattle*>(World::getWorld());
    m_track = m_world->getTrack();

    // Don't call our own setControllerName, since this will add a
    // billboard showing 'AIBaseController' to the kart.
    Controller::setControllerName("BattleAI");

}   // BattleAI

//-----------------------------------------------------------------------------

BattleAI::~BattleAI()
{
#ifdef AI_DEBUG
    irr_driver->removeNode(m_debug_sphere);
    irr_driver->removeNode(m_debug_sphere_next);
#endif
}   //  ~BattleAI

//-----------------------------------------------------------------------------
void BattleAI::findClosestKart(bool use_difficulty, bool find_sta)
{
    float distance = 99999.9f;
    int closest_kart_num = 0;
    const int end = m_world->getNumKarts();

    for (int start_id =
        find_sta ? end - race_manager->getNumSpareTireKarts() : 0;
        start_id < end; start_id++)
    {
        const AbstractKart* kart = m_world->getKart(start_id);
        const SpareTireAI* sta =
            dynamic_cast<const SpareTireAI*>(kart->getController());
        if (kart->isEliminated() && !(find_sta && sta && sta->isMoving()))
            continue;

        if (kart->getWorldKartId() == m_kart->getWorldKartId())
            continue; // Skip the same kart

        // Test whether takes current difficulty into account for closest kart
        // Notice: it don't affect aiming, this function will be called once
        // more when use items, which ignore difficulty.
        if (m_cur_difficulty == RaceManager::DIFFICULTY_EASY && use_difficulty)
        {
            // Skip human players for novice mode unless only they are left
            const AbstractKart* temp = m_world->getKart(start_id);
            if (temp->getController()->isPlayerController() &&
               (m_world->getCurrentNumKarts() -
                m_world->getCurrentNumPlayers()) > 1)
                continue;
        }
        else if (m_cur_difficulty == RaceManager::DIFFICULTY_BEST &&
            use_difficulty)
        {
            // Skip AI players for supertux mode
            const AbstractKart* temp = m_world->getKart(start_id);
            if (!(temp->getController()->isPlayerController()))
                continue;
        }

        float dist_to_kart = m_graph->getDistance(getCurrentNode(),
            m_world->getSectorForKart(kart));
        if (dist_to_kart <= distance)
        {
            distance = dist_to_kart;
            closest_kart_num = start_id;
        }
    }

    m_closest_kart = m_world->getKart(closest_kart_num);
    m_closest_kart_node = m_world->getSectorForKart(m_closest_kart);
    m_closest_kart_point = m_closest_kart->getXYZ();

}   // findClosestKart

//-----------------------------------------------------------------------------
void BattleAI::findTarget()
{
    // Find the closest kart first, it's used as fallback if no item is found.
    // It takes the current difficulty into account, also collect life from
    // spare tire karts when neccessary

    // Collect life depends on current difficulty:
    // Novice and intermediate - collect them only AI has 1 life only
    // Expert and supertux - collect them if AI dones't have 3 lives
    // Also when actually spare tire karts are spawned
    bool find_sta = m_world->spareTireKartsSpawned() ?
        ((m_cur_difficulty == RaceManager::DIFFICULTY_EASY ||
        m_cur_difficulty == RaceManager::DIFFICULTY_MEDIUM) &&
        m_world->getKartLife(m_kart->getWorldKartId()) == 1 ?
        true :
        (m_cur_difficulty == RaceManager::DIFFICULTY_HARD ||
        m_cur_difficulty == RaceManager::DIFFICULTY_BEST) &&
        m_world->getKartLife(m_kart->getWorldKartId()) != 3 ?
        true : false) : false;

    findClosestKart(find_sta ? false : true/*use_difficulty*/, find_sta);

    // Find a suitable target to drive to, either powerup or kart
    if (m_kart->getPowerup()->getType() == PowerupManager::POWERUP_NOTHING &&
        m_kart->getAttachment()->getType() != Attachment::ATTACH_SWATTER &&
        !find_sta)
        collectItemInArena(&m_target_point , &m_target_node);
    else
    {
        m_target_point = m_closest_kart_point;
        m_target_node  = m_closest_kart_node;
    }
}   // findTarget

//-----------------------------------------------------------------------------
int BattleAI::getCurrentNode() const
{
    return m_world->getSectorForKart(m_kart);
}   // getCurrentNode

//-----------------------------------------------------------------------------
bool BattleAI::isWaiting() const
{
    return m_world->isStartPhase();
}   // isWaiting

//-----------------------------------------------------------------------------
float BattleAI::getKartDistance(const AbstractKart* kart) const
{
    return m_graph->getDistance(getCurrentNode(),
        m_world->getSectorForKart(kart));
}   // getKartDistance

//-----------------------------------------------------------------------------
bool BattleAI::isKartOnRoad() const
{
    return m_world->isOnRoad(m_kart->getWorldKartId());
}   // isKartOnRoad
