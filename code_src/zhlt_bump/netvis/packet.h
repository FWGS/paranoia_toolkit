#ifndef PACKET_H__
#define	PACKET_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "basictypes.h"
#include "hlassert.h"
#include "vis.h"

#define NETVIS_PROTOCOL_VERSION    2042

#define MAX_PACKET_SIZE           65000

#define PORTAL_ARRAY_MAX_SIZE    128000
#define UNINITIALIZED_PORTAL_INDEX   -1
#define WAITING_FOR_PORTAL_INDEX     -2
#define	NO_PORTAL_INDEX              -3

#define VARIABLE_LENGTH_PACKET	     -1

// WORK_QUEUE_SIZE must be even for byte packing resons (8 byte alignment on structures)
#define WORK_QUEUE_SIZE               8

#ifdef __cplusplus

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <iostream>

#include "EndianMath.h"

class NetvisSession;

class basePacket
{
protected: // data
    UINT32          header;
    UINT32          pad;

public:    
    enum PACKETtypes
    {
        VIS_PACKET_NULL,

        VIS_PACKET_LOGIN,            // Never change this id (it breaks graceful protocol negotiation)
        VIS_PACKET_LOGIN_NAK,        // Never change this id (it breaks graceful protocol negotiation)
        VIS_PACKET_LOGIN_ACK,        // Never change this id (it breaks graceful protocol negotiation)

        VIS_PACKET_WANT_BSP_DATA,    // Requests download of .bsp file image
        VIS_PACKET_BSP_DATA_NAK,     // Server not ready, try again later
        VIS_PACKET_BSP_DATA,         // .bsp image download
        VIS_PACKET_BSP_NAME,         // .bsp filename on server (for informational purposes on client)

        VIS_PACKET_WANT_PRT_DATA,    // Requests download of .prt file image
        VIS_PACKET_PRT_DATA_NAK,     // Server not ready, try again later
        VIS_PACKET_PRT_DATA,         // .prt image download

        VIS_PACKET_WANT_MIGHTSEE_DATA,  // Requests download of baseportalvis's calculated mightsee & nummightsee data
        VIS_PACKET_MIGHTSEE_DATA_NAK,   // Server is still crunching BasePortalVis, try again shortly
        VIS_PACKET_MIGHTSEE_DATA,       // One of (many) packets containing baseportalvis mightsee & nummightsee data

        VIS_PACKET_LEAFTHREAD,       // Tells server client BasePortalVis has completed and is awaiting work
        VIS_PACKET_LEAFTHREAD_ACK,   // Tells client LeafThread work is ready for request
        VIS_PACKET_LEAFTHREAD_NAK,   // Tells client server is not ready to parcel out LeafThread work yet

        VIS_PACKET_WANT_FULL_SYNC,   // Tells server that the client wants a sync
        VIS_PACKET_DONE_PORTAL,		 // Gives server a completed work packet
        VIS_PACKET_SYNC_PORTAL,      // Internal packet for SYNC_PORTAL_CLUSTER
        VIS_PACKET_SYNC_PORTAL_CLUSTER,  // Multiple IS_SYNC_PORTAL packets embedded in a single packet, client to server sync up of work completed by all the other clients

        VIS_PACKET_GOING_DOWN        // Tells remote system connection is going to be terminated
    };

public: // static methods
    static  INT getPacketSizeByType(INT type);
    static void HandleIncomingPacket(basePacket* packet, NetvisSession* socket);
    static void DroppedClientForPortalIndex(long clientid, long index);
    
public: // construction
    basePacket(UINT8 type)
    {
        setType(type);
        setFiller(0);
        INT size = getPacketSizeByType(type);
        if (size != VARIABLE_LENGTH_PACKET)
        {
            setSize((UINT16)size);
        }
        else
        {
            size = 0;
        }
    }
    
public: // methods
    void setType(UINT8 type)
    {
        UINT8* pHeader = reinterpret_cast<UINT8*>(&header);
        pHeader[0] = type;
    }
    void setFiller(UINT8 filler)
    {
        UINT8* pHeader = reinterpret_cast<UINT8*>(&header);
        pHeader[1] = filler;
    }
    void setSize(UINT16 size)
    {
        UINT16* pHeader = reinterpret_cast<UINT16*>(&header);
#ifdef __LITTLE_ENDIAN__
        pHeader[1] = size;
#endif
#ifdef __BIG_ENDIAN__
        pHeader[1] = Endian::Flip(size);
#endif
    }

    UINT8 getType() const
    { 
        const UINT8* pHeader = reinterpret_cast<const UINT8*>(&header);
        return pHeader[0];
    }
    UINT8 getFiller() const
    { 
        const UINT8* pHeader = reinterpret_cast<const UINT8*>(&header);
        return pHeader[1];
    }
    UINT16 getSize() const
    {
        const UINT16* pHeader = reinterpret_cast<const UINT16*>(&header);
#ifdef __LITTLE_ENDIAN__
        return pHeader[1];
#endif
#ifdef __BIG_ENDIAN__
        return Endian::Flip(pHeader[1]);
#endif
    }
    bool validate() const
    { 
        int size = getPacketSizeByType(getType());
        if (size != 0)
        {
            if (size == getSize())
            {
                return true;
            }
            if (size == VARIABLE_LENGTH_PACKET)
            {
                return true;
            }
        }
        return false;
    }
};



class fixedPacket : public basePacket
{
public: // construction
    fixedPacket(UINT8 type) : basePacket(type) {}
public: // methods
};



class varPacket : public basePacket
{
public: // construction
    varPacket(UINT8 type) : basePacket(type) {}
};



class VIS_PACKET : public fixedPacket
{
public: // data
    CHAR            data[MAX_PACKET_SIZE - sizeof(fixedPacket)];
public: // construction
    VIS_PACKET() : fixedPacket(VIS_PACKET_NULL) {}
};



class GOING_DOWN : public fixedPacket
{
public: // construction
    GOING_DOWN() : fixedPacket(VIS_PACKET_GOING_DOWN) {}
};



class WANT_FULL_SYNC : public fixedPacket
{
public: // construction
    WANT_FULL_SYNC() : fixedPacket(VIS_PACKET_WANT_FULL_SYNC) {}
};



class VIS_DONE_PORTAL : public varPacket
{	// ATTN: This is a variable-length packet.  Changing this structure requires fixing up the ::Send code, as well as the getPacketSizeByType method)
protected: // data
    INT32           portalindex;
    INT32           numcansee;
public:
    CHAR            data[4000];
public: // construction
    VIS_DONE_PORTAL() : varPacket(VIS_PACKET_DONE_PORTAL) { setSize(myoffsetof(VIS_DONE_PORTAL, data) + g_bitbytes); }
#ifdef __LITTLE_ENDIAN__
    INT32 getPortalIndex() const
    {
        return portalindex;
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = val;
    }
    INT32 getNumCanSee() const
    {
        return numcansee;
    }
    void setNumCanSee(INT32 val)
    {
        numcansee = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    INT32 getPortalIndex() const
    {
        return Endian::Flip(portalindex);
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = Endian::Flip(val);
    }
    INT32 getNumCanSee() const
    {
        return Endian::Flip(numcansee);
    }
    void setNumCanSee(INT32 val)
    {
        numcansee = Endian::Flip(val);
    }
#endif
};



class VIS_SYNC_PORTAL : public varPacket
{	// ATTN: This is a variable-length packet.  Changing this structure requires fixing up the ::Send code, as well as the getPacketSizeByType method)
protected: // data
    INT32           portalindex;
    INT32           numcansee;
    INT32           fromclient; // which client this packet originated from
    INT32           pad2;
public:
    CHAR            data[4000];
public: // construction
    VIS_SYNC_PORTAL() : varPacket(VIS_PACKET_SYNC_PORTAL) { setSize(myoffsetof(VIS_SYNC_PORTAL, data) + g_bitbytes); }
#ifdef __LITTLE_ENDIAN__
    INT32 getPortalIndex() const
    {
        return portalindex;
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = val;
    }
    INT32 getNumCanSee() const
    {
        return numcansee;
    }
    void setNumCanSee(INT32 val)
    {
        numcansee = val;
    }
    INT32 getFromClient() const
    {
        return fromclient;
    }
    void setFromClient(INT32 val)
    {
        fromclient = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    INT32 getPortalIndex() const
    {
        return Endian::Flip(portalindex);
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = Endian::Flip(val);
    }
    INT32 getNumCanSee() const
    {
        return Endian::Flip(numcansee);
    }
    void setNumCanSee(INT32 val)
    {
        numcansee = Endian::Flip(val);
    }
    INT32 getFromClient() const
    {
        return Endia::Flip(fromclient);
    }
    void setFromClient(INT32 val)
    {
        fromclient = Endian::Flip(val);
    }
#endif
};



class VIS_SYNC_PORTAL_CLUSTER : public varPacket
{	// ATTN: This is a variable-length packet.  Changing this structure requires fixing up the ::Send code, as well as the getPacketSizeByType method)
protected: // data
    INT32           index[WORK_QUEUE_SIZE];  // >= 0 signifies this is a DONE sync, and to process this index
    INT32           more_to_sync;   // > 0 signifies there is more to sync, and a sync request should be fired off, 31 bits are available here if you really need them
    INT32           server_total;   // g_PortalArrayIndex of the server
    INT32           subpacket_count;          // number of subpackets
public:
    CHAR			data[MAX_PACKET_SIZE - sizeof(varPacket) - (sizeof(INT32) * (WORK_QUEUE_SIZE+2))];    // data must be the last structure (unless the use of offsetof is changed to work it out)
public: // construction
    VIS_SYNC_PORTAL_CLUSTER() : varPacket(VIS_PACKET_SYNC_PORTAL_CLUSTER) 
        { 
            hlassert(sizeof(VIS_SYNC_PORTAL_CLUSTER) <= 65535);
            setSize(myoffsetof(VIS_SYNC_PORTAL_CLUSTER, data));
            setSubpacketCount(0);
            for (int i = 0; i < WORK_QUEUE_SIZE; i++)
            {
                index[i] = UNINITIALIZED_PORTAL_INDEX;
            }
            more_to_sync = false; 
        }
public: // methods
    bool addSubpacket(VIS_SYNC_PORTAL* subpacket)
    {
        unsigned size = subpacket->getSize();
        hlassert(size == getPacketSizeByType(VIS_PACKET_SYNC_PORTAL));
        if ((getSize() + size) < sizeof(VIS_SYNC_PORTAL_CLUSTER))
        {
            char* pDataDest = reinterpret_cast<char*>(this) + getSize();
            setSubpacketCount(getSubpacketCount() + 1);
            setSize((UINT16) (getSize()+ size));
            memcpy(pDataDest, subpacket, size);
            return true;
        }
        return false;
    }
    VIS_SYNC_PORTAL* getSubpacket(UINT32 index)
    {
        hlassert(index < getSubpacketCount());
        int size = getPacketSizeByType(VIS_PACKET_SYNC_PORTAL);
        int offset = size * index;
        hlassert(offset < getSize());   // rough cut, should never happen (packet header actually offsets this abit but its a fatal error either way)
        if (offset > getSize())
        {
            return NULL;
        }
        return reinterpret_cast<VIS_SYNC_PORTAL*>(data + offset);
    }
#ifdef __LITTLE_ENDIAN__
    INT32 getWork(int offset) const
    {
        return index[offset];
    }
    void setWork(INT32 val, int offset)
    {
        index[offset] = val;
    }
    INT32 getSubpacketCount() const
    {
        return subpacket_count;
    }
    void setSubpacketCount(INT32 val)
    {
        subpacket_count = val;
    }
    INT32 getMoreToSync() const
    {
        return more_to_sync;
    }
    void setMoreToSync(INT32 val)
    {
        more_to_sync = val;
    }
    INT32 getServerIndex() const
    {
        return server_total;
    }
    void setServerIndex(INT32 val)
    {
        server_total= val;
    }
#endif
#ifdef __BIG_ENDIAN__
    INT32 getWork(int offset) const
    {
        return Endian::Flip(index[offset]);
    }
    void setWork(INT32 val, int offset)
    {
        index[offset] = Endian::Flip(val);
    }
    INT32 getSubpacketCount() const
    {
        return Endian::Flip(subpacket_count);
    }
    void setSubpacketCount(INT32 val)
    {
        subpacket_count = Endian::Flip(val);
    }
    INT32 getMoreToSync() const
    {
        return Endian::Flip(more_to_sync);
    }
    void setMoreToSync(INT32 val)
    {
        more_to_sync = Endian::Flip(val);
    }
    INT32 getServerIndex() const
    {
        return Endian::Flip(server_total);
    }
    void setServerIndex(INT32 val)
    {
        server_total = Endian::Flip(val);
    }
#endif
};



// Never change this packet header structure (it breaks graceful protocol negotiation)
class VIS_LOGIN : public fixedPacket
{
public: // data
    CHAR            hostname[128];
    UINT32          protocol_version;
public: // construction
    VIS_LOGIN() : fixedPacket(VIS_PACKET_LOGIN) {setProtocolVersion(NETVIS_PROTOCOL_VERSION);}
#ifdef __LITTLE_ENDIAN__
    UINT32 getProtocolVersion() const
    {
        return protocol_version;
    }
    void setProtocolVersion(UINT32 val)
    {
        protocol_version = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getProtocolVersion() const
    {
        return Endian::Flip(protocol_version);
    }
    void setProtocolVersion(UINT32 val)
    {
        protocol_version = Endian::Flip(val);
    }
#endif
};


// Never change this packet header structure (it breaks graceful protocol negotiation)
class VIS_LOGIN_NAK : public fixedPacket
{
public: // data
    UINT32          protocol_version;
public: // construction
    VIS_LOGIN_NAK() : fixedPacket(VIS_PACKET_LOGIN_NAK) {setProtocolVersion(NETVIS_PROTOCOL_VERSION);}
#ifdef __LITTLE_ENDIAN__
    UINT32 getProtocolVersion() const
    {
        return protocol_version;
    }
    void setProtocolVersion(UINT32 val)
    {
        protocol_version = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getProtocolVersion() const
    {
        return Endian::Flip(protocol_version);
    }
    void setProtocolVersion(UINT32 val)
    {
        protocol_version = Endian::Flip(val);
    }
#endif
};

#define flagFullvis (1 << 0)

// Never change this packet header structure (it breaks graceful protocol negotiation)
class VIS_LOGIN_ACK : public fixedPacket
{
protected: // data
    UINT32			clientid;
    UINT32          flags;
public: // construction
    VIS_LOGIN_ACK() : fixedPacket(VIS_PACKET_LOGIN_ACK) {}
#ifdef __LITTLE_ENDIAN__
    UINT32 getClientId() const
    {
        return clientid;
    }
    void setClientId(UINT32 val)
    {
        clientid = val;
    }
    bool getFullvis() const
    {
        return flags & flagFullvis;
    }
    void setFullvis(bool on)
    {
        if (on)
        {
            flags |= flagFullvis;
        }
        else
        {
            flags &= ~flagFullvis;
        }
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getClientId() const
    {
        return Endian::Flip(clientid);
    }
    void setClientId(UINT32 val)
    {
        clientid = Endian::Flip(val);
    }
    bool getFullvis() const
    {
        UINT32 tmp = Endian::Flip(flags);
        return tmp & flagFullvis;
    }
    void setFullvis(bool on)
    {
        UINT32 tmp = Endian::Flip(flags);
        if (on)
        {
            tmp |= flagFullvis
        }
        else
        {
            tmp &= ~flagFullvis;
        }
        flags = Endian::Flip(tmp);
    }
#endif
};



//
// Packets to download BSP data from server to client
//

class VIS_WANT_BSP_DATA : public fixedPacket
{
public: // construction
    VIS_WANT_BSP_DATA() : fixedPacket(VIS_PACKET_WANT_BSP_DATA) {}
};


class VIS_BSP_DATA_NAK : public fixedPacket
{
public: // construction
    VIS_BSP_DATA_NAK() : fixedPacket(VIS_PACKET_BSP_DATA_NAK) {}
};

class VIS_BSP_NAME : public fixedPacket
{
protected:
    CHAR            bspname[512];
public: // construction
    VIS_BSP_NAME(const char* const name) : fixedPacket(VIS_PACKET_BSP_NAME) 
    {
        setName(name);
    }
public:
    const char* getName() const
    {
        return bspname;
    }
    void setName(const char* const name)
    {
        safe_strncpy(bspname, name, sizeof(bspname));
    }
};

#define VIS_BSP_DATA_MAX_SIZE  (1024*1024*5)
#define VIS_PRT_DATA_MAX_SIZE  (1024*1024*1)

class VIS_BSP_DATA : public varPacket
{
protected: // data
    UINT32          compressed_size;        // Total size of image in compressed bytes
    UINT32          uncompressed_size;      // Total size of image in uncompressed bytes
    UINT32          fragment_size;
    UINT32          offset;
public:
    CHAR			data[MAX_PACKET_SIZE - sizeof(varPacket) - (sizeof(UINT32) * 4)];  // data must be the last structure (unless the use of offsetof is changed to work it out)
public: // construction
    VIS_BSP_DATA() : varPacket(VIS_PACKET_BSP_DATA) { setSize(sizeof(VIS_BSP_DATA)); }
    bool myValidate(void)   // Since we can't use virtual functions in network protocol packets, cant use validate()
    {
        hlassert(uncompressed_size <= VIS_BSP_DATA_MAX_SIZE);
        if (uncompressed_size > VIS_BSP_DATA_MAX_SIZE)
        {
            return false;
        }
        hlassert(fragment_size <= sizeof(data));
        if (fragment_size > sizeof(data))
        {
            return false;
        }
        hlassert((offset + fragment_size) <= compressed_size);
        if ((offset + fragment_size) > compressed_size)
        {
            return false;
        }
        return true;
    }
#ifdef __LITTLE_ENDIAN__
    UINT32 getCompressedSize() const
    {
        return compressed_size;
    }
    void setCompressedSize(UINT32 val)
    {
        compressed_size = val;
    }
    UINT32 getUncompressedSize() const
    {
        return uncompressed_size;
    }
    void setUncompressedSize(UINT32 val)
    {
        uncompressed_size = val;
    }
    UINT32 getFragmentSize() const
    {
        return fragment_size;
    }
    void setFragmentSize(UINT32 val)
    {
        fragment_size = val;
    }
    UINT32 getOffset() const
    {
        return offset;
    }
    void setOffset(UINT32 val)
    {
        offset = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getCompressedSize() const
    {
        return Endian::Flip(compressed_size);
    }
    void setCompressedSize(UINT32 val)
    {
        compressed_size = Endian::Flip(compressed_size);
    }
    UINT32 getUncompressedSize() const
    {
        return Endian::Flip(uncompressed_size);
    }
    void setUncompressedSize(UINT32 val)
    {
        uncompressed_size = Endian::Flip(uncompressed_size);
    }
    UINT32 getFragmentSize() const
    {
        return Endian::Flip(fragment_size);
    }
    void setFragmentSize(UINT32 val)
    {
        fragment_size = Endian::Flip(val);
    }
    UINT32 getOffset() const
    {
        return Endian::Flip(offset);
    }
    void setOffset(UINT32 val)
    {
        offset = Endian::Flip(val);
    }
#endif
};


//
// Packets to download PRT data from server to client
//


class VIS_WANT_PRT_DATA : public fixedPacket
{
public: // construction
    VIS_WANT_PRT_DATA() : fixedPacket(VIS_PACKET_WANT_PRT_DATA) {}
};


class VIS_PRT_DATA_NAK : public fixedPacket
{
public: // construction
    VIS_PRT_DATA_NAK() : fixedPacket(VIS_PACKET_PRT_DATA_NAK) {}
};


class VIS_PRT_DATA : public varPacket
{
protected: // data
    UINT32          compressed_size;        // Total size of image in compressed bytes
    UINT32          uncompressed_size;      // Total size of image in uncompressed bytes
    UINT32          fragment_size;
    UINT32          offset;
public:
    CHAR			data[MAX_PACKET_SIZE - sizeof(varPacket) - (sizeof(UINT32) * 4)];  // data must be the last structure (unless the use of offsetof is changed to work it out)
public: // construction
    VIS_PRT_DATA() : varPacket(VIS_PACKET_PRT_DATA) { setSize(sizeof(VIS_PRT_DATA)); }
    bool myValidate(void)   // Since we can't use virtual functions in network protocol packets, cant use validate()
    {
        if (uncompressed_size > VIS_PRT_DATA_MAX_SIZE)
        {
            return false;
        }
        if (fragment_size > sizeof(data))
        {
            return false;
        }
        if (offset + fragment_size > compressed_size)
        {
            return false;
        }
        return true;
    }
#ifdef __LITTLE_ENDIAN__
    UINT32 getCompressedSize() const
    {
        return compressed_size;
    }
    void setCompressedSize(UINT32 val)
    {
        compressed_size = val;
    }
    UINT32 getUncompressedSize() const
    {
        return uncompressed_size;
    }
    void setUncompressedSize(UINT32 val)
    {
        uncompressed_size = val;
    }
    UINT32 getFragmentSize() const
    {
        return fragment_size;
    }
    void setFragmentSize(UINT32 val)
    {
        fragment_size = val;
    }
    UINT32 getOffset() const
    {
        return offset;
    }
    void setOffset(UINT32 val)
    {
        offset = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getCompressedSize() const
    {
        return Endian::Flip(compressed_size);
    }
    void setCompressedSize(UINT32 val)
    {
        compressed_size = Endian::Flip(compressed_size);
    }
    UINT32 getUncompressedSize() const
    {
        return Endian::Flip(uncompressed_size);
    }
    void setUncompressedSize(UINT32 val)
    {
        uncompressed_size = Endian::Flip(uncompressed_size);
    }
    UINT32 getFragmentSize() const
    {
        return Endian::Flip(fragment_size);
    }
    void setFragmentSize(UINT32 val)
    {
        fragment_size = Endian::Flip(val);
    }
    UINT32 getOffset() const
    {
        return Endian::Flip(offset);
    }
    void setOffset(UINT32 val)
    {
        offset = Endian::Flip(val);
    }
#endif
};


//
// Packets to sync up clients with server's version of BasePortalVis data
//

class VIS_WANT_MIGHTSEE_DATA : public fixedPacket
{
public: // construction
    VIS_WANT_MIGHTSEE_DATA() : fixedPacket(VIS_PACKET_WANT_MIGHTSEE_DATA) {}
};


class VIS_MIGHTSEE_DATA_NAK : public fixedPacket
{
public: // construction
    VIS_MIGHTSEE_DATA_NAK() : fixedPacket(VIS_PACKET_MIGHTSEE_DATA_NAK) {}
};

class VIS_MIGHTSEE_DATA : public varPacket
{	// ATTN: This is a variable-length packet.  Changing this structure requires fixing up the ::Send code, as well as the getPacketSizeByType method)
protected: // data
    INT32           portalindex;
    INT32           nummightsee;
public:
    CHAR            data[4000];
public: // construction
    VIS_MIGHTSEE_DATA() : varPacket(VIS_PACKET_MIGHTSEE_DATA) { setSize(myoffsetof(VIS_MIGHTSEE_DATA, data) + g_bitbytes); }
#ifdef __LITTLE_ENDIAN__
    INT32 getPortalIndex() const
    {
        return portalindex;
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = val;
    }
    INT32 getNumMightSee() const
    {
        return nummightsee;
    }
    void setNumMightSee(INT32 val)
    {
        nummightsee = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    INT32 getPortalIndex() const
    {
        return Endian::Flip(portalindex);
    }
    void setPortalIndex(INT32 val)
    {
        portalindex = Endian::Flip(val);
    }
    INT32 getNumMightSee() const
    {
        return Endian::Flip(nummightsee);
    }
    void setNumMightSee(INT32 val)
    {
        nummightsee = Endian::Flip(val);
    }
#endif
};




//
//
// Packets to negotiate start of real client work
//


class VIS_LEAFTHREAD : public fixedPacket
{
protected: // data
    INT32           portalleafs;
    INT32           numportals;
    INT32           bitbytes;
    INT32           pad2;
public: // construction
    VIS_LEAFTHREAD() : fixedPacket(VIS_PACKET_LEAFTHREAD) {}
#ifdef __LITTLE_ENDIAN__
    INT32 getPortalLeafs() const
    {
        return portalleafs;
    }
    void setPortalLeafs(INT32 val)
    {
        portalleafs = val;
    }
    INT32 getNumPortals() const
    {
        return numportals;
    }
    void setNumPortals(INT32 val)
    {
        numportals = val;
    }
    INT32 getBitBytes() const
    {
        return bitbytes;
    }
    void setBitBytes(INT32 val)
    {
        bitbytes = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    INT32 getPortalLeafs() const
    {
        return Endian::Flip(portalleafs);
    }
    void setPortalLeafs(INT32 val)
    {
        portalleafs = Endian::Flip(val);
    }
    INT32 getNumPortals() const
    {
        return Endian::Flip(numportals);
    }
    void setNumPortals(INT32 val)
    {
        numportals = Endian::Flip(val);
    }
    INT32 getBitBytes() const
    {
        return Endian::Flip(bitbytes);
    }
    void setBitBytes(INT32 val)
    {
        bitbytes = Endian::Flip(val);
    }
#endif
};



class VIS_LEAFTHREAD_ACK : public fixedPacket
{
public: // construction
    VIS_LEAFTHREAD_ACK() : fixedPacket(VIS_PACKET_LEAFTHREAD_ACK) {}
};



class VIS_LEAFTHREAD_NAK : public fixedPacket
{
protected: // data
    INT32           portalleafs;
    INT32           numportals;
    INT32           bitbytes;
    INT32           pad2;
public: // construction
    VIS_LEAFTHREAD_NAK() : fixedPacket(VIS_PACKET_LEAFTHREAD_NAK) {}
#ifdef __LITTLE_ENDIAN__
    UINT32 getPortalLeafs() const
    {
        return portalleafs;
    }
    void setPortalLeafs(INT32 val)
    {
        portalleafs = val;
    }
    UINT32 getNumPortals() const
    {
        return numportals;
    }
    void setNumPortals(INT32 val)
    {
        numportals = val;
    }
    UINT32 getBitBytes() const
    {
        return bitbytes;
    }
    void setBitBytes(INT32 val)
    {
        bitbytes = val;
    }
#endif
#ifdef __BIG_ENDIAN__
    UINT32 getPortalLeafs() const
    {
        return Endian::Flip(portalleafs);
    }
    void setPortalLeafs(INT32 val)
    {
        portalleafs = Endian::Flip(val);
    }
    UINT32 getNumPortals() const
    {
        return Endian::Flip(numportals);
    }
    void setNumPortals(INT32 val)
    {
        numportals = Endian::Flip(val);
    }
    UINT32 getBitBytes() const
    {
        return Endian::Flip(bitbytes);
    }
    void setBitBytes(INT32 val)
    {
        bitbytes = Endian::Flip(val);
    }
#endif
};



#endif /* __cplusplus */


//
// Function Prototypes
//

extern void     Send_VIS_GOING_DOWN(NetvisSession* socket);
extern void     Send_VIS_WANT_FULL_SYNC_ping(void);
extern void     Send_VIS_WANT_FULL_SYNC(void);
extern void     Flag_VIS_DONE_PORTAL(long portalindex);
extern void     Send_VIS_DONE_PORTAL(long portalindex, portal_t* p);
extern void     Send_VIS_LOGIN(void);
extern void     Send_VIS_WANT_BSP_DATA(void);
extern void     Send_VIS_WANT_PRT_DATA(void);
extern void     Send_VIS_WANT_MIGHTSEE_DATA(void);
extern void     Send_VIS_LEAFTHREAD(long portalleafs, long numportals, long bitbytes);
extern void     Send_VIS_SYNC_PORTALs(NetvisSession* socket);

extern int      outOfWork(void);
extern int      stillOutOfWork(void);
extern int      needServerUpdate(void);
extern void     addWorkToClientQueue(long index);
extern long     getWorkFromClientQueue(void);


#endif // PACKET_H__
