//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006-2015 SuperTuxKart-Team
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

#ifndef HEADER_KART_PROPERTIES_HPP
#define HEADER_KART_PROPERTIES_HPP

#include <memory>
#include <string>
#include <vector>

#include <SColor.h>
#include <irrString.h>
namespace irr
{
    namespace video { class ITexture; }
}
using namespace irr;

#include "audio/sfx_manager.hpp"
#include "karts/kart_model.hpp"
#include "io/xml_node.hpp"
#include "race/race_manager.hpp"
#include "utils/interpolation_array.hpp"
#include "utils/vec3.hpp"

class AbstractCharacteristic;
class AIProperties;
class CombinedCharacteristic;
class Material;
class XMLNode;

/**
 *  \brief This class stores the properties of a kart.
 *  This includes size, name, identifier, physical properties etc.
 *  It is atm also the base class for STKConfig, which stores the default values
 *  for all physics constants.
 *  Note that KartProperties is copied (when setting the default values from
 *  stk_config.
 *
 * \ingroup karts
 */
class KartProperties
{
private:
    /** Base directory for this kart. */
    std::string              m_root;

    /** AI Properties for this kart, as a separate object in order to
     *  reduce dependencies (and therefore compile time) when changing
     *  any AI property. There is one separate object for each
     *  difficulty. */
    std::shared_ptr<AIProperties> m_ai_properties[RaceManager::DIFFICULTY_COUNT];

    /** The absolute path of the icon texture to use. */
    Material                *m_icon_material;

    /** The minimap icon file. */
    std::string              m_minimap_icon_file;

    /** The texture to use in the minimap. If not defined, a simple
     *  color dot is used. */
    video::ITexture         *m_minimap_icon;

    /** The kart model and wheels. It is mutable since the wheels of the
     *  KartModel can rotate and turn, and animations are played, but otherwise
     *  the kart_properties object is const. */
    mutable KartModel       *m_kart_model;

    /** List of all groups the kart belongs to. */
    std::vector<std::string> m_groups;

    /** Dummy value to detect unset properties. */
    static float UNDEFINED;

    /** Version of the .kart file. */
    int   m_version;

    // SFX files
    // ---------------
    std::vector<int> m_custom_sfx_id;     /**< Vector of custom SFX ids */

    // Display and gui
    // ---------------
    std::string m_name;               /**< The human readable Name of the kart
                                       *   driver. */
    std::string m_ident;              /**< The computer readable-name of the
                                       *   kart driver. */
    std::string m_icon_file;          /**< Filename of icon that represents the
                                       *   kart in the statusbar and the
                                       *   character select screen. */
    std::string m_shadow_file;        /**< Filename of the image file that
                                       *   contains the shadow for this kart.*/
    float m_shadow_scale;             /**< Scale of the shadow plane
                                       *   for this kart.*/
    float m_shadow_x_offset;          /**< X offset of the shadow plane
                                       *   for this kart.*/
    float m_shadow_z_offset;          /**< Z offset of the shadow plane
                                       *   for this kart.*/
    video::ITexture *m_shadow_texture;/**< The texture with the shadow. */
    video::SColor m_color;            /**< Color the represents the kart in the
                                       *   status bar and on the track-view. */
    int  m_shape;                     /**< Number of vertices in polygon when
                                       *   drawing the dot on the mini map. */

    /** The physical, item, etc. characteristics of this kart that are loaded
     *  from the xml file.
     */
    std::shared_ptr<AbstractCharacteristic> m_characteristic;
    /** The base characteristics combined with the characteristics of this kart. */
    std::shared_ptr<CombinedCharacteristic> m_combined_characteristic;
    /** The cached combined characteristics. */
    std::unique_ptr<CachedCharacteristic> m_cached_characteristic;

    // Physic properties
    // -----------------
    /** If != 0 a bevelled box shape is used by using a point cloud as a
     *  collision shape. */
    Vec3  m_bevel_factor;

    /** The position of the physical wheel is a weighted average of the
     *  two ends of the beveled shape. This determines the weight: 0 =
     *  a the widest end, 1 = at the narrowest front end. If the value is
     *  < 0, the old physics settings are used which places the raycast
     *  wheels outside of the chassis - but result in a more stable
     *  physics behaviour (which is therefore atm still the default).
     */
    float m_physical_wheel_position;

    /** Minimum time during which nitro is consumed when pressing
     *  the nitro key (to prevent using in very small bursts)
     */
    float m_nitro_min_consumption;

    /** Type of the kart (for the properties) */
    std::string m_kart_type;

    /** Filename of the wheel models. */
    std::string m_wheel_filename[4];
    /** An additional Y offset added to the y position of the graphical
     *  chassis. Useful for karts that don't have enough space for suspension
     *  compression. */
    float       m_graphical_y_offset;
    /** Wheel base of the kart. */
    float       m_wheel_base;

    /** The maximum roll a kart graphics should show when driving in a fast
     *  curve. This is read in as degrees, but stored in radians. */
     float      m_max_lean;

     /** The speed with which the roll (when leaning in a curve) changes
      *  (in radians/second). */
     float      m_lean_speed;

    /** Engine sound effect. */
    std::string m_engine_sfx_type;

    // bullet physics data
    // -------------------
    float m_friction_slip;

    /** Parameters for the speed-weighted objects */
    SpeedWeightedObject::Properties   m_speed_weighted_object_properties;

    /** Shift of center of gravity. */
    Vec3  m_gravity_center_shift;

public:
    /** STK can add an impulse to push karts away from the track in case
     *  of a kart-track collision. This can be done in two ways: either
     *  apply the impulse in the direction of the normal, or towards the
     *  driveline. The later works nice as long as the kart is driving
     *  on the main track, but can work very bad if the kart is drivling
     *  off-track (and a wrong driveline is selected). */
    enum TerrainImpulseType {IMPULSE_NONE, IMPULSE_NORMAL,
                             IMPULSE_TO_DRIVELINE};
private:
    TerrainImpulseType m_terrain_impulse_type;

    /** An additional impulse to push a kart away if it hits terrain */
    float m_collision_terrain_impulse;

    /** An additiojnal artificial impulse that pushes two karts in a
     *  side-side collision away from each other. */
    float m_collision_impulse;

    /** How long the collision impulse should be applied. */
    float m_collision_impulse_time;

    /** The restitution factor to be used in collsions for this kart. */
    float m_restitution;


    void  load              (const std::string &filename,
                             const std::string &node);
    void combineCharacteristics();

public:
    /** Returns the string representation of a per-player difficulty. */
    static std::string      getPerPlayerDifficultyAsString(PerPlayerDifficulty d);

          KartProperties    (const std::string &filename="");
         ~KartProperties    ();
    void  copyForPlayer     (const KartProperties *source);
    void  copyFrom          (const KartProperties *source);
    void  getAllData        (const XMLNode * root);
    void  checkAllSet       (const std::string &filename);
    bool  isInGroup         (const std::string &group) const;
    bool operator<(const KartProperties &other) const;

    // ------------------------------------------------------------------------
    /** Returns the characteristics for this kart. */
    const AbstractCharacteristic* getCharacteristic() const;
    // ------------------------------------------------------------------------
    /** Returns the characteristics for this kart combined with the base
     *  characteristic. This value isn't used for the race, because the
     *  difficulty is missing, but it can be used e.g. for the kart stats widget.
     */
    const AbstractCharacteristic* getCombinedCharacteristic() const;

    // ------------------------------------------------------------------------
    /** Returns the material for the kart icons. */
    Material*     getIconMaterial    () const {return m_icon_material;        }

    // ------------------------------------------------------------------------
    /** Returns the texture to use in the minimap, or NULL if not defined. */
    video::ITexture *getMinimapIcon  () const {return m_minimap_icon;         }

    // ------------------------------------------------------------------------
    /** Returns a pointer to the KartModel object. */
    KartModel*    getKartModelCopy   () const
                                            {return m_kart_model->makeCopy(); }

    // ------------------------------------------------------------------------
    /** Returns a pointer to the main KartModel object. This copy
     *  should not be modified, not attachModel be called on it. */
    const KartModel& getMasterKartModel() const {return *m_kart_model;        }

    // ------------------------------------------------------------------------
    /** Sets the name of a mesh to be used for this kart.
     *  \param hat_name Name of the mesh.
     */
    void setHatMeshName(const std::string &hat_name)
    {
        m_kart_model->setHatMeshName(hat_name);
    }   // setHatMeshName
    // ------------------------------------------------------------------------
    /** Returns the name of this kart.
        \note Pass it through fridibi as needed, this is the LTR name
      */
    core::stringw getName() const
    {
        return _LTR(m_name.c_str());
    }

    // ------------------------------------------------------------------------
    const std::string getNonTranslatedName() const {return m_name;}

    // ------------------------------------------------------------------------
    /** Returns the internal identifier of this kart. */
    const std::string& getIdent      () const {return m_ident;                }

    // ------------------------------------------------------------------------
    /** Returns the type of this kart. */
    const std::string& getKartType   () const { return m_kart_type;           }

    // ------------------------------------------------------------------------
    /** Returns the shadow texture to use. */
    video::ITexture *getShadowTexture() const {return m_shadow_texture;       }

    // ------------------------------------------------------------------------
    /** Returns the absolute path of the icon file of this kart. */
    const std::string& getAbsoluteIconFile() const      { return m_icon_file; }

    // ------------------------------------------------------------------------
    /** Returns custom sound effects for this kart. */
    const int          getCustomSfxId (SFXManager::CustomSFX type)
                                       const  {return m_custom_sfx_id[type];  }

    // ------------------------------------------------------------------------
    /** Returns the version of the .kart file. */
    int   getVersion                () const {return m_version;               }

    // ------------------------------------------------------------------------
    /** Returns the dot color to use for this kart in the race gui. */
    const video::SColor &getColor   () const {return m_color;                 }

    // ------------------------------------------------------------------------
    /** Returns the number of edges for the polygon used to draw the dot of
     *  this kart on the mini map of the race gui. */
    int   getShape                  () const {return m_shape;                 }

    // ------------------------------------------------------------------------
    /** Returns the list of groups this kart belongs to. */
    const std::vector<std::string>&
                  getGroups         () const {return m_groups;                }

    // ------------------------------------------------------------------------
    /** Returns the engine type (used to change sfx depending on kart size). */
    const std::string& getEngineSfxType    () const {return m_engine_sfx_type;}

    // Bullet physics get functions
    //-----------------------------
    /** Returns friction slip. */
    float getFrictionSlip           () const {return m_friction_slip;         }

    // ------------------------------------------------------------------------
    /** Returns parameters for the speed-weighted objects */
    const SpeedWeightedObject::Properties& getSpeedWeightedObjectProperties() const
    {
        return m_speed_weighted_object_properties;
    }

    // ------------------------------------------------------------------------
    /** Returns the wheel base (distance front to rear axis). */
    float getWheelBase              () const {return m_wheel_base;            }

    // ------------------------------------------------------------------------
    /** Returns a shift of the center of mass (lowering the center of mass
     *  makes the karts more stable. */
    const Vec3&getGravityCenterShift() const {return m_gravity_center_shift;  }

    // ------------------------------------------------------------------------
    /** Returns an artificial impulse to push karts away from the terrain
     *  it hits. */
    float getCollisionTerrainImpulse() const
                                          {return m_collision_terrain_impulse;}

    // ------------------------------------------------------------------------
    /** Returns what kind of impulse STK should use in case of a kart-track
     *  collision. */
    TerrainImpulseType getTerrainImpulseType() const
                                             { return m_terrain_impulse_type; }
    // ------------------------------------------------------------------------
    /** Returns the (artificial) collision impulse this kart will apply
     *  to another kart in case of a non-frontal collision. */
    float getCollisionImpulse       () const {return m_collision_impulse;}

    // ------------------------------------------------------------------------
    /** Returns how long the collision impulse should be applied. */
    float getCollisionImpulseTime() const { return m_collision_impulse_time;}

    // ------------------------------------------------------------------------
    /** Returns the restitution factor for this kart. */
    float getRestitution            () const { return m_restitution; }

    // ------------------------------------------------------------------------
    /** Returns the scale factor by which the shadow plane
     *  had to be set. */
    float getShadowScale            () const {return m_shadow_scale;          }

    // ------------------------------------------------------------------------
    /** Returns the scale factor by which the shadow plane
     *  had to be set. */
    float getShadowXOffset          () const {return m_shadow_x_offset;       }

    // ------------------------------------------------------------------------
    /** Returns the scale factor by which the shadow plane
     *  had to be set. */
    float getShadowZOffset          () const {return m_shadow_z_offset;       }

    // ------------------------------------------------------------------------
    /** Returns a pointer to the AI properties. */
    const AIProperties *getAIPropertiesForDifficulty() const
    {
        return m_ai_properties[race_manager->getDifficulty()].get();
    }   // getAIProperties

    // ------------------------------------------------------------------------
    /** Returns the full path where the files for this kart are stored. */
    const std::string& getKartDir   () const {return m_root;                  }

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    /** Returns minimum time during which nitro is consumed when pressing nitro
     *  key, to prevent using nitro in very short bursts
     */
    float getNitroMinConsumptionTime() const { return m_nitro_min_consumption; }
    // ------------------------------------------------------------------------
    /** Returns the bevel factor (!=0 indicates to use a bevelled box). */
    const Vec3 &getBevelFactor() const { return m_bevel_factor; }
    // ------------------------------------------------------------------------
    /** Returns position of the physical wheel is a weighted average of the
     *  two ends of the beveled shape. This determines the weight: 0 =
     *  a the widest end, 1 = at the narrowest, front end. If the value is <0,
     *  the old physics position is picked, which placed the raycast wheels
     *  outside of the chassis, but gives more stable physics. */
    const float getPhysicalWheelPosition() const
    {
        return m_physical_wheel_position;
    }   // getPhysicalWheelPosition

    // ------------------------------------------------------------------------
    float getAvgPower() const;


    // Script-generated content generated by tools/create_kart_properties.py defs
    // Please don't change the following tag. It will be automatically detected
    // by the script and replace the contained content.
    // To update the code, use tools/update_characteristics.py
    /* <characteristics-start kpdefs> */

    float getSuspensionStiffness() const;
    float getSuspensionRest() const;
    float getSuspensionTravel() const;
    bool getSuspensionExpSpringResponse() const;
    float getSuspensionMaxForce() const;

    float getStabilityRollInfluence() const;
    float getStabilityChassisLinearDamping() const;
    float getStabilityChassisAngularDamping() const;
    float getStabilityDownwardImpulseFactor() const;
    float getStabilityTrackConnectionAccel() const;
    float getStabilitySmoothFlyingImpulse() const;

    InterpolationArray getTurnRadius() const;
    float getTurnTimeResetSteer() const;
    InterpolationArray getTurnTimeFullSteer() const;

    float getEnginePower() const;
    float getEngineMaxSpeed() const;
    float getEngineBrakeFactor() const;
    float getEngineBrakeTimeIncrease() const;
    float getEngineMaxSpeedReverseRatio() const;

    std::vector<float> getGearSwitchRatio() const;
    std::vector<float> getGearPowerIncrease() const;

    float getMass() const;

    float getWheelsDampingRelaxation() const;
    float getWheelsDampingCompression() const;

    float getCameraDistance() const;
    float getCameraForwardUpAngle() const;
    float getCameraBackwardUpAngle() const;

    float getJumpAnimationTime() const;

    float getLeanMax() const;
    float getLeanSpeed() const;

    float getAnvilDuration() const;
    float getAnvilWeight() const;
    float getAnvilSpeedFactor() const;

    float getParachuteFriction() const;
    float getParachuteDuration() const;
    float getParachuteDurationOther() const;
    float getParachuteLboundFraction() const;
    float getParachuteUboundFraction() const;
    float getParachuteMaxSpeed() const;

    float getBubblegumDuration() const;
    float getBubblegumSpeedFraction() const;
    float getBubblegumTorque() const;
    float getBubblegumFadeInTime() const;
    float getBubblegumShieldDuration() const;

    float getZipperDuration() const;
    float getZipperForce() const;
    float getZipperSpeedGain() const;
    float getZipperMaxSpeedIncrease() const;
    float getZipperFadeOutTime() const;

    float getSwatterDuration() const;
    float getSwatterDistance() const;
    float getSwatterSquashDuration() const;
    float getSwatterSquashSlowdown() const;

    float getPlungerBandMaxLength() const;
    float getPlungerBandForce() const;
    float getPlungerBandDuration() const;
    float getPlungerBandSpeedIncrease() const;
    float getPlungerBandFadeOutTime() const;
    float getPlungerInFaceTime() const;

    std::vector<float> getStartupTime() const;
    std::vector<float> getStartupBoost() const;

    float getRescueDuration() const;
    float getRescueVertOffset() const;
    float getRescueHeight() const;

    float getExplosionDuration() const;
    float getExplosionRadius() const;
    float getExplosionInvulnerabilityTime() const;

    float getNitroDuration() const;
    float getNitroEngineForce() const;
    float getNitroConsumption() const;
    float getNitroSmallContainer() const;
    float getNitroBigContainer() const;
    float getNitroMaxSpeedIncrease() const;
    float getNitroFadeOutTime() const;
    float getNitroMax() const;

    float getSlipstreamDuration() const;
    float getSlipstreamLength() const;
    float getSlipstreamWidth() const;
    float getSlipstreamCollectTime() const;
    float getSlipstreamUseTime() const;
    float getSlipstreamAddPower() const;
    float getSlipstreamMinSpeed() const;
    float getSlipstreamMaxSpeedIncrease() const;
    float getSlipstreamFadeOutTime() const;

    float getSkidIncrease() const;
    float getSkidDecrease() const;
    float getSkidMax() const;
    float getSkidTimeTillMax() const;
    float getSkidVisual() const;
    float getSkidVisualTime() const;
    float getSkidRevertVisualTime() const;
    float getSkidMinSpeed() const;
    std::vector<float> getSkidTimeTillBonus() const;
    std::vector<float> getSkidBonusSpeed() const;
    std::vector<float> getSkidBonusTime() const;
    std::vector<float> getSkidBonusForce() const;
    float getSkidPhysicalJumpTime() const;
    float getSkidGraphicalJumpTime() const;
    float getSkidPostSkidRotateFactor() const;
    float getSkidReduceTurnMin() const;
    float getSkidReduceTurnMax() const;
    bool getSkidEnabled() const;

    /* <characteristics-end kpdefs> */
};   // KartProperties

#endif
