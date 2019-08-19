//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2019 SuperTuxKart-Team
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

#ifdef WIN32
#include "ws2tcpip.h"
#else
#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#endif

// ----------------------------------------------------------------------------
/** Workaround of a bug in iOS 9 where port number is not written. */
extern "C" int getaddrinfo_compat(const char* hostname,
                                  const char* servname,
                                  const struct addrinfo* hints,
                                  struct addrinfo** res)
{
#ifdef IOS_STK
    int err;
    int numericPort;

    // If we're given a service name and it's a numeric string,
    // set `numericPort` to that, otherwise it ends up as 0.
    numericPort = servname != NULL ? atoi(servname) : 0;

    // Call `getaddrinfo` with our input parameters.
    err = getaddrinfo(hostname, servname, hints, res);

    // Post-process the results of `getaddrinfo` to work around
    if ((err == 0) && (numericPort != 0))
    {
        for (const struct addrinfo* addr = *res; addr != NULL;
             addr = addr->ai_next)
        {
            in_port_t* portPtr;
            switch (addr->ai_family)
            {
                case AF_INET:
                {
                    portPtr = &((struct sockaddr_in*)addr->ai_addr)->sin_port;
                }
                break;
                case AF_INET6:
                {
                    portPtr = &((struct sockaddr_in6*)addr->ai_addr)->sin6_port;
                }
                break;
                default:
                {
                    portPtr = NULL;
                }
                break;
            }
            if ((portPtr != NULL) && (*portPtr == 0))
            {
                *portPtr = htons(numericPort);
            }
        }
    }
    return err;
#else
    return getaddrinfo(hostname, servname, hints, res);
#endif
}

#ifndef ENABLE_IPV6
#include "network/unix_ipv6.hpp"
// ----------------------------------------------------------------------------
int isIPV6()
{
    return 0;
}   // isIPV6

// ----------------------------------------------------------------------------
std::string getIPV6ReadableFromMappedAddress(const ENetAddress* ea)
{
    return "";
}   // getIPV6ReadableFromMappedAddress

#else

#include "network/unix_ipv6.hpp"
#include "network/transport_address.hpp"
#include "utils/string_utils.hpp"
#include "utils/log.hpp"
#include "utils/types.hpp"

#include <algorithm>
#include <utility>
#include <vector>

// ============================================================================
uint32_t g_mapped_ipv6_used;
int g_ipv6;
std::vector<std::pair<ENetAddress, struct sockaddr_in6> > g_mapped_ips;
// ============================================================================
int isIPV6()
{
    return g_ipv6;
}   // isIPV6

// ----------------------------------------------------------------------------
void setIPV6(int val)
{
    g_ipv6 = val;
}   // setIPV6

// ----------------------------------------------------------------------------
void unixInitialize()
{
    // Clear previous setting, in case user changed wifi or mobile data
    g_mapped_ipv6_used = 0;
    g_ipv6 = 1;
    g_mapped_ips.clear();
}   // unixInitialize

// ----------------------------------------------------------------------------
std::string getIPV6ReadableFromIn6(const struct sockaddr_in6* in)
{
    std::string result;
    char ipv6[INET6_ADDRSTRLEN] = {};
    inet_ntop(AF_INET6, &(in->sin6_addr), ipv6, INET6_ADDRSTRLEN);
    result = ipv6;
    return result;
}   // getIPV6ReadableFromIn6

// ----------------------------------------------------------------------------
/* Called when a peer is disconnected and we remove its reference to the ipv6.
 */
void removeMappedAddress(const ENetAddress* ea)
{
    auto it = std::find_if(g_mapped_ips.begin(), g_mapped_ips.end(),
        [ea](const std::pair<ENetAddress, struct sockaddr_in6>& addr)
        {
            return ea->host == addr.first.host && ea->port == addr.first.port;
        });
    if (it != g_mapped_ips.end())
    {
        TransportAddress addr(it->first);
        Log::debug("IPV6", "Removing %s, ipv4 address %s.",
            getIPV6ReadableFromIn6(&it->second).c_str(),
            addr.toString().c_str());
        g_mapped_ips.erase(it);
        Log::debug("IPV6", "Mapped address size now: %d.",
            g_mapped_ips.size());
    }
}   // removeMappedAddress

// ----------------------------------------------------------------------------
std::string getIPV6ReadableFromMappedAddress(const ENetAddress* ea)
{
    std::string result;
    auto it = std::find_if(g_mapped_ips.begin(), g_mapped_ips.end(),
        [ea](const std::pair<ENetAddress, struct sockaddr_in6>& addr)
        {
            return ea->host == addr.first.host && ea->port == addr.first.port;
        });
    if (it != g_mapped_ips.end())
        result = getIPV6ReadableFromIn6(&it->second);
    return result;
}   // getIPV6ReadableFromMappedAddress

// ----------------------------------------------------------------------------
/** Add a (fake or synthesized by ios / osx) ipv4 address and map it to an ipv6
 *  one, used in client to set the game server address or server to initialize
 *  host.
 */
void addMappedAddress(const ENetAddress* ea, const struct sockaddr_in6* in6)
{
    g_mapped_ips.emplace_back(*ea, *in6);
}   // addMappedAddress

// ----------------------------------------------------------------------------
/* This is called when enet needs to sent to an mapped ipv4 address, we look up
 * the map here and get the real ipv6 address, so you need to call
 * addMappedAddress above first (for client mostly).
 */
void getIPV6FromMappedAddress(const ENetAddress* ea, struct sockaddr_in6* in6)
{
    auto it = std::find_if(g_mapped_ips.begin(), g_mapped_ips.end(),
        [ea](const std::pair<ENetAddress, struct sockaddr_in6>& addr)
        {
            return ea->host == addr.first.host && ea->port == addr.first.port;
        });
    if (it != g_mapped_ips.end())
        memcpy(in6, &it->second, sizeof(struct sockaddr_in6));
    else
        memset(in6, 0, sizeof(struct sockaddr_in6));
}   // getIPV6FromMappedAddress

// ----------------------------------------------------------------------------
bool sameIPV6(const struct sockaddr_in6* in_1, const struct sockaddr_in6* in_2)
{
    // Check port first, then address
    if (in_1->sin6_port != in_2->sin6_port)
        return false;

    const struct in6_addr* a = &(in_1->sin6_addr);
    const struct in6_addr* b = &(in_2->sin6_addr);
    for (unsigned i = 0; i < sizeof(struct in6_addr); i++)
    {
        if (a->s6_addr[i] != b->s6_addr[i])
            return false;
    }
    return true;
}   // sameIPV6

// ----------------------------------------------------------------------------
/* This is called when enet recieved a packet from its socket, we create an
 * real ipv4 address out of it or a fake one if it's from ipv6 connection.
 */
void getMappedFromIPV6(const struct sockaddr_in6* in6, ENetAddress* ea)
{
    auto it = std::find_if(g_mapped_ips.begin(), g_mapped_ips.end(),
        [in6](const std::pair<ENetAddress, struct sockaddr_in6>& addr)
        {
            return sameIPV6(in6, &addr.second);
        });
    if (it != g_mapped_ips.end())
    {
        *ea = it->first;
        return;
    }

    uint16_t w0 = in6->sin6_addr.s6_addr16[0];
    uint16_t w1 = in6->sin6_addr.s6_addr16[1];
    uint16_t w2 = in6->sin6_addr.s6_addr16[2];
    uint16_t w3 = in6->sin6_addr.s6_addr16[3];
    uint16_t w4 = in6->sin6_addr.s6_addr16[4];
    uint16_t w5 = in6->sin6_addr.s6_addr16[5];
    if (w0 == 0 && w1 == 0 && w2 == 0 && w3 == 0 && w4 == 0 && w5 == 0xFFFF)
    {
        ea->host = ((in_addr*)(in6->sin6_addr.s6_addr + 12))->s_addr;
        ea->port = ntohs(in6->sin6_port);
        TransportAddress addr(*ea);
        addMappedAddress(ea, in6);
    }
    else
    {
        // Create a fake ipv4 address of 0.x.x.x if it's a real ipv6 connection
        if (g_mapped_ipv6_used >= 16777215)
            g_mapped_ipv6_used = 0;
        TransportAddress addr(++g_mapped_ipv6_used, ntohs(in6->sin6_port));
        *ea = addr.toEnetAddress();
        Log::debug("IPV6", "Fake IPV4 address %s mapped to %s",
            addr.toString().c_str(), getIPV6ReadableFromIn6(in6).c_str());
        addMappedAddress(ea, in6);
    }
}   // getMappedFromIPV6

#endif
