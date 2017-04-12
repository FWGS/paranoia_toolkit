//
// particlemsg.cpp
//
// implementation of CHudParticle class
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "particlemgr.h"
#include "particlesys.h"

#include <string.h>
#include <stdio.h>

//extern ParticleSystemManager* g_pParticleSystems; // buz

DECLARE_MESSAGE(m_Particle, Particle)
DECLARE_MESSAGE(m_Particle, DelParticle) // buz

int CHudParticle::Init(void)
{
//	ConsolePrint("Hooking Particle message\n"); // 30/08/02 November235: Just a debug
	HOOK_MESSAGE(Particle);
	gHUD.AddHudElem(this);

/*	if (g_pParticleSystems)
	{
		delete g_pParticleSystems;
		g_pParticleSystems = NULL;
	}

	g_pParticleSystems = new ParticleSystemManager();*/ // buz

	return 1;
};

int CHudParticle::VidInit(void)
{
//	g_pParticleSystems->ClearSystems();
	g_pParticleSystems.ClearSystems(); // buz
	return 1;
};

int CHudParticle:: MsgFunc_Particle(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
//	int type = READ_BYTE();
	int entindex = READ_SHORT();
//	float x = READ_COORD();
//	float y = READ_COORD();
//	float z = READ_COORD();
//	float ax = 0;//READ_COORD();
//	float ay = 0;//READ_COORD();
//	float az = 0;//READ_COORD();
	char *sz = READ_STRING();
//	gEngfuncs.Con_Printf("Message received\n");
	//char sz[255];

	ParticleSystem *pSystem = new ParticleSystem(entindex, sz);//"aurora/smoke.aur");

//	g_pParticleSystems->AddSystem(pSystem);
	g_pParticleSystems.AddSystem(pSystem); // buz

	return 1;
}

// buz
int CHudParticle:: MsgFunc_DelParticle(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int entindex = READ_BYTE();
	g_pParticleSystems.DeleteSystemWithEntity( entindex );
	return 1;
}

int CHudParticle::Draw(float flTime)
{
	return 1;
}
