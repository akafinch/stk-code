//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 Glenn De Jonghe
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

#define DEBUG_MENU_ITEM 0

#include "states_screens/networking_lobby.hpp"

#include <string>
#include <iostream>

#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "guiengine/widgets/bubble_widget.hpp"
#include "guiengine/widgets/icon_button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/list_widget.hpp"
#include "guiengine/widgets/ribbon_widget.hpp"
#include "guiengine/widgets/text_box_widget.hpp"
#include "input/device_manager.hpp"
#include "input/input_manager.hpp"
#include "io/file_manager.hpp"
#include "network/network_player_profile.hpp"
#include "network/protocols/client_lobby.hpp"
#include "network/protocols/server_lobby.hpp"
#include "network/servers_manager.hpp"
#include "network/stk_host.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/dialogs/message_dialog.hpp"
#include "utils/translation.hpp"

using namespace Online;
using namespace GUIEngine;

DEFINE_SCREEN_SINGLETON( NetworkingLobby );

/** This is the lobby screen that is shown on all clients, but not on the
 *  server. It shows currently connected clients, and allows the 'master'
 *  client (i.e. the stk instance that created the server) to control the
 *  server. This especially means that it can initialise the actual start
 *  of the racing.
 *  This class is responsible for creating the ActivePlayers data structure
 *  for all local players (the ActivePlayer maps device to player, i.e.
 *  controls which device is used by which player). Note that a server
 *  does not create an instance of this class and will create the ActivePlayer
 *  data structure in LobbyProtocol::loadWorld().
 */
// ----------------------------------------------------------------------------
NetworkingLobby::NetworkingLobby() : Screen("online/networking_lobby.stkgui")
{
    m_player_list = NULL;
}   // NetworkingLobby

// ----------------------------------------------------------------------------

void NetworkingLobby::loadedFromFile()
{
    m_back_widget = getWidget<IconButtonWidget>("back");
    assert(m_back_widget != NULL);

    m_start_button = getWidget<IconButtonWidget>("start");
    assert(m_start_button!= NULL);

    m_text_bubble = getWidget<LabelWidget>("text");
    assert(m_text_bubble != NULL);

    m_chat_box = getWidget<TextBoxWidget>("chat");
    assert(m_chat_box != NULL);

    m_player_list = getWidget<ListWidget>("players");
    assert(m_player_list!= NULL);

    m_exit_widget = getWidget<IconButtonWidget>("exit");
    assert(m_exit_widget != NULL);

}   // loadedFromFile

// ---------------------------------------------------------------------------
void NetworkingLobby::beforeAddingWidget()
{
} // beforeAddingWidget

// ----------------------------------------------------------------------------
/** This function is a callback from the parent class, and is called each time
 *  this screen is shown to initialise the screen. This class is responsible
 *  for creating the active players and binding them to the right device.
 */
void NetworkingLobby::init()
{
    m_server_info.clear();
    Screen::init();
    setInitialFocus();
    Server* server = ServersManager::get()->getJoinedServer();
    if (server)
    {
        m_server_name = server->getName();
        core::stringw each_line;
        //I18N: In the networking lobby
        each_line = _("Server name: %s", m_server_name);
        m_server_info.push_back(each_line);

        const core::stringw& difficulty_name =
            race_manager->getDifficultyName(server->getDifficulty());
        //I18N: In the networking lobby
        each_line = _("Difficulty: %s", difficulty_name);
        m_server_info.push_back(each_line);

        //I18N: In the networking lobby
        each_line = _("Max players: %d", server->getMaxPlayers());
        m_server_info.push_back(each_line);

        //I18N: In the networking lobby
        core::stringw mode = RaceManager::getNameOf(server->getRaceMinorMode());
        each_line = _("Game mode: %s", mode);

        race_manager->setMinorMode(server->getRaceMinorMode());
        race_manager->setMajorMode(server->getRaceMajorMode());
        race_manager->setDifficulty(server->getDifficulty());

        m_server_info.push_back(each_line);
    }

    m_start_button->setVisible(false);

    // For now create the active player and bind it to the right
    // input device.
    InputDevice *device = input_manager->getDeviceManager()->getLatestUsedDevice();
    PlayerProfile* profile = PlayerManager::getCurrentPlayer();
    StateManager::get()->createActivePlayer(profile, device);
}   // init

// ----------------------------------------------------------------------------
void NetworkingLobby::addMoreServerInfo(const core::stringw& info)
{
}   // addMoreServerInfo

// ----------------------------------------------------------------------------
void NetworkingLobby::onUpdate(float delta)
{
    auto lp = LobbyProtocol::get<LobbyProtocol>();
    if (!lp)
    {
        const core::stringw connect_msg = StringUtils::loadingDots(
            _("Connecting to server %s", m_server_name));
        m_text_bubble->setText(connect_msg, true);
        m_start_button->setVisible(false);
    }
    else
    {
        core::stringw total_msg;
        for (auto& string : m_server_info)
        {
            total_msg += string;
            total_msg += L"\n";
        }
        m_text_bubble->setText(total_msg, true);
    }
    if (NetworkConfig::get()->isClient() &&
        STKHost::get()->isAuthorisedToControl())
    {
        m_start_button->setVisible(true);
    }

}   // onUpdate

// ----------------------------------------------------------------------------

void NetworkingLobby::eventCallback(Widget* widget, const std::string& name,
                                    const int playerID)
{
    if (name == m_back_widget->m_properties[PROP_ID])
    {
        StateManager::get()->escapePressed();
        return;
    }

    RibbonWidget* ribbon = dynamic_cast<RibbonWidget*>(widget);
    if (ribbon == NULL) return;
    const std::string &selection =
                     ribbon->getSelectionIDString(PLAYER_ID_GAME_MASTER);

    if (selection == m_exit_widget->m_properties[PROP_ID])
    {
        StateManager::get()->escapePressed();
    }
    else if (selection == m_start_button->m_properties[PROP_ID])
    {
        if (NetworkConfig::get()->isServer())
        {
            auto slrp = LobbyProtocol::get<ServerLobby>();
            slrp->startSelection();
        }
        else
        {
            // Send a message to the server to start
            NetworkString start(PROTOCOL_LOBBY_ROOM);
            start.setSynchronous(true);
            start.addUInt8(LobbyProtocol::LE_REQUEST_BEGIN);
            STKHost::get()->sendToServer(&start, true);
        }
    }
}   // eventCallback

// ----------------------------------------------------------------------------

void NetworkingLobby::tearDown()
{
}   // tearDown

// ----------------------------------------------------------------------------

bool NetworkingLobby::onEscapePressed()
{
    STKHost::get()->shutdown();
    return true; // close the screen
}   // onEscapePressed

// ----------------------------------------------------------------------------
void NetworkingLobby::onDisabledItemClicked(const std::string& item)
{

}   // onDisabledItemClicked

// ----------------------------------------------------------------------------
void NetworkingLobby::setInitialFocus()
{
}   // setInitialFocus

// ----------------------------------------------------------------------------
void NetworkingLobby::onDialogClose()
{
    setInitialFocus();
}   // onDialogClose()

// ----------------------------------------------------------------------------
void NetworkingLobby::addPlayer(NetworkPlayerProfile *profile)
{
    // In GUI-less server this function will be called without proper
    // initialisation
    if(m_player_list)
        m_player_list->addItem(StringUtils::toString(profile->getGlobalPlayerId()),
                               profile->getName());
}  // addPlayer

// ----------------------------------------------------------------------------
void NetworkingLobby::removePlayer(NetworkPlayerProfile *profile)
{
}   // removePlayer
