//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2015 SuperTuxKart-Team
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

#ifndef SOCCER_WORLD_HPP
#define SOCCER_WORLD_HPP

#include "modes/world_with_rank.hpp"
#include "states_screens/race_gui_base.hpp"
#include "karts/abstract_kart.hpp"

#include <IMesh.h>
#include <string>

class PhysicalObject;
class AbstractKart;
class Controller;

/** An implementation of World, to provide the soccer game mode
 *  Notice: In soccer world, true team means blue, false means red.
 * \ingroup modes
 */
class SoccerWorld : public WorldWithRank
{
protected:
    virtual AbstractKart *createKart(const std::string &kart_ident, int index,
                             int local_player_id, int global_player_id,
                             RaceManager::KartType type,
                             PerPlayerDifficulty difficulty);

private:
    /** Number of goals needed to win */
    int m_goal_target;
    bool m_count_down_reached_zero;

    /** Whether or not goals can be scored (they are disabled when a point is scored
    and re-enabled when the next game can be played)*/
    bool m_can_score_points;
    SFXBase *m_goal_sound;

    /** Timer for displaying goal text*/
    float m_goal_timer;
    int m_last_kart_to_hit_ball;

    /** Number of goals each team scored */
    std::vector<int> m_team_goals;
    std::vector<int> m_red_scorers;
    std::vector<float> m_red_score_times;
    std::vector<int> m_blue_scorers;
    std::vector<float> m_blue_score_times;
    std::map<int, bool> m_kart_team_map;

    /** Data generated from navmesh */
    std::vector<int> m_kart_on_node;
    int m_ball_on_node;
    Vec3 m_ball_position;
    int m_red_goal_node;
    int m_blue_goal_node;

    /** Set the team for the karts */
    void initKartList();
    /** Function to init the locations of two goals on the polygon map */
    void initGoalNodes();
    /** Function to update the locations of all karts on the polygon map */
    void updateKartNodes();
    /** Function to update the location the ball on the polygon map */
    void updateBallPosition();
    /** Clean up */
    void resetAllNodes();

public:

    SoccerWorld();
    virtual ~SoccerWorld();

    virtual void init();

    // clock events
    virtual bool isRaceOver();
    virtual void terminateRace();
    virtual void countdownReachedZero() OVERRIDE;

    // overriding World methods
    virtual void reset();

    virtual bool useFastMusicNearEnd() const { return false; }
    virtual void getKartsDisplayInfo(
                 std::vector<RaceGUIBase::KartIconDisplayInfo> *info) {}

    virtual bool raceHasLaps() { return false; }

    virtual const std::string& getIdent() const;

    virtual void update(float dt);
    // ------------------------------------------------------------------------
    void onCheckGoalTriggered(bool first_goal);
    // ------------------------------------------------------------------------
    void setLastKartTohitBall(unsigned int kart_id);
    // ------------------------------------------------------------------------
    /** Get the soccer result of kart in soccer world (including AIs) */
    bool getKartSoccerResult(unsigned int kart_id) const;
    // ------------------------------------------------------------------------
    /** Get the team of kart in soccer world (including AIs) */
    bool getKartTeam(unsigned int kart_id) const;
    // ------------------------------------------------------------------------
    const int getScore(bool team) const
                        { return (team ? m_team_goals[0] : m_team_goals[1]); }
    // ------------------------------------------------------------------------
    const std::vector<int>& getScorers(bool team) const
                           { return (team ? m_blue_scorers : m_red_scorers); }
    // ------------------------------------------------------------------------
    const std::vector<float>& getScoreTimes(bool team) const
                   { return (team ? m_blue_score_times : m_red_score_times); }
    // ------------------------------------------------------------------------
    const int& getKartNode(unsigned int kart_id) const
                                          {  return m_kart_on_node[kart_id]; }
    // ------------------------------------------------------------------------
    const int& getBallNode() const
                                          {  return m_ball_on_node;          }
    // ------------------------------------------------------------------------
    const Vec3& getBallPosition() const
                                          {  return m_ball_position;         }
    // ------------------------------------------------------------------------
    const int& getGoalNode(bool team) const
                       { return (team ? m_blue_goal_node : m_red_goal_node); }

};   // SoccerWorld


#endif
