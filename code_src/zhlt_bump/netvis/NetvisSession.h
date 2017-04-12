#ifndef NETVISSESSION_H__
#define	NETVISSESSION_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cc++/socket.h"
#include "cc++/thread.h"
#include "ZHLTThread.h"

#include <string>
#include <deque>
#include <list>
#include <map>
#include <set>

#include "ReferenceArray.h"
#include "ReferencePtr.h"

#include "basictypes.h"

class basePacket;
class VIS_PACKET;
class NetvisSession;
class NetvisSocketServer;

class Packet
{
public:
    Packet(const basePacket* packet);
    Packet(const Packet& other)
    { copy(other); }
    Packet& operator=(const Packet& other)
    { copy(other); return *this; }
    const VIS_PACKET& data() const;

protected:
    void copy(const Packet& other)
    {
        m_Payload = other.m_Payload;
    }

protected:
    ReferenceArray<CHAR>    m_Payload;
};


class NetvisSocket : public TCPSocket
{
protected:
    virtual bool OnAccept(const InetHostAddress &ia, tpport_t port);
public:
    NetvisSocket(const InetAddress &ia, tpport_t port);
};


class NetvisSession : public ZHLTThread
{
private:
    virtual void Run();
    virtual void Final() {};
public:
    NetvisSession(NetvisSocketServer* parent, TCPSocket& server);           // Netvis Server Session
    NetvisSession(const InetHostAddress &host, tpport_t port);              // Netvis Client Session
    virtual ~NetvisSession()
    {
        Terminate();
    }

    void SendPacket(const basePacket* packet);
    void DisplayClientStats();
    void DisplayServerStats();



protected:
    bool WritePacket(const VIS_PACKET& packet);       // returns true on success (false == assume socket died on us)
    bool ReadPacket(VIS_PACKET& packet);              // returns true on success (false == assume socket died on us)

protected:
    TCPStream                           m_TCPStream;
    NetvisSocketServer*                 m_Parent;           // server variable for handling socket disconnects

public:
    long                                m_ClientId;         // server var
    bool                                m_Active;           // server var (both technically)
    std::string                         m_RemoteHostname;   // both

    ReferenceCounter                    m_SyncIndex;        // server var
    ReferenceCounter                    m_PortalCount;      // server var
    ReferenceCounter                    m_PacketsCount;     // number of pending packets (prevents unnecessary calls to Mutex locking)
    std::deque< Packet >                m_Packets;          // both (outbound packets only)
    Mutex                               m_PacketsLock;      // both (outbound packets only)
    std::set< long >                    m_WorkDone;         // server var
    Mutex                               m_WorkDoneLock;     // server var
    std::deque< long >                  m_WorkQueue;        // server var
    Mutex                               m_WorkQueueLock;    // server var

    typedef std::deque< long >::iterator      WorkQueue_i;
    typedef std::set< long >::iterator        WorkDone_i;
    typedef std::deque< Packet >::iterator    Packet_i;
};



class NetvisSocketServer : public ZHLTThread
{
private:
    virtual void Run();

public:
    NetvisSocketServer(tpport_t port);
    virtual ~NetvisSocketServer()
    {
        Terminate();
    }
    static void DisplayClientStats(long clientid, unsigned long portalcount, const char* hostname, bool inactive);
    void DisplayServerStats();

protected:
    tpport_t                        m_Port;
    std::deque< NetvisSession* >    m_Clients;
    Mutex                           m_ClientsLock;

    typedef std::deque< NetvisSession* >::iterator Clients_i;
};

extern NetvisSession*  g_ClientSession;

#endif//NETVISSESSION_H__
