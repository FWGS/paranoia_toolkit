//
// written by BUzer for HL: Paranoia modification
//
//		2006

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "glmanager.h"
#include "gl_texloader.h"
#include "particlemgr.h"

#include "studio_util.h"
#include "r_studioint.h"
#include "ref_params.h"

#include "pm_defs.h"
#include "pmtrace.h"
#include "parsemsg.h"

#define MAX_SKY_ENTS	256

int skyloaded = 0;
int sky_base_texturenum = 0;
char current_skyname[64];


extern vec3_t	render_origin;
extern vec3_t	render_angles;

cvar_t *cv_customsky;
//cvar_t *cv_skybrightness;

extern msurface_t *skychain;
extern int no_sky_visible;

vec3_t sky_origin;
vec3_t sky_world_origin;
float  sky_speed;

int skylight_amb;
int skylight_shade;

vec3_t skyangles = Vector(0, -90, 90);


int numskyents = 0;
cl_entity_t *skyents[MAX_SKY_ENTS];
void R_DrawSprite ( cl_entity_t *e );



void StoreSkyboxEntity(cl_entity_t *ent)
{
	if (numskyents >= MAX_SKY_ENTS)
	{
		CONPRINT("too many skybox entites!!!\n");
		ONCE( Log("too many skybox entites!!!\n"); );
		return;
	}

	skyents[numskyents] = ent;
	numskyents++;
}

void SetSkyAngles()
{
	if ( gEngfuncs.Cmd_Argc() <= 3 )
	{
		gEngfuncs.Con_Printf( "usage: skyangles <pitch> <yaw> <roll>\n" );
		gEngfuncs.Con_Printf( "( default skyangles is 0 -90 90 )\n" );
		return;
	}

	skyangles[0] = atof(gEngfuncs.Cmd_Argv(1));
	skyangles[1] = atof(gEngfuncs.Cmd_Argv(2));
	skyangles[2] = atof(gEngfuncs.Cmd_Argv(3));
}

#define	SURF_DRAWSKY		4


#define SKYFLAG_USE_AMBLIGHT_HACK		1
#define SKYFLAG_USE_SHADELIGHT_HACK		2

// 3d skybox
int MsgSkyMarker_Sky(const char *pszName, int iSize, void *pbuf)
{
	Log("Got 3d-sky sky marker\n");
	BEGIN_READ( pbuf, iSize );

	sky_origin.x = READ_COORD();
	sky_origin.y = READ_COORD();
	sky_origin.z = READ_COORD();
	int flags = READ_BYTE();
	if (flags & SKYFLAG_USE_AMBLIGHT_HACK)
	{
		Log("  sky uses ambient light hack\n");
		skylight_amb = READ_BYTE();
	}
	if (flags & SKYFLAG_USE_SHADELIGHT_HACK)
	{
		Log("  sky uses direct light hack\n");
		skylight_shade = READ_BYTE();
	}
	return 1;
}

// 3d skybox
int MsgSkyMarker_World(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	sky_world_origin.x = READ_COORD();
	sky_world_origin.y = READ_COORD();
	sky_world_origin.z = READ_COORD();
	sky_speed = READ_COORD();

	Log("Got 3d-sky world marker (speed %f)\n", sky_speed);
	return 1;
}

void ResetSky()
{
	VectorClear(sky_origin);
	VectorClear(sky_world_origin);
	sky_speed = 0;
	skylight_amb = -1;
	skylight_shade = -1;
}

void InitSky()
{
	current_skyname[0] = 0;
	gEngfuncs.pfnAddCommand ("skyangles", SetSkyAngles);
//	gEngfuncs.pfnAddCommand ("killsky", KillSky);
	cv_customsky = gEngfuncs.pfnRegisterVariable( "gl_customsky","1", 0 );
	gEngfuncs.pfnHookUserMsg("skymark_sky", MsgSkyMarker_Sky);
	gEngfuncs.pfnHookUserMsg("skymark_w", MsgSkyMarker_World);
//	cv_skybrightness = gEngfuncs.pfnRegisterVariable( "gl_skybright","0", 0 );
}






void CheckSky()
{
	if (!sky_base_texturenum)
	{
		sky_base_texturenum = current_ext_texture_id;
		current_ext_texture_id += 6;
		Log("sky base texturenum set\n");
	}

	const char *skyname = CVAR_GET_STRING("sv_skyname");
	if (!skyname)
	{
		ConLog("Sky error: cant get sv_skyname value\n");
		ConLog("Use gl_customsky 1 to reenable sky\n");
		gEngfuncs.Cvar_SetValue( "gl_customsky", 0 );
		return;
	}

	if (current_skyname[0] != 0 && !strcmp(current_skyname, skyname))
		return;

	skyloaded = 0;

	static char* skysuffixes[] = {"lf", "bk", "rt", "ft", "up", "dn"};

	for (int i = 0; i < 6; i++)
	{
		char path[256];
		sprintf(path, "gfx/env/%s%s.tga", skyname, skysuffixes[i]);
		int ret = CreateTexture(path, MIPS_NO, sky_base_texturenum + i);
		if (!ret)
		{
			ConLog("Sky error: cant load sky face %s\n", path);
			ConLog("Use gl_customsky 1 to reenable sky\n");
			gEngfuncs.Cvar_SetValue( "gl_customsky", 0 );
			return;
		}
		gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}

	strcpy(current_skyname, skyname);
	skyloaded = 1;
	ConLog("Loaded skybox %s\n", skyname);
}


void DrawSky()
{
	int i;

	if (!cv_customsky->value)
		return;

	CheckSky();
	if (!skyloaded)
		return;

	if (no_sky_visible)
		return;

//	if (!skychain)
//		return; // no sky visible

	// calc sky ents offset
	vec3_t trans = render_origin - sky_origin;
	if (sky_speed)
		trans = trans - (render_origin - sky_world_origin) / sky_speed;

	gl.glDepthRange (0.9, 1);
	gl.glDisable(GL_BLEND);
	gl.glDepthMask(GL_FALSE);

	// disable texturing on all units except first
	gl.glActiveTextureARB( GL_TEXTURE0_ARB );
	gl.glEnable(GL_TEXTURE_2D);
	gl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	vec3_t fwd, up, rt;
	AngleVectors(skyangles, fwd, up, rt);
//	float dist = CVAR_GET_FLOAT("sv_zmax") * 0.5;
	float dist = 10; // chislo s potolka
	fwd = fwd * dist;
	up = up * -dist;
	rt = rt * dist;

	vec3_t points[8];
	points[0] = render_origin + fwd - rt + up;
	points[1] = render_origin + fwd + rt + up;
	points[2] = render_origin - fwd + rt + up;
	points[3] = render_origin - fwd - rt + up;
	points[4] = render_origin + fwd - rt - up;
	points[5] = render_origin + fwd + rt - up;
	points[6] = render_origin - fwd + rt - up;
	points[7] = render_origin - fwd - rt - up;

	int idx[6][4] = {
		{1, 2, 6, 5},
		{2, 3, 7, 6},
		{3, 0, 4, 7},
		{0, 1, 5, 4},
		{2, 1, 0, 3},
		{7, 4, 5, 6},
	};

	for (i = 0; i < 6; i++)
	{
		gl.glBindTexture(GL_TEXTURE_2D, sky_base_texturenum + i);
		gl.glBegin(GL_POLYGON);

			gl.glTexCoord2f (0.003, 0.003);
			gl.glVertex3fv (points[idx[i][0]]);
			gl.glTexCoord2f (0.997, 0.003);
			gl.glVertex3fv (points[idx[i][1]]);
			gl.glTexCoord2f (0.997, 0.997);
			gl.glVertex3fv (points[idx[i][2]]);
			gl.glTexCoord2f (0.003, 0.997);
			gl.glVertex3fv (points[idx[i][3]]);

		gl.glEnd ();
	}

	if (numskyents)
	{
		gl.glDepthRange (0.8, 0.9);
		gl.glTranslatef(trans[0], trans[1], trans[2]);
	
		for (i = 0; i < numskyents; i++)
		{
			R_DrawSprite ( skyents[i] );
		}

	//	CONPRINT("dsdasda %d\n", numskyents);
		gl.glTranslatef(-trans[0], -trans[1], -trans[2]);
	}

	gl.glDepthRange (0, 0.8);
}
