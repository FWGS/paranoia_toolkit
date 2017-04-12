/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/


/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
#include "extdll.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "../hud_iface.h"

extern "C"
{
	void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
}

//extern float g_lastFOV; buz
vec3_t previousorigin;
struct local_state_s *g_finalstate;

void HUD_GetLastOrg( float *org )
{
	int i;
	
	// Return last origin
	for ( i = 0; i < 3; i++ )
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg( void )
{
	int i;
	
	// Offset final origin by view_offset
	for ( i = 0; i < 3; i++ )
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[ i ];
	}
}

void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	to->client.fov = 0;//g_lastFOV; buz

	g_finalstate = to;

	HUD_SetLastOrg();
}
