//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2016 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006-2016 SuperTuxKart-Team, Joerg Henrichs, Steve Baker
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

#include "karts/kart.hpp"

#include "audio/sfx_manager.hpp"
#include "audio/sfx_base.hpp"
#include "challenges/challenge_status.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "config/user_config.hpp"
#include "font/bold_face.hpp"
#include "font/font_manager.hpp"
#include "graphics/camera.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/explosion.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/particle_emitter.hpp"
#include "graphics/particle_kind_manager.hpp"
#include "graphics/shadow.hpp"
#include "graphics/skid_marks.hpp"
#include "graphics/slip_stream.hpp"
#include "graphics/stk_text_billboard.hpp"
#include "graphics/stars.hpp"
#include "guiengine/scalable_font.hpp"
#include "io/file_manager.hpp"
#include "items/attachment.hpp"
#include "items/item_manager.hpp"
#include "items/powerup.hpp"
#include "items/projectile_manager.hpp"
#include "karts/abstract_characteristic.hpp"
#include "karts/abstract_kart_animation.hpp"
#include "karts/cached_characteristic.hpp"
#include "karts/controller/end_controller.hpp"
#include "karts/controller/spare_tire_ai.hpp"
#include "karts/explosion_animation.hpp"
#include "karts/kart_gfx.hpp"
#include "karts/kart_model.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "karts/kart_rewinder.hpp"
#include "karts/max_speed.hpp"
#include "karts/rescue_animation.hpp"
#include "karts/skidding.hpp"
#include "main_loop.hpp"
#include "modes/overworld.hpp"
#include "modes/soccer_world.hpp"
#include "modes/world.hpp"
#include "modes/linear_world.hpp"
#include "modes/overworld.hpp"
#include "modes/soccer_world.hpp"
#include "modes/world.hpp"
#include "network/network_config.hpp"
#include "network/race_event_manager.hpp"
#include "network/rewind_manager.hpp"
#include "physics/btKart.hpp"
#include "physics/btKartRaycast.hpp"
#include "physics/physics.hpp"
#include "race/history.hpp"
#include "tracks/terrain_info.hpp"
#include "tracks/drive_graph.hpp"
#include "tracks/drive_node.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "tracks/track_sector.hpp"
#include "utils/constants.hpp"
#include "utils/log.hpp" //TODO: remove after debugging is done
#include "utils/vs.hpp"
#include "utils/profiler.hpp"

#include <ICameraSceneNode.h>
#include <ISceneManager.h>

#include <algorithm> // for min and max
#include <iostream>
#include <cmath>


#if defined(WIN32) && !defined(__CYGWIN__)  && !defined(__MINGW32__)
   // Disable warning for using 'this' in base member initializer list
#  pragma warning(disable:4355)
#endif

/** The kart constructor.
 *  \param ident  The identifier for the kart model to use.
 *  \param position The position (or rank) for this kart (between 1 and
 *         number of karts). This is used to determine the start position.
 *  \param init_transform  The initial position and rotation for this kart.
 */
Kart::Kart (const std::string& ident, unsigned int world_kart_id,
            int position, const btTransform& init_transform,
            PerPlayerDifficulty difficulty, std::shared_ptr<RenderInfo> ri)
     : AbstractKart(ident, world_kart_id, position, init_transform,
             difficulty, ri)

#if defined(WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#  pragma warning(1:4355)
#endif
{
    m_max_speed            = new MaxSpeed(this);
    m_terrain_info         = new TerrainInfo();
    m_powerup              = new Powerup(this);
    m_last_used_powerup    = PowerupManager::POWERUP_NOTHING;
    m_vehicle              = NULL;
    m_initial_position     = position;
    m_race_position        = position;
    m_collected_energy     = 0;
    m_finished_race        = false;
    m_race_result          = false;
    m_finish_time          = 0.0f;
    m_bubblegum_ticks      = 0;
    m_bubblegum_torque     = 0.0f;
    m_invulnerable_ticks   = 0;
    m_squash_ticks         = 0;

    m_shadow               = NULL;
    m_wheel_box            = NULL;
    m_collision_particles  = NULL;
    m_slipstream           = NULL;
    m_skidmarks            = NULL;
    m_controller           = NULL;
    m_saved_controller     = NULL;
    m_flying               = false;
    m_stars_effect         = NULL;
    m_is_jumping           = false;
    m_min_nitro_ticks      = 0;
    m_fire_clicked         = 0;
    m_boosted_ai           = false;
    m_type                 = RaceManager::KT_AI;

    m_xyz_history_size     = stk_config->time2Ticks(XYZ_HISTORY_TIME);

    Vec3 initial_position = getXYZ();
    for (int i=0;i<m_xyz_history_size;i++)
    {
        m_previous_xyz.push_back(initial_position);
        m_previous_xyz_times.push_back(0.0f);
    }
    m_time_previous_counter = 0.0f;

    m_view_blocked_by_plunger = 0;
    m_has_caught_nolok_bubblegum = false;

    // Initialize custom sound vector (TODO: add back when properly done)
    // m_custom_sounds.resize(SFXManager::NUM_CUSTOMS);

    // Set position and heading:
    m_reset_transform         = init_transform;
    m_speed                   = 0.0f;
    m_smoothed_speed          = 0.0f;
    m_last_factor_engine_sound = 0.0f;

    m_kart_model->setKart(this);

    // Create SFXBase for each custom sound (TODO: add back when properly done)
    /*
    for (int n = 0; n < SFXManager::NUM_CUSTOMS; n++)
    {
        int id = m_kart_properties->getCustomSfxId((SFXManager::CustomSFX)n);

        // If id == -1 the custom sound was not defined in the .irrkart config file
        if (id != -1)
        {
            m_custom_sounds[n] = SFXManager::get()->newSFX(id);
        }
    }*/

    m_horn_sound = SFXManager::get()->getBuffer("horn");
    m_crash_sounds[0] = SFXManager::get()->getBuffer("crash");
    m_crash_sounds[1] = SFXManager::get()->getBuffer("crash2");
    m_crash_sounds[2] = SFXManager::get()->getBuffer("crash3");
    m_goo_sound = SFXManager::get()->getBuffer("goo");
    m_boing_sound = SFXManager::get()->getBuffer("boing");

    m_engine_sound  = SFXManager::get()->createSoundSource(m_kart_properties->getEngineSfxType());
    
    for (int i = 0; i < EMITTER_COUNT; i++)
        m_emitters[i] = SFXManager::get()->createSoundSource("crash");

    m_skid_sound    = SFXManager::get()->createSoundSource( "skid"  );
    m_nitro_sound   = SFXManager::get()->createSoundSource( "nitro" );
    m_terrain_sound          = NULL;
    m_last_sound_material    = NULL;
    m_previous_terrain_sound = NULL;

}   // Kart

// -----------------------------------------------------------------------------
/** This is a second initialisation phase, necessary since in the constructor
 *  virtual functions are not called for any superclasses.
 *  \param type Type of the kart.
*/
void Kart::init(RaceManager::KartType type)
{
    m_type = type;

    // In multiplayer mode, sounds are NOT positional
    if (race_manager->getNumLocalPlayers() > 1)
    {
        float factor = 1.0f / race_manager->getNumberOfKarts();
        // players have louder sounds than AIs
        if (type == RaceManager::KT_PLAYER)
            factor = std::min(1.0f, race_manager->getNumLocalPlayers()/2.0f);

        for (int i = 0; i < EMITTER_COUNT; i++)
            m_emitters[i]->setVolume(factor);

        m_skid_sound->setVolume(factor);
        m_nitro_sound->setVolume(factor);
    }   // if getNumLocalPlayers > 1

    if(!m_engine_sound)
    {
        Log::error("Kart","Could not allocate a sfx object for the kart. Further errors may ensue!");
    }


#ifdef SERVER_ONLY
    bool animations = false;  // server never animates
#else
    bool animations = UserConfigParams::m_animated_characters;
#endif
    loadData(type, animations);

    m_kart_gfx = new KartGFX(this, Track::getCurrentTrack()->getIsDuringDay());
    m_skidding = new Skidding(this);
    // Create the stars effect
    m_stars_effect = new Stars(this);

    reset();
}   // init

// ----------------------------------------------------------------------------
/** The destructor frees the memory of this kart, but note that the actual kart
 *  model is still stored in the kart_properties (m_kart_model variable), so
 *  it is not reloaded).
 */
Kart::~Kart()
{
    // Delete all custom sounds (TODO: add back when properly done)
    /*
    for (int n = 0; n < SFXManager::NUM_CUSTOMS; n++)
    {
        if (m_custom_sounds[n] != NULL)
            SFXManager::get()->deleteSFX(m_custom_sounds[n]);
    }*/

    m_engine_sound->deleteSFX();
    m_skid_sound  ->deleteSFX();

    for (int i = 0; i < EMITTER_COUNT; i++)
        m_emitters[i]->deleteSFX();

    m_nitro_sound ->deleteSFX();
    delete m_kart_gfx;
    if(m_terrain_sound)          m_terrain_sound->deleteSFX();
    if(m_previous_terrain_sound) m_previous_terrain_sound->deleteSFX();
    if(m_collision_particles)    delete m_collision_particles;
    if(m_slipstream)             delete m_slipstream;
    if(m_attachment)             delete m_attachment;
    if(m_stars_effect)          delete m_stars_effect;

#ifndef SERVER_ONLY
    delete m_shadow;
    if(m_skidmarks) delete m_skidmarks ;
#endif

    if (m_wheel_box) m_wheel_box->remove();

    // Ghost karts don't have a body
    if(m_body)
    {
        Physics::getInstance()->removeKart(this);
        delete m_vehicle;
        delete m_vehicle_raycaster;
    }

    for(int i=0; i<m_kart_chassis.getNumChildShapes(); i++)
    {
        delete m_kart_chassis.getChildShape(i);
    }
    delete m_skidding;
    delete m_max_speed;
    delete m_terrain_info;
    delete m_powerup;

    if(m_controller)
        delete m_controller;
    if(m_saved_controller)
        delete m_saved_controller;
}   // ~Kart

//-----------------------------------------------------------------------------
/** Reset before a new race. It will remove all attachments, and
 *  puts the kart back at its original start position.
 */
void Kart::reset()
{
    if (m_flying)
    {
        m_flying = false;
        stopFlying();
    }

    // Add karts back in case that they have been removed (i.e. in battle
    // mode) - but only if they actually have a body (e.g. ghost karts
    // don't have one).
    if(m_body)
    {
        Physics::getInstance()->removeKart(this);
        Physics::getInstance()->addKart(this);
    }

    m_min_nitro_ticks = 0;

    // Reset star effect in case that it is currently being shown.
    m_stars_effect->reset();
    m_max_speed->reset();
    m_powerup->reset();

    // Reset animations and wheels
    m_kart_model->reset();

    // If the controller was replaced (e.g. replaced by end controller),
    // restore the original controller.
    if(m_saved_controller)
    {
        delete m_controller;
        m_controller       = m_saved_controller;
        m_saved_controller = NULL;
    }
    m_kart_model->setAnimation(KartModel::AF_DEFAULT);
    m_attachment->clear();
    m_kart_gfx->reset();
    m_skidding->reset();

#ifndef SERVER_ONLY
    if (m_collision_particles)
        m_collision_particles->setCreationRateAbsolute(0.0f);
#endif

    m_race_position        = m_initial_position;
    m_finished_race        = false;
    m_eliminated           = false;
    m_finish_time          = 0.0f;
    m_bubblegum_ticks      = 0;
    m_bubblegum_torque     = 0.0f;
    m_invulnerable_ticks   = 0;
    m_squash_ticks         = 0;
    m_node->setScale(core::vector3df(1.0f, 1.0f, 1.0f));
    m_collected_energy     = 0;
    m_has_started          = false;
    m_bounce_back_ticks    = 0;
    m_brake_ticks          = 0;
    m_ticks_last_crash     = 0;
    m_speed                = 0.0f;
    m_smoothed_speed       = 0.0f;
    m_current_lean         = 0.0f;
    m_falling_time         = 0.0f;
    m_view_blocked_by_plunger = 0;
    m_has_caught_nolok_bubblegum = false;
    m_is_jumping           = false;

    for (int i=0;i<m_xyz_history_size;i++)
    {
        m_previous_xyz[i] = getXYZ();
        m_previous_xyz_times[i] = 0.0f;
    }
    m_time_previous_counter = 0.0f;

    // In case that the kart was in the air, in which case its
    // linear damping is 0
    if(m_body)
        m_body->setDamping(m_kart_properties->getStabilityChassisLinearDamping(),
                           m_kart_properties->getStabilityChassisAngularDamping());

    if(m_terrain_sound)
    {
        m_terrain_sound->deleteSFX();
        m_terrain_sound = NULL;
    }
    if(m_previous_terrain_sound)
    {
        m_previous_terrain_sound->deleteSFX();
        m_previous_terrain_sound = NULL;
    }

    if(m_engine_sound)
        m_engine_sound->stop();

    m_controls.reset();
    m_slipstream->reset();
    
    if(m_vehicle)
    {
        for (unsigned int i = 0; i < 4; i++)
        {
            m_vehicle->getWheelInfo(i).m_steering = 0;
        }
            
        m_vehicle->reset();
    }

    setTrans(m_reset_transform);

    applyEngineForce (0.0f);

    AbstractKart::reset();
#ifndef SERVER_ONLY
    if (m_skidmarks)
    {
        m_skidmarks->reset();
    }
#endif

    Vec3 front(0, 0, getKartLength()*0.5f);
    m_xyz_front = getTrans()(front);

    // Base on update() below, require if starting point of kart is not near
    // 0, 0, 0 (like in battle arena)
    m_terrain_info->update(getTrans().getBasis(),
        getTrans().getOrigin() + getTrans().getBasis() * Vec3(0, 0.3f, 0));

    // Reset is also called when the kart is created, at which time
    // m_controller is not yet defined, so this has to be tested here.
    if(m_controller)
        m_controller->reset();

    // 3 strikes mode can hide the wheels
    scene::ISceneNode** wheels = getKartModel()->getWheelNodes();
    if(wheels[0]) wheels[0]->setVisible(true);
    if(wheels[1]) wheels[1]->setVisible(true);
    if(wheels[2]) wheels[2]->setVisible(true);
    if(wheels[3]) wheels[3]->setVisible(true);

}   // reset

// -----------------------------------------------------------------------------
void Kart::setXYZ(const Vec3& a)
{
    AbstractKart::setXYZ(a);
    Vec3 front(0, 0, getKartLength()*0.5f);
    m_xyz_front = getTrans()(front);
}    // setXYZ

// -----------------------------------------------------------------------------
void Kart::increaseMaxSpeed(unsigned int category, float add_speed,
                            float engine_force, int duration,
                            int fade_out_time)
{
    m_max_speed->increaseMaxSpeed(category, add_speed, engine_force,
                                  duration, fade_out_time);
}   // increaseMaxSpeed

// -----------------------------------------------------------------------------
void Kart::instantSpeedIncrease(unsigned int category, float add_max_speed,
                               float speed_boost, float engine_force,
                               int duration, int fade_out_time)
{
    m_max_speed->instantSpeedIncrease(category, add_max_speed, speed_boost,
                                      engine_force, duration, fade_out_time);
}   // instantSpeedIncrease

// -----------------------------------------------------------------------------
void Kart::setSlowdown(unsigned int category, float max_speed_fraction,
                       int fade_in_time)
{
    m_max_speed->setSlowdown(category, max_speed_fraction,  fade_in_time);
}   // setSlowdown

// -----------------------------------------------------------------------------
float Kart::getCurrentMaxSpeed() const
{
    return m_max_speed->getCurrentMaxSpeed();
}   // getCurrentMaxSpeed
// -----------------------------------------------------------------------------
int Kart::getSpeedIncreaseTicksLeft(unsigned int category) const
{
    return m_max_speed->getSpeedIncreaseTicksLeft(category);
}   // getSpeedIncreaseTimeLeft

// -----------------------------------------------------------------------------
void Kart::setBoostAI(bool boosted)
{
    m_boosted_ai = boosted;
}   // setBoostAI
// -----------------------------------------------------------------------------
bool Kart::getBoostAI() const
{
    return m_boosted_ai;
}   // getBoostAI

// -----------------------------------------------------------------------------
/** Returns the current material the kart is on. */
const Material *Kart::getMaterial() const
{
    return m_terrain_info->getMaterial();
}   // getMaterial

// -----------------------------------------------------------------------------
/** Returns the previous material the kart was one (which might be
 *  the same as getMaterial() ). */
const Material *Kart::getLastMaterial() const
{
    return m_terrain_info->getLastMaterial();
}   // getLastMaterial
// -----------------------------------------------------------------------------
/** Returns the pitch of the terrain depending on the heading. */
float Kart::getTerrainPitch(float heading) const
{
    return m_terrain_info->getTerrainPitch(heading);
}   // getTerrainPitch

// -----------------------------------------------------------------------------
/** Returns the height of the terrain. we're currently above */
float Kart::getHoT() const
{
    return m_terrain_info->getHoT();
}   // getHoT

// ----------------------------------------------------------------------------
/** Sets the powerup this kart has collected.
 *  \param t Type of the powerup.
 *  \param n Number of powerups collected.
 */
void Kart::setPowerup(PowerupManager::PowerupType t, int n)
{
    m_powerup->set(t, n);
}   // setPowerup

// ----------------------------------------------------------------------------
/** Sets the powerup this kart has last used. Number is always 1.
 *  \param t Type of the powerup.
 */
void Kart::setLastUsedPowerup(PowerupManager::PowerupType t)
{
    m_last_used_powerup = t;
}   // setLastUsedPowerup

// -----------------------------------------------------------------------------
int Kart::getNumPowerup() const
{
    return m_powerup->getNum();
}   // getNumPowerup

// -----------------------------------------------------------------------------
/** Saves the old controller in m_saved_controller and stores a new
 *  controller. The save controller is needed in case of a reset.
 *  \param controller The new controller to use (atm it's always an
 *         end controller).
 */
void Kart::setController(Controller *controller)
{
    assert(m_saved_controller==NULL);
    m_saved_controller = m_controller;
    m_controller       = controller;
}   // setController

// -----------------------------------------------------------------------------
/** Sets the position in race this kart has .
 *  The position in this race for this kart (1<=p<=n)
 */
void Kart::setPosition(int p)
{
    m_controller->setPosition(p);
    m_race_position = p;
}   // setPosition

// -----------------------------------------------------------------------------
/** Sets that the view is blocked by a plunger. The duration depends on
 *  the difficulty, see KartPorperties getPlungerInFaceTime.
 */
void Kart::blockViewWithPlunger()
{
    // Avoid that a plunger extends the plunger time
    if(m_view_blocked_by_plunger<=0 && !isShielded())
        m_view_blocked_by_plunger = 
        stk_config->time2Ticks(m_kart_properties->getPlungerInFaceTime());
    if(isShielded())
    {
        decreaseShieldTime();
    }
}   // blockViewWithPlunger

// -----------------------------------------------------------------------------
/** Returns a transform that will align an object with the kart: the heading
 *  and the pitch will be set appropriately. A custom pitch value can be
 *  specified in order to overwrite the terrain pitch (which would be used
 *  otherwise).
 *  \param custom_pitch Pitch value to overwrite the terrain pitch. A value of
 *         -1 indicates that the custom_pitch value should not be used, instead
 *         the actual pitch of the terrain is to be used.
 */
btTransform Kart::getAlignedTransform(const float custom_pitch)
{
    btTransform trans = getTrans();
    /*
    float pitch = (custom_pitch == -1 ? getTerrainPitch(getHeading())
                                      : custom_pitch);

    btMatrix3x3 m;
    m.setEulerZYX(pitch, getHeading()+m_skidding->getVisualSkidRotation(),
                  0.0f);
    trans.setBasis(m);
    */
    btTransform trans2;
    trans2.setIdentity();
    trans2.setRotation(btQuaternion(m_skidding->getVisualSkidRotation(), 0, 0));
    trans *= trans2;
    
    return trans;
}   // getAlignedTransform

// ----------------------------------------------------------------------------
float Kart::getTimeFullSteer(float steer) const
{
    return m_kart_properties->getTurnTimeFullSteer().get(steer);
}   // getTimeFullSteer

// ----------------------------------------------------------------------------
/** Creates the physical representation of this kart. Atm it uses the actual
 *  extention of the kart model to determine the size of the collision body.
 */
void Kart::createPhysics()
{
    // First: Create the chassis of the kart
    // -------------------------------------
    const float kart_length = getKartLength();
    const float kart_width  = getKartWidth();
    float       kart_height = getKartHeight();

    // improve physics for tall karts
    if (kart_height > kart_length*0.6f)
    {
        kart_height = kart_length*0.6f;
    }

    const Vec3 &bevel = m_kart_properties->getBevelFactor();
    Vec3 wheel_pos[4];

    Vec3 orig_factor(1, 1, 1 - bevel.getZ());
    Vec3 bevel_factor(1.0f - bevel.getX(), 1.0f - bevel.getY(), 1.0f);
    btConvexHullShape *hull = new btConvexHullShape();
    for (int y = -1; y <= 1; y += 2)
    {
        for (int z = -1; z <= 1; z += 2)
        {
            for (int x = -1; x <= 1; x += 2)
            {
                Vec3 p(x*kart_width  *0.5f,
                       y*kart_height *0.5f,
                       z*kart_length *0.5f);

                hull->addPoint(p*orig_factor);
                // Only add bevelled point if bevel is defined (i.e.!=0)
                if(bevel.length2()>0)
                    hull->addPoint(p*bevel_factor);
                if (y == -1)
                {
                    int index = (x + 1) / 2 + 1 - z;  // get index of wheel
                    float f = m_kart_properties->getPhysicalWheelPosition();
                    // f < 0 indicates to use the old physics position, i.e.
                    // to place the wheels outside of the chassis
                    if(f<0)
                    {
                        // All wheel positions are relative to the center of
                        // the collision shape.
                        wheel_pos[index].setX(x*0.5f*kart_width);
                        wheel_pos[index].setZ((0.5f*kart_length-0.25f)* z);
                    }
                    else
                    {
                        // Store the x/z position for the wheels as a weighted average
                        // of the two bevelled points (y is set below).
                        wheel_pos[index] = p*(orig_factor*(1.0f - f) + bevel_factor*f);
                    }
                    // The y position of the wheels (i.e. the points where
                    // the suspension is attached to) is just at the
                    // bottom of the kart (independent of collision shape). 
                    // That is half the kart height down.
                    wheel_pos[index].setY(-0.5f*kart_height);
                }  // if y==-1
            }   // for x
        }   // for z
    }   // for y

    // This especially enables proper drawing of the point cloud
    hull->initializePolyhedralFeatures();

    btTransform shiftCenterOfGravity;
    shiftCenterOfGravity.setIdentity();
    // Shift center of gravity downwards, so that the kart
    // won't topple over too easy.
    shiftCenterOfGravity.setOrigin(m_kart_properties->getGravityCenterShift());
    m_kart_chassis.addChildShape(shiftCenterOfGravity, hull);

    // Set mass and inertia
    // --------------------
    float mass = m_kart_properties->getMass();

    // Position the chassis
    // --------------------
    btTransform trans;
    trans.setIdentity();
    createBody(mass, trans, &m_kart_chassis,
               m_kart_properties->getRestitution());
    std::vector<float> ang_fact = m_kart_properties->getStabilityAngularFactor();
    // The angular factor (with X and Z values <1) helps to keep the kart
    // upright, especially in case of a collision.
    m_body->setAngularFactor(Vec3(ang_fact[0], ang_fact[1], ang_fact[2]));
    m_body->setFriction(m_kart_properties->getFrictionKartFriction());
    m_user_pointer.set(this);
    m_body->setDamping(m_kart_properties->getStabilityChassisLinearDamping(),
                       m_kart_properties->getStabilityChassisAngularDamping() );

    // Reset velocities
    // ----------------
    m_body->setLinearVelocity (btVector3(0.0f,0.0f,0.0f));
    m_body->setAngularVelocity(btVector3(0.0f,0.0f,0.0f));

    // Create the actual vehicle
    // -------------------------
    m_vehicle_raycaster =
        new btKartRaycaster(Physics::getInstance()->getPhysicsWorld(),
                            stk_config->m_smooth_normals &&
                            Track::getCurrentTrack()->smoothNormals());
    m_vehicle = new btKart(m_body, m_vehicle_raycaster, this);

    // never deactivate the vehicle
    m_body->setActivationState(DISABLE_DEACTIVATION);

    // Add wheels
    // ----------
    float suspension_rest = m_kart_properties->getSuspensionRest();

    btVector3 wheel_direction(0.0f, -1.0f, 0.0f);
    btVector3 wheel_axle(-1.0f, 0.0f, 0.0f);

    btKart::btVehicleTuning tuning;
    tuning.m_maxSuspensionTravel =
        m_kart_properties->getSuspensionTravel();
    tuning.m_maxSuspensionForce    =
        m_kart_properties->getSuspensionMaxForce();

    const Vec3 &cs = m_kart_properties->getGravityCenterShift();
    for(unsigned int i=0; i<4; i++)
    {
        bool is_front_wheel = i<2;
        btWheelInfo& wheel = m_vehicle->addWheel(
                            wheel_pos[i]+cs,
                            wheel_direction, wheel_axle, suspension_rest,
                            m_kart_model->getWheelGraphicsRadius(i),
                            tuning, is_front_wheel);
        wheel.m_suspensionStiffness      = m_kart_properties->getSuspensionStiffness();
        wheel.m_wheelsDampingRelaxation  = m_kart_properties->getWheelsDampingRelaxation();
        wheel.m_wheelsDampingCompression = m_kart_properties->getWheelsDampingCompression();
        wheel.m_frictionSlip             = m_kart_properties->getFrictionSlip();
        wheel.m_rollInfluence            = m_kart_properties->getStabilityRollInfluence();
    }
    // Body to be added in reset() which allows complete reset kart when
    // restarting the race

}   // createPhysics

// ----------------------------------------------------------------------------

void Kart::flyUp()
{
    m_flying = true;
    Moveable::flyUp();
}   // flyUp

// ----------------------------------------------------------------------------
void Kart::flyDown()
{
    if (isNearGround())
    {
        stopFlying();
        m_flying = false;
    }
    else
    {
        Moveable::flyDown();
    }
}   // flyDown

// ----------------------------------------------------------------------------
/** Starts the engine sound effect. Called once the track intro phase is over.
 */
void Kart::startEngineSFX()
{
    if(!m_engine_sound)
        return;

    // In multiplayer mode, sounds are NOT positional (because we have
    // multiple listeners) so the engine sounds of all AIs is constantly
    // heard. So reduce volume of all sounds.
    if (race_manager->getNumLocalPlayers() > 1)
    {
        const int np = race_manager->getNumLocalPlayers();
        const int nai = race_manager->getNumberOfKarts() - np;

        // player karts twice as loud as AIs toghether
        const float players_volume = (np * 2.0f) / (np*2.0f + np);

        if (m_controller->isLocalPlayerController())
            m_engine_sound->setVolume( players_volume / np );
        else
            m_engine_sound->setVolume( (1.0f - players_volume) / nai );
    }

    m_engine_sound->setSpeed(0.6f);
    m_engine_sound->setLoop(true);
    m_engine_sound->play();
}   // startEngineSFX

//-----------------------------------------------------------------------------
/** Returns true if the kart is 'resting', i.e. (nearly) not moving.
 */
bool Kart::isInRest() const
{
    return fabsf(m_body->getLinearVelocity ().y())<0.2f;
}  // isInRest

//-----------------------------------------------------------------------------
/** Multiplies the velocity of the kart by a factor f (both linear
 *  and angular). This is used by anvils, which suddenly slow down the kart
 *  when they are attached.
 */
void Kart::adjustSpeed(float f)
{
    m_body->setLinearVelocity(m_body->getLinearVelocity()*f);
    m_body->setAngularVelocity(m_body->getAngularVelocity()*f);
}   // adjustSpeed

//-----------------------------------------------------------------------------
/** This method is to be called every time the mass of the kart is updated,
 *  which includes attaching an anvil to the kart (and detaching).
 */
void Kart::updateWeight()
{
    float mass = m_kart_properties->getMass() + m_attachment->weightAdjust();

    btVector3 inertia;
    m_kart_chassis.calculateLocalInertia(mass, inertia);
    m_body->setMassProps(mass, inertia);
}   // updateWeight

// ------------------------------------------------------------------------
/** Returns the (maximum) speed for a given turn radius.
 *  \param radius The radius for which the speed needs to be computed. */
float Kart::getSpeedForTurnRadius(float radius) const
{
    InterpolationArray turn_angle_at_speed = m_kart_properties->getTurnRadius();
    // Convert the turn radius into turn angle
    for(int i = 0; i < (int)turn_angle_at_speed.size(); i++)
        turn_angle_at_speed.setY(i, sin(m_kart_properties->getWheelBase() /
            turn_angle_at_speed.getY(i)));

    float angle = sin(m_kart_properties->getWheelBase() / radius);
    return turn_angle_at_speed.getReverse(angle);
}   // getSpeedForTurnRadius

// ------------------------------------------------------------------------
/** Returns the maximum steering angle (depending on speed). */
float Kart::getMaxSteerAngle(float speed) const
{
    InterpolationArray turn_angle_at_speed = m_kart_properties->getTurnRadius();
    // Convert the turn radius into turn angle
    for(int i = 0; i < (int)turn_angle_at_speed.size(); i++)
        turn_angle_at_speed.setY(i, sin(m_kart_properties->getWheelBase() /
            turn_angle_at_speed.getY(i)));

    return turn_angle_at_speed.get(speed);
}   // getMaxSteerAngle

//-----------------------------------------------------------------------------
/** Sets that this kart has finished the race and finishing time. It also
 *  notifies the race_manager about the race completion for this kart.
 *  \param time The finishing time for this kart. It can either be the
 *         actual time when the kart finished (in which case time() =
 *         world->getTime()), or the estimated time in case that all
 *         player kart have finished the race and all AI karts get
 *         an estimated finish time set.
 *  \param from_server In a network game, only the server can notify
 *         about a kart finishing a race. This parameter is to distinguish
 *         between a local detection (which is ignored on clients in a
 *         network game), and a server notification.
 */
void Kart::finishedRace(float time, bool from_server)
{
    // m_finished_race can be true if e.g. an AI kart was set to finish
    // because the race was over (i.e. estimating the finish time). If
    // this kart then crosses the finish line (with the end controller)
    // it would trigger a race end again.
    if (m_finished_race) return;

    if (NetworkConfig::get()->isNetworking() && !from_server)
    {
        if (NetworkConfig::get()->isServer())
        {
            RaceEventManager::getInstance()->kartFinishedRace(this, time);
        }   // isServer

        // Ignore local detection of a kart finishing a race in a 
        // network game.
        else if (NetworkConfig::get()->isClient())
        {
            return;
        }
    }   // !from_server

    m_finished_race = true;

    m_finish_time   = time;

    m_controller->finishedRace(time);
    m_kart_model->finishedRace();
    race_manager->kartFinishedRace(this, time);

    // If this is spare tire kart, end now
    if (dynamic_cast<SpareTireAI*>(m_controller) != NULL) return;

    if ((race_manager->getMinorMode() == RaceManager::MINOR_MODE_NORMAL_RACE ||
         race_manager->getMinorMode() == RaceManager::MINOR_MODE_TIME_TRIAL  ||
         race_manager->getMinorMode() == RaceManager::MINOR_MODE_FOLLOW_LEADER)
         && m_controller->isPlayerController())
    {
        RaceGUIBase* m = World::getWorld()->getRaceGUI();
        if (m)
        {
            if (race_manager->
                getMinorMode() == RaceManager::MINOR_MODE_FOLLOW_LEADER &&
                getPosition() == 2)
                m->addMessage(_("You won the race!"), this, 2.0f);
            else if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_NORMAL_RACE ||
                     race_manager->getMinorMode() == RaceManager::MINOR_MODE_TIME_TRIAL)
            {
                m->addMessage((getPosition() == 1 ?
                _("You won the race!") : _("You finished the race!")) ,
                this, 2.0f);
            }
        }
    }

    if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_NORMAL_RACE   ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_TIME_TRIAL    ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_FOLLOW_LEADER ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_3_STRIKES     ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_SOCCER        ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_EASTER_EGG)
    {
        // Save for music handling in race result gui
        setRaceResult();
        if (!isGhostKart())
        {
            setController(new EndController(this, m_controller));
        }
        // Skip animation if this kart is eliminated
        if (m_eliminated || isGhostKart()) return;

        m_kart_model->setAnimation(m_race_result ?
            KartModel::AF_WIN_START : KartModel::AF_LOSE_START);
    }
}   // finishedRace

//-----------------------------------------------------------------------------
void Kart::setRaceResult()
{
    if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_NORMAL_RACE ||
        race_manager->getMinorMode() == RaceManager::MINOR_MODE_TIME_TRIAL)
    {
        if (m_controller->isLocalPlayerController()) // if player is on this computer
        {
            PlayerProfile *player = PlayerManager::getCurrentPlayer();
            const ChallengeStatus *challenge = player->getCurrentChallengeStatus();
            // In case of a GP challenge don't make the end animation depend
            // on if the challenge is fulfilled
            if (challenge && !challenge->getData()->isGrandPrix())
            {
                if (challenge->getData()->isChallengeFulfilled())
                    m_race_result = true;
                else
                    m_race_result = false;
            }
            else if (this->getPosition() <= 0.5f *
                World::getWorld()->getCurrentNumKarts() ||
                this->getPosition() == 1)
                m_race_result = true;
            else
                m_race_result = false;
        }
        else
        {
            if (this->getPosition() <= 0.5f *
                World::getWorld()->getCurrentNumKarts() ||
                this->getPosition() == 1)
                m_race_result = true;
            else
                m_race_result = false;
        }
    }
    else if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_FOLLOW_LEADER ||
             race_manager->getMinorMode() == RaceManager::MINOR_MODE_3_STRIKES)
    {
        // the kart wins if it isn't eliminated
        m_race_result = !this->isEliminated();
    }
    else if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_SOCCER)
    {
        SoccerWorld* sw = dynamic_cast<SoccerWorld*>(World::getWorld());
        m_race_result = sw->getKartSoccerResult(this->getWorldKartId());
    }
    else if (race_manager->getMinorMode() == RaceManager::MINOR_MODE_EASTER_EGG)
    {
        // Easter egg mode only has one player, so always win
        m_race_result = true;
    }
    else
        Log::warn("Kart", "Unknown game mode given.");

}   // setRaceResult

//-----------------------------------------------------------------------------
/** Called when an item is collected. It will either adjust the collected
 *  energy, or update the attachment or powerup for this kart.
 *  \param item_state The item that was hit.
 */
void Kart::collectedItem(ItemState *item_state)
{
    float old_energy          = m_collected_energy;
    const Item::ItemType type = item_state->getType();

    switch (type)
    {
    case Item::ITEM_BANANA:
        m_attachment->hitBanana(item_state);
        break;
    case Item::ITEM_NITRO_SMALL:
        m_collected_energy += m_kart_properties->getNitroSmallContainer();
        break;
    case Item::ITEM_NITRO_BIG:
        m_collected_energy += m_kart_properties->getNitroBigContainer();
        break;
    case Item::ITEM_BONUS_BOX  :
        {
            m_powerup->hitBonusBox(*item_state);
            break;
        }
    case Item::ITEM_BUBBLEGUM:
        m_has_caught_nolok_bubblegum = 
            (item_state->getPreviousOwner()&&
             item_state->getPreviousOwner()->getIdent() == "nolok");

        // slow down
        m_bubblegum_ticks =
            stk_config->time2Ticks(m_kart_properties->getBubblegumDuration());
        m_bubblegum_torque = ((rand()%2)
                           ?  m_kart_properties->getBubblegumTorque()
                           : -m_kart_properties->getBubblegumTorque());
        m_max_speed->setSlowdown(MaxSpeed::MS_DECREASE_BUBBLE,
                                 m_kart_properties->getBubblegumSpeedFraction() ,
                                 m_kart_properties->getBubblegumFadeInTicks(),
                                 m_bubblegum_ticks);
        getNextEmitter()->play(getXYZ(), m_goo_sound);

        // Play appropriate custom character sound
        playCustomSFX(SFXManager::CUSTOM_GOO);
        break;
    default        : break;
    }   // switch TYPE

    if ( m_collected_energy > m_kart_properties->getNitroMax())
        m_collected_energy = m_kart_properties->getNitroMax();
    m_controller->collectedItem(*item_state, old_energy);

}   // collectedItem

//-----------------------------------------------------------------------------
/** Called the first time a kart accelerates after 'ready-set-go'. It searches
 *  through the startup times to find the appropriate slot, and returns the
 *  speed-boost from the corresponding entry.
 *  If the kart started too slow (i.e. slower than the longest time in the
 *  startup times list), it returns 0.
 */
float Kart::getStartupBoost() const
{
    float t = stk_config->ticks2Time(World::getWorld()->getTicksSinceStart());
    std::vector<float> startup_times = m_kart_properties->getStartupTime();
    for (unsigned int i = 0; i < startup_times.size(); i++)
    {
        if (t <= startup_times[i])
            return m_kart_properties->getStartupBoost()[i];
    }
    return 0;
}   // getStartupBoost

//-----------------------------------------------------------------------------
/** Simulates gears by adjusting the force of the engine. It also takes the
 *  effect of the zipper into account.
 */
float Kart::getActualWheelForce()
{
    float add_force = m_max_speed->getCurrentAdditionalEngineForce();
    assert(!std::isnan(add_force));
    const std::vector<float>& gear_ratio=m_kart_properties->getGearSwitchRatio();
    for(unsigned int i=0; i<gear_ratio.size(); i++)
    {
        if(m_speed <= m_kart_properties->getEngineMaxSpeed() * gear_ratio[i])
        {
            assert(!std::isnan(m_kart_properties->getEnginePower()));
            assert(!std::isnan(m_kart_properties->getGearPowerIncrease()[i]));
            return m_kart_properties->getEnginePower()
                 * m_kart_properties->getGearPowerIncrease()[i]
                 + add_force;
        }
    }
    assert(!std::isnan(m_kart_properties->getEnginePower()));
    return m_kart_properties->getEnginePower() + add_force * 2;

}   // getActualWheelForce

//-----------------------------------------------------------------------------
/** The kart is on ground if all 4 wheels touch the ground, and if no special
 *  animation (rescue, explosion etc.) is happening).
 */
bool Kart::isOnGround() const
{
    return ((int)m_vehicle->getNumWheelsOnGround() == m_vehicle->getNumWheels()
          && !getKartAnimation());
}   // isOnGround

//-----------------------------------------------------------------------------
/** The kart is near the ground, but not necessarily on it (small jumps). This
 *  is used to determine when to stop flying.
*/
bool Kart::isNearGround() const
{
    if((m_terrain_info->getHitPoint() - getXYZ()).length() ==Track::NOHIT)
        return false;
    else
        return ((getXYZ().getY() - m_terrain_info->getHoT())
                 < stk_config->m_near_ground);
}   // isNearGround

// ------------------------------------------------------------------------
/** Enables a kart shield protection for a certain amount of time.
 */
void Kart::setShieldTime(float t)
{
    if(isShielded())
    {
        getAttachment()->setTicksLeft(stk_config->time2Ticks(t));
    }
}   // setShieldTime

// ------------------------------------------------------------------------
/**
 * Returns true if the kart is protected by a shield.
 */
bool Kart::isShielded() const
{
    if(getAttachment() != NULL)
    {
        Attachment::AttachmentType type = getAttachment()->getType();
        return type == Attachment::ATTACH_BUBBLEGUM_SHIELD ||
               type == Attachment::ATTACH_NOLOK_BUBBLEGUM_SHIELD;
    }
    else
    {
        return false;
    }
}   // isShielded

// ------------------------------------------------------------------------
/**
 *Returns the remaining time the kart is protected by a shield.
 */
float Kart::getShieldTime() const
{
    if (isShielded())
        return stk_config->ticks2Time(getAttachment()->getTicksLeft());
    else
        return 0.0f;
}   // getShieldTime

// ------------------------------------------------------------------------
/**
 * Decreases the kart's shield time.
 * \param t The time substracted from the shield timer. If t == 0.0f, the
             default amout of time is substracted.
 */
void Kart::decreaseShieldTime()
{
    if (isShielded())
    {
        getAttachment()->setTicksLeft(0);
    }
}   // decreaseShieldTime

//-----------------------------------------------------------------------------
/** Shows the star effect for a certain time.
 *  \param t Time to show the star effect for.
 */
void Kart::showStarEffect(float t)
{
    m_stars_effect->showFor(t);
}   // showStarEffect

//-----------------------------------------------------------------------------
void Kart::eliminate()
{
    if (!getKartAnimation())
    {
        Physics::getInstance()->removeKart(this);
    }
    if (m_stars_effect)
    {
        m_stars_effect->reset();
        m_stars_effect->update(1);
    }

    if (m_attachment)
    {
        m_attachment->clear();
    }

    m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_TERRAIN, 0);
    m_kart_gfx->setGFXInvisible();
    if (m_engine_sound)
        m_engine_sound->stop();

    m_eliminated = true;

#ifndef SERVER_ONLY
    if (m_shadow)
    {
        m_shadow->update(false);
    }
#endif
    m_node->setVisible(false);
}   // eliminate

//-----------------------------------------------------------------------------
/** Updates the kart in each time step. It updates the physics setting,
 *  particle effects, camera position, etc.
 *  \param dt Time step size.
 */
void Kart::update(int ticks)
{
    if(m_speed < 5.0f)
        m_body->setRestitution(1.0f);
    else if(m_speed<20.0f)
        m_body->setRestitution(1.0f-(m_speed-5.0f)/15.0f);
    else
        m_body->setRestitution(0.0f);
    // Reset any instand speed increase in the bullet kart
    m_vehicle->setMinSpeed(0);

    // update star effect (call will do nothing if stars are not activated)
    m_stars_effect->update(stk_config->ticks2Time(ticks));

    if(m_squash_ticks>=0)
    {
        m_squash_ticks-=ticks;
        // If squasing time ends, reset the model
        if(m_squash_ticks<=0)
        {
            m_node->setScale(core::vector3df(1.0f, 1.0f, 1.0f));
            scene::ISceneNode* node =
                m_kart_model->getAnimatedNode() ?
                m_kart_model->getAnimatedNode() : m_node;
            if (m_vehicle->getNumWheels() > 0)
            {
                scene::ISceneNode **wheels = m_kart_model->getWheelNodes();
                for (int i = 0; i < 4 && i < m_vehicle->getNumWheels(); ++i)
                {
                    if (wheels[i])
                        wheels[i]->setParent(node);
                }
            }
        }
    }   // if squashed

    if (m_bubblegum_ticks > 0.0f)
    {
        m_bubblegum_ticks -= ticks;
        if (m_bubblegum_ticks <= 0.0f)
        {
            m_bubblegum_torque = 0.0f;
        }
    }

    // This is to avoid a rescue immediately after an explosion
    const bool has_animation_before = m_kart_animation != NULL;
    // A kart animation can change the xyz position. This needs to be done
    // before updating the graphical position (which is done in
    // Moveable::update() ), otherwise 'stuttering' can happen (caused by
    // graphical and physical position not being the same).
    float dt = stk_config->ticks2Time(ticks);
    if (has_animation_before)
    {
        m_kart_animation->update(dt);
    }

    m_time_previous_counter += dt;
    while (m_time_previous_counter > stk_config->ticks2Time(1))
    {
        m_previous_xyz[0] = getXYZ();
        m_previous_xyz_times[0] = World::getWorld()->getTime();
        for (int i=m_xyz_history_size-1;i>0;i--)
        {
            m_previous_xyz[i] = m_previous_xyz[i-1];
            m_previous_xyz_times[i] = m_previous_xyz_times[i-1];
        }
        m_time_previous_counter -= stk_config->ticks2Time(1);
    }

    // Update the position and other data taken from the physics (or
    // an animation which calls setXYZ(), which also updates the kart
    // physical position).
    Moveable::update(ticks);

    Vec3 front(0, 0, getKartLength()*0.5f);
    m_xyz_front = getTrans()(front);
    // Update the locally maintained speed of the kart (m_speed), which 
    // is used furthermore for engine power, camera distance etc
    updateSpeed();

    m_controller->update(ticks);

#ifndef SERVER_ONLY
#undef DEBUG_CAMERA_SHAKE
#ifdef DEBUG_CAMERA_SHAKE
    Log::verbose("camera", "%s t %f %d xyz %f %f %f v %f %f %f d3 %f d2 %f",
        getIdent().c_str(),
        World::getWorld()->getTime(), ticks,
        getXYZ().getX(), getXYZ().getY(), getXYZ().getZ(),
        getVelocity().getX(), getVelocity().getY(), getVelocity().getZ(),
        (Camera::getCamera(0)->getXYZ()-getXYZ()).length(),
        (Camera::getCamera(0)->getXYZ()-getXYZ()).length_2d()
        );
#endif
#endif

#undef DEBUG_TO_COMPARE_KART_PHYSICS
#ifdef DEBUG_TO_COMPARE_KART_PHYSICS
    // This information is useful when comparing kart physics, e.g. to
    // see top speed, acceleration (i.e. time to top speed) etc.
    Log::verbose("physics", "     %s t %f %d xyz(9-11) %f %f %f v(13-15) %f %f %f steerf(17) %f maxangle(19) %f speed(21) %f steering(23-24) %f %f clock %lf",
        getIdent().c_str(),
        World::getWorld()->getTime(), ticks,
        getXYZ().getX(), getXYZ().getY(), getXYZ().getZ(),
        getVelocity().getX(), getVelocity().getY(), getVelocity().getZ(),  //13,14,15
        m_skidding->getSteeringFraction(), //19
        getMaxSteerAngle(),  //20
        m_speed,  //21
        m_vehicle->getWheelInfo(0).m_steering,  //23
        m_vehicle->getWheelInfo(1).m_steering,  //24
        StkTime::getRealTime()
        );
#endif

    // if its view is blocked by plunger, decrease remaining time
    if(m_view_blocked_by_plunger > 0) m_view_blocked_by_plunger -= ticks;
    //unblock the view if kart just became shielded
    if(isShielded())
        m_view_blocked_by_plunger = 0;
    // Decrease remaining invulnerability time
    if(m_invulnerable_ticks>0)
    {
        m_invulnerable_ticks -= ticks;
    }

    m_slipstream->update(ticks);

    // TODO: hiker said this probably will be moved to btKart or so when updating bullet engine.
    // Neutralize any yaw change if the kart leaves the ground, so the kart falls more or less
    // straight after jumping, but still allowing some "boat shake" (roll and pitch).
    // Otherwise many non perfect jumps end in a total roll over or a serious change of
    // direction, sometimes 90 or even full U turn (real but less fun for a karting game).
    // As side effect steering becames a bit less responsive (any wheel on air), but not too bad.
    if(!isOnGround()) {
        btVector3 speed = m_body->getAngularVelocity();
        speed.setX(speed.getX() * 0.95f);
        speed.setY(speed.getY() * 0.25f); // or 0.0f for sharp neutralization of yaw
        speed.setZ(speed.getZ() * 0.95f);
        m_body->setAngularVelocity(speed);
        // This one keeps the kart pointing "100% as launched" instead,
        // like in ski jump sports, too boring but also works.
        //m_body->setAngularVelocity(btVector3(0,0,0));
        // When the kart is jumping, linear damping reduces the falling speed
        // of a kart so much that it can appear to be in slow motion. So
        // disable linear damping if a kart is in the air
        m_body->setDamping(0, m_kart_properties->getStabilityChassisAngularDamping());
    }
    else
    {
        m_body->setDamping(m_kart_properties->getStabilityChassisLinearDamping(),
                           m_kart_properties->getStabilityChassisAngularDamping());
    }

    // Used to prevent creating a rescue animation after an explosion animation
    // got deleted

    m_attachment->update(ticks);

    m_kart_gfx->update(dt);
    if (m_collision_particles) m_collision_particles->update(dt);

    PROFILER_PUSH_CPU_MARKER("Kart::updatePhysics", 0x60, 0x34, 0x7F);
    updatePhysics(ticks);
    PROFILER_POP_CPU_MARKER();

    if(!m_controls.getFire()) m_fire_clicked = 0;

    if(m_controls.getFire() && !m_fire_clicked && !m_kart_animation)
    {
        if (m_powerup->getType() != PowerupManager::POWERUP_NOTHING)
        {
            setLastUsedPowerup(m_powerup->getType());
        }
        // use() needs to be called even if there currently is no collecteable
        // since use() can test if something needs to be switched on/off.
        m_powerup->use() ;
        World::getWorld()->onFirePressed(getController());
        m_fire_clicked = 1;
    }

    /* (TODO: add back when properly done)
    for (int n = 0; n < SFXManager::NUM_CUSTOMS; n++)
    {
        if (m_custom_sounds[n] != NULL) m_custom_sounds[n]->position   ( getXYZ() );
    }
     */

    for (int i = 0; i < EMITTER_COUNT; i++)
        m_emitters[i]->setPosition(getXYZ());

    m_skid_sound->setPosition   ( getXYZ() );
    m_nitro_sound->setPosition  ( getXYZ() );

    // Check if a kart is (nearly) upside down and not moving much -->
    // automatic rescue
    // But only do this if auto-rescue is enabled (i.e. it will be disabled in
    // battle mode), and the material the kart is driving on does not have
    // gravity (which atm affects the roll angle).

    // To be used later
    float dist_to_sector = 0.0f;
    LinearWorld* lw = dynamic_cast<LinearWorld*>(World::getWorld());
    if (lw && DriveGraph::get())
    {
        const int sector =
            lw->getTrackSector(getWorldKartId())->getCurrentGraphNode();
        dist_to_sector = getXYZ().distance
            (DriveGraph::get()->getNode(sector)->getCenter());

        const Vec3& quad_normal = DriveGraph::get()->getNode(sector)
            ->getNormal();
        const btQuaternion& q = getTrans().getRotation();
        const float roll = quad_normal.angle
               ((Vec3(0, 1, 0).rotate(q.getAxis(), q.getAngle())));

        if (Track::getCurrentTrack()->isAutoRescueEnabled() &&
            (!m_terrain_info->getMaterial() ||
            !m_terrain_info->getMaterial()->hasGravity()) &&
            !has_animation_before && fabs(roll) > 60 * DEGREE_TO_RAD &&
            fabs(getSpeed()) < 3.0f)
        {
            new RescueAnimation(this, /*is_auto_rescue*/true);
            m_last_factor_engine_sound = 0.0f;
        }
    }

    // Make sure that the ray doesn't hit the kart. This is done by
    // resetting the collision filter group, so that this collision
    // object is ignored during raycasting.
    short int old_group = 0;
    if(m_body->getBroadphaseHandle())
    {
        old_group = m_body->getBroadphaseHandle()->m_collisionFilterGroup;
        m_body->getBroadphaseHandle()->m_collisionFilterGroup = 0;
    }
#undef XX
#ifdef XX
    Log::verbose("physicsafter", "%s t %f %d xyz(9-11) %f %f %f %f %f %f "
        "v(16-18) %f %f %f steerf(20) %f maxangle(22) %f speed(24) %f "
        "steering(26-27) %f %f clock(29) %lf skidstate(31) %d factor(33) %f "
        "maxspeed(35) %f engf(37) %f braketick(39) %d brakes(41) %d heading(43) %f "
        "noderot(45) %f suslen %f",
        getIdent().c_str(),
        World::getWorld()->getTime(), World::getWorld()->getTimeTicks(),
        getXYZ().getX(), getXYZ().getY(), getXYZ().getZ(),
        m_body->getWorldTransform().getOrigin().getX(),
        m_body->getWorldTransform().getOrigin().getY(),
        m_body->getWorldTransform().getOrigin().getZ(),
        getVelocity().getX(), getVelocity().getY(), getVelocity().getZ(),  //16-18
        m_skidding->getSteeringFraction(), //20
        getMaxSteerAngle(),  //22
        m_speed,  //24
        m_vehicle->getWheelInfo(0).m_steering,  //26
        m_vehicle->getWheelInfo(1).m_steering,  //27
        StkTime::getRealTime(),  //29
        m_skidding->getSkidState(), //31
        m_skidding->getSkidFactor(),    //33
        m_max_speed->getCurrentMaxSpeed(),
        m_max_speed->getCurrentAdditionalEngineForce(),  // 37
        m_brake_ticks, //39
        m_controls.getButtonsCompressed(),  //41
        getHeading(),  //43
        m_node->getAbsoluteTransformation().getRotationDegrees().Y,  //45
        m_vehicle->getWheelInfo(0).m_raycastInfo.m_suspensionLength
    );
#endif
    // After the physics step was done, the position of the wheels (as stored
    // in wheelInfo) is actually outdated, since the chassis was moved
    // according to the force acting from the wheels. So the center of the
    // chassis is not at the center of the wheels anymore, it is somewhat
    // moved forward (depending on speed and fps). In very extreme cases
    // (see bug 2246) the center of the chassis can actually be ahead of the
    // front wheels. So if we do a raycast to detect the terrain from the
    // current chassis, that raycast might be ahead of the wheels - which
    // results in incorrect rescues (the wheels are still on the ground,
    // but the raycast happens ahead of the front wheels and are over
    // a rescue texture).
    // To avoid this problem, we do the raycast for terrain detection from
    // the center of the 4 wheel positions (in world coordinates).

    Vec3 from(0, 0, 0);
    for (unsigned int i = 0; i < 4; i++)
        from += m_vehicle->getWheelInfo(i).m_raycastInfo.m_hardPointWS;

    // Add a certain epsilon (0.3) to the height of the kart. This avoids
    // problems of the ray being cast from under the track (which happened
    // e.g. on tux tollway when jumping down from the ramp, when the chassis
    // partly tunnels through the track). While tunneling should not be
    // happening (since Z velocity is clamped), the epsilon is left in place
    // just to be on the safe side (it will not hit the chassis itself).
    from = from/4 + (getTrans().getBasis() * Vec3(0,0.3f,0));

    m_terrain_info->update(getTrans().getBasis(), from);

    if(m_body->getBroadphaseHandle())
    {
        m_body->getBroadphaseHandle()->m_collisionFilterGroup = old_group;
    }

    PROFILER_PUSH_CPU_MARKER("Kart::Update (material)", 0x60, 0x34, 0x7F);
    const Material* material=m_terrain_info->getMaterial();
    if (!material)   // kart falling off the track
    {
        if (!m_flying)
        {
            float g = Track::getCurrentTrack()->getGravity();
            Vec3 gravity(0, -g, 0);
            btRigidBody *body = getVehicle()->getRigidBody();
            body->setGravity(gravity);
        }
        // let kart fall a bit before rescuing
        const Vec3 *min, *max;
        Track::getCurrentTrack()->getAABB(&min, &max);

        if((min->getY() - getXYZ().getY() > 17 || dist_to_sector > 25) && !m_flying &&
           !getKartAnimation())
        {
            new RescueAnimation(this);
            m_last_factor_engine_sound = 0.0f;
        }
    }
    else
    {
        if (!m_flying)
        {
            float g = Track::getCurrentTrack()->getGravity();
            Vec3 gravity(0.0f, -g, 0.0f);
            btRigidBody *body = getVehicle()->getRigidBody();
            // If the material should overwrite the gravity,
            if (material->hasGravity())
            {
                Vec3 normal = m_terrain_info->getNormal();
                gravity = normal * -g;
            }
            body->setGravity(gravity);
        }   // if !flying
        if     (material->isDriveReset() && isOnGround())
        {
            new RescueAnimation(this);
            m_last_factor_engine_sound = 0.0f;
        }
        else if(material->isZipper()     && isOnGround())
        {
            handleZipper(material);
            showZipperFire();
        }
        else
        {
            m_max_speed->setSlowdown(MaxSpeed::MS_DECREASE_TERRAIN,
                                     material->getMaxSpeedFraction(),
                                     material->getSlowDownTicks()    );
#ifdef DEBUG
            if(UserConfigParams::m_material_debug)
            {
                Log::info("Kart","World %d %s\tfraction %f\ttime %d.",
                       World::getWorld()->getTimeTicks(),
                       material->getTexFname().c_str(),
                       material->getMaxSpeedFraction(),
                       material->getSlowDownTicks()       );
            }
#endif
        }
    }   // if there is material
    PROFILER_POP_CPU_MARKER();

    ItemManager::get()->checkItemHit(this);

    const bool emergency = getKartAnimation()!=NULL;

    if (emergency)
    {
        m_view_blocked_by_plunger = 0;
        if (m_flying)
        {
            stopFlying();
            m_flying = false;
        }
    }

    // Remove the shadow if the kart is not on the ground (if a kart
    // is rescued isOnGround might still be true, since the kart rigid
    // body was removed from the physics, but still retain the old
    // values for the raycasts).
    if (!isOnGround() && !getKartAnimation())
    {
        const Material *m      = getMaterial();
        const Material *last_m = getLastMaterial();

        // A jump starts only the kart isn't already jumping, is on a new
        // (or no) texture.
        if (!m_is_jumping && last_m && last_m != m &&
            m_kart_model->getAnimation() == KartModel::AF_DEFAULT)
        {
            float v = getVelocity().getY();
            float force = Track::getCurrentTrack()->getGravity();
            // Velocity / force is the time it takes to reach the peak
            // of the jump (i.e. when vertical speed becomes 0). Assuming
            // that jump start height and end height are the same, it will
            // take the same time again to reach the bottom
            float t = 2.0f * v/force;

            // Jump if either the jump is estimated to be long enough, or
            // the texture has the jump property set.
            if (t > m_kart_properties->getJumpAnimationTime() ||
                last_m->isJumpTexture())
            {
                m_kart_model->setAnimation(KartModel::AF_JUMP_START);
            }

            m_is_jumping = true;
        }
    }
    else if (m_is_jumping)
    {
        // Kart touched ground again
        m_is_jumping = false;
        m_kart_model->setAnimation(KartModel::AF_DEFAULT);

        if (!getKartAnimation())
        {
            HitEffect *effect =  new Explosion(getXYZ(), "jump",
                                              "jump_explosion.xml");
            projectile_manager->addHitEffect(effect);
        }
    }

}   // update

//-----------------------------------------------------------------------------
void Kart::handleRewoundTransform()
{
    if (!m_controller->isLocalPlayerController())
    {
        if (RewindManager::get()->isRewinding())
            m_rewound_transforms.push_back(getTrans());
        else if (!m_rewound_transforms.empty())
        {
            setTrans(m_rewound_transforms.front());
            m_rewound_transforms.pop_front();
        }
    }
}   // handleRewoundTransform

//-----------------------------------------------------------------------------
/** Updates the local speed based on the current physical velocity. The value
 *  is smoothed exponentially to avoid camera stuttering (camera distance
 *  is dependent on speed)
 */
void Kart::updateSpeed()
{
    // Compute the speed of the kart. Smooth it with previous speed to make
    // the camera smoother (because of capping the speed in m_max_speed
    // the speed value jitters when approaching maximum speed. This results
    // in the distance between kart and camera to jitter as well (typically
    // only in the order of centimetres though). Smoothing the speed value
    // gets rid of this jitter, and also r
    m_speed = getVehicle()->getRigidBody()->getLinearVelocity().length();

    // calculate direction of m_speed
    const btTransform& chassisTrans = getVehicle()->getChassisWorldTransform();
    btVector3 forwardW(
        chassisTrans.getBasis()[0][2],
        chassisTrans.getBasis()[1][2],
        chassisTrans.getBasis()[2][2]);

    // In theory <0 should be sufficient, but floating point errors can cause
    // flipping from +eps to -eps and back, resulting in animation flickering
    // if the kart has backpedal animations.
    if (forwardW.dot(getVehicle()->getRigidBody()->getLinearVelocity()) 
        < btScalar(-0.01f))
    {
        m_speed = -m_speed;
    }

    float f = 0.3f;
    m_smoothed_speed = f*m_speed + (1.0f - f)*m_smoothed_speed;

    // At low velocity, forces on kart push it back and forth so we ignore this
    // - quick'n'dirty workaround for bug 1776883
    if (fabsf(m_speed) < 0.2f                                   ||
        dynamic_cast<RescueAnimation*>   ( getKartAnimation() ) ||
        dynamic_cast<ExplosionAnimation*>( getKartAnimation() )    )
    {
        m_speed          = 0;
        m_smoothed_speed = 0;
    }
}   // updateSpeed

//-----------------------------------------------------------------------------
/** Show fire to go with a zipper.
 */
void Kart::showZipperFire()
{
    m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_ZIPPER, 800.0f);
}

//-----------------------------------------------------------------------------
/** Squashes this kart: it will scale the kart in up direction, and causes
 *  a slowdown while this kart is squashed.
 *  \param time How long the kart will be squashed. A value of 0 will reset
 *         the kart to be unsquashed.
 *  \param slowdown Reduction of max speed.
 */
void Kart::setSquash(float time, float slowdown)
{
    if (isInvulnerable()) return;

    if (isShielded())
    {
        decreaseShieldTime();
        return;
    }

    if(m_attachment->getType()==Attachment::ATTACH_BOMB && time>0)
    {
        ExplosionAnimation::create(this);
        return;
    }
    m_node->setScale(core::vector3df(1.0f, 0.5f, 1.0f));
    m_max_speed->setSlowdown(MaxSpeed::MS_DECREASE_SQUASH, slowdown,
                             stk_config->time2Ticks(0.1f), 
                             stk_config->time2Ticks(time));
    if (m_vehicle->getNumWheels() > 0)
    {
        if (!m_wheel_box)
            m_wheel_box = irr_driver->getSceneManager()->addDummyTransformationSceneNode(m_node);
        scene::ISceneNode **wheels = m_kart_model->getWheelNodes();
        for (int i = 0; i < 4 && i < m_vehicle->getNumWheels(); ++i)
        {
            if (wheels[i])
                wheels[i]->setParent(m_wheel_box);
        }
        m_wheel_box->getRelativeTransformationMatrix().setScale(core::vector3df(1.0f, 2.0f, 1.0f));
    }
    m_squash_ticks  = stk_config->time2Ticks(time);
}   // setSquash

//-----------------------------------------------------------------------------
/** Plays any terrain specific sound effect.
 */
void Kart::handleMaterialSFX()
{
    // If a terrain specific sfx is already being played, when a new
    // terrain is entered, an old sfx should be finished (once, not
    // looped anymore of course). The m_terrain_sound is then copied
    // to a m_previous_terrain_sound, for which looping is disabled.
    // In case that three sfx needed to be played (i.e. a previous is
    // playing, a current is playing, and a new terrain with sfx is
    // entered), the oldest (previous) sfx is stopped and deleted.

    // FIXME: if there are already two sfx playing, don't add another
    // one. This should reduce the performance impact when driving 
    // on the bridge in Cocoa.
    const Material *material = m_terrain_info->getMaterial();

    // We can not use getLastMaterial() since, since the last material might
    // be updated several times during the physics updates, not indicating
    // that we have reached a new material with regards to the sound effect.
    // So we separately save the material last used for a sound effect and
    // then use this for comparison.
    if(m_last_sound_material!=material)
    {
        // First stop any previously playing terrain sound
        // and remove it, so that m_previous_terrain_sound
        // can be used again.
        if(m_previous_terrain_sound)
        {
            m_previous_terrain_sound->deleteSFX();
        }

        // Disable looping for the current terrain sound, and
        // make it the previous terrain sound.
        if (m_terrain_sound) m_terrain_sound->setLoop(false);
        m_previous_terrain_sound = m_terrain_sound;

        const std::string &sound_name = material ? material->getSFXName() : "";

        // In multiplayer mode sounds are NOT positional, because we have
        // multiple listeners. This would make the sounds of all AIs be
        // audible at all times. So silence AI karts.
        if (!sound_name.empty() && (race_manager->getNumPlayers()==1 ||
                                    m_controller->isLocalPlayerController() ) )
        {
            m_terrain_sound = SFXManager::get()->createSoundSource(sound_name);
            m_terrain_sound->play();
            m_terrain_sound->setLoop(true);
        }
        else
        {
            m_terrain_sound = NULL;
        }
    }

    // Check if a previous terrain sound (now not looped anymore)
    // is finished and can be deleted.
    if(m_previous_terrain_sound &&
        m_previous_terrain_sound->getStatus()==SFXBase::SFX_STOPPED)
    {
        // We don't modify the position of m_previous_terrain_sound
        // anymore, so that it keeps on playing at the place where the
        // kart left the material.
        m_previous_terrain_sound->deleteSFX();
        m_previous_terrain_sound = NULL;
    }

    bool m_schedule_pause = m_flying ||
                        dynamic_cast<RescueAnimation*>(getKartAnimation()) ||
                        dynamic_cast<ExplosionAnimation*>(getKartAnimation());

    // terrain sound is not necessarily a looping sound so check its status before
    // setting its speed, to avoid 'ressuscitating' sounds that had already stopped
    if(m_terrain_sound && 
        (m_terrain_sound->getStatus()==SFXBase::SFX_PLAYING ||
         m_terrain_sound->getStatus()==SFXBase::SFX_PAUSED)    )
    {
        m_terrain_sound->setPosition(getXYZ());
        if(material)
            material->setSFXSpeed(m_terrain_sound, m_speed, m_schedule_pause);
    }

    m_last_sound_material = material;
}   // handleMaterialSFX

//-----------------------------------------------------------------------------
/** Handles material specific GFX, mostly particle effects. Particle
 *  effects can be triggered by two different situations: either
 *  because a kart drives on top of a terrain with a special effect,
 *  or because the kart is driving or falling under a surface (e.g.
 *  water), and the terrain effect is coming from that surface. Those
 *  effects are exclusive - you either get the effect from the terrain
 *  you are driving on, or the effect from a surface the kart is
 *  (partially) under. The surface effect is triggered, if either the
 *  kart is falling, or if the surface the kart is driving on has
 *  the 'isBelowSurface' property set. This function is called once
 *  per rendered frame from updateGraphics().
 *  \param dt Time step size.
 */
void Kart::handleMaterialGFX(float dt)
{
    const Material *material = getMaterial();

    // First test: give the terrain effect, if the kart is
    // on top of a surface (i.e. not falling), actually touching
    // something with the wheels, and the material has not the
    // below surface property set.
    if (material && isOnGround() && !material->isBelowSurface() &&
        !getKartAnimation()      && UserConfigParams::m_particles_effects > 1)
    {

        // Get the appropriate particle data depending on
        // wether the kart is skidding or driving.
        const ParticleKind* pk =
            material->getParticlesWhen(m_skidding->isSkidding()
                                                    ? Material::EMIT_ON_SKID
                                                    : Material::EMIT_ON_DRIVE);
        if(!pk)
        {
            // Disable potentially running particle effects
            m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_TERRAIN, 0);
            return;  // no particle effect, return
        }
        m_kart_gfx->updateTerrain(pk);
        return;
    }

    // Now the kart is either falling, or driving on a terrain which
    // has the 'below surface' flag set. Detect if there is a surface
    // on top of the kart.
    // --------------------------------------------------------------
    if (m_controller->isLocalPlayerController() && !hasFinishedRace())
    {
        bool falling = material && material->hasFallingEffect() && !m_flying;
        if (falling)
        {
            m_falling_time -= dt;
            if (m_falling_time < 0)
                m_falling_time = 0;
        }
        else
            m_falling_time = 0.35f;

        for(unsigned int i=0; i<Camera::getNumCameras(); i++)
        {
            Camera *camera = Camera::getCamera(i);
            if(camera->getKart()!=this) continue;

            if (falling && m_falling_time <= 0)
            {
                camera->setMode(Camera::CM_FALLING);
            }
            else if (camera->getMode() != Camera::CM_NORMAL &&
                     camera->getMode() != Camera::CM_REVERSE)
            {
                camera->setMode(Camera::CM_NORMAL);
            }
        }   // for i in all cameras for this kart
    }   // camera != final camera

    if (UserConfigParams::m_particles_effects < 2)
        return;

    // Use the middle of the contact points of the two rear wheels
    // as the point from which to cast the ray upwards
    const btWheelInfo::RaycastInfo &ri2 =
        getVehicle()->getWheelInfo(2).m_raycastInfo;
    const btWheelInfo::RaycastInfo &ri3 =
        getVehicle()->getWheelInfo(3).m_raycastInfo;
    Vec3 from = (ri2.m_contactPointWS + ri3.m_contactPointWS)*0.5f;
    Vec3 xyz;
    const Material *surface_material;
    if(!m_terrain_info->getSurfaceInfo(from, &xyz, &surface_material))
    {
        m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_TERRAIN, 0);
        return;
    }
    const ParticleKind *pk =
        surface_material->getParticlesWhen(Material::EMIT_ON_DRIVE);

    if(!pk || m_flying || dynamic_cast<RescueAnimation*>(getKartAnimation()))
        return;

    // Now the kart is under a surface, and there is a surface effect
    // --------------------------------------------------------------
    m_kart_gfx->setParticleKind(KartGFX::KGFX_TERRAIN, pk);
    m_kart_gfx->setXYZ(KartGFX::KGFX_TERRAIN, xyz);

    const float distance = xyz.distance2(from);
    float ratio;
    if      (distance < 2.0f) ratio = 1.0f;
    else if (distance < 4.0f) ratio = (4.0f-distance)*0.5f;
    else                      ratio = -1.0f;  // No more particles
    m_kart_gfx->setCreationRateRelative(KartGFX::KGFX_TERRAIN, ratio);

    // Play special sound effects for this terrain
    // -------------------------------------------
    const std::string &s = surface_material->getSFXName();
    if (s != "" && !dynamic_cast<RescueAnimation*>(getKartAnimation())&&
        (m_terrain_sound == NULL ||
         m_terrain_sound->getStatus() == SFXBase::SFX_STOPPED))
    {
        if (m_previous_terrain_sound) m_previous_terrain_sound->deleteSFX();
        m_previous_terrain_sound = m_terrain_sound;
        if(m_previous_terrain_sound)
            m_previous_terrain_sound->setLoop(false);

        m_terrain_sound = SFXManager::get()->createSoundSource(s);
        m_terrain_sound->play();
        m_terrain_sound->setLoop(false);
    }

}   // handleMaterialGFX

//-----------------------------------------------------------------------------
/** Sets zipper time, and apply one time additional speed boost. It can be
 *  used with a specific material, in which case the zipper parmaters are
 *  taken from this material (parameters that are <0 will be using the
 *  kart-specific values from kart-properties.
 *  \param material If not NULL, will be used to determine the zipper
 *                  parameters, otherwise the defaults from kart properties
 *                  will be used.
 * \param play_sound If true this will cause a sfx to be played even if the
 *                  terrain hasn't changed. It is used by the zipper powerup.
 */
void Kart::handleZipper(const Material *material, bool play_sound)
{
    /** The additional speed allowed on top of the kart-specific maximum kart
     *  speed. */
    float max_speed_increase;

    /**Time the zipper stays activated. */
    float duration;
    /** A one time additional speed gain - the kart will instantly add this
     *  amount of speed to its current speed. */
    float speed_gain;
    /** Time it takes for the zipper advantage to fade out. */
    float fade_out_time;
    /** Additional engine force. */
    float engine_force;

    if(material)
    {
        material->getZipperParameter(&max_speed_increase, &duration,
                                     &speed_gain, &fade_out_time, &engine_force);
        if(max_speed_increase<0)
            max_speed_increase = m_kart_properties->getZipperMaxSpeedIncrease();
        if(duration<0)
            duration           = m_kart_properties->getZipperDuration();
        if(speed_gain<0)
            speed_gain         = m_kart_properties->getZipperSpeedGain();
        if(fade_out_time<0)
            fade_out_time      = m_kart_properties->getZipperFadeOutTime();
        if(engine_force<0)
            engine_force       = m_kart_properties->getZipperForce();
    }
    else
    {
        max_speed_increase = m_kart_properties->getZipperMaxSpeedIncrease();
        duration           = m_kart_properties->getZipperDuration();
        speed_gain         = m_kart_properties->getZipperSpeedGain();
        fade_out_time      = m_kart_properties->getZipperFadeOutTime();
        engine_force       = m_kart_properties->getZipperForce();
    }
    // Ignore a zipper that's activated while braking
    if(m_controls.getBrake() || m_speed<0) return;

    m_max_speed->instantSpeedIncrease(MaxSpeed::MS_INCREASE_ZIPPER,
                                     max_speed_increase, speed_gain,
                                     engine_force,
                                     stk_config->time2Ticks(duration),
                                     stk_config->time2Ticks(fade_out_time));
    // Play custom character sound (weee!)
    playCustomSFX(SFXManager::CUSTOM_ZIPPER);
    m_controller->handleZipper(play_sound);
}   // handleZipper

// -----------------------------------------------------------------------------
/** Updates the current nitro status.
 *  \param ticks Number of physics time steps - should be 1.
 */
void Kart::updateNitro(int ticks)
{
    if (m_controls.getNitro() && m_min_nitro_ticks <= 0)
    {
        m_min_nitro_ticks = m_kart_properties->getNitroMinConsumptionTicks();
    }
    if (m_min_nitro_ticks > 0)
    {
        m_min_nitro_ticks -= ticks;

        // when pressing the key, don't allow the min time to go under zero.
        // If it went under zero, it would be reset
        // As the time deduction happens before, it can be an arbitrarily
        // small number > 0. Smaller means more responsive controls.
        if (m_controls.getNitro() && m_min_nitro_ticks <= 0)
            m_min_nitro_ticks = 1;
    }

    bool increase_speed = (m_min_nitro_ticks > 0 && isOnGround());
    if (!increase_speed && m_min_nitro_ticks <= 0)
    {
        if (m_nitro_sound->getStatus() == SFXBase::SFX_PLAYING)
            m_nitro_sound->stop();
        return;
    }

    float dt = stk_config->ticks2Time(ticks);
    m_collected_energy -= dt * m_kart_properties->getNitroConsumption();
    if (m_collected_energy < 0)
    {
        if(m_nitro_sound->getStatus() == SFXBase::SFX_PLAYING)
            m_nitro_sound->stop();
        m_collected_energy = 0;
        return;
    }

    if (increase_speed)
    {
        if(m_nitro_sound->getStatus() != SFXBase::SFX_PLAYING)
            m_nitro_sound->play();
        m_max_speed->increaseMaxSpeed(MaxSpeed::MS_INCREASE_NITRO,
            m_kart_properties->getNitroMaxSpeedIncrease(),
            m_kart_properties->getNitroEngineForce(),
            stk_config->time2Ticks(m_kart_properties->getNitroDuration()),
            stk_config->time2Ticks(m_kart_properties->getNitroFadeOutTime()));
    }
    else
    {
        if(m_nitro_sound->getStatus() == SFXBase::SFX_PLAYING)
            m_nitro_sound->stop();
    }
}   // updateNitro

// -----------------------------------------------------------------------------
/** Activates a slipstream effect */
void Kart::setSlipstreamEffect(float f)
{
    m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_ZIPPER, f);
}   // setSlipstreamEffect

// -----------------------------------------------------------------------------
/** Called when the kart crashes against another kart.
 *  \param k The kart that was hit.
 *  \param update_attachments If true the attachment of this kart and the
 *          other kart hit will be updated (e.g. bombs will be moved)
 */
void Kart::crashed(AbstractKart *k, bool update_attachments)
{
    if(update_attachments)
    {
        assert(k);
        getAttachment()->handleCollisionWithKart(k);
    }
    m_controller->crashed(k);
    playCrashSFX(NULL, k);
}   // crashed(Kart, update_attachments

// -----------------------------------------------------------------------------
/** Kart hits the track with a given material.
 *  \param m Material hit, can be NULL if no specific material exists.
 *  \param normal The normal of the hit (used to push a kart back, which avoids
 *         that karts sometimes can get stuck).
 */
void Kart::crashed(const Material *m, const Vec3 &normal)
{
    playCrashSFX(m, NULL);
#ifdef DEBUG
    // Simple debug output for people playing without sound.
    // This makes it easier to see if a kart hit the track (esp.
    // after a jump).
    // FIXME: This should be removed once the physics are fixed.
    if(UserConfigParams::m_physics_debug)
    {
        // Add a counter to make it easier to see if a new line of
        // output was added.
        static int counter=0;
        Log::info("Kart","Kart %s hit track: %d material %s.",
               getIdent().c_str(), counter++,
               m ? m->getTexFname().c_str() : "None");
    }
#endif

    const LinearWorld *lw = dynamic_cast<LinearWorld*>(World::getWorld());
    if(m_kart_properties->getTerrainImpulseType()
                             ==KartProperties::IMPULSE_NORMAL &&
        m_vehicle->getCentralImpulseTime()<=0                     )
    {
        // Restrict impule to plane defined by gravity (i.e. X/Z plane).
        // This avoids the problem that karts can be pushed up, e.g. above
        // a fence.
        btVector3 gravity = m_body->getGravity();
        gravity.normalize();
        Vec3 impulse =  normal - gravity* btDot(normal, gravity);
        if(impulse.getX() || impulse.getZ())
            impulse.normalize();
        else
            impulse = Vec3(0, 0, -1); // Arbitrary
        // Impulse depends of kart speed - and speed can be negative
        // If the speed is too low, karts can still get stuck into a wall
        // so make sure there is always enough impulse to avoid this
        float abs_speed = fabsf(getSpeed());
        impulse *= ( abs_speed<10 ? 10.0f : sqrt(abs_speed) )
                 * m_kart_properties->getCollisionTerrainImpulse();
        m_bounce_back_ticks = 0;
        impulse = Vec3(0, 0, 0);
        //m_vehicle->setTimedCentralImpulse(0.1f, impulse);
        m_vehicle->setTimedCentralImpulse(0.0, impulse);
    }
    // If there is a quad graph, push the kart towards the previous
    // graph node center (we have to use the previous point since the
    // kart might have only now reached the new quad, meaning the kart
    // would be pushed forward).
    else if(m_kart_properties->getTerrainImpulseType()
                                 ==KartProperties::IMPULSE_TO_DRIVELINE &&
            lw && m_vehicle->getCentralImpulseTime()<=0 &&
            Track::getCurrentTrack()->isPushBackEnabled())
    {
        int sector = lw->getSectorForKart(this);
        if(sector!=Graph::UNKNOWN_SECTOR)
        {
            // Use the first predecessor node, which is the most
            // natural one (i.e. the one on the main driveline).
            const DriveNode* dn = DriveGraph::get()->getNode(
                DriveGraph::get()->getNode(sector)->getPredecessor(0));
            Vec3 impulse = dn->getCenter() - getXYZ();
            impulse.setY(0);
            if(impulse.getX() || impulse.getZ())
                impulse.normalize();
            else
                impulse = Vec3(0, 0, -1); // Arbitrary
            impulse *= m_kart_properties->getCollisionTerrainImpulse();
            m_bounce_back_ticks = stk_config->time2Ticks(0.2f);
            m_vehicle->setTimedCentralImpulse(0.1f, impulse);
        }

    }
    /** If a kart is crashing against the track, the collision is often
     *  reported more than once, resulting in a machine gun effect, and too
     *  long disabling of the engine. Therefore, this reaction is disabled
     *  for 0.5 seconds after a crash.
     */
    if(m && m->getCollisionReaction() != Material::NORMAL &&
        !getKartAnimation())
    {
#ifndef SERVER_ONLY
        std::string particles = m->getCrashResetParticles();
        if (particles.size() > 0 && UserConfigParams::m_particles_effects > 0)
        {
            ParticleKind* kind =
                ParticleKindManager::get()->getParticles(particles);
            if (kind != NULL)
            {
                if (m_collision_particles == NULL)
                {
                    Vec3 position(-getKartWidth()*0.35f, 0.06f,
                                  getKartLength()*0.5f);
                    m_collision_particles  =
                        new ParticleEmitter(kind, position, getNode());
                }
                else
                {
                    m_collision_particles->setParticleType(kind);
                }
            }
            else
            {
                Log::error("Kart","Unknown particles kind <%s> in material "
                                "crash-reset properties\n", particles.c_str());
            }
        }
#endif
        if (m->getCollisionReaction() == Material::RESCUE)
        {
            new RescueAnimation(this);
            m_last_factor_engine_sound = 0.0f;
        }
        else if (m->getCollisionReaction() == Material::PUSH_BACK)
        {
            // This variable is set to 0.2 in case of a kart-terrain collision
            if (m_bounce_back_ticks <= stk_config->time2Ticks(0.2f))
            {
                btVector3 push = m_body->getLinearVelocity().normalized();
                push[1] = 0.1f;
                m_body->applyCentralImpulse( -4000.0f*push );
                m_bounce_back_ticks = stk_config->time2Ticks(2.0f);
            }   // if m_bounce_back_ticks <= 0.2f
        }   // if (m->getCollisionReaction() == Material::PUSH_BACK)
    }   // if(m && m->getCollisionReaction() != Material::NORMAL &&
        //   !getKartAnimation())
    m_controller->crashed(m);
}   // crashed(Material)

// -----------------------------------------------------------------------------
/** Common code used when a kart or a material was hit.
 * @param m The material collided into, or NULL if none
 * @param k The kart collided into, or NULL if none
 */
void Kart::playCrashSFX(const Material* m, AbstractKart *k)
{
    int ticks_since_start = World::getWorld()->getTicksSinceStart();
    if(ticks_since_start-m_ticks_last_crash < 60) return;

    m_ticks_last_crash = ticks_since_start;
    // After a collision disable the engine for a short time so that karts
    // can 'bounce back' a bit (without this the engine force will prevent
    // karts from bouncing back, they will instead stuck towards the obstable).
    if(m_bounce_back_ticks <= 0)
    {
        if (getVelocity().length()> 0.555f)
        {
            const float speed_for_max_volume = 15; //The speed at which the sound plays at maximum volume
            const float max_volume = 1; //The maximum volume a sound is played at 
            const float min_volume = 0.2f; //The minimum volume a sound is played at 
            
            float volume; //The volume the crash sound will be played at
            
            if (k == NULL) //Collision with wall
            {
                volume = sqrt( abs(m_speed / speed_for_max_volume));
            }
            else
            {
                const Vec3 ThisKartVelocity = getVelocity();
                const Vec3 OtherKartVelocity = k->getVelocity();
                const Vec3 VelocityDifference = ThisKartVelocity - OtherKartVelocity;
                const float LengthOfDifference = VelocityDifference.length();
            
                volume = sqrt( abs(LengthOfDifference / speed_for_max_volume));
            }
            
            if (volume > max_volume) { volume = max_volume; }
            else if (volume < min_volume) { volume = min_volume; }

            SFXBase* crash_sound_emitter = getNextEmitter();
            crash_sound_emitter->setVolume(volume);
            
            // In case that the sfx is longer than 0.5 seconds, only play it if
            // it's not already playing.
            if (isShielded() || (k != NULL && k->isShielded()))
            {
                crash_sound_emitter->play(getXYZ(), m_boing_sound);
            }
            else
            {
                int idx = rand() % CRASH_SOUND_COUNT;

                SFXBuffer* buffer = m_crash_sounds[idx];
                crash_sound_emitter->play(getXYZ(), buffer);
            }
        }    // if lin_vel > 0.555
    }   // if m_bounce_back_ticks <= 0
}   // playCrashSFX

// -----------------------------------------------------------------------------
/** Plays a beep sfx.
 */
void Kart::beep()
{
    // If the custom horn can't play (isn't defined) then play the default one
    if (!playCustomSFX(SFXManager::CUSTOM_HORN))
    {
        getNextEmitter()->play(getXYZ(), m_horn_sound);
    }

} // beep

// -----------------------------------------------------------------------------
/*
    playCustomSFX()

    This function will play a particular character voice for this kart.  It
    returns whether or not a character voice sample exists for the particular
    event.  If there is no voice sample, a default can be played instead.

    Use entries from the CustomSFX enumeration as a parameter (see
    SFXManager::get().hpp).  eg. playCustomSFX(SFXManager::CUSTOM_CRASH)

    Obviously we don't want a certain character voicing multiple phrases
    simultaneously.  It just sounds bad.  There are two ways of avoiding this:

    1.  If there is already a voice sample playing for the character
        don't play another until it is finished.

    2.  If there is already a voice sample playing for the character
        stop the sample, and play the new one.

    Currently we're doing #2.

    rforder

*/

bool Kart::playCustomSFX(unsigned int type)
{
    // (TODO: add back when properly done)
    return false;

    /*
    bool ret = false;

    // Stop all other character voices for this kart before playing a new one
    // we don't want overlapping phrases coming from the same kart
    for (unsigned int n = 0; n < SFXManager::NUM_CUSTOMS; n++)
    {
        if (m_custom_sounds[n] != NULL)
        {
            // If the sound we're trying to play is already playing
            // don't stop it, we'll just let it finish.
            if (type != n) m_custom_sounds[n]->stop();
        }
    }

    if (type < SFXManager::NUM_CUSTOMS)
    {
        if (m_custom_sounds[type] != NULL)
        {
            ret = true;
            //printf("Kart SFX: playing %s for %s.\n",
            //    SFXManager::get()->getCustomTagName(type),
            //    m_kart_properties->getIdent().c_str());
            // If it's already playing, let it finish
            if (m_custom_sounds[type]->getStatus() != SFXManager::SFX_PLAYING)
            {
                m_custom_sounds[type]->play();
            }
        }
    }
    return ret;
     */
}
// ----------------------------------------------------------------------------
/** Updates the physics for this kart: computing the driving force, set
 *  steering, handles skidding, terrain impact on kart, ...
 *  \param ticks Number if physics time steps - should be 1.
 */
void Kart::updatePhysics(int ticks)
{
    // Check if accel is pressed for the first time. The actual timing
    // is done in getStartupBoost - it returns 0 if the start was actually
    // too slow to qualify for a boost.
    if(!m_has_started && m_controls.getAccel())
    {
        m_has_started = true;
        float f       = getStartupBoost();
        if(f >= 0.0f)
        {
            m_kart_gfx->setCreationRateAbsolute(KartGFX::KGFX_ZIPPER, 100*f);
            m_max_speed->instantSpeedIncrease(MaxSpeed::MS_INCREASE_ZIPPER,
                            0.9f*f, f,
                            /*engine_force*/200.0f,
                            /*duration*/stk_config->time2Ticks(5.0f),
                            /*fade_out_time*/stk_config->time2Ticks(5.0f));
        }
    }

    m_bounce_back_ticks-=ticks;

    updateEnginePowerAndBrakes(ticks);

    // apply flying physics if relevant
    if (m_flying)
        updateFlying();

    m_skidding->update(ticks, isOnGround(), m_controls.getSteer(),
                       m_controls.getSkidControl());
    if( ( m_skidding->getSkidState() == Skidding::SKID_ACCUMULATE_LEFT ||
          m_skidding->getSkidState() == Skidding::SKID_ACCUMULATE_RIGHT  ) &&
       !m_skidding->isJumping()                                              )
    {
        if(m_skid_sound->getStatus()!=SFXBase::SFX_PLAYING && !isWheeless())
            m_skid_sound->play(getXYZ());
    }
    else if(m_skid_sound->getStatus()==SFXBase::SFX_PLAYING)
    {
        m_skid_sound->stop();
    }

    float steering = getMaxSteerAngle() * m_skidding->getSteeringFraction();
    m_vehicle->setSteeringValue(steering, 0);
    m_vehicle->setSteeringValue(steering, 1);

    updateSliding();

    // Cap speed if necessary
    const Material *m = getMaterial();

    float min_speed =  m && m->isZipper() ? m->getZipperMinSpeed() : -1.0f;
    m_max_speed->setMinSpeed(min_speed);
    m_max_speed->update(ticks);

#ifdef XX
    Log::info("Kart","angVel %f %f %f heading %f suspension %f %f %f %f"
       ,m_body->getAngularVelocity().getX()
       ,m_body->getAngularVelocity().getY()
       ,m_body->getAngularVelocity().getZ()
       ,getHeading()
       ,m_vehicle->getWheelInfo(0).m_raycastInfo.m_suspensionLength
       ,m_vehicle->getWheelInfo(1).m_raycastInfo.m_suspensionLength
       ,m_vehicle->getWheelInfo(2).m_raycastInfo.m_suspensionLength
       ,m_vehicle->getWheelInfo(3).m_raycastInfo.m_suspensionLength
       );
#endif

}   // updatephysics

//-----------------------------------------------------------------------------
/** Adjust the engine sound effect depending on the speed of the kart. This 
 *  is called during updateGraphics, i.e. once per rendered frame only.
 *  \param dt Time step size.
 */
void Kart::updateEngineSFX(float dt)
{
    // Only update SFX during the last substep (otherwise too many SFX commands
    // in one frame), and if sfx are enabled
    if(!m_engine_sound || !SFXManager::get()->sfxAllowed()  )
        return;

    // when going faster, use higher pitch for engine
    if(isOnGround())
    {
        float max_speed = m_kart_properties->getEngineMaxSpeed();

        // Engine noise is based half in total speed, half in fake gears:
        // With a sawtooth graph like /|/|/| we get 3 even spaced gears,
        // ignoring the gear settings from stk_config, but providing a
        // good enough brrrBRRRbrrrBRRR sound effect. Speed factor makes
        // it a "staired sawtooth", so more acoustically rich.
        float f = max_speed > 0 ? m_speed/max_speed : 1.0f;
        // Speed at this stage is not yet capped, reduce the amount beyond 1
        if (f> 1.0f) f = 1.0f + (1.0f-1.0f/f);

        float fc = f;
        if (fc>1.0f) fc = 1.0f;
        float gears = 3.0f * fmod(fc, 0.333334f);
        assert(!std::isnan(f));
        m_last_factor_engine_sound = (0.9f*f + gears) * 0.35f;
        m_engine_sound->setSpeedPosition(0.6f + m_last_factor_engine_sound, getXYZ());
    }
    else
      {
        // When flying, reduce progressively the sound engine (since we can't accelerate)
        m_last_factor_engine_sound *= (1.0f-0.1f*dt);
        m_engine_sound->setSpeedPosition(0.6f + m_last_factor_engine_sound, getXYZ());
        if (m_speed < 0.1f) m_last_factor_engine_sound = 0.0f;
      }
}   // updateEngineSFX

//-----------------------------------------------------------------------------
/** Sets the engine power. It considers the engine specs, items that influence
 *  the available power, and braking/steering.
 */
void Kart::updateEnginePowerAndBrakes(int ticks)
{
    updateNitro(ticks);
    float engine_power = getActualWheelForce();

    // apply parachute physics if relevant
    if(m_attachment->getType()==Attachment::ATTACH_PARACHUTE)
        engine_power*=0.2f;

    // apply bubblegum physics if relevant
    if (m_bubblegum_ticks > 0)
    {
        engine_power = 0.0f;
        m_body->applyTorque(btVector3(0.0, m_bubblegum_torque, 0.0));
    }

    if(m_controls.getAccel())   // accelerating
    {
        // For a short time after a collision disable the engine,
        // so that the karts can bounce back a bit from the obstacle.
        if(m_bounce_back_ticks>0.0f)
            engine_power = 0.0f;
        // let a player going backwards accelerate quickly (e.g. if a player
        // hits a wall, he needs to be able to start again quickly after
        // going backwards)
        else if(m_speed < 0.0f)
            engine_power *= 5.0f;

        // Lose some traction when skidding, to balance the advantage
        if (m_controls.getSkidControl() &&
            m_kart_properties->getSkidVisualTime() == 0)
            engine_power *= 0.5f;

        applyEngineForce(engine_power*m_controls.getAccel());

        // Either all or no brake is set, so test only one to avoid
        // resetting all brakes most of the time.
        if(m_vehicle->getWheelInfo(0).m_brake &&
            !World::getWorld()->isStartPhase())
            m_vehicle->setAllBrakes(0);
        m_brake_ticks = 0;
    }
    else
    {   // not accelerating
        if(m_controls.getBrake())
        {   // check if the player is currently only slowing down
            // or moving backwards
            if(m_speed > 0.0f)
            {   // Still going forward while braking
                applyEngineForce(-engine_power*2.5f);
                m_brake_ticks += ticks;
                // Apply the brakes - include the time dependent brake increase
                float f = 1.0f + stk_config->ticks2Time(m_brake_ticks)
                               * m_kart_properties->getEngineBrakeTimeIncrease();
                m_vehicle->setAllBrakes(m_kart_properties->getEngineBrakeFactor() * f);
            }
            else   // m_speed < 0
            {
                m_vehicle->setAllBrakes(0);
                // going backward, apply reverse gear ratio (unless he goes
                // too fast backwards)
                if ( -m_speed <  m_max_speed->getCurrentMaxSpeed()
                                 *m_kart_properties->getEngineMaxSpeedReverseRatio())
                {
                    // The backwards acceleration is artificially increased to
                    // allow players to get "unstuck" quicker if they hit e.g.
                    // a wall.
                    applyEngineForce(-engine_power*2.5f);
                }
                else  // -m_speed >= max speed on this terrain
                {
                    applyEngineForce(0.0f);
                }

            }   // m_speed <00
        }
        else   // !m_brake
        {
            m_brake_ticks = 0;
            // lift the foot from throttle, brakes with 10% engine_power
            assert(!std::isnan(m_controls.getAccel()));
            assert(!std::isnan(engine_power));
            applyEngineForce(-m_controls.getAccel()*engine_power*0.1f);

            // If not giving power (forward or reverse gear), and speed is low
            // we are "parking" the kart, so in battle mode we can ambush people
            if(std::abs(m_speed) < 5.0f)
                m_vehicle->setAllBrakes(20.0f);
            else
                m_vehicle->setAllBrakes(0);
        }   // !m_brake
    }   // not accelerating
}   // updateEnginePowerAndBrakes

// ----------------------------------------------------------------------------
/** Handles sliding, i.e. the kart sliding off terrain that is too steep.
 *  Dynamically determine friction so that the kart looses its traction
 *  when trying to drive on too steep surfaces. Below angles of 0.25 rad,
 *  you have full traction; above 0.5 rad angles you have absolutely none;
 *  inbetween  there is a linear change in friction. This is done for each
 *  wheel individually (since otherwise karts were still able with enough
 *  speed to drive on walls - one wheel 'on a wall' would not tilt the
 *  kart chassis enough to trigger sliding, but since that wheel had still
 *  full friction, give the kart an upwards velocity).
 */
void Kart::updateSliding()
{
    // Allow the sliding to be disabled per material (for so called
    // high adhesion material), which is useful for e.g. banked curves.
    // We don't have per-wheel material, so the test for special material
    // with high adhesion is done per kart (not per wheel).
    const Material * material = getMaterial();
    if (material && material->highTireAdhesion())
    {
        for (int i = 0; i < m_vehicle->getNumWheels(); i++)
        {
            btWheelInfo &wheel = m_vehicle->getWheelInfo(i);
            wheel.m_frictionSlip = m_kart_properties->getFrictionSlip();
        }
        m_vehicle->setSliding(false);

    }

    // Now test for each wheel if it should be sliding
    // -----------------------------------------------
    bool enable_sliding = false;

    // We need the 'up' vector, which can be affected by material
    // with gravity. So use the body's gravity to determine up:
    Vec3 up = -m_body->getGravity();
    up.normalize();
    for (int i = 0; i < m_vehicle->getNumWheels(); i++)
    {
        const btWheelInfo &wheel = m_vehicle->getWheelInfo(i);
        if (!wheel.m_raycastInfo.m_isInContact) continue;

        const btVector3 &norm = m_vehicle->getWheelInfo(i).m_raycastInfo.m_contactNormalWS;
        float distanceFromUp = norm.dot(up);
        float friction;
        if (distanceFromUp < 0.85f)
        {
            friction = 0.0f;
            enable_sliding = true;
        }
        else if (distanceFromUp > 0.9f)
        {
            friction = 1.0f;
        }
        else
        {
            friction = (distanceFromUp - 0.85f) / 0.5f;
            enable_sliding = true;
        }
        m_vehicle->getWheelInfo(i).m_frictionSlip = friction * m_kart_properties->getFrictionSlip();
    }   // for i < numWheels

    m_vehicle->setSliding(enable_sliding);
}   // updateSliding

// ----------------------------------------------------------------------------
/** Adjusts kart translation if the kart is flying (in debug mode).
 */
void Kart::updateFlying()
{
    m_body->setLinearVelocity(m_body->getLinearVelocity() * 0.99f);

    if (m_controls.getAccel())
    {
        btVector3 velocity = m_body->getLinearVelocity();
        if (velocity.length() < 25)
        {
            float orientation = getHeading();
            m_body->applyCentralImpulse(btVector3(100.0f*sin(orientation), 0.0,
                100.0f*cos(orientation)));
        }
    }
    else if (m_controls.getBrake())
    {
        btVector3 velocity = m_body->getLinearVelocity();
        if (velocity.length() > -15)
        {
            float orientation = getHeading();
            m_body->applyCentralImpulse(btVector3(-100.0f*sin(orientation), 0.0,
                -100.0f*cos(orientation)));
        }
    }

    if (m_controls.getSteer()!= 0.0f)
    {
        m_body->applyTorque(btVector3(0.0, m_controls.getSteer()*3500.0f, 0.0));
    }

    // dampen any roll while flying, makes the kart hard to control
    btVector3 velocity = m_body->getAngularVelocity();
    velocity.setX(0);
    velocity.setZ(0);
    m_body->setAngularVelocity(velocity);

}   // updateFlying

// ----------------------------------------------------------------------------
/** Attaches the right model, creates the physics and loads all special
 *  effects (particle systems etc.)
 *  \param type Type of the kart.
 *  \param is_animated_model True if the model is animated.
 */
void Kart::loadData(RaceManager::KartType type, bool is_animated_model)
{
    bool always_animated = (type == RaceManager::KT_PLAYER &&
        race_manager->getNumLocalPlayers() == 1);
    m_node = m_kart_model->attachModel(is_animated_model, always_animated);

#ifdef DEBUG
    m_node->setName( (getIdent()+"(lod-node)").c_str() );
#endif

    // Attachment must be created after attachModel, since only then the
    // scene node will exist (to which the attachment is added). But the
    // attachment is needed in createPhysics (which gets the mass, which
    // is dependent on the attachment).
    m_attachment = new Attachment(this);
    createPhysics();

    m_slipstream = new SlipStream(this);

#ifndef SERVER_ONLY
    if (m_kart_properties->getSkidEnabled() && CVS->isGLSL())
    {
        m_skidmarks = new SkidMarks(*this);
    }

    if (CVS->isGLSL() && !CVS->isShadowEnabled() && m_kart_properties
        ->getShadowMaterial()->getSamplerPath(0) != "unicolor_white")
    {
        m_shadow = new Shadow(m_kart_properties->getShadowMaterial(), *this);
    }
#endif
    World::getWorld()->kartAdded(this, m_node);
}   // loadData

// ----------------------------------------------------------------------------
/** Applies engine power to all the wheels that are traction capable,
 *  so other parts of code do not have to be adjusted to simulate different
 *  kinds of vehicles in the general case, only if they are trying to
 *  simulate traction control, diferentials or multiple independent electric
 *  engines, they will have to tweak the power in a per wheel basis.
 */
void Kart::applyEngineForce(float force)
{
    assert(!std::isnan(force));
    // Split power to simulate a 4WD 40-60, other values possible
    // FWD or RWD is a matter of putting a 0 and 1 in the right place
    float frontForce = force*0.4f;
    float rearForce = force*0.6f;
    // Front wheels
    for(unsigned int i=0; i<2; i++)
    {
        m_vehicle->applyEngineForce (frontForce, i);
    }
    // Rear wheels
    for(unsigned int i=2; i<4; i++)
    {
        m_vehicle->applyEngineForce (rearForce, i);
    }
}   // applyEngineForce

//-----------------------------------------------------------------------------
/** Computes the transform of the graphical kart chasses with regards to the
 *  physical chassis. This function is called once the kart comes to rest
 *  before the race starts (see World::resetAllKarts). Based on the current
 *  physical kart position it computes an (at this stage Y-only) offset by
 *  which the graphical chassis is moved so that it appears the way it is
 *  designed in blender. This means that the distance of the wheels from the
 *  chassis (i.e. suspension) appears as in blender when karts are in rest.
 *  See updateGraphics for more details.
 */
void Kart::kartIsInRestNow()
{
    AbstractKart::kartIsInRestNow();
    float f = 0;
    for(int i=0; i<m_vehicle->getNumWheels(); i++)
    {
        const btWheelInfo &wi = m_vehicle->getWheelInfo(i);
        f += wi.m_raycastInfo.m_suspensionLength;
    }

    // The offset 'lowest point' is added to avoid that the
    // visual chassis appears in the ground (it could be any
    // constant, there is no real reason to use the lowest point
    // but that value has worked good in the past). See documentation
    // for updateGraphics() for full details.
    m_graphical_y_offset = -f / m_vehicle->getNumWheels()
                         + m_kart_model->getLowestPoint();

    m_kart_model->setDefaultSuspension();
}   // kartIsInRestNow

//-----------------------------------------------------------------------------

SFXBase* Kart::getNextEmitter()
{
    m_emitter_id = (m_emitter_id + 1) % 3;
    return m_emitters[m_emitter_id];
}

//-----------------------------------------------------------------------------
/** Updates the graphics model. It is responsible for positioning the graphical
 *  chasses at an 'appropriate' position: typically, the physical model has
 *  much longer suspension, so if the graphical chassis would be at the same
 *  location as the physical chassis, the wheels would be too far away.
 *  Instead the default suspension length is saved once the kart have been
 *  settled at start up (see World::resetAllKarts, which just runs several
 *  physics-only simulation steps so that karts come to a rest). Then at
 *  race time, only the difference between the current suspension length and
 *  this default suspension length is used. The graphical kart chassis will be
 *  offset so that when the kart is in rest, i.e. suspension length ==
 *  default suspension length, the kart will look the way it was modelled in
 *  blender. To explain the various offsets used, here a view from the side
 *  focusing on the Y axis only (X/Z position of the graphical chassis is
 *  identical to the physical chassis):
 *
 * Y|     | visual kart                |       physical kart
 *  |     |                            |
 *  |     |                            |
 *  |     |                            |
 *  |     |                            +-------------COG---------------
 *  |     |                            :
 *  |     +---------low------          :
 *  |     O                            :
 *  +--------------------------------------------------------------------------
 *                                                                            X
 *  'O'   : visual wheel                ':'  : raycast from physics
 *  'low' : lowest Y coordinate of      COG  : Center of gravity (at bottom of
 *          model                              chassis)
 *
 *  The visual kart is stored so that if it is shown at (0,0,0) it would be
 *  the same as in blender. This on the other hand means, if the kart is shown
 *  at the position of the physical chassis (which is at COG in the picture
 *  above), the kart and its wheels would be floating in the air (exactly by
 *  as much as the suspension length), and the wheels would be exactly at the
 *  bottom of the physical chassis (i.e. just on the plane going through COG
 *  and parallel to the ground).
 *  If we want to align the visual chassis to be the same as the physical
 *  chassis, we would need to subtract 'low' from the physical position.
 *  If the kart is then displayed at COG.y-low, the bottom of the kart (which
 *  is at 'low' above ground) would be at COG.y-low + low = COG.y --> visual
 *  and physical chassis are identical.
 *
 *  Unfortunately, the suspension length used in the physics is much too high,
 *  the karts would be way above their wheels, basically disconneccted
 *  (typical physical suspension length is around 0.28, while the distance
 *  between wheel and chassis in blender is in the order of 0.10 --> so there
 *  would be an additional distance of around 0.18 between wheel chassis as
 *  designed in blender and in stk - even more if the kart is driving downhill
 *  when the suspension extends further to keep contact with the ground).
 *  To make the visuals look closer to what they are in blender, an additional
 *  offset is added: before the start of a race the physics simulation is run
 *  to find a stable position for each kart (see World::resetAllKarts). Once
 *  a kart is stable, we save this suspension length in m_graphical_y_offset.
 *  This offset is subtracted from the COG of the kart. So if the kart is in
 *  rest (i.e. suspenion == default_suspension == m_graphical_y_offset),
 *  The kart is showen exactly at the same height above ground as it is in
 *  blender. If the suspension is shorter by DY (because the kart is
 *  accelerating, the ground goes up, ...), the visual chassis is lowered by
 *  DY as well.
 *
 *  While the above algorithm indeed results in the right position of the
 *  visual chassis, in reality the visual chassis is too low. For example,
 *  nolok's chassis has its lowest point at the rear at around 0.10 above the
 *  ground (and the lowest point overall is 0.05, though this is at the front
 *  and so not easily visible), so if the suspension is compressed by more than
 *  that, the chassiswill appear to be in the ground. Testing on the sand track
 *  shows that the suspension is compressed by 0.12 (and up to 0.16 in some
 *  extreme points), which means that the chassis will appear to be in the
 *  ground quite easily. Therefore the chassis is actually moved up a bit to
 *  avoid this problem. Historically (due to never sorting out that formula
 *  properly) the chassis was moved twice as high as its lowest point, e.g.
 *  nolok's lowest point is at 0.05, so the whole chassis was raised by 0.05
 *  (this was not done by design, but because of a bug ;)  ). Since this visual
 *  offset has worked well in the past, the visual chassis is moved by the
 *  same amount higher.
 *
 *  Of course this means that the Y position of the wheels (relative to the
 *  visual kart chassis) needs to be adjusted: if the kart is in rest, the
 *  wheels are exactly on the ground. If the suspension is shorter, that wheel
 *  would appear to be partly in the ground, and if the suspension is longer,
 *  the wheel would not touch the ground.
 *
 *  The wheels must be offset by how much the current suspension length is
 *  longer or shorter than the default (i.e. at rest) suspension length.
 *  This is done in KartModel (pos is the position of the wheel relative
 *  to the visual kart chassis):
 *          pos.Y += m_default_physics_suspension[i]
 *                  - wi.m_raycastInfo.m_suspensionLength
 *  But since the chassis is raised an additional 'getLowestPoint' (see
 *  desctiption two paragraphs above), the wheels need to be lowered by that
 *  amount so that they still touch the ground (the wheel nodes are child of
 *  the chassis scene node, so if the chassis is raised by X, the wheels need
 *  to be lowered by X).
 *  This function also takes additional graphical effects into account, e.g.
 *  a (visual only) jump when skidding, and leaning of the kart.
 */
void Kart::updateGraphics(float dt)
{
    static video::SColor pink(255, 255, 133, 253);
    static video::SColor green(255, 61, 87, 23);

#ifndef SERVER_ONLY
    // draw skidmarks if relevant (we force pink skidmarks on when hitting 
    // a bubblegum)
    if (m_kart_properties->getSkidEnabled() && m_skidmarks)
    {
        m_skidmarks->update(dt,
            m_bubblegum_ticks > 0,
            (m_bubblegum_ticks > 0
                ? (m_has_caught_nolok_bubblegum ? &green
                    : &pink)
                : NULL));
    }
#endif

    // Upate particle effects (creation rate, and emitter size
    // depending on speed)
    // --------------------------------------------------------
    float nitro_frac = 0;
    if ( (m_controls.getNitro() || m_min_nitro_ticks > 0) &&
         m_collected_energy > 0                               )
    {
        // fabs(speed) is important, otherwise the negative number will
        // become a huge unsigned number in the particle scene node!
        nitro_frac = fabsf(getSpeed()) / (m_kart_properties->getEngineMaxSpeed());
        // The speed of the kart can be higher (due to powerups) than
        // the normal maximum speed of the kart.
        if(nitro_frac>1.0f) nitro_frac = 1.0f;
    }
    m_kart_gfx->updateNitroGraphics(nitro_frac);
    
    // Handle leaning of karts
    // -----------------------
    // Note that we compare with maximum speed of the kart, not
    // maximum speed including terrain effects. This avoids that
    // leaning might get less if a kart gets a special that increases
    // its maximum speed, but not the current speed (by much). On the
    // other hand, that ratio can often be greater than 1.
    float speed_frac = m_speed / m_kart_properties->getEngineMaxSpeed();
    if(speed_frac>1.0f)
        speed_frac = 1.0f;
    else if (speed_frac < 0.0f)  // no leaning when backwards driving
        speed_frac = 0.0f;

    const float steer_frac = m_skidding->getSteeringFraction();

    const float roll_speed = m_kart_properties->getLeanSpeed() * DEGREE_TO_RAD;

    if(speed_frac > 0.8f && fabsf(steer_frac)>0.5f)
    {
        // Use steering ^ 7, which means less effect at lower
        // steering
        const float f = m_skidding->getSteeringFraction();
        const float f2 = f*f;
        const float max_lean = -m_kart_properties->getLeanMax() * DEGREE_TO_RAD
                             * f2*f2*f2*f
                             * speed_frac;
        if(max_lean>0)
        {
            m_current_lean += dt* roll_speed;
            if(m_current_lean > max_lean)
                m_current_lean = max_lean;
        }
        else if(max_lean<0)
        {
            m_current_lean -= dt*roll_speed;
            if(m_current_lean < max_lean)
                m_current_lean = max_lean;
        }
    }
    else if(m_current_lean!=0.0f)
    {
        // Disable any potential roll factor that is still applied
        // --------------------------------------------------------
        if(m_current_lean>0)
        {
            m_current_lean -= dt * roll_speed;
            if(m_current_lean < 0.0f)
                m_current_lean = 0.0f;
        }
        else
        {
            m_current_lean += dt * roll_speed;
            if(m_current_lean>0.0f)
                m_current_lean = 0.0f;
        }
    }

    // If the kart is leaning, part of the kart might end up 'in' the track.
    // To avoid this, raise the kart enough to offset the leaning.
    float lean_height = tan(m_current_lean) * getKartWidth()*0.5f;

    Vec3 center_shift(0, 0, 0);

    // Update the skidding jump height:
    float jump_height = m_skidding->updateGraphics(dt);
    center_shift.setY(jump_height + fabsf(lean_height) + m_graphical_y_offset);
    center_shift = getTrans().getBasis() * center_shift;

    float heading = m_skidding->getVisualSkidRotation();
    Moveable::updateGraphics(dt, center_shift,
                             btQuaternion(heading, 0, -m_current_lean));

    // m_speed*dt is the distance the kart has moved, which determines
    // how much the wheels need to rotate.
    m_kart_model->update(dt, m_speed*dt, getSteerPercent(), m_speed, lean_height);

#ifndef SERVER_ONLY
    // Determine the shadow position from the terrain Y position. This
    // leaves the shadow on the ground even if the kart is jumping because
    // of skidding (shadows are disabled when wheel are not on the track).
    if (m_shadow)
    {
        const bool emergency = getKartAnimation() != NULL;
        m_shadow->update(isOnGround() && !emergency);
    }
#endif

    handleMaterialGFX(dt);
    updateEngineSFX(dt);
    handleMaterialSFX();
}   // updateGraphics

// ----------------------------------------------------------------------------
btQuaternion Kart::getVisualRotation() const
{
    return getRotation()
         * btQuaternion(m_skidding->getVisualSkidRotation(), 0, 0);
}   // getVisualRotation

// ----------------------------------------------------------------------------
/** Sets a text that is being displayed on top of a kart. This can be 'leader'
 *  for the leader kart in a FTL race, the name of a driver, or even debug
 *  output.
 *  \param text The text to display
 */
void Kart::setOnScreenText(const wchar_t *text)
{
#ifndef SERVER_ONLY
    BoldFace* bold_face = font_manager->getFont<BoldFace>();
    core::dimension2d<u32> textsize = bold_face->getDimension(text);

    if (CVS->isGLSL())
    {
        STKTextBillboard* tb =
            new STKTextBillboard(
            GUIEngine::getSkin()->getColor("font::bottom"),
            GUIEngine::getSkin()->getColor("font::top"),
            getNode(), irr_driver->getSceneManager(), -1,
            core::vector3df(0.0f, 1.5f, 0.0f),
            core::vector3df(1.0f, 1.0f, 1.0f));
        tb->init(text, bold_face);
        tb->drop();
    }
    else
    {
        scene::ISceneManager* sm = irr_driver->getSceneManager();
        sm->addBillboardTextSceneNode(GUIEngine::getFont() ? GUIEngine::getFont()
                                                           : GUIEngine::getTitleFont(),
            text,
            getNode(),
            core::dimension2df(textsize.Width/55.0f,
            textsize.Height/55.0f),
            core::vector3df(0.0f, 1.5f, 0.0f),
            -1, // id
            GUIEngine::getSkin()->getColor("font::bottom"),
            GUIEngine::getSkin()->getColor("font::top"));
    }

    // No need to store the reference to the billboard scene node:
    // It has one reference to the parent, and will get deleted
    // when the parent is deleted.
#endif
}   // setOnScreenText

// ------------------------------------------------------------------------
/** Returns the normal of the terrain the kart is over atm. This is
  * defined even if the kart is flying.
  */
const Vec3& Kart::getNormal() const
{
    return m_terrain_info->getNormal();
}   // getNormal

// ------------------------------------------------------------------------
/** Returns the position 0.25s before */
const Vec3& Kart::getPreviousXYZ() const
{
    return m_previous_xyz[m_xyz_history_size-1];
}   // getPreviousXYZ

// ------------------------------------------------------------------------
/** Returns a more recent different previous position */
const Vec3& Kart::getRecentPreviousXYZ() const
{
    //Not the most recent, because the angle variations would be too
    //irregular on some tracks whose roads are not smooth enough
    return m_previous_xyz[m_xyz_history_size/5];
}   // getRecentPreviousXYZ

// ------------------------------------------------------------------------
/** Returns the time at which the recent previoux position occured */
const float Kart::getRecentPreviousXYZTime() const
{
    return m_previous_xyz_times[m_xyz_history_size/5];
}   // getRecentPreviousXYZTime

// ------------------------------------------------------------------------
void Kart::playSound(SFXBuffer* buffer)
{
    getNextEmitter()->play(getXYZ(), buffer);
}   // playSound

// ------------------------------------------------------------------------
const video::SColor& Kart::getColor() const
{
    return m_kart_properties->getColor();
}   // getColor

// ------------------------------------------------------------------------
bool Kart::isVisible()
{
    return m_node && m_node->isVisible();
}   // isVisible

/* EOF */
