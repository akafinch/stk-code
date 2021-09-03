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

#include "states_screens/online/networking_lobby.hpp"

#include <cmath>
#include <algorithm>
#include <string>

#include "addons/addons_manager.hpp"
#include "config/user_config.hpp"
#include "config/player_manager.hpp"
#include "font/font_manager.hpp"
#include "graphics/irr_driver.hpp"
#include "guiengine/CGUISpriteBank.hpp"
#include "guiengine/emoji_keyboard.hpp"
#include "guiengine/scalable_font.hpp"
#include "guiengine/widgets/CGUIEditBox.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/icon_button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/list_widget.hpp"
#include "guiengine/widgets/ribbon_widget.hpp"
#include "input/device_manager.hpp"
#include "input/input_manager.hpp"
#include "io/file_manager.hpp"
#include "network/game_setup.hpp"
#include "network/race_event_manager.hpp"
#include "network/network_config.hpp"
#include "network/network_string.hpp"
#include "network/protocols/connect_to_server.hpp"
#include "network/protocols/game_protocol.hpp"
#include "network/protocols/client_lobby.hpp"
#include "network/server.hpp"
#include "network/stk_host.hpp"
#include "network/network_timer_synchronizer.hpp"
#include "states_screens/dialogs/addons_pack.hpp"
#include "states_screens/dialogs/splitscreen_player_dialog.hpp"
#include "states_screens/dialogs/network_player_dialog.hpp"
#include "states_screens/dialogs/server_configuration_dialog.hpp"
#include "states_screens/state_manager.hpp"
#include "tracks/track.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

using namespace Online;
using namespace GUIEngine;

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
    m_server_info_height = 0;
    m_header_text_width = 0;

    m_back_widget = NULL;
    m_header = NULL;
    m_text_bubble = NULL;
    m_timeout_message = NULL;
    m_start_button = NULL;
    m_player_list = NULL;
    m_chat_box = NULL;
    m_send_button = NULL;
    m_icon_bank = NULL;
    m_reload_server_info = false;
    m_addon_install = NULL;

    // Allows one to update chat and counter even if dialog window is opened
    setUpdateInBackground(true);
}   // NetworkingLobby

// ----------------------------------------------------------------------------

void NetworkingLobby::loadedFromFile()
{
    m_header = getWidget<LabelWidget>("lobby-text");
    assert(m_header != NULL);

    m_back_widget = getWidget<IconButtonWidget>("back");
    assert(m_back_widget != NULL);

    m_start_button = getWidget<IconButtonWidget>("start");
    assert(m_start_button!= NULL);

    m_config_button = getWidget<IconButtonWidget>("config");
    assert(m_config_button!= NULL);

    m_text_bubble = getWidget<LabelWidget>("text");
    assert(m_text_bubble != NULL);

    m_timeout_message = getWidget<LabelWidget>("timeout-message");
    assert(m_timeout_message != NULL);

    m_chat_box = getWidget<TextBoxWidget>("chat");
    assert(m_chat_box != NULL);

    m_send_button = getWidget<ButtonWidget>("send");
    assert(m_send_button != NULL);

    m_emoji_button = getWidget<ButtonWidget>("emoji");
    assert(m_emoji_button != NULL);

    m_icon_bank = new irr::gui::STKModifiedSpriteBank(GUIEngine::getGUIEnv());
    video::ITexture* icon_1 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "crown.png"));
    video::ITexture* icon_2 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "difficulty_medium.png"));
    video::ITexture* icon_3 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "main_help.png"));
    video::ITexture* icon_4 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "hourglass.png"));
    video::ITexture* icon_5 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "green_check.png"));
    m_config_texture = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "main_options.png"));
    m_spectate_texture = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "screen_other.png"));
    video::ITexture* icon_6 = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "robot.png"));
    m_addon_texture = irr_driver->getTexture
        (file_manager->getAsset(FileManager::GUI_ICON, "package-update.png"));
    m_icon_bank->addTextureAsSprite(icon_1);
    m_icon_bank->addTextureAsSprite(icon_2);
    m_icon_bank->addTextureAsSprite(icon_3);
    m_icon_bank->addTextureAsSprite(icon_4);
    m_icon_bank->addTextureAsSprite(icon_5);
    m_icon_bank->addTextureAsSprite(m_spectate_texture);
    m_icon_bank->addTextureAsSprite(icon_6);

    m_icon_bank->setScale((float)GUIEngine::getFontHeight() / 96.0f);
    m_icon_bank->setTargetIconSize(128, 128);
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
    Screen::init();

    m_player_list = getWidget<ListWidget>("players");
    assert(m_player_list!= NULL);

    m_server_configurable = false;
    m_player_names.clear();
    m_allow_change_team = false;
    m_has_auto_start_in_server = false;
    m_client_live_joinable = false;
    m_addon_install = NULL;
    m_ping_update_timer = 0;
    m_start_timeout = std::numeric_limits<float>::max();
    m_cur_starting_timer = std::numeric_limits<int64_t>::max();
    m_min_start_game_players = 0;
    m_timeout_message->setVisible(false);

    m_start_text = _("Start race");
    //I18N: In the networking lobby, ready button is to allow player to tell
    //server that he is ready for next game for owner less server
    m_ready_text = _("Ready");
    //I18N: Live join is displayed in networking lobby to allow players
    //to join the current started in-progress game
    m_live_join_text = _("Live join");
    //I18N: In networking lobby to configuration server settings
    m_configuration_text = _("Configuration");
    //I18N: Spectate is displayed in networking lobby to allow players
    //to join the current started in-progress game
    m_spectate_text = _("Spectate");
    m_install_addon_text = _("Install addon");

    //I18N: In the networking lobby
    setHeader(_("Lobby"));

    m_server_info_height = GUIEngine::getFont()->getDimension(L"X").Height;
    m_start_button->setVisible(false);
    m_config_button->setVisible(false);
    m_state = LS_CONNECTING;
    m_chat_box->setVisible(false);
    m_chat_box->setActive(false);
    m_chat_box->setTextBoxType(TBT_CAP_SENTENCES);
    m_send_button->setVisible(false);
    m_send_button->setActive(false);
    // Unicode enter arrow
    m_send_button->setText(L"\u21B2");
    m_emoji_button->setVisible(false);
    m_emoji_button->setActive(false);
    // Unicode smile emoji
    m_emoji_button->setText(L"\u263A");

    // Connect to server now if we have saved players and not disconnected
    if (!LobbyProtocol::get<LobbyProtocol>() &&
        !NetworkConfig::get()->getNetworkPlayers().empty())
        std::make_shared<ConnectToServer>(m_joined_server)->requestStart();

    if (NetworkConfig::get()->getNetworkPlayers().empty())
    {
        m_state = LS_ADD_PLAYERS;
    }
    else if (NetworkConfig::get()->isClient())
    {
        m_chat_box->clearListeners();
        if (UserConfigParams::m_lobby_chat)
        {
            m_chat_box->addListener(this);
            m_chat_box->setText("");
            m_chat_box->setVisible(true);
            m_chat_box->setActive(true);
            m_send_button->setVisible(true);
            m_send_button->setActive(true);
            m_emoji_button->setVisible(true);
            m_emoji_button->setActive(true);
        }
        else
        {
            m_chat_box->setText(
                _("Chat is disabled, enable in options menu."));
            m_chat_box->setVisible(true);
            m_chat_box->setActive(false);
            m_send_button->setVisible(true);
            m_send_button->setActive(false);
            m_emoji_button->setVisible(true);
            m_emoji_button->setActive(false);
        }
        if (auto cl = LobbyProtocol::get<ClientLobby>())
        {
            if (cl->isLobbyReady())
                updatePlayers();
        }
    }
#ifndef SERVER_ONLY
    gui::IGUIStaticText* st =
        m_text_bubble->getIrrlichtElement<gui::IGUIStaticText>();
    st->setMouseCallback([](gui::IGUIStaticText* text, SEvent::SMouseInput mouse)->bool
    {
        if (mouse.Event == EMIE_LMOUSE_PRESSED_DOWN)
        {
            core::position2di p;
            p.X = mouse.X - text->getAbsolutePosition().UpperLeftCorner.X;
            p.Y = mouse.Y - text->getAbsolutePosition().UpperLeftCorner.Y;
            if (p.X < 0 || p.Y < 0)
                return false;

            const std::vector<GlyphLayout>& gls = text->getGlyphLayouts();
            if (gls.empty())
                return false;

            gui::IGUIFont* font = GUIEngine::getFont();
            auto width_per_line = gui::getGlyphLayoutsWidthPerLine(gls,
                font->getInverseShaping(), font->getScale());
            if (width_per_line.empty())
                return false;

            // Check if the line is RTL
            auto position = text->getAbsolutePosition();
            bool rtl = (gls[0].flags & gui::GLF_RTL_LINE) != 0;
            int offset = 0;
            int cur_line = 0;
            if (rtl)
                offset = (s32)(position.getWidth() - width_per_line[cur_line]);

            float next_line_height = font->getHeightPerLine();
            if (width_per_line.size() > 1 &&
                width_per_line.size() * next_line_height > position.getHeight())
            {
                next_line_height = (float)position.getHeight() /
                    (float)width_per_line.size();
            }

            int idx = -1;
            core::recti r;
            r.UpperLeftCorner.X = r.LowerRightCorner.X = offset;
            r.LowerRightCorner.Y = (int)next_line_height;
            bool line_changed = false;
            for (unsigned i = 0; i < gls.size(); i++)
            {
                const GlyphLayout& glyph_layout = gls[i];
                if ((glyph_layout.flags & GLF_NEWLINE) != 0)
                {
                    r.UpperLeftCorner.Y += (int)next_line_height;
                    r.LowerRightCorner.Y += (int)next_line_height;
                    cur_line++;
                    line_changed = true;
                    continue;
                }
                if (line_changed)
                {
                    line_changed = false;
                    rtl = (glyph_layout.flags & gui::GLF_RTL_LINE) != 0;
                    offset = 0;
                    if (rtl)
                    {
                        offset = (s32)
                            (position.getWidth() - width_per_line[cur_line]);
                    }
                    r.UpperLeftCorner.X = r.LowerRightCorner.X = offset;
                }
                r.LowerRightCorner.X += int(
                    (float)glyph_layout.x_advance * font->getInverseShaping() *
                    font->getScale());
                if (r.isPointInside(p))
                {
                    idx = i;
                    break;
                }
            }
            if (idx == -1)
                return false;

            auto s = gls[idx].orig_string;
            unsigned cluster = gls[idx].cluster.front();
            if (cluster > s->size())
                return false;

            size_t start = s->substr(0, cluster).rfind(U'\n');
            size_t end = s->substr(cluster, s->size()).find(U'\n');
            if (start == std::string::npos)
                start = 0;
            else
                start += 1; // Skip newline character
            if (end == std::string::npos)
                end = s->size();
            else
                end += cluster - start;
            if (idx == -1)
                return false;
            std::u32string substr = s->substr(start, end);
            int local_pos = (int)cluster - (int)start;
            if ((size_t)local_pos > substr.size())
                return false;

            size_t space_pos_1, space_pos_2;
            size_t addon = substr.substr(0, local_pos).rfind(U"/installaddon");
            if (addon == std::string::npos)
                goto handle_player_message_copy;
            space_pos_1 = substr.substr(0, local_pos).rfind(U' ');
            space_pos_2 = substr.substr(local_pos, substr.size()).find(U' ');
            if (space_pos_1 == std::string::npos || space_pos_1 < 13)
                goto handle_player_message_copy;
            if (space_pos_2 == std::string::npos)
                space_pos_2 = substr.size();
            else
                space_pos_2 += local_pos - space_pos_1 - 1;
            if (space_pos_1 != std::string::npos && addon + 13 == space_pos_1)
            {
                std::string argv = StringUtils::utf32ToUtf8(
                    substr.substr(addon + 14, space_pos_2));
                AddonsPack::install(argv);
                return true;
            }

handle_player_message_copy:
            for (auto& p : NetworkingLobby::getInstance()->m_player_names)
            {
                size_t colon = substr.substr(0, local_pos).rfind(U": ");
                if (colon == std::string::npos)
                    continue;
                std::u32string player_name = substr.substr(0, colon);
                if (player_name.empty())
                    continue;
                int padding = 2;
                // RTL handling
                if (player_name[0] == 0x200F || player_name[0] == 0x200E)
                {
                    player_name = player_name.substr(1, player_name.size() - 1);
                    padding++;
                }
                if (StringUtils::wideToUtf32(p.second.m_user_name)
                    .find(player_name) != std::string::npos)
                {
                    GUIEngine::getGUIEnv()->getOSOperator()->copyToClipboard(
                        StringUtils::utf32ToUtf8(
                        substr.substr(player_name.size() + padding)).c_str());
                    return true;
                }
            }
            GUIEngine::getGUIEnv()->getOSOperator()->copyToClipboard(
                StringUtils::utf32ToUtf8(substr).c_str());
            return true;
        }
        return false;
    });
#endif
}   // init

// ----------------------------------------------------------------------------
void NetworkingLobby::addMoreServerInfo(core::stringw info)
{
#ifndef SERVER_ONLY
    const unsigned box_width = m_text_bubble->getDimension().Width;
    const float box_height = m_text_bubble->getDimension().Height;
    std::vector<GlyphLayout> cur_info;
    font_manager->initGlyphLayouts(info, cur_info, gui::SF_DISABLE_CACHE);
    gui::IGUIFont* font = GUIEngine::getFont();
    gui::breakGlyphLayouts(cur_info, box_width,
        font->getInverseShaping(), font->getScale());
    m_server_info.insert(m_server_info.end(), cur_info.begin(),
        cur_info.end());
    gui::eraseTopLargerThan(m_server_info, font->getHeightPerLine(),
        box_height);

    // Don't take the newly added newline marker int layouts for linebreaking
    // height calculation
    gui::GlyphLayout new_line = { 0 };
    new_line.flags = gui::GLF_NEWLINE;
    m_server_info.push_back(new_line);
    updateServerInfos();
#endif
}   // addMoreServerInfo

// ----------------------------------------------------------------------------
void NetworkingLobby::updateServerInfos()
{
#ifndef SERVER_ONLY
    if (GUIEngine::getCurrentScreen() != this)
        return;

    gui::IGUIStaticText* st =
        m_text_bubble->getIrrlichtElement<gui::IGUIStaticText>();
    st->setUseGlyphLayoutsOnly(true);
    st->setGlyphLayouts(m_server_info);
#endif
}   // updateServerInfos

// ----------------------------------------------------------------------------
void NetworkingLobby::onUpdate(float delta)
{
    m_addon_install = NULL;
    if (NetworkConfig::get()->isServer() || !STKHost::existHost())
        return;

    if (m_header_text_width > m_header->m_w)
    {
        m_header->update(delta);
        if (m_header->scrolledOff())
            m_header->setText(m_header->getText(), true);
    }

    if (m_reload_server_info)
    {
        m_reload_server_info = false;
        updateServerInfos();
    }

    if (m_has_auto_start_in_server)
    {
        m_start_button->setLabel(m_ready_text);
    }
    else
        m_start_button->setLabel(m_start_text);

    m_start_button->setVisible(false);

    m_config_button->setLabel(m_configuration_text);
    m_config_button->setVisible(false);
    m_config_button->setImage(m_config_texture);
    m_client_live_joinable = false;

    if (m_player_list && StkTime::getMonoTimeMs() > m_ping_update_timer)
    {
        m_ping_update_timer = StkTime::getMonoTimeMs() + 2000;
        updatePlayerPings();
    }

    auto cl = LobbyProtocol::get<ClientLobby>();
    if (cl && UserConfigParams::m_lobby_chat)
    {
        if (cl->serverEnabledChat() && !m_send_button->isActivated())
        {
            m_chat_box->setActive(true);
            m_send_button->setActive(true);
            m_emoji_button->setActive(true);
        }
        else if (!cl->serverEnabledChat() && m_send_button->isActivated())
        {
            m_chat_box->setActive(false);
            m_send_button->setActive(false);
            m_emoji_button->setActive(false);
        }
    }
    if (cl && cl->isWaitingForGame())
    {
        m_start_button->setVisible(false);
        m_timeout_message->setVisible(true);
        auto progress = cl->getGameStartedProgress();
        core::stringw msg;
        core::stringw current_track;
        Track* t = cl->getPlayingTrack();
        if (t)
            current_track = t->getName();
        std::string missing_addon_track_id;
        // Show addon identity so player can install it live in lobby
        if (current_track.empty())
        {
            std::string track_id = cl->getPlayingTrackIdent();
            if (StringUtils::startsWith(track_id, "addon_"))
            {
                missing_addon_track_id = track_id.substr(6).c_str();
                current_track = missing_addon_track_id.c_str();
            }
        }
        if (progress.first != std::numeric_limits<uint32_t>::max())
        {
            if (!current_track.empty())
            {
                //I18N: In the networking lobby, show when player is required to
                //wait before the current game finish with remaining time,
                //showing the current track name inside bracket
                msg = _("Please wait for the current game's (%s) end, "
                    "estimated remaining time: %s.", current_track,
                    StringUtils::timeToString((float)progress.first).c_str());
            }
            else
            {
                //I18N: In the networking lobby, show when player is required
                //to wait before the current game finish with remaining time
                msg = _("Please wait for the current game's end, "
                    "estimated remaining time: %s.",
                    StringUtils::timeToString((float)progress.first).c_str());
            }
        }
        else if (progress.second != std::numeric_limits<uint32_t>::max())
        {
            if (!current_track.empty())
            {
                //I18N: In the networking lobby, show when player is required
                //to wait before the current game finish with progress in
                //percent, showing the current track name inside bracket
                msg = _("Please wait for the current game's (%s) end, "
                    "estimated progress: %s%.", current_track,
                    progress.second);
            }
            else
            {
                //I18N: In the networking lobby, show when player is required
                //to wait before the current game finish with progress in
                //percent
                msg = _("Please wait for the current game's end, "
                    "estimated progress: %d%.", progress.second);
            }
        }
        else
        {
            //I18N: In the networking lobby, show when player is required to
            //wait before the current game finish
            msg = _("Please wait for the current game's end.");
        }

        // You can live join or spectator if u have the current play track
        // and network timer is synchronized, and no game protocols exist
        bool no_gep = !RaceEventManager::get() ||
            RaceEventManager::get()->protocolStopped();
        bool no_gp = GameProtocol::emptyInstance();
        if (t &&
            STKHost::get()->getNetworkTimerSynchronizer()->isSynchronised() &&
            cl->isServerLiveJoinable() && no_gep && no_gp)
        {
            m_client_live_joinable = true;
        }
        else
            m_client_live_joinable = false;

        m_timeout_message->setText(msg, false);
        m_cur_starting_timer = std::numeric_limits<int64_t>::max();
#ifndef SERVER_ONLY
        if (!GUIEngine::ModalDialog::isADialogActive() &&
            !ScreenKeyboard::isActive())
        {
            m_addon_install = addons_manager->getAddon(
                Addon::createAddonId(missing_addon_track_id));
            if (m_addon_install)
            {
                m_config_button->setLabel(m_install_addon_text);
                m_config_button->setImage(m_addon_texture);
                m_config_button->setVisible(true);
                return;
            }
        }
#endif

        if (m_client_live_joinable)
        {
            if (RaceManager::get()->supportsLiveJoining())
            {
                m_start_button->setVisible(true);
                m_start_button->setLabel(m_live_join_text);
            }
            else
                m_start_button->setVisible(false);
            m_config_button->setLabel(m_spectate_text);
            m_config_button->setImage(m_spectate_texture);
            m_config_button->setVisible(true);
        }
        return;
    }

    if (m_has_auto_start_in_server && m_player_list)
    {
        m_timeout_message->setVisible(true);
        unsigned cur_player = m_player_list->getItemCount();
        if (cur_player >= m_min_start_game_players &&
            m_cur_starting_timer == std::numeric_limits<int64_t>::max())
        {
            m_cur_starting_timer = (int64_t)StkTime::getMonoTimeMs() +
                (int64_t)(m_start_timeout * 1000.0);
        }
        else if (cur_player < m_min_start_game_players)
        {
            m_cur_starting_timer = std::numeric_limits<int64_t>::max();
            //I18N: In the networking lobby, display the number of players
            //required to start a game for owner-less server
            core::stringw msg =
                _P("Game will start if there is more than %d player.",
               "Game will start if there are more than %d players.",
               (int)(m_min_start_game_players - 1));
            m_timeout_message->setText(msg, false);
        }

        if (m_cur_starting_timer != std::numeric_limits<int64_t>::max())
        {
            int64_t remain = (m_cur_starting_timer -
                (int64_t)StkTime::getMonoTimeMs()) / 1000;
            if (remain < 0)
                remain = 0;
            //I18N: In the networking lobby, display the starting timeout
            //for owner-less server to begin a game
            core::stringw msg = _P("Starting after %d second, "
                "or once everyone has pressed the 'Ready' button.",
                "Starting after %d seconds, "
                "or once everyone has pressed the 'Ready' button.",
                (int)remain);
            m_timeout_message->setText(msg, false);
        }
    }
    else
    {
        m_timeout_message->setVisible(false);
    }

    if (m_state == LS_ADD_PLAYERS)
    {
        m_text_bubble->getIrrlichtElement<gui::IGUIStaticText>()
            ->setUseGlyphLayoutsOnly(false);
        m_text_bubble->setText(_("Everyone:\nPress the 'Select' button to "
                                          "join the game"), false);
        m_start_button->setVisible(false);
        if (!GUIEngine::ModalDialog::isADialogActive())
        {
            input_manager->getDeviceManager()->setAssignMode(DETECT_NEW);
            input_manager->getDeviceManager()->mapFireToSelect(true);
        }
        return;
    }

    m_start_button->setVisible(false);
    if (!cl || !cl->isLobbyReady())
    {
        m_text_bubble->getIrrlichtElement<gui::IGUIStaticText>()
            ->setUseGlyphLayoutsOnly(false);
        core::stringw connect_msg;
        if (m_joined_server)
        {
            connect_msg = StringUtils::loadingDots(
                _("Connecting to server %s", m_joined_server->getName()));
        }
        else
        {
            connect_msg =
                StringUtils::loadingDots(_("Finding a quick play server"));
        }
        m_text_bubble->setText(connect_msg, false);
        m_start_button->setVisible(false);
    }

    m_config_button->setVisible(STKHost::get()->isAuthorisedToControl() &&
        m_server_configurable);

    if (STKHost::get()->isAuthorisedToControl() ||
        (m_has_auto_start_in_server &&
        m_cur_starting_timer != std::numeric_limits<int64_t>::max()))
    {
        m_start_button->setVisible(true);
    }

}   // onUpdate

// ----------------------------------------------------------------------------
void NetworkingLobby::updatePlayerPings()
{
    auto peer_pings = STKHost::get()->getPeerPings();
    for (auto& p : m_player_names)
    {
        core::stringw name_with_ping = p.second.m_user_name;
        const core::stringw& flag = StringUtils::getCountryFlag(
            p.second.m_country_code);
        if (!flag.empty())
        {
            name_with_ping += L" ";
            name_with_ping += flag;
        }
        auto host_online_ids = StringUtils::splitToUInt(p.first, '_');
        if (host_online_ids.size() != 3)
            continue;
        unsigned ping = 0;
        uint32_t host_id = host_online_ids[0];
        if (peer_pings.find(host_id) != peer_pings.end())
            ping = peer_pings.at(host_id);
        if (ping != 0)
        {
            name_with_ping = StringUtils::insertValues(L"%s (%dms)",
                name_with_ping, ping);
        }
        else
            continue;
        int id = m_player_list->getItemID(p.first);
        if (id != -1)
        {
            m_player_list->renameItem(id, name_with_ping, p.second.m_icon_id);
            // Don't show chosen team color for spectator
            if (p.second.isSpectator())
                m_player_list->markItemRed(id, false/*red*/);
            else if (p.second.m_kart_team == KART_TEAM_RED)
                m_player_list->markItemRed(id);
            else if (p.second.m_kart_team == KART_TEAM_BLUE)
                m_player_list->markItemBlue(id);
        }
    }
}   // updatePlayerPings

// ----------------------------------------------------------------------------
bool NetworkingLobby::onEnterPressed(const irr::core::stringw& text)
{
    if (auto cl = LobbyProtocol::get<ClientLobby>())
    {
        if (!text.empty())
        {
            if (text[0] == L'/' && text.size() > 1)
            {
                std::string cmd = StringUtils::wideToUtf8(text);
                cl->handleClientCommand(cmd.erase(0, 1));
            }
            else
                cl->sendChat(text, KART_TEAM_NONE);
        }
    }
    return true;
}   // onEnterPressed

// ----------------------------------------------------------------------------
void NetworkingLobby::eventCallback(Widget* widget, const std::string& name,
                                    const int playerID)
{
    if (name == m_back_widget->m_properties[PROP_ID])
    {
        StateManager::get()->escapePressed();
        return;
    }
    else if (name == m_player_list->m_properties[GUIEngine::PROP_ID])
    {
        auto host_online_local_ids = StringUtils::splitToUInt
            (m_player_list->getSelectionInternalName(), '_');
        if (host_online_local_ids.size() != 3)
        {
            return;
        }
        LobbyPlayer& lp =
            m_player_names.at(m_player_list->getSelectionInternalName());
        // For client server AI it doesn't make any sense to open the dialog
        // There is no way to kick or add handicap to them
        if (STKHost::get()->isClientServer() > 0 && lp.isAI())
            return;
        new NetworkPlayerDialog(host_online_local_ids[0],
            host_online_local_ids[1], host_online_local_ids[2],
            lp.m_user_name, lp.m_country_code, m_allow_change_team,
            lp.m_handicap);
    }   // click on a user
    else if (name == m_send_button->m_properties[PROP_ID])
    {
        onEnterPressed(m_chat_box->getText());
        m_chat_box->setText("");
    }   // send chat message
    else if (name == m_emoji_button->m_properties[PROP_ID] &&
        !ScreenKeyboard::isActive())
    {
        EmojiKeyboard* ek = new EmojiKeyboard(1.0f, 0.40f,
            m_chat_box->getIrrlichtElement<CGUIEditBox>());
        ek->init();
    }
    else if (name == m_start_button->m_properties[PROP_ID])
    {
        if (m_client_live_joinable)
        {
            auto cl = LobbyProtocol::get<ClientLobby>();
            if (cl)
                cl->startLiveJoinKartSelection();
        }
        else
        {
            // Send a message to the server to start
            NetworkString start(PROTOCOL_LOBBY_ROOM);
            start.addUInt8(LobbyProtocol::LE_REQUEST_BEGIN);
            STKHost::get()->sendToServer(&start, true);
        }
    }
    else if (name == m_config_button->m_properties[PROP_ID])
    {
#ifndef SERVER_ONLY
        if (m_addon_install)
        {
            AddonsPack::install(m_addon_install->getDirName());
            return;
        }
#endif
        auto cl = LobbyProtocol::get<ClientLobby>();
        if (m_client_live_joinable && cl)
        {
            NetworkString start(PROTOCOL_LOBBY_ROOM);
            start.setSynchronous(true);
            start.addUInt8(LobbyProtocol::LE_LIVE_JOIN)
                // is spectating
                .addUInt8(1);
            STKHost::get()->sendToServer(&start, true);
            return;
        }
        if (cl)
        {
            new ServerConfigurationDialog(
                RaceManager::get()->isSoccerMode() &&
                cl->getGameSetup()->isSoccerGoalTarget());
        }
    }
}   // eventCallback

// ----------------------------------------------------------------------------
void NetworkingLobby::unloaded()
{
    delete m_icon_bank;
    m_icon_bank = NULL;
}   // unloaded

// ----------------------------------------------------------------------------
void NetworkingLobby::tearDown()
{
    gui::IGUIStaticText* st =
        m_text_bubble->getIrrlichtElement<gui::IGUIStaticText>();
    st->setMouseCallback(nullptr);
    m_player_list = NULL;
    m_joined_server.reset();
    // Server has a dummy network lobby too
    if (!NetworkConfig::get()->isClient())
        return;
    input_manager->getDeviceManager()->mapFireToSelect(false);
    input_manager->getDeviceManager()->setAssignMode(NO_ASSIGN);
}   // tearDown

// ----------------------------------------------------------------------------
bool NetworkingLobby::onEscapePressed()
{
    if (NetworkConfig::get()->isAddingNetworkPlayers())
        NetworkConfig::get()->cleanNetworkPlayers();
    m_joined_server.reset();
    input_manager->getDeviceManager()->mapFireToSelect(false);
    input_manager->getDeviceManager()->setAssignMode(NO_ASSIGN);
    STKHost::get()->shutdown();
    return true; // close the screen
}   // onEscapePressed

// ----------------------------------------------------------------------------
void NetworkingLobby::updatePlayers()
{
    // In GUI-less server this function will be called without proper
    // initialisation
    if (!m_player_list)
        return;

    std::string selected_name = m_player_list->getSelectionInternalName();
    m_player_list->clear();
    m_player_names.clear();

    auto cl = LobbyProtocol::get<ClientLobby>();
    if (!cl)
        return;

    const auto& players = cl->getLobbyPlayers();
    if (players.empty())
        return;

    irr::gui::STKModifiedSpriteBank* icon_bank = m_icon_bank;
    for (unsigned i = 0; i < players.size(); i++)
    {
        const LobbyPlayer& player = players[i];
        if (icon_bank)
        {
            m_player_list->setIcons(icon_bank);
            icon_bank = NULL;
        }
        KartTeam cur_team = player.m_kart_team;
        m_allow_change_team = cur_team != KART_TEAM_NONE;
        const std::string internal_name =
            StringUtils::toString(player.m_host_id) + "_" +
            StringUtils::toString(player.m_online_id) + "_" +
            StringUtils::toString(player.m_local_player_id);
        core::stringw player_name = player.m_user_name;
        const core::stringw& flag = StringUtils::getCountryFlag(
            player.m_country_code);
        if (!flag.empty())
        {
            player_name += L" ";
            player_name += flag;
        }
        m_player_list->addItem(internal_name, player_name,
            player.m_icon_id);
        // Don't show chosen team color for spectator
        if (player.isSpectator())
            m_player_list->markItemRed(i, false/*red*/);
        else if (cur_team == KART_TEAM_RED)
            m_player_list->markItemRed(i);
        else if (cur_team == KART_TEAM_BLUE)
            m_player_list->markItemBlue(i);
        m_player_names[internal_name] = player;
    }
    updatePlayerPings();
    if (!selected_name.empty())
    {
        int id = m_player_list->getItemID(selected_name);
        if (id != -1)
            m_player_list->setSelectionID(id);
    }
}   // updatePlayers

// ----------------------------------------------------------------------------
void NetworkingLobby::openSplitscreenDialog(InputDevice* device)
{
    new SplitscreenPlayerDialog(device);
}   // openSplitscreenDialog

// ----------------------------------------------------------------------------
void NetworkingLobby::addSplitscreenPlayer(irr::core::stringw name)
{
    if (!m_player_list)
        return;
    m_player_list->setIcons(m_icon_bank);
    m_player_list->addItem(StringUtils::wideToUtf8(name), name, 1);
}   // addSplitscreenPlayer

// ----------------------------------------------------------------------------
void NetworkingLobby::finishAddingPlayers()
{
    m_state = LS_CONNECTING;
    std::make_shared<ConnectToServer>(m_joined_server)->requestStart();
    m_start_button->setVisible(false);
    m_chat_box->clearListeners();
    if (UserConfigParams::m_lobby_chat)
    {
        m_chat_box->addListener(this);
        m_chat_box->setVisible(true);
        m_chat_box->setActive(true);
        m_send_button->setVisible(true);
        m_send_button->setActive(true);
        m_emoji_button->setVisible(true);
        m_emoji_button->setActive(true);
    }
    else
    {
        m_chat_box->setText(_("Chat is disabled, enable in options menu."));
        m_chat_box->setVisible(true);
        m_chat_box->setActive(false);
        m_send_button->setVisible(true);
        m_send_button->setActive(false);
        m_emoji_button->setVisible(true);
        m_emoji_button->setActive(false);
    }
}   // finishAddingPlayers

// ----------------------------------------------------------------------------
void NetworkingLobby::cleanAddedPlayers()
{
    if (!m_player_list)
        return;
    m_player_list->clear();
    m_player_names.clear();
}   // cleanAddedPlayers

// ----------------------------------------------------------------------------
void NetworkingLobby::initAutoStartTimer(bool grand_prix_started,
                                         unsigned min_players,
                                         float start_timeout,
                                         unsigned server_max_player)
{
    if (min_players == 0 || start_timeout == 0.0f)
        return;

    m_has_auto_start_in_server = true;
    m_min_start_game_players = grand_prix_started ? 0 : min_players;
    m_start_timeout = start_timeout;
}   // initAutoStartTimer

// ----------------------------------------------------------------------------
void NetworkingLobby::setStartingTimerTo(float t)
{
    m_cur_starting_timer =
        (int64_t)StkTime::getMonoTimeMs() + (int64_t)(t * 1000.0f);
}   // setStartingTimerTo

// ----------------------------------------------------------------------------
void NetworkingLobby::setHeader(const core::stringw& header)
{
    if (!m_header || m_header->getText() == header)
        return;
    m_header_text_width =
        GUIEngine::getTitleFont()->getDimension(header.c_str()).Width;
    m_header->getIrrlichtElement()->remove();
    if (m_header_text_width > m_header->m_w)
    {
        m_header->setScrollSpeed(GUIEngine::getTitleFontHeight() / 2);
        m_header->add();
        m_header->setText(header, true);
    }
    else
    {
        m_header->setScrollSpeed(0);
        m_header->add();
        m_header->setText(header, true);
    }
}   // setHeader
