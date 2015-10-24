//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 SuperTuxKart-Team
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

#include "network/protocols/connect_to_server.hpp"

#include "network/client_network_manager.hpp"
#include "network/event.hpp"
#include "network/protocols/get_public_address.hpp"
#include "network/protocols/get_peer_address.hpp"
#include "network/protocols/show_public_address.hpp"
#include "network/protocols/hide_public_address.hpp"
#include "network/protocols/request_connection.hpp"
#include "network/protocols/ping_protocol.hpp"
#include "network/protocols/quick_join_protocol.hpp"
#include "network/protocols/client_lobby_room_protocol.hpp"
#include "utils/time.hpp"
#include "utils/log.hpp"

#ifdef WIN32
#  include <iphlpapi.h>
#else
#include <ifaddrs.h>
#endif

// ----------------------------------------------------------------------------
/** Quick join/
 */
ConnectToServer::ConnectToServer() : Protocol(NULL, PROTOCOL_CONNECTION)
{
    m_server_id  = 0;
    m_host_id    = 0;
    m_quick_join = true;
    m_state      = NONE;
}   // ConnectToServer()

// ----------------------------------------------------------------------------
/** Specify server to connect to.
 *  \param server_id Id of server to connect to.
 *  \param host_id Id of host.
 */
ConnectToServer::ConnectToServer(uint32_t server_id, uint32_t host_id)
               : Protocol(NULL, PROTOCOL_CONNECTION)
{
    m_server_id  = server_id;
    m_host_id    = host_id;
    m_quick_join = false;
    m_state      = NONE;
}   // ConnectToServer(server, host)

// ----------------------------------------------------------------------------
/** Destructor. 
 */
ConnectToServer::~ConnectToServer()
{
}   // ~ConnectToServer

// ----------------------------------------------------------------------------

bool ConnectToServer::notifyEventAsynchronous(Event* event)
{
    if (event->getType() == EVENT_TYPE_CONNECTED)
    {
        Log::info("ConnectToServer", "The Connect To Server protocol has "
                "received an event notifying that he's connected to the peer.");
        m_state = CONNECTED; // we received a message, we are connected
    }
    return true;
}   // notifyEventAsynchronous

// ----------------------------------------------------------------------------
void ConnectToServer::setup()
{
    Log::info("ConnectToServer", "SETUP");
    m_server_address.clear();
    m_state            = NONE;
    m_current_protocol = NULL;
}   // setup

// ----------------------------------------------------------------------------
void ConnectToServer::asynchronousUpdate()
{
    switch(m_state)
    {
        case NONE:
        {
            Log::info("ConnectToServer", "Protocol starting");
            m_current_protocol = new GetPublicAddress();
            m_current_protocol->requestStart();
            m_state = GETTING_SELF_ADDRESS;
            break;
        }
        case GETTING_SELF_ADDRESS:
            // Wait till we know the public address
            if (m_current_protocol->getState() == PROTOCOL_STATE_TERMINATED)
            {
                m_state = SHOWING_SELF_ADDRESS;
                m_current_protocol = new ShowPublicAddress();
                m_current_protocol->requestStart();
                Log::info("ConnectToServer", "Public address known");
            }
            break;
        case SHOWING_SELF_ADDRESS:
            if (m_current_protocol->getState() == PROTOCOL_STATE_TERMINATED)
            {
                // now our public address is in the database
                Log::info("ConnectToServer", "Public address shown");
                if (m_quick_join)
                {
                    m_current_protocol = new QuickJoinProtocol(&m_server_address,
                                                               &m_server_id);
                    m_current_protocol->requestStart();
                    m_state = REQUESTING_CONNECTION;
                }
                else
                {
                    m_current_protocol = new GetPeerAddress(m_host_id,
                                                            &m_server_address);
                    m_current_protocol->requestStart();
                    m_state = GETTING_SERVER_ADDRESS;
                }
            }
            break;
        case GETTING_SERVER_ADDRESS:
            // Wait till we know the server address
            if (m_current_protocol->getState() == PROTOCOL_STATE_TERMINATED)
            {
                Log::info("ConnectToServer", "Server's address known");
                // we're in the same lan (same public ip address) !!
                if (m_server_address.getIP() ==
                    STKHost::get()->getPublicAddress().getIP())
                {
                    Log::info("ConnectToServer",
                              "Server appears to be in the same LAN.");
                }
                m_state = REQUESTING_CONNECTION;
                m_current_protocol = new RequestConnection(m_server_id);
                m_current_protocol->requestStart();
            }
            break;
        case REQUESTING_CONNECTION:
            if (m_current_protocol->getState() == PROTOCOL_STATE_TERMINATED)
            {
                // Server knows we want to connect
                Log::info("ConnectToServer", "Connection request made");
                if (m_server_address.getIP() == 0 ||
                    m_server_address.getPort() == 0  )
                { 
                    // server data not correct, hide address and stop
                    m_state = HIDING_ADDRESS;
                    Log::error("ConnectToServer", "Server address is %s",
                               m_server_address.toString().c_str());
                    m_current_protocol = new HidePublicAddress();
                    m_current_protocol->requestStart();
                    return;
                }
                if (m_server_address.getIP() 
                      == STKHost::get()->getPublicAddress().getIP())
                {
                    // we're in the same lan (same public ip address) !!
                    handleSameLAN();
                }
                else
                {
                    m_state = CONNECTING;
                    m_current_protocol = new PingProtocol(m_server_address, 2.0);
                    m_current_protocol->requestStart();
                }
            }
            break;
        case CONNECTING: // waiting the server to answer our connection
            {
                static double timer = 0;
                if (StkTime::getRealTime() > timer+5.0) // every 5 seconds
                {
                    timer = StkTime::getRealTime();
                    NetworkManager::getInstance()->connect(m_server_address);
                    Log::info("ConnectToServer", "Trying to connect to %s",
                              m_server_address.toString().c_str());
                }
                break;
            }
        case CONNECTED:
        {
            Log::info("ConnectToServer", "Connected");
            // Kill the ping protocol because we're connected
            m_current_protocol->requestTerminate();

            m_current_protocol = new HidePublicAddress();
            m_current_protocol->requestStart();
            ClientNetworkManager::getInstance()->setConnected(true);
            m_state = HIDING_ADDRESS;
            break;
        }
        case HIDING_ADDRESS:
            // Wait till we have hidden our address
            if (m_current_protocol->getState() == PROTOCOL_STATE_TERMINATED)
            {
                Log::info("ConnectToServer", "Address hidden");
                m_state = DONE;
                // lobby room protocol if we're connected only
                if (ClientNetworkManager::getInstance()->isConnected())
                {
                    Protocol *p = new ClientLobbyRoomProtocol(m_server_address);
                    p->requestStart();
                }
            }
            break;
        case DONE:
            requestTerminate();
            m_state = EXITING;
            break;
        case EXITING:
            break;
    }
}

// ----------------------------------------------------------------------------
/** Called when the server is on the same LAN. It uses broadcast to
 *  find and conntect to the server.
 */
void ConnectToServer::handleSameLAN()
{
    // just send a broadcast packet, the client will know our 
    // ip address and will connect
    STKHost* host = STKHost::get();
    host->stopListening(); // stop the listening

    TransportAddress broadcast_address;
    broadcast_address.setIP(-1); // 255.255.255.255
    broadcast_address.setPort(7321);
    char data2[] = "aloha_stk\0";
    host->sendRawPacket((uint8_t*)(data2), 10, broadcast_address);

    Log::info("ConnectToServer", "Waiting broadcast message.");

    TransportAddress sender;
    // get the sender
    const uint8_t* received_data = host->receiveRawPacket(&sender);

    host->startListening(); // start listening again
    const char data[] = "aloha_stk\0";
    if (strcmp(data, (char*)(received_data)) == 0)
    {
        Log::info("ConnectToServer", "LAN Server found : %s",
                   sender.toString().c_str());
#ifndef WIN32
        // just check if the ip is ours : if so, 
        // then just use localhost (127.0.0.1)
        struct ifaddrs *ifap, *ifa;
        struct sockaddr_in *sa;
        getifaddrs(&ifap); // get the info
        for (ifa = ifap; ifa; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                sa = (struct sockaddr_in *) ifa->ifa_addr;

                // This interface is ours
                if (ntohl(sa->sin_addr.s_addr) == sender.getIP())
                    sender.setIP(0x7f000001); // 127.0.0.1
            }
        }
        freeifaddrs(ifap);
#else
        // Query the list of all IP addresses on the local host
        // First call to GetIpAddrTable with 0 bytes buffer
        // will return insufficient buffer error, and size
        // will contain the number of bytes needed for all
        // data. Repeat the process of querying the size
        // using GetIpAddrTable in a while loop since it
        // can happen that an interface comes online between
        // the previous call to GetIpAddrTable and the next
        // call.
        MIB_IPADDRTABLE *table = NULL;
        unsigned long size = 0;
        int error = GetIpAddrTable(table, &size, 0);
        // Also add a count to limit the while loop - in
        // case that something strange is going on.
        int count = 0;
        while (error == ERROR_INSUFFICIENT_BUFFER && count < 10)
        {
            delete[] table;   // deleting NULL is legal
            table = (MIB_IPADDRTABLE*)new char[size];
            error = GetIpAddrTable(table, &size, 0);
            count++;
        }   // while insufficient buffer
        for (unsigned int i = 0; i < table->dwNumEntries; i++)
        {
            unsigned int ip = ntohl(table->table[i].dwAddr);
            if (sender.getIP() == ip) // this interface is ours
            {
                sender.setIP(0x7f000001); // 127.0.0.1
                break;
            }
        }
        delete[] table;

#endif
        m_server_address.copy(sender);
        m_state = CONNECTING;
    }
}  // handleSameLAN
