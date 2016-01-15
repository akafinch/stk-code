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
#include "modes/three_strikes_battle.hpp"

#ifdef AI_DEBUG
#include "irrlicht.h"
#include <iostream>
using namespace irr;
using namespace std;
#endif

BattleAI::BattleAI(AbstractKart *kart,
                   StateManager::ActivePlayer *player)
         : ArenaAI(kart, player)
{

    reset();

#ifdef AI_DEBUG
    video::SColor col_debug(128, 128, 0, 0);
    m_debug_sphere = irr_driver->addSphere(1.0f, col_debug);
    m_debug_sphere->setVisible(true);
#endif

    if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_3_STRIKES)
    {
        m_world     = dynamic_cast<ThreeStrikesBattle*>(World::getWorld());
        m_track     = m_world->getTrack();
    }
    else
    {
        // Those variables are not defined in a battle mode (m_world is
        // a linear world, since it assumes the existance of drivelines)
        m_world           = NULL;
        m_track           = NULL;
    }

    // Don't call our own setControllerName, since this will add a
    // billboard showing 'AIBaseController' to the kart.
    Controller::setControllerName("BattleAI");

}   // BattleAI

//-----------------------------------------------------------------------------

BattleAI::~BattleAI()
{
#ifdef AI_DEBUG
    irr_driver->removeNode(m_debug_sphere);
#endif
}   //  ~BattleAI

//-----------------------------------------------------------------------------
/** Resets the AI when a race is restarted.
 */
void BattleAI::reset()
{
    ArenaAI::reset();
    AIBaseController::reset();
}   // reset

//-----------------------------------------------------------------------------
void BattleAI::update(float dt)
{
    ArenaAI::update(dt);
}   // update

//-----------------------------------------------------------------------------
void BattleAI::findClosestKart(bool use_difficulty)
{
    float distance = 99999.9f;
    const unsigned int n = m_world->getNumKarts();
    int closest_kart_num = 0;

    for (unsigned int i = 0; i < n; i++)
    {
        const AbstractKart* kart = m_world->getKart(i);
        if (kart->isEliminated()) continue;

        if (kart->getWorldKartId() == m_kart->getWorldKartId())
            continue; // Skip the same kart

        // Test whether takes current difficulty into account for closest kart
        // Notice: it don't affect aiming, this function will be called once
        // more in handleArenaItems, which ignore difficulty.
        if (m_cur_difficulty == RaceManager::DIFFICULTY_EASY && use_difficulty)
        {
            // Skip human players for novice mode unless only human players left
            const AbstractKart* temp = m_world->getKart(i);
            if (temp->getController()->isPlayerController() &&
               (m_world->getCurrentNumKarts() -
                m_world->getCurrentNumPlayers()) > 1)
                continue;
        }
        else if (m_cur_difficulty == RaceManager::DIFFICULTY_BEST && use_difficulty)
        {
            // Skip AI players for supertux mode
            const AbstractKart* temp = m_world->getKart(i);
            if (!(temp->getController()->isPlayerController()))
                continue;
        }

        Vec3 d = kart->getXYZ() - m_kart->getXYZ();
        if (d.length_2d() <= distance)
        {
            distance = d.length_2d();
            closest_kart_num = i;
        }
    }

    const AbstractKart* closest_kart = m_world->getKart(closest_kart_num);
    m_closest_kart_node = m_world->getKartNode(closest_kart_num);
    m_closest_kart_point = closest_kart->getXYZ();

    if (!use_difficulty)
    {
        m_closest_kart = m_world->getKart(closest_kart_num);
        checkPosition(m_closest_kart_point, &m_closest_kart_pos_data);
    }
}   // findClosestKart

//-----------------------------------------------------------------------------
void BattleAI::findTarget()
{
    // Find a suitable target to drive to, either powerup or kart
    if (m_kart->getPowerup()->getType() == PowerupManager::POWERUP_NOTHING &&
        m_kart->getAttachment()->getType() != Attachment::ATTACH_SWATTER)
        collectItemInArena(&m_target_point , &m_target_node);
    else
    {
        m_target_point = m_closest_kart_point;
        m_target_node  = m_closest_kart_node;
    }
}   // findTarget

// ------------------------------------------------------------------------
int BattleAI::getCurrentNode() const
{
    return m_world->getKartNode(m_kart->getWorldKartId());
}   // getCurrentNode
// ------------------------------------------------------------------------
bool BattleAI::isWaiting() const
{
    return m_world->isStartPhase();
}   // isWaiting
