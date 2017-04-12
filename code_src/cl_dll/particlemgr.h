// 02/08/02 November235: Particle System
#pragma once

#include "particlesys.h"

class ParticleSystemManager
{
public:
				ParticleSystemManager( void );
				~ParticleSystemManager( void );
	void		AddSystem( ParticleSystem* );
	ParticleSystem *FindSystem( cl_entity_t* pEntity );
	void		UpdateSystems( float frametime, int sky );
	void		ClearSystems( void );
	void		SortSystems( void );
//	void		DeleteSystem( ParticleSystem* );
	void		DeleteSystemWithEntity( int entindex ); // buz

//private:
	ParticleSystem* m_pFirstSystem;
	//ParticleSystem* systemio;
};

//extern ParticleSystemManager* g_pParticleSystems; 
extern ParticleSystemManager	g_pParticleSystems; // buz
