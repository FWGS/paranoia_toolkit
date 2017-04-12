#include "netvis.h"

#include "zlib.h"

portal_t*       GetPortalPtr(long index);
int             GetNextPortalIndex(void);

static ReferenceCounter s_OutstandingSyncRequests;
static ReferenceCounter s_IdleCounter;
static long     s_PortalArray[PORTAL_ARRAY_MAX_SIZE];
long            g_PortalArrayIndex = 0;                    // Current offset
static Mutex    s_PortalArrayLock;                         // protects g_PortalArrayIndex, s_PortalArray
bool            g_NetvisAbort = false;

static void     Handle_VIS_LOGIN(VIS_LOGIN* packet, NetvisSession* socket);
static void     Handle_VIS_LOGIN_NAK(VIS_LOGIN_NAK* packet, NetvisSession* socket);
static void     Handle_VIS_LOGIN_ACK(VIS_LOGIN_ACK* packet, NetvisSession* socket);

static void     Handle_VIS_WANT_BSP_DATA(VIS_WANT_BSP_DATA* packet, NetvisSession* socket);
static void     Handle_VIS_BSP_DATA_NAK(VIS_BSP_DATA_NAK* packet, NetvisSession* socket);
static void     Handle_VIS_BSP_DATA(VIS_BSP_DATA* packet, NetvisSession* socket);
static void     Handle_VIS_BSP_NAME(VIS_BSP_NAME* packet, NetvisSession* socket);

static void     Handle_VIS_WANT_PRT_DATA(VIS_WANT_PRT_DATA* packet, NetvisSession* socket);
static void     Handle_VIS_PRT_DATA_NAK(VIS_PRT_DATA_NAK* packet, NetvisSession* socket);
static void     Handle_VIS_PRT_DATA(VIS_PRT_DATA* packet, NetvisSession* socket);

static void     Handle_VIS_WANT_MIGHTSEE_DATA(VIS_WANT_MIGHTSEE_DATA* packet, NetvisSession* socket);
static void     Handle_VIS_MIGHTSEE_DATA_NAK(VIS_MIGHTSEE_DATA_NAK* packet, NetvisSession* socket);
static void     Handle_VIS_MIGHTSEE_DATA(VIS_MIGHTSEE_DATA* packet, NetvisSession* socket);

static void     Handle_VIS_LEAFTHREAD(VIS_LEAFTHREAD* packet, NetvisSession* socket);
static void     Handle_VIS_LEAFTHREAD_ACK(VIS_LEAFTHREAD_ACK* packet, NetvisSession* socket);
static void     Handle_VIS_LEAFTHREAD_NAK(VIS_LEAFTHREAD_NAK* packet, NetvisSession* socket);

static void     Handle_VIS_WANT_FULL_SYNC(WANT_FULL_SYNC* packet, NetvisSession* socket);
static void     Handle_VIS_DONE_PORTAL(VIS_DONE_PORTAL* packet, NetvisSession* socket);
static void     Handle_VIS_SYNC_PORTAL_CLUSTER(VIS_SYNC_PORTAL_CLUSTER* packet, NetvisSession* socket);

static void     Handle_VIS_GOING_DOWN(GOING_DOWN* packet, NetvisSession* socket);

void            basePacket::DroppedClientForPortalIndex(long clientid, long index)
{
    if (index < 0)
        return;

    portal_t*       p = GetPortalPtr(index);

    hlassert(p);

    if (p->status == stat_working)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_WARNING, "[#%d] Connection closed: portal %d reverted\n", clientid, index));
        p->status = stat_none;
    }
}

INT             basePacket::getPacketSizeByType(INT type)
{
    switch ((PACKETtypes) type)
    {
    case VIS_PACKET_LOGIN:
        return sizeof(VIS_LOGIN);
    case VIS_PACKET_LOGIN_NAK:
        return sizeof(VIS_LOGIN_NAK);
    case VIS_PACKET_LOGIN_ACK:
        return sizeof(VIS_LOGIN_ACK);

    case VIS_PACKET_WANT_BSP_DATA:
        return sizeof(VIS_WANT_BSP_DATA);
    case VIS_PACKET_BSP_DATA_NAK:
        return sizeof(VIS_BSP_DATA_NAK);
    case VIS_PACKET_BSP_DATA:
        return VARIABLE_LENGTH_PACKET;
    case VIS_PACKET_BSP_NAME:
        return sizeof(VIS_BSP_NAME);

    case VIS_PACKET_WANT_PRT_DATA:
        return sizeof(VIS_WANT_PRT_DATA);
    case VIS_PACKET_PRT_DATA_NAK:
        return sizeof(VIS_PRT_DATA_NAK);
    case VIS_PACKET_PRT_DATA:
        return VARIABLE_LENGTH_PACKET;

    case VIS_PACKET_WANT_MIGHTSEE_DATA:
        return sizeof(VIS_WANT_MIGHTSEE_DATA);
    case VIS_PACKET_MIGHTSEE_DATA_NAK:
        return sizeof(VIS_MIGHTSEE_DATA_NAK);
    case VIS_PACKET_MIGHTSEE_DATA:
        return VARIABLE_LENGTH_PACKET;

    case VIS_PACKET_LEAFTHREAD:
        return sizeof(VIS_LEAFTHREAD);
    case VIS_PACKET_LEAFTHREAD_ACK:
        return sizeof(VIS_LEAFTHREAD_ACK);
    case VIS_PACKET_LEAFTHREAD_NAK:
        return sizeof(VIS_LEAFTHREAD_NAK);

    case VIS_PACKET_WANT_FULL_SYNC:
        return sizeof(WANT_FULL_SYNC);
    case VIS_PACKET_DONE_PORTAL:                          // variable size in code, fixed size at runtime (depending on map data)
        return myoffsetof(VIS_DONE_PORTAL, data) + g_bitbytes;
    case VIS_PACKET_SYNC_PORTAL:
        return myoffsetof(VIS_SYNC_PORTAL, data) + g_bitbytes;
    case VIS_PACKET_SYNC_PORTAL_CLUSTER:
        return VARIABLE_LENGTH_PACKET;

    case VIS_PACKET_GOING_DOWN:
        return sizeof(GOING_DOWN);

    default:
        return 0;
    }
}

void            basePacket::HandleIncomingPacket(basePacket* packet, NetvisSession* socket)
{
    switch (packet->getType())
    {
    case VIS_PACKET_LOGIN:
        Handle_VIS_LOGIN((VIS_LOGIN*) packet, socket);
        break;
    case VIS_PACKET_LOGIN_NAK:
        Handle_VIS_LOGIN_NAK((VIS_LOGIN_NAK*) packet, socket);
        break;
    case VIS_PACKET_LOGIN_ACK:
        Handle_VIS_LOGIN_ACK((VIS_LOGIN_ACK*) packet, socket);
        break;

    case VIS_PACKET_WANT_BSP_DATA:
        Handle_VIS_WANT_BSP_DATA((VIS_WANT_BSP_DATA*) packet, socket);
        break;
    case VIS_PACKET_BSP_DATA_NAK:
        Handle_VIS_BSP_DATA_NAK((VIS_BSP_DATA_NAK*) packet, socket);
        break;
    case VIS_PACKET_BSP_DATA:
        Handle_VIS_BSP_DATA((VIS_BSP_DATA*) packet, socket);
        break;
    case VIS_PACKET_BSP_NAME:
        Handle_VIS_BSP_NAME((VIS_BSP_NAME*) packet, socket);
        break;

    case VIS_PACKET_WANT_PRT_DATA:
        Handle_VIS_WANT_PRT_DATA((VIS_WANT_PRT_DATA*) packet, socket);
        break;
    case VIS_PACKET_PRT_DATA_NAK:
        Handle_VIS_PRT_DATA_NAK((VIS_PRT_DATA_NAK*) packet, socket);
        break;
    case VIS_PACKET_PRT_DATA:
        Handle_VIS_PRT_DATA((VIS_PRT_DATA*) packet, socket);
        break;

    case VIS_PACKET_WANT_MIGHTSEE_DATA:
        Handle_VIS_WANT_MIGHTSEE_DATA((VIS_WANT_MIGHTSEE_DATA*) packet, socket);
        break;
    case VIS_PACKET_MIGHTSEE_DATA_NAK:
        Handle_VIS_MIGHTSEE_DATA_NAK((VIS_MIGHTSEE_DATA_NAK*) packet, socket);
        break;
    case VIS_PACKET_MIGHTSEE_DATA:
        Handle_VIS_MIGHTSEE_DATA((VIS_MIGHTSEE_DATA*) packet, socket);
        break;

    case VIS_PACKET_LEAFTHREAD:
        Handle_VIS_LEAFTHREAD((VIS_LEAFTHREAD*) packet, socket);
        break;
    case VIS_PACKET_LEAFTHREAD_ACK:
        Handle_VIS_LEAFTHREAD_ACK((VIS_LEAFTHREAD_ACK*) packet, socket);
        break;
    case VIS_PACKET_LEAFTHREAD_NAK:
        Handle_VIS_LEAFTHREAD_NAK((VIS_LEAFTHREAD_NAK*) packet, socket);
        break;

    case VIS_PACKET_WANT_FULL_SYNC:
        Handle_VIS_WANT_FULL_SYNC((WANT_FULL_SYNC*) packet, socket);
        break;
    case VIS_PACKET_DONE_PORTAL:
        Handle_VIS_DONE_PORTAL((VIS_DONE_PORTAL*) packet, socket);
        break;
    case VIS_PACKET_SYNC_PORTAL_CLUSTER:
        Handle_VIS_SYNC_PORTAL_CLUSTER((VIS_SYNC_PORTAL_CLUSTER*) packet, socket);
        break;

    case VIS_PACKET_GOING_DOWN:
        Handle_VIS_GOING_DOWN((GOING_DOWN*) packet, socket);
        break;
    }
}

void            Handle_VIS_GOING_DOWN(GOING_DOWN* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive GOING_DOWN\n", socket->m_ClientId));
    Warning("[#%d] disconnected\n", socket->m_ClientId);

    if (g_vismode == VIS_MODE_SERVER)
    {
        socket->m_WorkQueueLock.EnterMutex();
        NetvisSession::WorkQueue_i it = socket->m_WorkQueue.begin();
        while (it != socket->m_WorkQueue.end())
        {
            packet->DroppedClientForPortalIndex(socket->m_ClientId, *it);
            it++;
        }
        socket->m_WorkQueue.clear();
        socket->m_WorkQueueLock.LeaveMutex();
    }
    else
    {
        socket->DisplayClientStats();
        g_NetvisAbort = true;

        NetvisSleep(2000);
        Error("Server dropped connection, exiting\n");
    }
}

void            Send_VIS_GOING_DOWN(NetvisSession* socket)
{
    GOING_DOWN      outpacket;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send GOING_DOWN\n"));
    socket->SendPacket(&outpacket);
    while (socket->m_PacketsCount)
    {
        NetvisSleep(2000);                                 // Wait long enough for server to receive our last few packets (if any)
    }
}

// Subroutine for Handle_VIS_WANT_FULL_SYNC
static bool     Create_SYNC_PORTAL_OUTPACKET(VIS_SYNC_PORTAL* outpacket, int socketIndex, int globalIndex, int clientId)
{
    int             num = s_PortalArray[socketIndex];
    portal_t*       tp = GetPortalPtr(num);

    if (tp)
    {
        if (tp->status == stat_done)
        {
            outpacket->setPortalIndex(num);
            outpacket->setNumCanSee(tp->numcansee);
            outpacket->setFromClient(tp->fromclient);
            memcpy(outpacket->data, tp->visbits, g_bitbytes);

            IfDebug(Developer(DEVELOPER_LEVEL_MEGASPAM, "[#%d] send VIS_SYNC_PORTAL, portal = %d, socketIndex = %d, globalIndex = %d, size = %d\n", clientId, num, socketIndex, globalIndex, outpacket->getSize()));
            return true;
        }
        else
        {
            IfDebug(Developer(DEVELOPER_LEVEL_ERROR, "[#%d] Handle_VIS_WANT_FULL_SYNC, portal = %d, socketIndex = %d, globalIndex = %d, s_PortalArray[socketIndex] = %d, portal not stat_done (stat was %d)\n", clientId, num, socketIndex, globalIndex, num, tp->status));
        }
    }
    else
    {
        IfDebug(Developer(DEVELOPER_LEVEL_ERROR, "[#%d] GetPortalPtr() failed, returned NULL, portal = %d, socketIndex = %d, globalIndex = %d, s_PortalArray[socketIndex] = %d\n", clientId, num, socketIndex, globalIndex, num));
    }
    return false;
}

void            Send_VIS_SYNC_PORTALs(NetvisSession* socket)
{
    s_PortalArrayLock.EnterMutex();
    int             socketIndex = socket->m_SyncIndex;
    int             globalIndex = g_PortalArrayIndex;
    int             range = globalIndex - socketIndex;

    s_PortalArrayLock.LeaveMutex();
    VIS_SYNC_PORTAL_CLUSTER outpacket;

    outpacket.setServerIndex(globalIndex);
    if (range)
    {
        bool            done = false;

        socket->m_WorkDoneLock.EnterMutex();

        while (!done && (socketIndex < globalIndex))
        {
            if (socket->m_WorkDone.find(s_PortalArray[socketIndex]) == socket->m_WorkDone.end())        // Prevent reflection
            {
                VIS_SYNC_PORTAL tmppacket;

                if (Create_SYNC_PORTAL_OUTPACKET(&tmppacket, socketIndex, globalIndex, socket->m_ClientId))
                {
                    if (outpacket.addSubpacket(&tmppacket))
                    {
                        socketIndex++;
                    }
                    else
                    {
                        done = true;
                    }
                }
                else
                {
                    done = true;                           // hit an error (probably portal has stat_done not set)
                }
            }
            else
            {
                IfDebug(Developer(DEVELOPER_LEVEL_MEGASPAM, "Not sending SYNC_PORTAL packet of portal %d to client %d, client sent it to us!\n", s_PortalArray[socketIndex], socket->m_ClientId));
                socketIndex++;
            }
        }

        socket->m_WorkDoneLock.LeaveMutex();

        if (socketIndex < globalIndex)
        {
            outpacket.setMoreToSync(1);
        }
        else
        {
            outpacket.setMoreToSync(0);
        }
        int             workindex = 0;

        socket->m_WorkQueueLock.EnterMutex();
        while (socket->m_WorkQueue.size() < WORK_QUEUE_SIZE)
        {
            long            work = GetNextPortalIndex();

            socket->m_WorkQueue.push_back(work);
            outpacket.setWork(work, workindex);
            workindex++;
            IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] Adding work %d to SYNC_PORTAL_CLUSTER\n", socket->m_ClientId, work));
        }
        socket->m_WorkQueueLock.LeaveMutex();
        // Remaining index[] should still be set to -1 from the constructor
        IfDebug(Developer(DEVELOPER_LEVEL_FLUFF,
                  "[#%d] Sending SYNC_PORTAL_CLUSTER with %d subpackets, %d work jobs, more_to_sync(%d)\n",
                  socket->m_ClientId, socketIndex - socket->m_SyncIndex, workindex, outpacket.getMoreToSync())); // TODO print all work sent

        socket->SendPacket(&outpacket);
        socket->m_SyncIndex = socketIndex;
    }
    else                                                   // Just more work
    {
        outpacket.setMoreToSync(0);
        int             workindex = 0;

        socket->m_WorkQueueLock.EnterMutex();
        while (socket->m_WorkQueue.size() < WORK_QUEUE_SIZE)
        {
            long            work = GetNextPortalIndex();

            socket->m_WorkQueue.push_back(work);
            outpacket.setWork(work, workindex);
            workindex++;
            IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] Adding work %d to SYNC_PORTAL_CLUSTER\n", socket->m_ClientId, work));
        }
        socket->m_WorkQueueLock.LeaveMutex();
        // Remaining index[] should still be set to -1 from the constructor
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] Sending SYNC_PORTAL_CLUSTER with %d work jobs\n", socket->m_ClientId,
                  workindex));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_WANT_FULL_SYNC(WANT_FULL_SYNC* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive WANT_FULL_SYNC\n", socket->m_ClientId));

    Send_VIS_SYNC_PORTALs(socket);
}

void            Send_VIS_WANT_FULL_SYNC_ping(void)
{
    if (!s_OutstandingSyncRequests)
    {
        Send_VIS_WANT_FULL_SYNC();
    }
    else
    {
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "Send_VIS_WANT_FULL_SYNC_ping skipped (outstanding:%d)\n",
                  (int)s_OutstandingSyncRequests));
    }
}

void            Send_VIS_WANT_FULL_SYNC(void)
{
    WANT_FULL_SYNC  outpacket;

    s_OutstandingSyncRequests++;
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send WANT_FULL_SYNC (outstanding:%d)\n", (int)s_OutstandingSyncRequests));
    g_ClientSession->SendPacket(&outpacket);
}

void            Flag_VIS_DONE_PORTAL(long portalindex)
{
    s_PortalArrayLock.EnterMutex();
    hlassert(g_PortalArrayIndex < PORTAL_ARRAY_MAX_SIZE);
    s_PortalArray[g_PortalArrayIndex] = portalindex;
    g_PortalArrayIndex++;
    s_PortalArrayLock.LeaveMutex();
}

void            Handle_VIS_DONE_PORTAL(VIS_DONE_PORTAL* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_DONE_PORTAL, index = %d\n", socket->m_ClientId, packet->getPortalIndex()));

    portal_t*       p = GetPortalPtr(packet->getPortalIndex());

    hlassert(p);
    hlassert(p->status == stat_working);
    if (p->status == stat_done)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_WARNING, "[#%d] WARNING: portal %d already evaluated by client (%d)\n",
                  socket->m_ClientId, packet->getPortalIndex(), p->fromclient));
        Send_VIS_SYNC_PORTALs(socket);                     // SYNC_PORTALs packet can now assign work (must always respond or count gets off)
        return;
    }

    p->numcansee = packet->getNumCanSee();
    p->visbits = (byte*) malloc(g_bitbytes);
    memcpy(p->visbits, packet->data, g_bitbytes);
    p->fromclient = socket->m_ClientId;
    p->status = stat_done;

    s_PortalArrayLock.EnterMutex();

    hlassert(g_PortalArrayIndex < PORTAL_ARRAY_MAX_SIZE);
    s_PortalArray[g_PortalArrayIndex] = packet->getPortalIndex();
    socket->m_WorkDoneLock.EnterMutex();
    socket->m_WorkDone.insert(packet->getPortalIndex());
    socket->m_WorkDoneLock.LeaveMutex();
    socket->m_PortalCount++;
    g_PortalArrayIndex++;

    s_PortalArrayLock.LeaveMutex();

    socket->m_WorkQueueLock.EnterMutex();
#ifdef _DEBUG
    int             queue_front_val = socket->m_WorkQueue.front();
    int             current_index = packet->getPortalIndex();

    hlassert(queue_front_val == current_index);            // Should alwasy be in FIFO order on client and server both, should never deviate
#endif
    socket->m_WorkQueue.pop_front();
    socket->m_WorkQueueLock.LeaveMutex();

    Send_VIS_SYNC_PORTALs(socket);                         // SYNC_PORTALs packet can now assign work
}

void            Send_VIS_DONE_PORTAL(long portalindex, portal_t* p)
{
    hlassert(g_vismode == VIS_MODE_CLIENT);

    VIS_DONE_PORTAL outpacket;

    hlassert(g_bitbytes <= sizeof(outpacket.data));

    outpacket.setPortalIndex(portalindex);
    outpacket.setNumCanSee(p->numcansee);
    memcpy(outpacket.data, p->visbits, g_bitbytes);

    g_visportalindex = WAITING_FOR_PORTAL_INDEX;
    g_ClientSession->m_PortalCount++;
    s_OutstandingSyncRequests++;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_DONE_PORTAL (outstanding:%d), index = %d, size = %d\n",
              (int)s_OutstandingSyncRequests, outpacket.getPortalIndex(), outpacket.getSize()));
    g_ClientSession->SendPacket(&outpacket);
}

void            Handle_VIS_SYNC_PORTAL_CLUSTER(VIS_SYNC_PORTAL_CLUSTER* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    s_OutstandingSyncRequests--;
    s_IdleCounter++;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "Received VIS_SYNC_PORTAL_CLUSTER (outstanding:%d), count = %d\n", (int)s_OutstandingSyncRequests, packet->getSubpacketCount()));

    g_serverindex = packet->getServerIndex();
    int             count = packet->getSubpacketCount();            // Can be zero, if fully synced but server is just sending us work

    //    hlassert(count); 
    for (int i = 0; i < count; i++)
    {
        VIS_SYNC_PORTAL* subpacket = packet->getSubpacket(i);

        hlassert(subpacket);
        hlassert(subpacket->validate());
        portal_t*       p = GetPortalPtr(subpacket->getPortalIndex());

        hlassert(p);

        IfDebug(Developer(DEVELOPER_LEVEL_MEGASPAM, "Received SYNC_PORTAL subpacket #%d, portal = %d, fromclient = %d\n", i, subpacket->getPortalIndex(), subpacket->getFromClient()));

        if (p->status == stat_none)
        {
            p->numcansee = subpacket->getNumCanSee();
            p->visbits = (byte*) malloc(g_bitbytes);
            memcpy(p->visbits, subpacket->data, g_bitbytes);
            p->fromclient = subpacket->getFromClient();
            p->status = stat_done;
        }
        else if (subpacket->getFromClient() != p->fromclient)
        {
            const char*     match;
            developer_level_t level;

            if (p->visbits)
            {
                if (memcmp(p->visbits, &subpacket->data[0], g_bitbytes))
                {
                    match = "different";
                    level = DEVELOPER_LEVEL_ERROR;
                }
                else
                {
                    match = "identical";
                    level = DEVELOPER_LEVEL_WARNING;
                }
            }
            else
            {
                match = "unexpected";
                level = DEVELOPER_LEVEL_ERROR;
            }
            IfDebug(Developer(level,
                      "Received %s SYNC_PORTAL packet for a portal(%d) originating from client (%d) which was already processed by client (%d)\n",
                      match, subpacket->getPortalIndex(), subpacket->getFromClient(), p->fromclient));
        }
    }
    for (int j = 0; j < WORK_QUEUE_SIZE; j++)
    {
        INT32           work = packet->getWork(j);

        if ((work >= 0) || (work == NO_PORTAL_INDEX))
        {
            IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "VIS_SYNC_PORTAL_CLUSTER, added work %d to queue\n", work));
            addWorkToClientQueue(work);
        }
    }
    if (packet->getMoreToSync() > 0)
    {
        Send_VIS_WANT_FULL_SYNC();
    }
}

void            Send_VIS_LOGIN(void)
{
    VIS_LOGIN       outpacket;

    gethostname(outpacket.hostname, sizeof(outpacket.hostname));

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_LOGIN\n"));
    g_ClientSession->SendPacket(&outpacket);
}

void            Handle_VIS_LOGIN(VIS_LOGIN* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LOGIN (version %u)\n", socket->m_ClientId, packet->getProtocolVersion()));
    if (packet->getProtocolVersion() == NETVIS_PROTOCOL_VERSION)
    {
        VIS_LOGIN_ACK   outpacket;

        socket->m_RemoteHostname = packet->hostname;
        int             ClientId = ++g_nextclientid;

        outpacket.setClientId(ClientId);
        outpacket.setFullvis(g_fullvis);
        socket->m_ClientId = ClientId;
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_LOGIN_ACK\n", socket->m_ClientId));
        socket->SendPacket(&outpacket);
    }
    else
    {
        VIS_LOGIN_NAK   outpacket;

        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_LOGIN_NAK\n", socket->m_ClientId));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_LOGIN_NAK(VIS_LOGIN_NAK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LOGIN_ACK\n", socket->m_ClientId));
    g_NetvisAbort = true;
    Error("Incompatible netvis version on server, server is version %u, client is %u\n", packet->getProtocolVersion(), NETVIS_PROTOCOL_VERSION);
}

void            Handle_VIS_LOGIN_ACK(VIS_LOGIN_ACK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    socket->m_ClientId = g_clientid = packet->getClientId();
    g_fullvis = packet->getFullvis();
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LOGIN_ACK\n", socket->m_ClientId));
}

//
// .BSP Download request/handling functions
//
static unsigned s_bsp_image_bytes_written = 0;
static char*    s_bsp_compressed_image = NULL;

void            Send_VIS_WANT_BSP_DATA(void)
{
    hlassert(g_vismode == VIS_MODE_CLIENT);
    VIS_WANT_BSP_DATA outpacket;

    s_bsp_image_bytes_written = 0;
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_WANT_BSP_DATA\n"));
    g_ClientSession->SendPacket(&outpacket);
}

static void     Send_VIS_BSP_DATA(NetvisSession* socket)
{
    hlassert(g_bsp_image);
    hlassert(g_bsp_size);
    hlassert(g_bsp_compressed_size);
    const unsigned  chunk_size = sizeofElement(VIS_BSP_DATA, data);
    unsigned        chunks = g_bsp_compressed_size / chunk_size;
    unsigned        remainder = g_bsp_compressed_size % chunk_size;

    hlassert(chunks || remainder);

    unsigned        x;
    unsigned        offset;
    VIS_BSP_DATA    outpacket;

    outpacket.setCompressedSize(g_bsp_compressed_size);
    outpacket.setUncompressedSize(g_bsp_size);
    outpacket.setFragmentSize(chunk_size);
    for (x = 0, offset = 0; x < chunks; x++, offset += chunk_size)
    {
        outpacket.setOffset(offset);
        memcpy(outpacket.data, g_bsp_image + offset, chunk_size);
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_BSP_DATA (fragment %u of %u)\n", socket->m_ClientId, x + 1,
                  chunks));
        socket->SendPacket(&outpacket);
    }
    if (remainder)
    {
        outpacket.setOffset(offset);
        outpacket.setFragmentSize(remainder);
        memcpy(outpacket.data, g_bsp_image + offset, remainder);
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_BSP_DATA (fragment %u of %u) (remainder size %u) \n",
                  socket->m_ClientId, x + 1, chunks, remainder));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_WANT_BSP_DATA(VIS_WANT_BSP_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);
    if (g_bsp_image)
    {
        VIS_BSP_NAME outpacket(g_Mapname);
        socket->SendPacket(&outpacket);

        Send_VIS_BSP_DATA(socket);
    }
    else
    {
        VIS_BSP_DATA_NAK outpacket;

        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_BSP_DATA_NAK\n", socket->m_ClientId));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_BSP_DATA(VIS_BSP_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(packet->myValidate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    hlassert(s_bsp_image_bytes_written + packet->getFragmentSize() <= packet->getCompressedSize());

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] received BSP data fragment (compressedsize %u, uncompressedsize %u, fragmentsize %u, offset %u)\n",
              socket->m_ClientId, packet->getCompressedSize(), packet->getUncompressedSize(), packet->getFragmentSize(), packet->getOffset()));

    if (s_bsp_compressed_image)
    {
        memcpy(s_bsp_compressed_image + packet->getOffset(), packet->data, packet->getFragmentSize());
        s_bsp_image_bytes_written += packet->getFragmentSize();
    }
    else
    {
        s_bsp_compressed_image = (char*)calloc(1, packet->getCompressedSize());
        memcpy(s_bsp_compressed_image + packet->getOffset(), packet->data, packet->getFragmentSize());
        s_bsp_image_bytes_written += packet->getFragmentSize();
    }

    if (s_bsp_image_bytes_written == packet->getCompressedSize())
    {
        unsigned long compressed_size = packet->getCompressedSize();
        unsigned long uncompressed_size = packet->getUncompressedSize();
        g_bsp_image = (char*)calloc(1, uncompressed_size);
    
        if (uncompress((byte*)g_bsp_image, &uncompressed_size, (byte*)s_bsp_compressed_image, compressed_size) != Z_OK)
        {
            Send_VIS_GOING_DOWN(socket);
            Error("zlib Uncompress error with bsp image\n");
        }

        free(s_bsp_compressed_image);
        s_bsp_compressed_image = NULL;
        g_bsp_downloaded = true;
    }
}

void            Handle_VIS_BSP_DATA_NAK(VIS_BSP_DATA_NAK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_BSP_DATA_NAK\n", socket->m_ClientId));

    Log("Waiting for bsp image, retrying after 5 seconds . . .\n");
    NetvisSleep(5000);                                     // Repeatedly try to get BSP data image
    Send_VIS_WANT_BSP_DATA();
}

void            Handle_VIS_BSP_NAME(VIS_BSP_NAME* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);

    safe_strncpy(g_Mapname, packet->getName(), _MAX_PATH);
    Log("Processing map '%s'\n", g_Mapname);
}

//
// .PRT Download request/handling functions
//
static unsigned s_prt_image_bytes_written = 0;
static char*    s_prt_compressed_image = NULL;

void            Send_VIS_WANT_PRT_DATA(void)
{
    hlassert(g_vismode == VIS_MODE_CLIENT);
    s_prt_image_bytes_written = 0;
    VIS_WANT_PRT_DATA outpacket;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_WANT_PRT_DATA\n"));
    g_ClientSession->SendPacket(&outpacket);
}

static void     Send_VIS_PRT_DATA(NetvisSession* socket)
{
    hlassert(g_prt_image);
    hlassert(g_prt_size);
    const unsigned  chunk_size = sizeofElement(VIS_PRT_DATA, data);
    unsigned        chunks = g_prt_compressed_size / chunk_size;
    unsigned        remainder = g_prt_compressed_size % chunk_size;

    hlassert(chunks || remainder);

    unsigned        x;
    unsigned        offset;
    VIS_PRT_DATA    outpacket;

    outpacket.setCompressedSize(g_prt_compressed_size);
    outpacket.setUncompressedSize(g_prt_size);
    outpacket.setFragmentSize(chunk_size);
    for (x = 0, offset = 0; x < chunks; x++, offset += chunk_size)
    {
        outpacket.setOffset(offset);
        memcpy(outpacket.data, g_prt_image + offset, chunk_size);
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_PRT_DATA (fragment %u of %u)\n", socket->m_ClientId, x + 1, chunks));
        socket->SendPacket(&outpacket);
    }
    if (remainder)
    {
        outpacket.setOffset(offset);
        outpacket.setFragmentSize(remainder);
        memcpy(outpacket.data, g_prt_image + offset, remainder);
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_PRT_DATA (fragment %u of %u) (remainder size %u) \n", socket->m_ClientId, x + 1, chunks, remainder));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_WANT_PRT_DATA(VIS_WANT_PRT_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);

    if (g_prt_image)
    {
        Send_VIS_PRT_DATA(socket);
    }
    else
    {
        VIS_PRT_DATA_NAK outpacket;

        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_PRT_DATA_NAK\n", socket->m_ClientId));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_PRT_DATA(VIS_PRT_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(packet->myValidate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    hlassert(s_prt_image_bytes_written + packet->getFragmentSize() <= packet->getCompressedSize());

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] received PRT data fragment (compressedsize %u, uncompressedsize %u, fragmentsize %u, offset %u)\n",
              socket->m_ClientId, packet->getCompressedSize(), packet->getUncompressedSize(), packet->getFragmentSize(), packet->getOffset()));

    if (s_prt_compressed_image)
    {
        memcpy(s_prt_compressed_image + packet->getOffset(), packet->data, packet->getFragmentSize());
        s_prt_image_bytes_written += packet->getFragmentSize();
    }
    else
    {
        s_prt_compressed_image = (char*)calloc(1, packet->getCompressedSize());
        memcpy(s_prt_compressed_image + packet->getOffset(), packet->data, packet->getFragmentSize());
        s_prt_image_bytes_written += packet->getFragmentSize();
    }

    if (s_prt_image_bytes_written == packet->getCompressedSize())
    {
        unsigned long compressed_size = packet->getCompressedSize();
        unsigned long uncompressed_size = packet->getUncompressedSize();
        g_prt_image = (char*)calloc(1, uncompressed_size);
    
        if (uncompress((byte*)g_prt_image, &uncompressed_size, (byte*)s_prt_compressed_image, compressed_size) != Z_OK)
        {
            Send_VIS_GOING_DOWN(socket);
            Error("zlib Uncompress error with prt image\n");
        }

        free(s_prt_compressed_image);
        s_prt_compressed_image = NULL;
        g_prt_downloaded = true;
    }
}

void            Handle_VIS_PRT_DATA_NAK(VIS_PRT_DATA_NAK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_PRT_DATA_NAK\n", socket->m_ClientId));

    Log("Waiting for prt image, retrying after 5 seconds . . .\n");
    NetvisSleep(5000);                                     // Repeatedly try to get PRT data image
    Send_VIS_WANT_PRT_DATA();
}

//
// BasePortalVis download
//

void            Send_VIS_WANT_MIGHTSEE_DATA(void)
{
    VIS_WANT_MIGHTSEE_DATA outpacket;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_WANT_MIGHTSEE_DATA\n"));
    g_ClientSession->SendPacket(&outpacket);
}

static void     Send_VIS_MIGHTSEE_DATA(NetvisSession* socket)
{

    unsigned x;
    const unsigned numportals = g_numportals * 2;
    portal_t* p = g_portals;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_MIGHTSEE_DATA (%u bytes)\n", socket->m_ClientId, g_bitbytes * numportals));

    for (x=0; x<numportals; x++, p++)
    {
        VIS_MIGHTSEE_DATA outpacket;
        outpacket.setPortalIndex(x);
        outpacket.setNumMightSee(p->nummightsee);
        memcpy(outpacket.data, p->mightsee, g_bitbytes);
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_WANT_MIGHTSEE_DATA(VIS_WANT_MIGHTSEE_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);

    if (g_visstate <= VIS_BASE_PORTAL_VIS)
    {
        VIS_MIGHTSEE_DATA_NAK outpacket;
        socket->SendPacket(&outpacket);
    }
    else if (g_visstate <= VIS_POST)
    {
        Send_VIS_MIGHTSEE_DATA(socket);
    }
    else
    {
        Send_VIS_GOING_DOWN(socket);
    }
}

void            Handle_VIS_MIGHTSEE_DATA_NAK(VIS_MIGHTSEE_DATA_NAK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_MIGHTSEE_DATA_NAK\n", socket->m_ClientId));

    Log("Waiting for BasePortalVis data from server, retrying after 5 seconds . . .\n");
    NetvisSleep(5000);                                     // Repeatedly try to get PRT data image
    Send_VIS_WANT_MIGHTSEE_DATA();
}

void            Handle_VIS_MIGHTSEE_DATA(VIS_MIGHTSEE_DATA* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);

    INT32 index = packet->getPortalIndex();
    IfDebug(Developer(DEVELOPER_LEVEL_MEGASPAM, "[#%d] receive VIS_MIGHTSEE_DATA (%d)\n", socket->m_ClientId, index));

    portal_t*       p = GetPortalPtr(index);
    hlassert(p);
    hlassert(p->mightsee == NULL);

    p->nummightsee = packet->getNumMightSee();
    p->mightsee = (byte*) malloc(g_bitbytes);
    memcpy(p->mightsee , packet->data, g_bitbytes);

    index++;    // Fix fencepost comparison
    if (index == (g_numportals * 2))
    {
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] mightsee data download complete (%d)\n", socket->m_ClientId));
        g_mightsee_downloaded = true;
    }
}

//
// Leafthread commmencement (client needs work) request
//

void            Send_VIS_LEAFTHREAD(long portalleafs, long numportals, long bitbytes)
{
    VIS_LEAFTHREAD  packet;

    packet.setPortalLeafs(portalleafs);
    packet.setNumPortals(numportals);
    packet.setBitBytes(bitbytes);

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "send VIS_LEAFTHREAD\n"));
    g_ClientSession->SendPacket(&packet);
}

void            Handle_VIS_LEAFTHREAD(VIS_LEAFTHREAD* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_SERVER);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LEAFTHREAD\n", socket->m_ClientId));

    long            visleafs = g_visleafs;                 // Prevent if() from reading zero and being written to afterwards (during output of packet and messages)
    long            visportals = g_visportals;             // same
    long            bitbytes = g_bitbytes;

    if ((packet->getPortalLeafs() == visleafs) && (packet->getNumPortals() == visportals)
        && (packet->getBitBytes() == bitbytes))
    {
        VIS_LEAFTHREAD_ACK outpacket;

        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_LEAFTHREAD_ACK\n", socket->m_ClientId));
        Log("Client #%d beginning work\n", socket->m_ClientId);
        socket->SendPacket(&outpacket);
    }
    else
    {
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM,
                  "[#%d] Client/Server portal mismatch : client portals(%d) leafs (%d) bitbytes (%d), server portals(%d) leafs(%d) bitbytes (%d)\n",
                  socket->m_ClientId, visportals, visleafs, bitbytes, packet->getNumPortals(), packet->getPortalLeafs(),
                  packet->getBitBytes()));
        VIS_LEAFTHREAD_NAK outpacket;

        outpacket.setPortalLeafs(visleafs);
        outpacket.setNumPortals(visportals);
        outpacket.setBitBytes(bitbytes);
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] send VIS_LEAFTHREAD_NAK\n", socket->m_ClientId));
        socket->SendPacket(&outpacket);
    }
}

void            Handle_VIS_LEAFTHREAD_ACK(VIS_LEAFTHREAD_ACK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LEAFTHREAD_ACK\n", socket->m_ClientId));

    g_visleafthread = 1;
}

void            Handle_VIS_LEAFTHREAD_NAK(VIS_LEAFTHREAD_NAK* packet, NetvisSession* socket)
{
    hlassert(packet->validate());
    hlassert(g_vismode == VIS_MODE_CLIENT);
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "[#%d] receive VIS_LEAFTHREAD_NAK\n", socket->m_ClientId));

    g_visleafthread = 0;

    IfDebug(Developer(DEVELOPER_LEVEL_SPAM,
              "[#%d] Client/Server portal mismatch : client portals(%d) leafs (%d) bitbytes(%d), server portals(%d) leafs(%d) bitbytes(%d)\n",
              socket->m_ClientId, packet->getNumPortals(), packet->getPortalLeafs(), packet->getBitBytes(),
              g_visportals, g_visleafs, g_bitbytes));
    if ((packet->getNumPortals() == 0) || (packet->getPortalLeafs() == 0) || (packet->getBitBytes() == 0))
    {
        // Client finished BasePortalVis before server, try again periodically
        NetvisSleep(5000);                                 // Repeatedly try LEAFTHREAD start every 5 seconds
        Send_VIS_LEAFTHREAD(g_visleafs, g_visportals, g_bitbytes);
    }
    else
    {
        Send_VIS_GOING_DOWN(socket);
    }
}

//
// Work queue functions
//

static          std::deque < long >s_ClientWorkQueue;
static Mutex    s_ClientWorkQueueLock;                     // protects s_ClientWorkQueue
static ReferenceCounter s_OutOfWorkFlag;

int             outOfWork(void)
{
    s_IdleCounter = 0;
    s_ClientWorkQueueLock.EnterMutex();
    s_OutOfWorkFlag = s_ClientWorkQueue.size();
    s_ClientWorkQueueLock.LeaveMutex();
    return (s_OutOfWorkFlag == 0);
}

int             stillOutOfWork(void)
{
    return (s_OutOfWorkFlag == 0);
}

int             needServerUpdate(void)
{
    return (s_IdleCounter == 0);
}

void            addWorkToClientQueue(long index)
{
    s_ClientWorkQueueLock.EnterMutex();                    // already locked from Handle_VIS_ code
    s_ClientWorkQueue.push_back(index);
    s_OutOfWorkFlag++;
    s_ClientWorkQueueLock.LeaveMutex();
    IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "addWorkToClientQueue(%d)\n", index));
}

long            getWorkFromClientQueue(void)
{
    s_ClientWorkQueueLock.EnterMutex();
    if (s_ClientWorkQueue.empty())
    {
        g_visportalindex = WAITING_FOR_PORTAL_INDEX;
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "getWorkFromClientQueue returning WAITING_FOR_PORTAL_INDEX\n"));
    }
    else
    {
        g_visportalindex = s_ClientWorkQueue.front();
        s_ClientWorkQueue.pop_front();
        IfDebug(Developer(DEVELOPER_LEVEL_SPAM, "getWorkFromClientQueue returning %d\n", g_visportalindex));
    }
    s_ClientWorkQueueLock.LeaveMutex();
    return g_visportalindex;
}
