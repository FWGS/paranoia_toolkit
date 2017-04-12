#ifndef C2CPP_H__
#define C2CPP_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "win32fix.h"

// This tracks the current step of execution
enum vis_states
{
    VIS_STARTUP,
    VIS_BASE_PORTAL_VIS,
    VIS_BASE_PORTAL_VIS_SERVER_WAIT,    // waiting on server to finish its baseportalvis()
    VIS_PORTAL_FLOW,
    VIS_WAIT_CLIENTS,
    VIS_POST,
    VIS_DONE,
    VIS_CLIENT_DONE,
    VIS_NO_SERVER
};

// These are the different types of execution
enum vis_modes
{
    VIS_MODE_NULL,
    VIS_MODE_SERVER,
    VIS_MODE_CLIENT
};


// SocketThreads functions (callable by hlvis code)

extern bool isConnectedToServer();

extern void ConnectToServer(const char* addr, short port);
extern void DisconnectFromServer();

extern void StartNetvisSocketServer(short port);
extern void StopNetvisSocketServer();

extern void StartPingThread();
extern void StopPingThread();

extern void StartStatusDisplayThread(unsigned int rateseconds);
extern void StopStatusDisplayThread();

extern void NetvisSleep(unsigned long amt);

// packet.cpp globals
extern bool                     g_NetvisAbort;

// vis.c globals
extern unsigned long            g_clientid;
extern volatile int             g_visportalindex;                      // a client's portal index
extern volatile int             g_visportals;                          // the total portals in the map
extern volatile int             g_visleafs;                            // the total portal leafs in the map
extern volatile int             g_vislocalportal;                      // number of portals solved locally
extern volatile enum vis_states g_visstate;                // current step of execution
extern volatile enum vis_modes  g_vismode;                  // style of execution (client or server)
extern volatile int             g_visleafthread;                       // control flag (are we ready to leafthread)
extern volatile unsigned long   g_serverindex;               // client only variable, server index for calculating percentage indicators on the client
extern volatile unsigned long   g_idletime;                  // Accumulated idle time in milliseconds
extern volatile double          g_starttime;                 // Start time (from I_FloatTime())

extern volatile bool            g_bsp_downloaded;
extern volatile bool            g_prt_downloaded;
extern volatile bool            g_mightsee_downloaded;
extern char*                    g_bsp_image;
extern char*                    g_prt_image;
extern unsigned long            g_bsp_compressed_size;
extern unsigned long            g_prt_compressed_size;
extern unsigned long            g_bsp_size;
extern unsigned long            g_prt_size;

#endif//C2CPP_H__
