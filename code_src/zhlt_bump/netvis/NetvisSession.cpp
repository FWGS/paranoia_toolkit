#include "netvis.h"

extern long            g_PortalArrayIndex;

extern double   I_FloatTime();

//
// Packet
//

Packet::Packet(const basePacket* packet)
{
    int size = packet->getSize();
    hlassert(size);
    CHAR* payload = new CHAR[size];
    memcpy(payload, packet, size);
    m_Payload = payload;
}

const VIS_PACKET& Packet::data() const
{
    const VIS_PACKET* pReturn = (reinterpret_cast<const VIS_PACKET*>((const CHAR*)m_Payload));
    return *pReturn;
}


//
// NetvisSocket
//

bool NetvisSocket::OnAccept(const InetHostAddress &ia, tpport_t port)
{
    Log("Connection accepted from %s\n", ia.getHostname());
    return true;
}

NetvisSocket::NetvisSocket(const InetAddress &ia, tpport_t port)
    : TCPSocket(ia, port)
{
}



//
// NetvisSession
//

void NetvisSession::Run()
{
    m_Active = true;

    VIS_PACKET inpacket;

    while (_zhlt_isCancelled() == false)
    {
        if (m_TCPStream.isPending(SOCKET_PENDING_ERROR, 0))
        {
            goto FatalHandler;
        }
        else if (m_TCPStream.isPending(SOCKET_PENDING_INPUT, 0))
        {
            IfDebug(memset(&inpacket, 0, sizeof(inpacket)));
            if (ReadPacket(inpacket))
            {
                inpacket.HandleIncomingPacket(&inpacket, this);
            }
            else
            {
                goto FatalHandler;
            }
        }
        else if (m_PacketsCount)
        {
            m_PacketsLock.EnterMutex();
            for (Packet_i it = m_Packets.begin(); it != m_Packets.end(); it++)
            {
                const VIS_PACKET& outpacket = it->data();
                if (!WritePacket(outpacket))
                {
                    m_Packets.clear();
                    m_PacketsCount = 0;
                    m_PacketsLock.LeaveMutex();
                    goto FatalHandler;
                }
            }
            m_TCPStream.sync();
            m_Packets.clear();
            m_PacketsCount = 0;
            m_PacketsLock.LeaveMutex();
        }
        else
        {
            Sleep(50);
        }
    }

    if (m_PacketsCount)
    {
        // Send any remaining packets first (typically a few IS_DONE_PORTAL's are here the main thread hits the terminate code so fast . . . )
        if (m_PacketsCount)
        {
            m_PacketsLock.EnterMutex();
            for (Packet_i it = m_Packets.begin(); it != m_Packets.end(); it++)
            {
                const VIS_PACKET& outpacket = it->data();
                WritePacket(outpacket);
            }
            m_TCPStream.sync();
            m_Packets.clear();
            m_PacketsCount = 0;
            m_PacketsLock.LeaveMutex();
        }
    }

FatalHandler:
    m_Active = false;
    GOING_DOWN packet;      // if socket dies, pretend it got the GOING_DOWN message so it cleans up properly
    packet.HandleIncomingPacket(&packet, this);
}

NetvisSession::NetvisSession(NetvisSocketServer* parent, TCPSocket &server)
    : m_TCPStream(server, 128*1024)
{
    m_Active = false;
    m_Parent = parent;
    m_SyncIndex = 0;
    m_ClientId = 0;
    m_PortalCount = 0;

    m_TCPStream.setCompletion(SOCKET_COMPLETION_IMMEDIATE);
}

NetvisSession::NetvisSession(const InetHostAddress &host, tpport_t port)
    : m_TCPStream(host, port, 128*1024)
{
    m_Active = false;
    m_Parent = NULL;
    m_SyncIndex = 0;
    m_ClientId = 0;
    m_PortalCount = 0;

    m_TCPStream.setCompletion(SOCKET_COMPLETION_IMMEDIATE);
}

void NetvisSession::SendPacket(const basePacket* pPacket) 
{
    if ((g_vismode == VIS_MODE_CLIENT) && (g_visstate == VIS_NO_SERVER))
    {
        while (1)
        {
            Sleep(1000);
        }
    }

    Packet packet(pPacket);

    m_PacketsLock.EnterMutex();
    m_Packets.push_back(packet);
    m_PacketsCount++;
    m_PacketsLock.LeaveMutex();
}

bool NetvisSession::WritePacket(const VIS_PACKET& packet)
{
    const CHAR* payload = reinterpret_cast<const CHAR*>(&packet);
    m_TCPStream.clear();
    m_TCPStream.write(payload, packet.getSize());
    m_TCPStream.flush();
    return m_TCPStream.good();
}

bool NetvisSession::ReadPacket(VIS_PACKET& packet)
{
    CHAR* payload = reinterpret_cast<CHAR*>(&packet);
    m_TCPStream.clear();
    m_TCPStream.read(payload, sizeof(basePacket));

    if (!m_TCPStream.good())
        return false;

    int expectedsize = packet.getPacketSizeByType(packet.getType());
    if ((expectedsize == packet.getSize()) || (expectedsize == VARIABLE_LENGTH_PACKET))
    {
        int remaining = packet.getSize() - myoffsetof(VIS_PACKET, data);
        hlassert(remaining >= 0);
        hlassert(remaining + sizeof(basePacket) <= sizeof(VIS_PACKET));
        if (remaining > 0)
        {
            m_TCPStream.read(packet.data, remaining);
        }
    }

    return m_TCPStream.good();
}


void NetvisSession::DisplayClientStats()
{
    float clientpercent = (float)(g_vislocalportal * 50) / (float)g_visportals;
    float serverpercent = (float)(g_serverindex * 50) / (float)g_visportals;

    double currenttime = I_FloatTime();
    double elapsed = currenttime - g_starttime;

    int idle_seconds = g_idletime / 1000;
    int work_seconds = (int) elapsed - idle_seconds;
    int total_seconds = (int) elapsed;

    Log(
        "==================================================\n"
        "Netvis Client #%d statistics [%-5d portals total]\n"
        "System Progress : [%-5d portals] (%3.2f percent)\n"
        "Client Progress : [%-5d portals] (%3.2f percent)\n"
        "Workload        : [%5d seconds] [%d work] [%5d idle]\n"
        "==================================================\n"
        ,g_clientid, g_visportals * 2
        ,g_serverindex, serverpercent
        ,(int)m_PortalCount, clientpercent
        ,total_seconds, work_seconds, idle_seconds);
}

//
// NetvisSocketServer
//

void NetvisSocketServer::Run()
{
    while (_zhlt_isCancelled() == false)
    {   
        try
        {
            InetAddress addr;
            NetvisSocket server(addr, m_Port);
        
            while (_zhlt_isCancelled() == false)
            {
                while (server.isPendingConnection(1000))
                {
                    NetvisSession* newSession = new NetvisSession(this, server);
                    m_ClientsLock.EnterMutex();
                    m_Clients.push_back(newSession);
                    m_ClientsLock.LeaveMutex();
                    newSession->Start();
                }
            }
        }
        catch (Socket* socket)
        {
// Workaround for a CommonC++ socket bug on Linux which leaves the socket bound after program termination
#ifdef SYSTEM_POSIX
            if (socket->getErrorNumber() == SOCKET_BINDING_FAILED)
            {
                Sleep(2000);
                Log("Socket error %d\n retrying . . .", socket->getErrorNumber());
                Sleep(8000);
            }
            else
            {
                Sleep(2000); // Wait just long enough for log() functions to be initialized properly in common/log.c
                Log("Socket error %d\n", socket->getErrorNumber());
            }
#else
            Sleep(2000); // Wait just long enough for log() functions to be initialized properly in common/log.c
            Error("Socket error %d\n", socket->getErrorNumber());
#endif
        }
    }

    DisplayServerStats();       // Final stats

    IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "NetvisSocketServer: waiting on client threads to finish\n"));
    m_ClientsLock.EnterMutex();
    for (Clients_i it = m_Clients.begin(); it != m_Clients.end(); it++)
    {
        delete (NetvisSession*)(*it);
    }
    m_Clients.clear();
    m_ClientsLock.LeaveMutex();
    IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "NetvisSocketServer: done waiting on client threads to finish\n"));
}

NetvisSocketServer::NetvisSocketServer(tpport_t port)
{
    m_Port = port;
}

void NetvisSocketServer::DisplayClientStats(long clientid, unsigned long portalcount, const char* hostname, bool active)
{
    float percent = (float)(portalcount * 50) / (float)g_visportals;

    Log("Client #%-2d Status : [%5u portals] (%3.2f percent) [%s%s]\n", clientid, portalcount, percent, hostname, active ? "" : ":INACTIVE");
}

void NetvisSocketServer::DisplayServerStats()
{
    float globalpercent = (float)(g_PortalArrayIndex * 50) / (float)g_visportals;
    float localpercent = (float)(g_vislocalportal * 50) / (float)g_visportals;

    Log("\n"
         "==================================================\n"
         "Netvis Server Statistics [%-5d portals total]\n"
         "System Progress   : [%-5d portals] (%3.2f percent)\n"
         "Server Progress   : [%-5d portals] (%3.2f percent)\n"
        ,g_visportals * 2
        ,g_PortalArrayIndex, globalpercent
        ,g_vislocalportal, localpercent);

    m_ClientsLock.EnterMutex();
    {    
        for (Clients_i it=m_Clients.begin(); it!=m_Clients.end(); it++)
        {
            DisplayClientStats((*it)->m_ClientId, (*it)->m_PortalCount, (*it)->m_RemoteHostname.c_str(), (*it)->m_Active);
        }
    }
    m_ClientsLock.LeaveMutex();
    Log( "==================================================\n");
}
