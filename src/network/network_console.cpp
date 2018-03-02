//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2015 Joerg Henrichs
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

#include "network/network_config.hpp"
#include "network/network_player_profile.hpp"
#include "network/stk_host.hpp"
#include "network/protocols/client_lobby.hpp"
#include "network/protocols/server_lobby.hpp"
#include "network/stk_peer.hpp"
#include "utils/log.hpp"
#include "utils/time.hpp"
#include "utils/vs.hpp"
#include "main_loop.hpp"

#include <iostream>

namespace NetworkConsole
{
// ----------------------------------------------------------------------------
void kickAllPlayers(STKHost* host)
{
    const std::vector<STKPeer*> &peers = host->getPeers();
    for (unsigned int i = 0; i < peers.size(); i++)
    {
        peers[i]->disconnect();
    }
}   // kickAllPlayers

// ----------------------------------------------------------------------------
void mainLoop(STKHost* host)
{
    VS::setThreadName("NetworkConsole");
    std::string str = "";
    while (!host->requestedShutdown())
    {
        getline(std::cin, str);
        if (str == "quit")
        {
            host->requestShutdown();
        }
        else if (str == "kickall" && NetworkConfig::get()->isServer())
        {
            kickAllPlayers(host);
        }
        else if (str == "start" && NetworkConfig::get()->isServer())
        {
            auto sl = LobbyProtocol::get<ServerLobby>();
            sl->signalRaceStartToClients();
        }
        else if (str == "selection" && NetworkConfig::get()->isServer())
        {
            auto sl = LobbyProtocol::get<ServerLobby>();
            sl->startSelection();
        }
        else if (str == "select" && NetworkConfig::get()->isClient())
        {
            std::string str2;
            getline(std::cin, str2);
            auto clrp = LobbyProtocol::get<ClientLobby>();
            std::vector<NetworkPlayerProfile*> players =
                host->getMyPlayerProfiles();
            // For now send a vote for each local player
            for(unsigned int i=0; i<players.size(); i++)
            {
                clrp->requestKartSelection(players[i]->getGlobalPlayerId(),
                                           str2);
            }   // for i in players
        }
        else if (str == "vote" && NetworkConfig::get()->isClient())
        {
            std::cout << "Vote for ? (track/laps/reversed/major/minor/race#) :";
            std::string str2;
            getline(std::cin, str2);
            auto clrp = LobbyProtocol::get<ClientLobby>();
            std::vector<NetworkPlayerProfile*> players =
                host->getMyPlayerProfiles();
            if (str2 == "track")
            {
                std::cin >> str2;
                // For now send a vote for each local player
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteTrack(i, str2);
            }
            else if (str2 == "laps")
            {
                int cnt;
                std::cin >> cnt;
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteLaps(i, cnt);
            }
            else if (str2 == "reversed")
            {
                bool cnt;
                std::cin >> cnt;
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteReversed(i, cnt);
            }
            else if (str2 == "major")
            {
                int cnt;
                std::cin >> cnt;
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteMajor(i, cnt);
            }
            else if (str2 == "minor")
            {
                int cnt;
                std::cin >> cnt;
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteMinor(i, cnt);
            }
            else if (str2 == "race#")
            {
                int cnt;
                std::cin >> cnt;
                for(unsigned int i=0; i<players.size(); i++)
                    clrp->voteRaceCount(i, cnt);
            }
            std::cout << "\n";
        }
        else
        {
            Log::info("Console", "Unknown command '%s'.", str.c_str());
        }
    }   // while !stop
    main_loop->abort();
}   // mainLoop

}
