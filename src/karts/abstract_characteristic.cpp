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

#include "karts/abstract_characteristic.hpp"

#include "utils/log.hpp"
#include "utils/interpolation_array.hpp"

#include <cmath>

AbstractCharacteristic::AbstractCharacteristic()
{
}

const SkiddingProperties* AbstractCharacteristic::getSkiddingProperties() const
{
    return nullptr;
}

void AbstractCharacteristic::process(CharacteristicType type, Value value, bool *is_set) const
{
    Log::warn("AbstractCharacteristic", "This type does not do anything");
}


AbstractCharacteristic::ValueType AbstractCharacteristic::getType(CharacteristicType type)
{
    switch (type)
    {
    case CHARACTERISTIC_COUNT:
        Log::fatal("AbstractCharacteristic::getType", "Can't get type of COUNT");
        break;
    // Script-generated content get-prop first part
    case SUSPENSION_STIFFNESS:
        return TYPE_FLOAT;
    case SUSPENSION_REST:
        return TYPE_FLOAT;
    case SUSPENSION_TRAVEL_CM:
        return TYPE_FLOAT;
    case SUSPENSION_EXP_SPRING_RESPONSE:
        return TYPE_BOOL;
    case SUSPENSION_MAX_FORCE:
        return TYPE_FLOAT;
    case STABILITY_ROLL_INFLUENCE:
        return TYPE_FLOAT;
    case STABILITY_CHASSIS_LINEAR_DAMPING:
        return TYPE_FLOAT;
    case STABILITY_CHASSIS_ANGULAR_DAMPING:
        return TYPE_FLOAT;
    case STABILITY_DOWNWARD_IMPULSE_FACTOR:
        return TYPE_FLOAT;
    case STABILITY_TRACK_CONNECTION_ACCEL:
        return TYPE_FLOAT;
    case STABILITY_SMOOTH_FLYING_IMPULSE:
        return TYPE_FLOAT;
    case TURN_RADIUS:
        return TYPE_INTERPOLATION_ARRAY;
    case TURN_TIME_RESET_STEER:
        return TYPE_FLOAT;
    case TURN_TIME_FULL_STEER:
        return TYPE_INTERPOLATION_ARRAY;
    case ENGINE_POWER:
        return TYPE_FLOAT;
    case ENGINE_MAX_SPEED:
        return TYPE_FLOAT;
    case ENGINE_BRAKE_FACTOR:
        return TYPE_FLOAT;
    case ENGINE_BRAKE_TIME_INCREASE:
        return TYPE_FLOAT;
    case ENGINE_MAX_SPEED_REVERSE_RATIO:
        return TYPE_FLOAT;
    case GEAR_SWITCH_RATIO:
        return TYPE_FLOAT_VECTOR;
    case GEAR_POWER_INCREASE:
        return TYPE_FLOAT_VECTOR;
    case MASS:
        return TYPE_FLOAT;
    case WHEELS_DAMPING_RELAXATION:
        return TYPE_FLOAT;
    case WHEELS_DAMPING_COMPRESSION:
        return TYPE_FLOAT;
    case WHEELS_RADIUS:
        return TYPE_FLOAT;
    case WHEELS_POSITION:
        return TYPE_FLOAT_VECTOR;
    case CAMERA_DISTANCE:
        return TYPE_FLOAT;
    case CAMERA_FORWARD_UP_ANGLE:
        return TYPE_FLOAT;
    case CAMERA_BACKWARD_UP_ANGLE:
        return TYPE_FLOAT;
    case JUMP_ANIMATION_TIME:
        return TYPE_FLOAT;
    case LEAN_MAX:
        return TYPE_FLOAT;
    case LEAN_SPEED:
        return TYPE_FLOAT;
    case ANVIL_DURATION:
        return TYPE_FLOAT;
    case ANVIL_WEIGHT:
        return TYPE_FLOAT;
    case ANVIL_SPEED_FACTOR:
        return TYPE_FLOAT;
    case PARACHUTE_FRICTION:
        return TYPE_FLOAT;
    case PARACHUTE_DURATION:
        return TYPE_FLOAT;
    case PARACHUTE_DURATION_OTHER:
        return TYPE_FLOAT;
    case PARACHUTE_LBOUND_FRACTION:
        return TYPE_FLOAT;
    case PARACHUTE_UBOUND_FRACTION:
        return TYPE_FLOAT;
    case PARACHUTE_MAX_SPEED:
        return TYPE_FLOAT;
    case BUBBLEGUM_DURATION:
        return TYPE_FLOAT;
    case BUBBLEGUM_SPEED_FRACTION:
        return TYPE_FLOAT;
    case BUBBLEGUM_TORQUE:
        return TYPE_FLOAT;
    case BUBBLEGUM_FADE_IN_TIME:
        return TYPE_FLOAT;
    case BUBBLEGUM_SHIELD_DURATION:
        return TYPE_FLOAT;
    case ZIPPER_DURATION:
        return TYPE_FLOAT;
    case ZIPPER_FORCE:
        return TYPE_FLOAT;
    case ZIPPER_SPEED_GAIN:
        return TYPE_FLOAT;
    case ZIPPER_MAX_SPEED_INCREASE:
        return TYPE_FLOAT;
    case ZIPPER_FADE_OUT_TIME:
        return TYPE_FLOAT;
    case SWATTER_DURATION:
        return TYPE_FLOAT;
    case SWATTER_DISTANCE:
        return TYPE_FLOAT;
    case SWATTER_SQUASH_DURATION:
        return TYPE_FLOAT;
    case SWATTER_SQUASH_SLOWDOWN:
        return TYPE_FLOAT;
    case PLUNGER_BAND_MAX_LENGTH:
        return TYPE_FLOAT;
    case PLUNGER_BAND_FORCE:
        return TYPE_FLOAT;
    case PLUNGER_BAND_DURATION:
        return TYPE_FLOAT;
    case PLUNGER_BAND_SPEED_INCREASE:
        return TYPE_FLOAT;
    case PLUNGER_BAND_FADE_OUT_TIME:
        return TYPE_FLOAT;
    case PLUNGER_IN_FACE_TIME:
        return TYPE_FLOAT;
    case STARTUP_TIME:
        return TYPE_FLOAT_VECTOR;
    case STARTUP_BOOST:
        return TYPE_FLOAT_VECTOR;
    case RESCUE_DURATION:
        return TYPE_FLOAT;
    case RESCUE_VERT_OFFSET:
        return TYPE_FLOAT;
    case RESCUE_HEIGHT:
        return TYPE_FLOAT;
    case EXPLOSION_DURATION:
        return TYPE_FLOAT;
    case EXPLOSION_RADIUS:
        return TYPE_FLOAT;
    case EXPLOSION_INVULNERABILITY_TIME:
        return TYPE_FLOAT;
    case NITRO_DURATION:
        return TYPE_FLOAT;
    case NITRO_ENGINE_FORCE:
        return TYPE_FLOAT;
    case NITRO_CONSUMPTION:
        return TYPE_FLOAT;
    case NITRO_SMALL_CONTAINER:
        return TYPE_FLOAT;
    case NITRO_BIG_CONTAINER:
        return TYPE_FLOAT;
    case NITRO_MAX_SPEED_INCREASE:
        return TYPE_FLOAT;
    case NITRO_FADE_OUT_TIME:
        return TYPE_FLOAT;
    case NITRO_MAX:
        return TYPE_FLOAT;
    case SLIPSTREAM_DURATION:
        return TYPE_FLOAT;
    case SLIPSTREAM_LENGTH:
        return TYPE_FLOAT;
    case SLIPSTREAM_WIDTH:
        return TYPE_FLOAT;
    case SLIPSTREAM_COLLECT_TIME:
        return TYPE_FLOAT;
    case SLIPSTREAM_USE_TIME:
        return TYPE_FLOAT;
    case SLIPSTREAM_ADD_POWER:
        return TYPE_FLOAT;
    case SLIPSTREAM_MIN_SPEED:
        return TYPE_FLOAT;
    case SLIPSTREAM_MAX_SPEED_INCREASE:
        return TYPE_FLOAT;
    case SLIPSTREAM_FADE_OUT_TIME:
        return TYPE_FLOAT;
    }
    Log::fatal("AbstractCharacteristic::getType", "Unknown type");
    return TYPE_FLOAT;
}

std::string AbstractCharacteristic::getName(CharacteristicType type)
{
    switch (type)
    {
    case CHARACTERISTIC_COUNT:
        return "CHARACTERISTIC_COUNT";
    // Script-generated content get-prop second part
    case SUSPENSION_STIFFNESS:
        return "SUSPENSION_STIFFNESS";
    case SUSPENSION_REST:
        return "SUSPENSION_REST";
    case SUSPENSION_TRAVEL_CM:
        return "SUSPENSION_TRAVEL_CM";
    case SUSPENSION_EXP_SPRING_RESPONSE:
        return "SUSPENSION_EXP_SPRING_RESPONSE";
    case SUSPENSION_MAX_FORCE:
        return "SUSPENSION_MAX_FORCE";
    case STABILITY_ROLL_INFLUENCE:
        return "STABILITY_ROLL_INFLUENCE";
    case STABILITY_CHASSIS_LINEAR_DAMPING:
        return "STABILITY_CHASSIS_LINEAR_DAMPING";
    case STABILITY_CHASSIS_ANGULAR_DAMPING:
        return "STABILITY_CHASSIS_ANGULAR_DAMPING";
    case STABILITY_DOWNWARD_IMPULSE_FACTOR:
        return "STABILITY_DOWNWARD_IMPULSE_FACTOR";
    case STABILITY_TRACK_CONNECTION_ACCEL:
        return "STABILITY_TRACK_CONNECTION_ACCEL";
    case STABILITY_SMOOTH_FLYING_IMPULSE:
        return "STABILITY_SMOOTH_FLYING_IMPULSE";
    case TURN_RADIUS:
        return "TURN_RADIUS";
    case TURN_TIME_RESET_STEER:
        return "TURN_TIME_RESET_STEER";
    case TURN_TIME_FULL_STEER:
        return "TURN_TIME_FULL_STEER";
    case ENGINE_POWER:
        return "ENGINE_POWER";
    case ENGINE_MAX_SPEED:
        return "ENGINE_MAX_SPEED";
    case ENGINE_BRAKE_FACTOR:
        return "ENGINE_BRAKE_FACTOR";
    case ENGINE_BRAKE_TIME_INCREASE:
        return "ENGINE_BRAKE_TIME_INCREASE";
    case ENGINE_MAX_SPEED_REVERSE_RATIO:
        return "ENGINE_MAX_SPEED_REVERSE_RATIO";
    case GEAR_SWITCH_RATIO:
        return "GEAR_SWITCH_RATIO";
    case GEAR_POWER_INCREASE:
        return "GEAR_POWER_INCREASE";
    case MASS:
        return "MASS";
    case WHEELS_DAMPING_RELAXATION:
        return "WHEELS_DAMPING_RELAXATION";
    case WHEELS_DAMPING_COMPRESSION:
        return "WHEELS_DAMPING_COMPRESSION";
    case WHEELS_RADIUS:
        return "WHEELS_RADIUS";
    case WHEELS_POSITION:
        return "WHEELS_POSITION";
    case CAMERA_DISTANCE:
        return "CAMERA_DISTANCE";
    case CAMERA_FORWARD_UP_ANGLE:
        return "CAMERA_FORWARD_UP_ANGLE";
    case CAMERA_BACKWARD_UP_ANGLE:
        return "CAMERA_BACKWARD_UP_ANGLE";
    case JUMP_ANIMATION_TIME:
        return "JUMP_ANIMATION_TIME";
    case LEAN_MAX:
        return "LEAN_MAX";
    case LEAN_SPEED:
        return "LEAN_SPEED";
    case ANVIL_DURATION:
        return "ANVIL_DURATION";
    case ANVIL_WEIGHT:
        return "ANVIL_WEIGHT";
    case ANVIL_SPEED_FACTOR:
        return "ANVIL_SPEED_FACTOR";
    case PARACHUTE_FRICTION:
        return "PARACHUTE_FRICTION";
    case PARACHUTE_DURATION:
        return "PARACHUTE_DURATION";
    case PARACHUTE_DURATION_OTHER:
        return "PARACHUTE_DURATION_OTHER";
    case PARACHUTE_LBOUND_FRACTION:
        return "PARACHUTE_LBOUND_FRACTION";
    case PARACHUTE_UBOUND_FRACTION:
        return "PARACHUTE_UBOUND_FRACTION";
    case PARACHUTE_MAX_SPEED:
        return "PARACHUTE_MAX_SPEED";
    case BUBBLEGUM_DURATION:
        return "BUBBLEGUM_DURATION";
    case BUBBLEGUM_SPEED_FRACTION:
        return "BUBBLEGUM_SPEED_FRACTION";
    case BUBBLEGUM_TORQUE:
        return "BUBBLEGUM_TORQUE";
    case BUBBLEGUM_FADE_IN_TIME:
        return "BUBBLEGUM_FADE_IN_TIME";
    case BUBBLEGUM_SHIELD_DURATION:
        return "BUBBLEGUM_SHIELD_DURATION";
    case ZIPPER_DURATION:
        return "ZIPPER_DURATION";
    case ZIPPER_FORCE:
        return "ZIPPER_FORCE";
    case ZIPPER_SPEED_GAIN:
        return "ZIPPER_SPEED_GAIN";
    case ZIPPER_MAX_SPEED_INCREASE:
        return "ZIPPER_MAX_SPEED_INCREASE";
    case ZIPPER_FADE_OUT_TIME:
        return "ZIPPER_FADE_OUT_TIME";
    case SWATTER_DURATION:
        return "SWATTER_DURATION";
    case SWATTER_DISTANCE:
        return "SWATTER_DISTANCE";
    case SWATTER_SQUASH_DURATION:
        return "SWATTER_SQUASH_DURATION";
    case SWATTER_SQUASH_SLOWDOWN:
        return "SWATTER_SQUASH_SLOWDOWN";
    case PLUNGER_BAND_MAX_LENGTH:
        return "PLUNGER_BAND_MAX_LENGTH";
    case PLUNGER_BAND_FORCE:
        return "PLUNGER_BAND_FORCE";
    case PLUNGER_BAND_DURATION:
        return "PLUNGER_BAND_DURATION";
    case PLUNGER_BAND_SPEED_INCREASE:
        return "PLUNGER_BAND_SPEED_INCREASE";
    case PLUNGER_BAND_FADE_OUT_TIME:
        return "PLUNGER_BAND_FADE_OUT_TIME";
    case PLUNGER_IN_FACE_TIME:
        return "PLUNGER_IN_FACE_TIME";
    case STARTUP_TIME:
        return "STARTUP_TIME";
    case STARTUP_BOOST:
        return "STARTUP_BOOST";
    case RESCUE_DURATION:
        return "RESCUE_DURATION";
    case RESCUE_VERT_OFFSET:
        return "RESCUE_VERT_OFFSET";
    case RESCUE_HEIGHT:
        return "RESCUE_HEIGHT";
    case EXPLOSION_DURATION:
        return "EXPLOSION_DURATION";
    case EXPLOSION_RADIUS:
        return "EXPLOSION_RADIUS";
    case EXPLOSION_INVULNERABILITY_TIME:
        return "EXPLOSION_INVULNERABILITY_TIME";
    case NITRO_DURATION:
        return "NITRO_DURATION";
    case NITRO_ENGINE_FORCE:
        return "NITRO_ENGINE_FORCE";
    case NITRO_CONSUMPTION:
        return "NITRO_CONSUMPTION";
    case NITRO_SMALL_CONTAINER:
        return "NITRO_SMALL_CONTAINER";
    case NITRO_BIG_CONTAINER:
        return "NITRO_BIG_CONTAINER";
    case NITRO_MAX_SPEED_INCREASE:
        return "NITRO_MAX_SPEED_INCREASE";
    case NITRO_FADE_OUT_TIME:
        return "NITRO_FADE_OUT_TIME";
    case NITRO_MAX:
        return "NITRO_MAX";
    case SLIPSTREAM_DURATION:
        return "SLIPSTREAM_DURATION";
    case SLIPSTREAM_LENGTH:
        return "SLIPSTREAM_LENGTH";
    case SLIPSTREAM_WIDTH:
        return "SLIPSTREAM_WIDTH";
    case SLIPSTREAM_COLLECT_TIME:
        return "SLIPSTREAM_COLLECT_TIME";
    case SLIPSTREAM_USE_TIME:
        return "SLIPSTREAM_USE_TIME";
    case SLIPSTREAM_ADD_POWER:
        return "SLIPSTREAM_ADD_POWER";
    case SLIPSTREAM_MIN_SPEED:
        return "SLIPSTREAM_MIN_SPEED";
    case SLIPSTREAM_MAX_SPEED_INCREASE:
        return "SLIPSTREAM_MAX_SPEED_INCREASE";
    case SLIPSTREAM_FADE_OUT_TIME:
        return "SLIPSTREAM_FADE_OUT_TIME";
    }
    Log::error("AbstractCharacteristic::getName", "Unknown type");
    return "Unknown type";
}

// Script-generated getter
float AbstractCharacteristic::getSuspensionStiffness() const
{
    float result;
    bool is_set = false;
    process(SUSPENSION_STIFFNESS, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SUSPENSION_STIFFNESS).c_str());
    return result;
}

float AbstractCharacteristic::getSuspensionRest() const
{
    float result;
    bool is_set = false;
    process(SUSPENSION_REST, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SUSPENSION_REST).c_str());
    return result;
}

float AbstractCharacteristic::getSuspensionTravelCm() const
{
    float result;
    bool is_set = false;
    process(SUSPENSION_TRAVEL_CM, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SUSPENSION_TRAVEL_CM).c_str());
    return result;
}

bool AbstractCharacteristic::getSuspensionExpSpringResponse() const
{
    bool result;
    bool is_set = false;
    process(SUSPENSION_EXP_SPRING_RESPONSE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SUSPENSION_EXP_SPRING_RESPONSE).c_str());
    return result;
}

float AbstractCharacteristic::getSuspensionMaxForce() const
{
    float result;
    bool is_set = false;
    process(SUSPENSION_MAX_FORCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SUSPENSION_MAX_FORCE).c_str());
    return result;
}

float AbstractCharacteristic::getStabilityRollInfluence() const
{
    float result;
    bool is_set = false;
    process(STABILITY_ROLL_INFLUENCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_ROLL_INFLUENCE).c_str());
    return result;
}

float AbstractCharacteristic::getStabilityChassisLinearDamping() const
{
    float result;
    bool is_set = false;
    process(STABILITY_CHASSIS_LINEAR_DAMPING, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_CHASSIS_LINEAR_DAMPING).c_str());
    return result;
}

float AbstractCharacteristic::getStabilityChassisAngularDamping() const
{
    float result;
    bool is_set = false;
    process(STABILITY_CHASSIS_ANGULAR_DAMPING, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_CHASSIS_ANGULAR_DAMPING).c_str());
    return result;
}

float AbstractCharacteristic::getStabilityDownwardImpulseFactor() const
{
    float result;
    bool is_set = false;
    process(STABILITY_DOWNWARD_IMPULSE_FACTOR, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_DOWNWARD_IMPULSE_FACTOR).c_str());
    return result;
}

float AbstractCharacteristic::getStabilityTrackConnectionAccel() const
{
    float result;
    bool is_set = false;
    process(STABILITY_TRACK_CONNECTION_ACCEL, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_TRACK_CONNECTION_ACCEL).c_str());
    return result;
}

float AbstractCharacteristic::getStabilitySmoothFlyingImpulse() const
{
    float result;
    bool is_set = false;
    process(STABILITY_SMOOTH_FLYING_IMPULSE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STABILITY_SMOOTH_FLYING_IMPULSE).c_str());
    return result;
}

InterpolationArray AbstractCharacteristic::getTurnRadius() const
{
    InterpolationArray result;
    bool is_set = false;
    process(TURN_RADIUS, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(TURN_RADIUS).c_str());
    return result;
}

float AbstractCharacteristic::getTurnTimeResetSteer() const
{
    float result;
    bool is_set = false;
    process(TURN_TIME_RESET_STEER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(TURN_TIME_RESET_STEER).c_str());
    return result;
}

InterpolationArray AbstractCharacteristic::getTurnTimeFullSteer() const
{
    InterpolationArray result;
    bool is_set = false;
    process(TURN_TIME_FULL_STEER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(TURN_TIME_FULL_STEER).c_str());
    return result;
}

float AbstractCharacteristic::getEnginePower() const
{
    float result;
    bool is_set = false;
    process(ENGINE_POWER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ENGINE_POWER).c_str());
    return result;
}

float AbstractCharacteristic::getEngineMaxSpeed() const
{
    float result;
    bool is_set = false;
    process(ENGINE_MAX_SPEED, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ENGINE_MAX_SPEED).c_str());
    return result;
}

float AbstractCharacteristic::getEngineBrakeFactor() const
{
    float result;
    bool is_set = false;
    process(ENGINE_BRAKE_FACTOR, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ENGINE_BRAKE_FACTOR).c_str());
    return result;
}

float AbstractCharacteristic::getEngineBrakeTimeIncrease() const
{
    float result;
    bool is_set = false;
    process(ENGINE_BRAKE_TIME_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ENGINE_BRAKE_TIME_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getEngineMaxSpeedReverseRatio() const
{
    float result;
    bool is_set = false;
    process(ENGINE_MAX_SPEED_REVERSE_RATIO, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ENGINE_MAX_SPEED_REVERSE_RATIO).c_str());
    return result;
}

std::vector<float> AbstractCharacteristic::getGearSwitchRatio() const
{
    std::vector<float> result;
    bool is_set = false;
    process(GEAR_SWITCH_RATIO, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(GEAR_SWITCH_RATIO).c_str());
    return result;
}

std::vector<float> AbstractCharacteristic::getGearPowerIncrease() const
{
    std::vector<float> result;
    bool is_set = false;
    process(GEAR_POWER_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(GEAR_POWER_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getMass() const
{
    float result;
    bool is_set = false;
    process(MASS, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(MASS).c_str());
    return result;
}

float AbstractCharacteristic::getWheelsDampingRelaxation() const
{
    float result;
    bool is_set = false;
    process(WHEELS_DAMPING_RELAXATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(WHEELS_DAMPING_RELAXATION).c_str());
    return result;
}

float AbstractCharacteristic::getWheelsDampingCompression() const
{
    float result;
    bool is_set = false;
    process(WHEELS_DAMPING_COMPRESSION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(WHEELS_DAMPING_COMPRESSION).c_str());
    return result;
}

float AbstractCharacteristic::getWheelsRadius() const
{
    float result;
    bool is_set = false;
    process(WHEELS_RADIUS, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(WHEELS_RADIUS).c_str());
    return result;
}

std::vector<float> AbstractCharacteristic::getWheelsPosition() const
{
    std::vector<float> result;
    bool is_set = false;
    process(WHEELS_POSITION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(WHEELS_POSITION).c_str());
    return result;
}

float AbstractCharacteristic::getCameraDistance() const
{
    float result;
    bool is_set = false;
    process(CAMERA_DISTANCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(CAMERA_DISTANCE).c_str());
    return result;
}

float AbstractCharacteristic::getCameraForwardUpAngle() const
{
    float result;
    bool is_set = false;
    process(CAMERA_FORWARD_UP_ANGLE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(CAMERA_FORWARD_UP_ANGLE).c_str());
    return result;
}

float AbstractCharacteristic::getCameraBackwardUpAngle() const
{
    float result;
    bool is_set = false;
    process(CAMERA_BACKWARD_UP_ANGLE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(CAMERA_BACKWARD_UP_ANGLE).c_str());
    return result;
}

float AbstractCharacteristic::getJumpAnimationTime() const
{
    float result;
    bool is_set = false;
    process(JUMP_ANIMATION_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(JUMP_ANIMATION_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getLeanMax() const
{
    float result;
    bool is_set = false;
    process(LEAN_MAX, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(LEAN_MAX).c_str());
    return result;
}

float AbstractCharacteristic::getLeanSpeed() const
{
    float result;
    bool is_set = false;
    process(LEAN_SPEED, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(LEAN_SPEED).c_str());
    return result;
}

float AbstractCharacteristic::getAnvilDuration() const
{
    float result;
    bool is_set = false;
    process(ANVIL_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ANVIL_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getAnvilWeight() const
{
    float result;
    bool is_set = false;
    process(ANVIL_WEIGHT, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ANVIL_WEIGHT).c_str());
    return result;
}

float AbstractCharacteristic::getAnvilSpeedFactor() const
{
    float result;
    bool is_set = false;
    process(ANVIL_SPEED_FACTOR, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ANVIL_SPEED_FACTOR).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteFriction() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_FRICTION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_FRICTION).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteDuration() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteDurationOther() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_DURATION_OTHER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_DURATION_OTHER).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteLboundFraction() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_LBOUND_FRACTION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_LBOUND_FRACTION).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteUboundFraction() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_UBOUND_FRACTION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_UBOUND_FRACTION).c_str());
    return result;
}

float AbstractCharacteristic::getParachuteMaxSpeed() const
{
    float result;
    bool is_set = false;
    process(PARACHUTE_MAX_SPEED, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PARACHUTE_MAX_SPEED).c_str());
    return result;
}

float AbstractCharacteristic::getBubblegumDuration() const
{
    float result;
    bool is_set = false;
    process(BUBBLEGUM_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(BUBBLEGUM_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getBubblegumSpeedFraction() const
{
    float result;
    bool is_set = false;
    process(BUBBLEGUM_SPEED_FRACTION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(BUBBLEGUM_SPEED_FRACTION).c_str());
    return result;
}

float AbstractCharacteristic::getBubblegumTorque() const
{
    float result;
    bool is_set = false;
    process(BUBBLEGUM_TORQUE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(BUBBLEGUM_TORQUE).c_str());
    return result;
}

float AbstractCharacteristic::getBubblegumFadeInTime() const
{
    float result;
    bool is_set = false;
    process(BUBBLEGUM_FADE_IN_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(BUBBLEGUM_FADE_IN_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getBubblegumShieldDuration() const
{
    float result;
    bool is_set = false;
    process(BUBBLEGUM_SHIELD_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(BUBBLEGUM_SHIELD_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getZipperDuration() const
{
    float result;
    bool is_set = false;
    process(ZIPPER_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ZIPPER_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getZipperForce() const
{
    float result;
    bool is_set = false;
    process(ZIPPER_FORCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ZIPPER_FORCE).c_str());
    return result;
}

float AbstractCharacteristic::getZipperSpeedGain() const
{
    float result;
    bool is_set = false;
    process(ZIPPER_SPEED_GAIN, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ZIPPER_SPEED_GAIN).c_str());
    return result;
}

float AbstractCharacteristic::getZipperMaxSpeedIncrease() const
{
    float result;
    bool is_set = false;
    process(ZIPPER_MAX_SPEED_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ZIPPER_MAX_SPEED_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getZipperFadeOutTime() const
{
    float result;
    bool is_set = false;
    process(ZIPPER_FADE_OUT_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(ZIPPER_FADE_OUT_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getSwatterDuration() const
{
    float result;
    bool is_set = false;
    process(SWATTER_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SWATTER_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getSwatterDistance() const
{
    float result;
    bool is_set = false;
    process(SWATTER_DISTANCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SWATTER_DISTANCE).c_str());
    return result;
}

float AbstractCharacteristic::getSwatterSquashDuration() const
{
    float result;
    bool is_set = false;
    process(SWATTER_SQUASH_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SWATTER_SQUASH_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getSwatterSquashSlowdown() const
{
    float result;
    bool is_set = false;
    process(SWATTER_SQUASH_SLOWDOWN, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SWATTER_SQUASH_SLOWDOWN).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerBandMaxLength() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_BAND_MAX_LENGTH, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_BAND_MAX_LENGTH).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerBandForce() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_BAND_FORCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_BAND_FORCE).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerBandDuration() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_BAND_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_BAND_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerBandSpeedIncrease() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_BAND_SPEED_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_BAND_SPEED_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerBandFadeOutTime() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_BAND_FADE_OUT_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_BAND_FADE_OUT_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getPlungerInFaceTime() const
{
    float result;
    bool is_set = false;
    process(PLUNGER_IN_FACE_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(PLUNGER_IN_FACE_TIME).c_str());
    return result;
}

std::vector<float> AbstractCharacteristic::getStartupTime() const
{
    std::vector<float> result;
    bool is_set = false;
    process(STARTUP_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STARTUP_TIME).c_str());
    return result;
}

std::vector<float> AbstractCharacteristic::getStartupBoost() const
{
    std::vector<float> result;
    bool is_set = false;
    process(STARTUP_BOOST, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(STARTUP_BOOST).c_str());
    return result;
}

float AbstractCharacteristic::getRescueDuration() const
{
    float result;
    bool is_set = false;
    process(RESCUE_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(RESCUE_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getRescueVertOffset() const
{
    float result;
    bool is_set = false;
    process(RESCUE_VERT_OFFSET, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(RESCUE_VERT_OFFSET).c_str());
    return result;
}

float AbstractCharacteristic::getRescueHeight() const
{
    float result;
    bool is_set = false;
    process(RESCUE_HEIGHT, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(RESCUE_HEIGHT).c_str());
    return result;
}

float AbstractCharacteristic::getExplosionDuration() const
{
    float result;
    bool is_set = false;
    process(EXPLOSION_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(EXPLOSION_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getExplosionRadius() const
{
    float result;
    bool is_set = false;
    process(EXPLOSION_RADIUS, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(EXPLOSION_RADIUS).c_str());
    return result;
}

float AbstractCharacteristic::getExplosionInvulnerabilityTime() const
{
    float result;
    bool is_set = false;
    process(EXPLOSION_INVULNERABILITY_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(EXPLOSION_INVULNERABILITY_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getNitroDuration() const
{
    float result;
    bool is_set = false;
    process(NITRO_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getNitroEngineForce() const
{
    float result;
    bool is_set = false;
    process(NITRO_ENGINE_FORCE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_ENGINE_FORCE).c_str());
    return result;
}

float AbstractCharacteristic::getNitroConsumption() const
{
    float result;
    bool is_set = false;
    process(NITRO_CONSUMPTION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_CONSUMPTION).c_str());
    return result;
}

float AbstractCharacteristic::getNitroSmallContainer() const
{
    float result;
    bool is_set = false;
    process(NITRO_SMALL_CONTAINER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_SMALL_CONTAINER).c_str());
    return result;
}

float AbstractCharacteristic::getNitroBigContainer() const
{
    float result;
    bool is_set = false;
    process(NITRO_BIG_CONTAINER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_BIG_CONTAINER).c_str());
    return result;
}

float AbstractCharacteristic::getNitroMaxSpeedIncrease() const
{
    float result;
    bool is_set = false;
    process(NITRO_MAX_SPEED_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_MAX_SPEED_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getNitroFadeOutTime() const
{
    float result;
    bool is_set = false;
    process(NITRO_FADE_OUT_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_FADE_OUT_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getNitroMax() const
{
    float result;
    bool is_set = false;
    process(NITRO_MAX, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(NITRO_MAX).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamDuration() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_DURATION, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_DURATION).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamLength() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_LENGTH, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_LENGTH).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamWidth() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_WIDTH, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_WIDTH).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamCollectTime() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_COLLECT_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_COLLECT_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamUseTime() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_USE_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_USE_TIME).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamAddPower() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_ADD_POWER, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_ADD_POWER).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamMinSpeed() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_MIN_SPEED, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_MIN_SPEED).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamMaxSpeedIncrease() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_MAX_SPEED_INCREASE, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_MAX_SPEED_INCREASE).c_str());
    return result;
}

float AbstractCharacteristic::getSlipstreamFadeOutTime() const
{
    float result;
    bool is_set = false;
    process(SLIPSTREAM_FADE_OUT_TIME, &result, &is_set);
    if (!is_set)
        Log::fatal("AbstractCharacteristic", "Can't get characteristic %s", getName(SLIPSTREAM_FADE_OUT_TIME).c_str());
    return result;
}

