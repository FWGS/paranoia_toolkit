#include "netvis.h"
#include "ZHLTThread.h"

NetvisSession*      g_ClientSession = NULL; // The client socket when g_visstate == VIS_MODE_CLIENT
NetvisSocketServer* g_SocketServer = NULL;  // The server socket server when g_visstate == VIS_MODE_SERVER

class PingThread : public ZHLTThread
{
protected:
    virtual void Run()
    {
        hlassert(g_vismode == VIS_MODE_CLIENT)

        while (_zhlt_isCancelled() == false)
        {
            // This fixes stalls, as well as keeping the clients updated with portals 
            // from the other clients while working on a time consuming portal
            if ((g_visstate == VIS_PORTAL_FLOW) && outOfWork())
            {
                Sleep(1000);
                if (stillOutOfWork() || needServerUpdate())
                {
                    Send_VIS_WANT_FULL_SYNC();
                }
            }
            if (g_visstate >= VIS_DONE)
            {
                return;
            }
            Sleep(1000);
        }
    }
};


class StatusDisplayThread : public ZHLTThread
{
protected:
    virtual void Run()
    {
        while (_zhlt_isCancelled() == false)
        {
            if ((g_vismode == VIS_MODE_CLIENT) && g_ClientSession)
            {
                g_ClientSession->DisplayClientStats();
            }
            else if ((g_vismode == VIS_MODE_SERVER) && g_SocketServer)
            {
                g_SocketServer->DisplayServerStats();
            }

            for (unsigned x = 0; ((x < m_RateSeconds) && (_zhlt_isCancelled() == false)); x++)
            {
                Sleep(1000);
            }
        }
    }

public:
    StatusDisplayThread(unsigned int rateseconds) // in SECONDS
    {
        m_RateSeconds = rateseconds;
    }

    unsigned int    m_RateSeconds;
};


static PingThread*          s_PingThread = NULL;
static StatusDisplayThread* s_StatusDisplayThread = NULL;


bool isConnectedToServer()
{
    bool rval = false;
    hlassert(g_vismode == VIS_MODE_CLIENT);
    if (g_ClientSession != NULL)
    {
        rval = g_ClientSession->m_Active;
    }
    return rval;
}


void ConnectToServer(const char* addr, short port)
{
    InetHostAddress ia = addr;

    try
    {
        g_ClientSession = new NetvisSession(ia, port);
        g_ClientSession->Start();
        StartPingThread();
    }
    catch (Socket* socket)
    {
        Error("Failed to connect to server %s on port %d", addr, port);
    }
}

void DisconnectFromServer()
{
    StopStatusDisplayThread();
    StopPingThread();
    if (g_ClientSession)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "DisconnectFromServer: waiting on socket thread to finish\n"));
        delete g_ClientSession;
        g_ClientSession = NULL;
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "DisconnectFromServer: done waiting on socket thread to finish\n"));
    }
}

void StartNetvisSocketServer(short port)
{
    g_SocketServer = new NetvisSocketServer(port);
    g_SocketServer->Start();
}

void StopNetvisSocketServer()
{
    StopStatusDisplayThread();
    if (g_SocketServer)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopNetvisSocketServer: waiting on socket server thread to finish\n"));
        delete g_SocketServer;
        g_SocketServer = NULL;
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopNetvisSocketServer: done waiting on socket server thread to finish\n"));
    }
}

void StartPingThread()
{
    s_PingThread = new PingThread();
    s_PingThread->Start();
}

void StopPingThread()
{
    if (s_PingThread)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopPingThread: waiting on ping thread to finish\n"));
        delete s_PingThread;
        s_PingThread = NULL;
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopPingThread: done waiting ping thread to finish\n"));
    }
}

void StartStatusDisplayThread(unsigned int rateseconds)
{
    // Rate passed in is in seconds, thread expects milliseconds
    s_StatusDisplayThread = new StatusDisplayThread(rateseconds);
    s_StatusDisplayThread->Start();
}

void StopStatusDisplayThread()
{
    if (s_StatusDisplayThread)
    {
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopStatusDisplay: waiting on status display thread to finish\n"));
        delete s_StatusDisplayThread;
        s_StatusDisplayThread = NULL;
        IfDebug(Developer(DEVELOPER_LEVEL_MESSAGE, "StopStatusDisplay: done waiting on status display thread to finish\n"));
    }
}


void NetvisSleep(unsigned long amt)
{
#ifdef SYSTEM_WIN32
    Sleep(amt);
#endif
#ifdef SYSTEM_POSIX
    usleep(amt * 1000);
#endif
}
