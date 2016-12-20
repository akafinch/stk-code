//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2014-2015 SuperTuxKart-Team
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

#include "states_screens/dialogs/multitouch_settings_dialog.hpp"

#include "config/user_config.hpp"
#include "guiengine/widgets/spinner_widget.hpp"
#include "utils/translation.hpp"

#include <IGUIEnvironment.h>


using namespace GUIEngine;
using namespace irr;
using namespace irr::core;
using namespace irr::gui;

// -----------------------------------------------------------------------------

MultitouchSettingsDialog::MultitouchSettingsDialog(const float w, const float h)
        : ModalDialog(w, h)
{
    loadFromFile("multitouch_settings.stkgui");
}

// -----------------------------------------------------------------------------

MultitouchSettingsDialog::~MultitouchSettingsDialog()
{
}

// -----------------------------------------------------------------------------

void MultitouchSettingsDialog::beforeAddingWidgets()
{
    updateValues();
}

// -----------------------------------------------------------------------------

GUIEngine::EventPropagation MultitouchSettingsDialog::processEvent(
                                                const std::string& eventSource)
{
    if (eventSource == "close")
    {
        SpinnerWidget* scale = getWidget<SpinnerWidget>("scale");
        assert(scale != NULL);
        UserConfigParams::m_multitouch_scale = scale->getValue() / 100.0f;
        
        SpinnerWidget* deadzone_edge = getWidget<SpinnerWidget>("deadzone_edge");
        assert(deadzone_edge != NULL);
        UserConfigParams::m_multitouch_deadzone_edge = 
                                        deadzone_edge->getValue() / 100.0f;
                    
        SpinnerWidget* deadzone_center = getWidget<SpinnerWidget>("deadzone_center");
        assert(deadzone_center != NULL);
        UserConfigParams::m_multitouch_deadzone_center = 
                                        deadzone_center->getValue() / 100.0f;
    
        user_config->saveConfig();

        ModalDialog::dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "restore")
    {
        UserConfigParams::m_multitouch_scale.revertToDefaults();
        UserConfigParams::m_multitouch_deadzone_edge.revertToDefaults();
        UserConfigParams::m_multitouch_deadzone_center.revertToDefaults();
        
        updateValues();
        
        return GUIEngine::EVENT_BLOCK;
    }

    return GUIEngine::EVENT_LET;
}   // processEvent

// -----------------------------------------------------------------------------

void MultitouchSettingsDialog::updateValues()
{
    SpinnerWidget* scale = getWidget<SpinnerWidget>("scale");
    assert(scale != NULL);
    scale->setValue((int)(UserConfigParams::m_multitouch_scale * 100.0f));
    
    SpinnerWidget* deadzone_edge = getWidget<SpinnerWidget>("deadzone_edge");
    assert(deadzone_edge != NULL);
    deadzone_edge->setValue(
                (int)(UserConfigParams::m_multitouch_deadzone_edge * 100.0f));
                
    SpinnerWidget* deadzone_center = getWidget<SpinnerWidget>("deadzone_center");
    assert(deadzone_center != NULL);
    deadzone_center->setValue(
                (int)(UserConfigParams::m_multitouch_deadzone_center * 100.0f));
}

// -----------------------------------------------------------------------------
