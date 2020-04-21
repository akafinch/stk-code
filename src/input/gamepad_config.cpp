//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010-2015 SuperTuxKart-Team
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


#include "input/gamepad_config.hpp"

#include "io/xml_node.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

#include <SKeyMap.h>

#include <assert.h>

#ifndef SERVER_ONLY
#include "input/sdl_controller.hpp"
#include <array>
#include <SDL.h>

static_assert(SDL_CONTROLLER_BUTTON_MAX - 1 == SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "non continous name");
enum AxisWithDirection
{
    SDL_CONTROLLER_AXIS_LEFTX_RIGHT = SDL_CONTROLLER_BUTTON_MAX,
    SDL_CONTROLLER_AXIS_LEFTX_LEFT,
    SDL_CONTROLLER_AXIS_LEFTY_DOWN,
    SDL_CONTROLLER_AXIS_LEFTY_UP,
    SDL_CONTROLLER_AXIS_RIGHTX_RIGHT,
    SDL_CONTROLLER_AXIS_RIGHTX_LEFT,
    SDL_CONTROLLER_AXIS_RIGHTY_DOWN,
    SDL_CONTROLLER_AXIS_RIGHTY_UP,
    // Triggers only have single direction
    SDL_CONTROLLER_AXIS_TRIGGERLEFT_UP,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT_UP,
    SDL_CONTROLLER_AXIS_WITH_DIRECTION_AND_BUTTON_MAX,
};

#endif

using namespace irr;

GamepadConfig::GamepadConfig( const std::string &name,
                              const int        axis_count,
                              const int        button_count )
             : DeviceConfig()
{
    setName(name);
    m_axis_count   = axis_count;
    m_button_count = button_count;
    m_hat_count    = 0;
    m_deadzone     = 4096;
    m_is_analog    = true;
    m_desensitize  = false;
    setDefaultBinds();
}   // GamepadConfig

//------------------------------------------------------------------------------

GamepadConfig::GamepadConfig() : DeviceConfig()
{
    m_axis_count   = 0;
    m_button_count = 0;
    m_hat_count    = 0;
    m_deadzone     = 4096;
    m_is_analog    = true;
    m_desensitize  = false;
    setDefaultBinds();
}   // GamepadConfig

//------------------------------------------------------------------------------
/** Loads this configuration from the given XML node.
 *  \param config The XML tree.
 *  \return False in case of an error.
 */
bool GamepadConfig::load(const XMLNode *config)
{
    config->get("deadzone",     &m_deadzone    );
    config->get("analog",       &m_is_analog   );
    config->get("desensitize",  &m_desensitize );
    bool ok = DeviceConfig::load(config);

    if(getName()=="")
    {
        Log::error("DeviceConfig", "Unnamed joystick in config file.");
        return false;
    }
    return ok;
}   // load

// ----------------------------------------------------------------------------
/** Saves the configuration to a file. It writes the name for a gamepad
 *  config, saves the device specific parameters, and calls
 *  DeviceConfig::save() to save the rest.
 *  \param stream The stream to save to.
 */
void GamepadConfig::save (std::ofstream& stream)
{
    stream << "<gamepad name =\"" << getName()
           << "\" deadzone=\""    << m_deadzone
           << "\" desensitize=\"" << m_desensitize
           << "\" analog=\""      << m_is_analog<<"\"\n";
    stream << "         ";
    DeviceConfig::save(stream);
    stream << "</gamepad>\n\n";
}   // save

//------------------------------------------------------------------------------

void GamepadConfig::setDefaultBinds ()
{
    setBinding(PA_STEER_LEFT,   Input::IT_STICKMOTION, 0, Input::AD_NEGATIVE);
    setBinding(PA_STEER_RIGHT,  Input::IT_STICKMOTION, 0, Input::AD_POSITIVE);
    setBinding(PA_ACCEL,        Input::IT_STICKBUTTON, 0, Input::AD_NEGATIVE);
    setBinding(PA_BRAKE,        Input::IT_STICKBUTTON, 3, Input::AD_POSITIVE);
    setBinding(PA_FIRE,         Input::IT_STICKBUTTON, 1);
    setBinding(PA_NITRO,        Input::IT_STICKBUTTON, 4);
    setBinding(PA_DRIFT,        Input::IT_STICKBUTTON, 5);
    setBinding(PA_RESCUE,       Input::IT_STICKBUTTON, 8);
    setBinding(PA_LOOK_BACK,    Input::IT_STICKBUTTON, 6);
    setBinding(PA_PAUSE_RACE,   Input::IT_STICKBUTTON, 9);

    setBinding(PA_MENU_UP,      Input::IT_STICKMOTION, 1, Input::AD_NEGATIVE);
    setBinding(PA_MENU_DOWN,    Input::IT_STICKMOTION, 1, Input::AD_POSITIVE);
    setBinding(PA_MENU_LEFT,    Input::IT_STICKMOTION, 0, Input::AD_NEGATIVE);
    setBinding(PA_MENU_RIGHT,   Input::IT_STICKMOTION, 0, Input::AD_POSITIVE);
    setBinding(PA_MENU_SELECT,  Input::IT_STICKBUTTON, 0);
    setBinding(PA_MENU_CANCEL,  Input::IT_STICKBUTTON, 3);
}   // setDefaultBinds

//------------------------------------------------------------------------------
core::stringw GamepadConfig::getBindingAsString(const PlayerAction action) const
{
#ifndef SERVER_ONLY
    std::array<core::stringw, SDL_CONTROLLER_AXIS_WITH_DIRECTION_AND_BUTTON_MAX> readable =
    {{
        "A", // SDL_CONTROLLER_BUTTON_A
        "B", // SDL_CONTROLLER_BUTTON_B
        "X", // SDL_CONTROLLER_BUTTON_X
        "Y", // SDL_CONTROLLER_BUTTON_Y
        // I18N: name of buttons on gamepads
        _("Back"), // SDL_CONTROLLER_BUTTON_BACK
        // I18N: name of buttons on gamepads
        _("Guide"), // SDL_CONTROLLER_BUTTON_GUIDE
        // I18N: name of buttons on gamepads
        _("Start"), // SDL_CONTROLLER_BUTTON_START
        // I18N: name of buttons on gamepads
        _("Left stick"), // SDL_CONTROLLER_BUTTON_LEFTSTICK
        // I18N: name of buttons on gamepads
        _("Right stick"), // SDL_CONTROLLER_BUTTON_RIGHTSTICK
        // I18N: name of buttons on gamepads
        _("Left shoulder"), // SDL_CONTROLLER_BUTTON_LEFTSHOULDER
        // I18N: name of buttons on gamepads
        _("Right shoulder"), // SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
        // I18N: name of buttons on gamepads
        _("DPad up"), // SDL_CONTROLLER_BUTTON_DPAD_UP
        // I18N: name of buttons on gamepads
        _("DPad down"), // SDL_CONTROLLER_BUTTON_DPAD_DOWN
        // I18N: name of buttons on gamepads
        _("DPad left"), // SDL_CONTROLLER_BUTTON_DPAD_LEFT
        // I18N: name of buttons on gamepads
        _("DPad right"), // SDL_CONTROLLER_BUTTON_DPAD_RIGHT

        // Below are extensions after SDL2 header SDL_CONTROLLER_BUTTON_MAX
        // I18N: name of buttons on gamepads
        _("Left thumbstick right"), // SDL_CONTROLLER_AXIS_LEFTX_RIGHT
        // I18N: name of buttons on gamepads
        _("Left thumbstick left"), // SDL_CONTROLLER_AXIS_LEFTX_LEFT
        // I18N: name of buttons on gamepads
        _("Left thumbstick down"), // SDL_CONTROLLER_AXIS_LEFTY_DOWN
        // I18N: name of buttons on gamepads
        _("Left thumbstick up"), // SDL_CONTROLLER_AXIS_LEFTY_UP
        // I18N: name of buttons on gamepads
        _("Right thumbstick right"), // SDL_CONTROLLER_AXIS_RIGHTX_RIGHT
        // I18N: name of buttons on gamepads
        _("Right thumbstick left"), // SDL_CONTROLLER_AXIS_RIGHTX_LEFT
        // I18N: name of buttons on gamepads
        _("Right thumbstick down"), // SDL_CONTROLLER_AXIS_RIGHTY_DOWN
        // I18N: name of buttons on gamepads
        _("Right thumbstick up"), // SDL_CONTROLLER_AXIS_RIGHTY_UP
        // I18N: name of buttons on gamepads
        _("Left trigger"), // SDL_CONTROLLER_AXIS_TRIGGERLEFT_UP
        // I18N: name of buttons on gamepads
        _("Right trigger") // SDL_CONTROLLER_AXIS_TRIGGERRIGHT_UP
    }};

    const Binding &b = getBinding(action);
    int id = b.getId();
    Input::AxisDirection ad = b.getDirection();
    Input::InputType it = b.getType();
    if (it == Input::IT_STICKBUTTON)
    {
        auto ret = m_sdl_mapping.find(std::make_tuple(id, ad));
        if (ret != m_sdl_mapping.end())
            return readable[ret->second];
    }
    else if (it == Input::IT_STICKMOTION)
    {
        auto ret = m_sdl_mapping.find(std::make_tuple(id, ad));
        if (ret != m_sdl_mapping.end())
        {
            core::stringw name = readable[ret->second];
            if (b.getRange() == Input::AR_FULL)
            {
                if (ad == Input::AD_POSITIVE)
                    name += L" (-+)";
                else if (ad == Input::AD_NEGATIVE)
                    name += L" (+-)";
            }
            return name;
        }
    }
#endif
    return DeviceConfig::getBindingAsString(action);
}   // getBindingAsString

//------------------------------------------------------------------------------
/** Converts the configuration to a string.
 */
irr::core::stringw GamepadConfig::toString()
{
    irr::core::stringw returnString = "";
    returnString += getName().c_str();
    returnString += "\n";
    returnString += DeviceConfig::toString();
    return returnString;
}   // toString

// ----------------------------------------------------------------------------
bool GamepadConfig::getMappingTuple(const std::string& rhs,
                                    std::tuple<int, Input::AxisDirection>& t)
{
#ifndef SERVER_ONLY
    if (rhs[0] == 'b')
    {
        int button = -1;
        if (StringUtils::fromString(&rhs[1], button) && button >= 0)
        {
            std::get<0>(t) = button;
            std::get<1>(t) = Input::AD_NEUTRAL;
            return true;
        }
    }
    if (rhs[0] == 'h')
    {
        int hat = -1;
        if (StringUtils::fromString(&rhs[1], hat) && hat >= 0)
        {
            int direction = -1;
            if (rhs.size() > 3 &&
                StringUtils::fromString(&rhs[3], direction) && direction >= 0)
            {
                if (m_hat_count == 0)
                    return false;
                int hat_start = m_button_count - (m_hat_count * 4);
                switch (direction)
                {
                case 1:
                {
                    std::get<0>(t) = hat_start + (hat * 4);
                    std::get<1>(t) = Input::AD_NEUTRAL;
                    return true;
                }
                case 2:
                {
                    std::get<0>(t) = hat_start + (hat * 4) + 1;
                    std::get<1>(t) = Input::AD_NEUTRAL;
                    return true;
                }
                case 4:
                {
                    std::get<0>(t) = hat_start + (hat * 4) + 2;
                    std::get<1>(t) = Input::AD_NEUTRAL;
                    return true;
                }
                case 8:
                {
                    std::get<0>(t) = hat_start + (hat * 4) + 3;
                    std::get<1>(t) = Input::AD_NEUTRAL;
                    return true;
                }
                default:
                    break;
                }
            }
        }
    }
    if ((rhs[0] == '+' || rhs[0] == '-') && rhs[1] == 'a' && rhs.size() > 2)
    {
        int axis = -1;
        if (StringUtils::fromString(&rhs[2], axis) && axis >= 0)
        {
            std::get<0>(t) = axis;
            bool positive = rhs[0] == '+';
            // Inverted axis
            if (rhs.back() == '~')
                positive = !positive;
            std::get<1>(t) = positive ?
                Input::AD_POSITIVE : Input::AD_NEGATIVE;
            return true;
        }
    }
#endif
    return false;
}   // getMappingTuple

// ----------------------------------------------------------------------------
void GamepadConfig::initSDLController(const std::string& mapping, int buttons,
                                      int axes, int hats)
{
    if (!m_sdl_mapping.empty() ||
        m_axis_count > 0 || m_button_count > 0 || m_hat_count > 0)
        return;

    m_button_count = buttons;
    m_axis_count = axes;
    m_hat_count = hats;
#ifndef SERVER_ONLY
    if (mapping.empty())
        return;

    // We need to maunally parse the mapping as API from SDL2 is not enough
    std::map<std::string, int> lhs_mapping =
    {
        { "a", SDL_CONTROLLER_BUTTON_A },
        { "b", SDL_CONTROLLER_BUTTON_B },
        { "x", SDL_CONTROLLER_BUTTON_X },
        { "y", SDL_CONTROLLER_BUTTON_Y },
        { "back", SDL_CONTROLLER_BUTTON_BACK },
        { "guide", SDL_CONTROLLER_BUTTON_GUIDE },
        { "start", SDL_CONTROLLER_BUTTON_START },
        { "leftstick", SDL_CONTROLLER_BUTTON_LEFTSTICK },
        { "rightstick", SDL_CONTROLLER_BUTTON_RIGHTSTICK },
        { "leftshoulder", SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
        { "rightshoulder", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
        { "dpup", SDL_CONTROLLER_BUTTON_DPAD_UP },
        { "dpdown", SDL_CONTROLLER_BUTTON_DPAD_DOWN },
        { "dpleft", SDL_CONTROLLER_BUTTON_DPAD_LEFT },
        { "dpright", SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
        { "+leftx", SDL_CONTROLLER_AXIS_LEFTX_RIGHT },
        { "-leftx", SDL_CONTROLLER_AXIS_LEFTX_LEFT },
        { "+lefty", SDL_CONTROLLER_AXIS_LEFTY_DOWN },
        { "-lefty", SDL_CONTROLLER_AXIS_LEFTY_UP },
        { "+rightx", SDL_CONTROLLER_AXIS_RIGHTX_RIGHT },
        { "-rightx", SDL_CONTROLLER_AXIS_RIGHTX_LEFT },
        { "+righty", SDL_CONTROLLER_AXIS_RIGHTY_DOWN },
        { "-righty", SDL_CONTROLLER_AXIS_RIGHTY_UP },
        { "lefttrigger", SDL_CONTROLLER_AXIS_TRIGGERLEFT_UP },
        { "righttrigger", SDL_CONTROLLER_AXIS_TRIGGERRIGHT_UP }
    };

    std::vector<std::string> m1 = StringUtils::split(mapping, ',');
    for (unsigned i = 2; i < m1.size(); i++)
    {
        std::vector<std::string> m2 = StringUtils::split(m1[i], ':');
        if (m2.size() != 2)
            continue;
        std::string& lhs = m2[0];
        std::string& rhs = m2[1];
        if (lhs.empty() || rhs.size() < 2)
            continue;
        std::tuple<int, Input::AxisDirection> t;
        auto ret = lhs_mapping.find(lhs);
        if (ret != lhs_mapping.end())
        {
            if ((lhs.compare("lefttrigger") == 0 ||
                lhs.compare("righttrigger") == 0) && rhs[0] == 'a')
            {
                // If trigger direction is not specified use positive
                rhs.insert(0, "+");
            }
            bool found = getMappingTuple(rhs, t);
            if (found)
                m_sdl_mapping[t] = ret->second;
            continue;
        }

        // Combined axes handling, extract them manually
        std::array<const char*, 4> axes =
        {{
            "leftx", "lefty", "rightx", "righty"
        }};
        for (const char* a : axes)
        {
            if (lhs.compare(a) == 0)
            {
                if ((rhs[0] == '+' || rhs[0] == '-') && rhs[1] == 'a')
                {
                    // Single half axis
                    lhs.insert(0, rhs[0] == '+' ? "+" : "-");
                    auto ret = lhs_mapping.find(lhs);
                    if (ret != lhs_mapping.end())
                    {
                        bool found = getMappingTuple(rhs, t);
                        if (found)
                            m_sdl_mapping[t] = ret->second;
                    }
                }
                else if (rhs[0] == 'a')
                {
                    auto ret = lhs_mapping.find(std::string("+") + a);
                    if (ret != lhs_mapping.end())
                    {
                        bool found = getMappingTuple(std::string("+") + rhs, t);
                        if (found)
                            m_sdl_mapping[t] = ret->second;
                    }
                    ret = lhs_mapping.find(std::string("-") + a);
                    if (ret != lhs_mapping.end())
                    {
                        bool found = getMappingTuple(std::string("-") + rhs, t);
                        if (found)
                            m_sdl_mapping[t] = ret->second;
                    }
                }
            }
        }
    }
#endif
}   // initSDLController

// ----------------------------------------------------------------------------
void GamepadConfig::initSDLMapping()
{
}   // initSDLMapping
