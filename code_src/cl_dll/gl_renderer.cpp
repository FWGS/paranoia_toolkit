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

#include "studio_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "parsemsg.h"

#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "gl_renderer.h"
#include "gl_dlight.h" // buz


extern vec3_t	render_origin;
extern vec3_t	render_angles;


float	savedMinmaxs[6];
int		renderer_prepared_for_this_frame = 0;
vec3_t	saved_view_orig;
int		newframe = 0;

int		reloaded = 1;
int		needkillsky = 0;
int		curvisframe;
int		framecount = 0;
int		no_sky_visible = 0;

extern msurface_t *skychain;

cvar_t *cv_renderer;
cvar_t *cv_bump;
cvar_t *cv_bumpvecs;
cvar_t *cv_detailtex;
cvar_t *cv_bumpdebug;
cvar_t *cv_specular;
cvar_t *cv_dynamiclight;
cvar_t *cv_entdynamiclight;
cvar_t *cv_specular_nocombiners;
cvar_t *cv_specular_noshaders;
cvar_t *cv_highspecular;
cvar_t *cv_speculartwopass;
cvar_t *cv_dyntwopass;
cvar_t *cv_gamma;
cvar_t *cv_brightness;
cvar_t *cv_contrast;
cvar_t *cv_blurtest;
cvar_t *cv_nsr; // new studio renderer :)
cvar_t *cv_lambert;
cvar_t *cv_drawbrushents;
cvar_t *cv_clipplanes;
cvar_t *cv_showplane;

int		norm_cubemap_id = 0;
int		default_normalmap_id = 0;

#define MAX_CLRENDER_ENTS	512

cl_entity_t *renderents[MAX_CLRENDER_ENTS];
int numrenderents = 0;
cl_entity_t *currententity;
//int maxmsgnum = -1;
//int addedents = 0;

/*
#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts;
extern	entity_t		*cl_visedicts[MAX_VISEDICTS];
*/

FrustumCheck viewFrustum;


// surface rendering
void PrepareFirstPass();
void DrawPolyFirstPass(msurface_t *s);
void RenderSecondPass();
void RenderAdditionalBumpPass();
void RenderSpecular();
void ResetCounters();

// lightmaps loading
void BumpMarkFaces();
void UpdateLightmaps();
void SetLightmapBaseIndex();

// decals
void LoadDecals();
void DrawDecals();
void InitDecals();
void DeleteDecals();
void DecalsShutdown();
void DecalsPrintDebugInfo();

// sky
void DrawSky();
void InitSky();
void ResetSky();
void StoreSkyboxEntity(cl_entity_t *ent);

// dymanic lighting
void DrawDynamicLights();
void DrawDynamicLightForEntity (cl_entity_t *e);
void ResetDynamicLights();
void MY_DecayLights ();
void CreateAttenuationTextures();

// debug
void DrawTextureVecs();
void DumpTextures();
void DumpLevel();
void DumpLeafInfo();
void PrintOtherDebugInfo();
void PrintBumpDebugInfo();
void DrawDebugFrustumPlane();

// vertex arrays
void GenerateVertexArray();
void FreeBuffer();
void EnableVertexArray();
void DisableVertexArray();

extern int numskyents;


// When rendering world, we must prevent engine from drawing world model,
// but allow him to draw models, sprites, beams, and other entities.
//
// Idea is to put node[0] bbox behind player's back, so engine will think that is invisible.
// After rendering, node[0] bbox must be restored by calling RestoreWorldDrawing
//
// Good thing that engine will precalculate PVS visibility info for us

void DisableWorldDrawing( ref_params_t *pparams )
{
	vec3_t wcoord;
	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;
	AngleVectors ( pparams->viewangles, pparams->forward, pparams->right, pparams->up );
	VectorMA(pparams->vieworg, -100, pparams->forward, wcoord);
	memcpy(savedMinmaxs, world->nodes[0].minmaxs, 6 * sizeof(float));
	VectorCopy(wcoord, world->nodes[0].minmaxs);
	VectorCopy(wcoord, world->nodes[0].minmaxs + 3);
}

void RestoreWorldDrawing()
{
	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;
	memcpy(world->nodes[0].minmaxs, savedMinmaxs, 6 * sizeof(float));
}


//===================================
// CreateNormalizationCubemap
//
//		thanks steps3d.narod.ru
//===================================
#define NORM_CUBE_SIZE	(32)

void getCubeVector ( int side, int x, int y, vec3_t &v )
{
    float s  = ((float) x + 0.5f) / (float) NORM_CUBE_SIZE;
    float t  = ((float) y + 0.5f) / (float) NORM_CUBE_SIZE;
    float sc = 2*s - 1;
    float tc = 2*t - 1;

    switch ( side )
    {
        case 0:
            v[0] = 1; v[1] = -tc; v[2] = -sc;
            break;

        case 1:
            v[0] = -1; v[1] = -tc; v[2] = sc;
            break;

        case 2:
            v[0] = sc; v[1] = 1; v[2] = tc;
            break;

        case 3:
            v[0] = sc; v[1] = -1; v[2] = -tc;
            break;

        case 4:
            v[0] = sc; v[1] = -tc; v[2] = 1;
            break;

        case 5:
            v[0] = -sc; v[1] = -tc; v[2] = -1;
            break;
    }
    VectorNormalize(v);
}

int CreateNormalizationCubemap()
{
	vec3_t v;
	byte pixels[NORM_CUBE_SIZE * NORM_CUBE_SIZE * 3];

    gl.glEnable        ( GL_TEXTURE_CUBE_MAP_ARB );
    gl.glBindTexture   ( GL_TEXTURE_CUBE_MAP_ARB, current_ext_texture_id );
    gl.glPixelStorei   ( GL_UNPACK_ALIGNMENT, 1 );
    gl.glTexParameteri ( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    gl.glTexParameteri ( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    for ( int side = 0; side < 6; side++ )
    {
        for ( int x = 0; x < NORM_CUBE_SIZE; x++ )
            for ( int y = 0; y < NORM_CUBE_SIZE; y++ )
            {
                int offs = 3 * (y * NORM_CUBE_SIZE + x);

                getCubeVector ( side, x, y, v );

                pixels [offs    ] = 128 + 127 * v[0];
                pixels [offs + 1] = 128 + 127 * v[1];
                pixels [offs + 2] = 128 + 127 * v[2];
			//	gEngfuncs.Con_Printf("%d, %d, %d\n", pixels[0], pixels[1], pixels[2]);
            }

         gl.glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + side, 0, GL_RGB,
					NORM_CUBE_SIZE, NORM_CUBE_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    }

	Log("Created normalization cubemap\n");
    gl.glTexParameteri ( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    gl.glTexParameteri ( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    gl.glDisable ( GL_TEXTURE_CUBE_MAP_ARB );

    current_ext_texture_id++;
	return (current_ext_texture_id - 1);
}

//===================================
// CreateDefaultNormalMap
//
// generates default normalmap texture
//===================================
int CreateDefaultNormalMap ()
{
	color24 buf[16*16];

	for (int i = 0; i < 256; i++)
	{
		buf[i].r = 127;
		buf[i].g = 127;
		buf[i].b = 255;
	}

	// upload image
	gl.glBindTexture(GL_TEXTURE_2D, current_ext_texture_id);
	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.glTexImage2D (GL_TEXTURE_2D, 0, 3, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);

	Log("Created default normal map\n");
	current_ext_texture_id++;
	return (current_ext_texture_id - 1);
}


void ResetRenderState()
{
	for (int i = 0; i < gl.MAX_TU_supported; i++)
	{
		gl.glActiveTextureARB( GL_TEXTURE0_ARB + i );
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
		gl.glDisable(GL_TEXTURE_2D);
	}

	gl.glActiveTextureARB( GL_TEXTURE0_ARB );
	gl.glEnable(GL_TEXTURE_2D);

	gl.glDisable(GL_BLEND);
	gl.glDepthMask(GL_TRUE);
}




/*
=================
R_DrawBrushModel
=================
*/
vec3_t vec_to_eyes;

void R_RotateForEntity (cl_entity_t *e)
{
    gl.glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    gl.glRotatef (e->angles[1],  0, 0, 1);
    gl.glRotatef (-e->angles[0],  0, 1, 0);
    gl.glRotatef (e->angles[2],  1, 0, 0);
}

int IsEntityMoved(cl_entity_t *e)
{
	if (e->angles[0] || e->angles[1] || e->angles[2] ||
		e->origin[0] || e->origin[1] || e->origin[2] ||
		e->curstate.renderfx == 70) // skybox models reques separate pass
		return TRUE;
	else
		return FALSE;
}

int IsEntityTransparent(cl_entity_t *e)
{
	if (e->curstate.rendermode == kRenderNormal ||
		e->curstate.rendermode == kRenderTransAlpha)
		return FALSE;
	else
		return TRUE;
}


void R_DrawBrushModel (cl_entity_t *e, int onlyfirstpass)
{
//	int			j, k;
	vec3_t		mins, maxs;
	int			i;//, numsurfaces;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	int			rotated;
	vec3_t		trans;

	currententity = e;
	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	if (e->curstate.renderfx == 70) // skybox entity
	{
		if (!gl.Paranoia_hacks)
			return;

		trans = render_origin - sky_origin;
		if (sky_speed)
		{
			trans = trans - (render_origin - sky_world_origin) / sky_speed;
			vec3_t skypos = sky_origin + (render_origin - sky_world_origin) / sky_speed;
			VectorSubtract (skypos, e->origin, vec_to_eyes);
		}
		else
			VectorSubtract (sky_origin, e->origin, vec_to_eyes);

		gl.glTranslatef(trans[0], trans[1], trans[2]);
		gl.glDepthRange (0.8, 0.9);		
	}
	else
	{
		if (viewFrustum.R_CullBox (mins, maxs))
			return;

		VectorSubtract (saved_view_orig, e->origin, vec_to_eyes);
	}

	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (vec_to_eyes, temp);
		AngleVectors (e->angles, forward, right, up);
		vec_to_eyes[0] = DotProduct (temp, forward);
		vec_to_eyes[1] = -DotProduct (temp, right);
		vec_to_eyes[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

    gl.glPushMatrix ();
e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
e->angles[0] = -e->angles[0];	// stupid quake bug

	//
	// draw polys
	//

	if (!onlyfirstpass)
		PrepareFirstPass();	

	if (e->curstate.rendermode == kRenderTransAlpha)
	{
		gl.glEnable(GL_ALPHA_TEST);
		gl.glAlphaFunc(GL_GREATER, 0.25);
	}
	else if (e->curstate.rendermode == kRenderTransTexture)
	{
		gl.glDepthMask(GL_FALSE);
		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl.glColor4ub(255, 255, 255, e->curstate.renderamt);
	}
	else if (e->curstate.rendermode == kRenderTransAdd)
	{
		gl.glDepthMask(GL_FALSE);
		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_ONE, GL_ONE);
		gl.glColor4ub(255, 255, 255, e->curstate.renderamt);
	}	

	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
		pplane = psurf->plane;
		dot = DotProduct (vec_to_eyes, pplane->normal) - pplane->dist;

		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			psurf->visframe = framecount;
			DrawPolyFirstPass(psurf);
		}
	}

	if (e->curstate.rendermode == kRenderTransAlpha)
	{
		gl.glDisable(GL_ALPHA_TEST);
	}
	else if (e->curstate.rendermode == kRenderTransTexture || e->curstate.rendermode == kRenderTransAdd)
	{
		gl.glDepthMask(GL_TRUE);
		gl.glDisable(GL_BLEND);
	}

	if (!onlyfirstpass)
	{
		RenderAdditionalBumpPass();
		if (e->curstate.renderfx != 70 && cv_dynamiclight->value && cv_entdynamiclight->value)
			DrawDynamicLightForEntity(e);
		RenderSecondPass();
		RenderSpecular();
	}

	gl.glPopMatrix ();

	if (e->curstate.renderfx == 70)
	{
		gl.glTranslatef(-trans[0], -trans[1], -trans[2]);
		gl.glDepthRange (0, 0.8);
	}
}


/*
=================
 World drawing
=================
*/
void RecursiveDrawWorld (mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != curvisframe)
		return;

	if (viewFrustum.R_CullBox (node->minmaxs, node->minmaxs+3))
		return;
	
// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mleaf_t *)node;
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = framecount;
				mark++;
			} while (--c);
		}
		return;
	}

// node is just a decision point, so go down the apropriate sides
// find which side of the node we are on
	plane = node->plane;
	switch (plane->type)
	{
	case PLANE_X:
		dot = saved_view_orig[0] - plane->dist;	break;
	case PLANE_Y:
		dot = saved_view_orig[1] - plane->dist;	break;
	case PLANE_Z:
		dot = saved_view_orig[2] - plane->dist;	break;
	default:
		dot = DotProduct (saved_view_orig, plane->normal) - plane->dist; break;
	}

	if (dot >= 0) side = 0;
	else side = 1;

// recurse down the children, front side first
	RecursiveDrawWorld (node->children[side]);

// draw stuff
	c = node->numsurfaces;
	if (c)
	{
		model_t *world = gEngfuncs.GetEntityByIndex(0)->model;
		surf = world->surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			for ( ; c ; c--, surf++)
			{
				if (surf->visframe != framecount)
					continue;

				// don't backface underwater surfaces, because they warp
				if ( !(surf->flags & SURF_UNDERWATER) && ( (dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)) )
					continue;		// wrong side

				DrawPolyFirstPass(surf);
			}
		}
	}

// recurse down the back side
	RecursiveDrawWorld (node->children[!side]);
}

//=========================
// PrepareRenderer
//
// sets frustum, disables world drawing, makes shure that everything is okay
//=========================
void PrepareRenderer( ref_params_t *pparams )
{
	ResetCounters();

	if (!cv_renderer->value)
		return;

	if (!gl.IsGLAllowed())
	{
		gEngfuncs.Cvar_SetValue( "gl_renderer", 0 );
		return;
	}

	if (!gl.Paranoia_hacks)
	{
		ConLog("No paranoia hacked opengl32.dll, custom renderer disabled\n");
		gEngfuncs.Cvar_SetValue( "gl_renderer", 0 );
		return;
	}

	if (!gEngfuncs.GetEntityByIndex(0)->model->lightdata)
	{
		ConLog("No lightdata, custom renderer disabled\n");
		gEngfuncs.Cvar_SetValue( "gl_renderer", 0 );
		return;
	}

	if (!gl.ARB_multitexture_supported || !gl.ARB_dot3_supported ||
		!gl.ARB_texture_cube_map_supported || gl.MAX_TU_supported < 2)
	{
		ConLog("Paranoia custom renderer is not supported by your videocard\n");
		gEngfuncs.Cvar_SetValue( "gl_renderer", 0 );
		return;
	}

	if (pparams->onlyClientDraw)
		return;

	if (!gl.stencilbits)
	{
		ConLog("Warning: no stencilbits!\n");
		gEngfuncs.Cvar_SetValue( "gl_clipplanes", 1 ); // it's impossible, but who knows?
	}	

	if (reloaded)
	{
		CreateExtDataForTextures();
		BumpMarkFaces();
		GenerateVertexArray();
		reloaded = 0;
	}

	UpdateLightmaps();

	if (!norm_cubemap_id)
		norm_cubemap_id = CreateNormalizationCubemap();

	if (!default_normalmap_id)
		default_normalmap_id = CreateDefaultNormalMap();

	CreateAttenuationTextures();
		
	VectorCopy (pparams->vieworg, saved_view_orig);
	viewFrustum.R_SetFrustum (pparams->viewangles, pparams->vieworg, pparams->viewsize);

	// i'd like to draw world here, but engine will erase
	// depth buffer after CalcRefDef returns :(

	DisableWorldDrawing( pparams );
	renderer_prepared_for_this_frame = 1;
}

//=========================
// DrawBump
//
//=========================
void DrawBump()
{
	if (!renderer_prepared_for_this_frame)
		return;

	RestoreWorldDrawing();
	renderer_prepared_for_this_frame = 0;

	// Current visframe number can be taken from leaf, where our eyes in.
	// obvious, eh?
	model_t *world = gEngfuncs.GetEntityByIndex(0)->model;
	mleaf_t *leaf = Mod_PointInLeaf ( saved_view_orig, world );
	curvisframe = leaf->visframe;
	
	EnableVertexArray();

	// Draw world
	VectorCopy(saved_view_orig, vec_to_eyes);
	currententity = gEngfuncs.GetEntityByIndex(0);

	PrepareFirstPass();
	RecursiveDrawWorld (world->nodes);

//	CONPRINT("static start\n");

	// Draw non-moved brush entities
	for (int k = 0; k < numrenderents; k++)
	{
		if (!IsEntityMoved(renderents[k])/* && !IsEntityTransparent(renderents[k])*/)
			R_DrawBrushModel (renderents[k], TRUE);
	}

	RenderAdditionalBumpPass();

	if (cv_dynamiclight->value)
		DrawDynamicLights();

	RenderSecondPass();
	RenderSpecular();

//	CONPRINT("static end\n");

	if (!skychain)
		no_sky_visible = TRUE;

	// Draw brush entities
	for (k = 0; k < numrenderents; k++)
	{
		if (IsEntityMoved(renderents[k])/* && !IsEntityTransparent(renderents[k])*/)
			R_DrawBrushModel (renderents[k], FALSE);
	}

	gl.glDepthFunc(GL_LEQUAL);

	DisableVertexArray();
	DrawDecals();

/*	// Draw transparent brush entities
	EnableVertexArray();
	for (k = 0; k < numrenderents; k++)
	{
		if (IsEntityTransparent(renderents[k]))
			R_DrawBrushModel (renderents[k], FALSE);
	}

	DisableVertexArray();*/
	ResetRenderState();

	// clear texture chains
	texture_t** tex = (texture_t**)world->textures;
	for (int i = 0; i < world->numtextures; i++)
		tex[i]->texturechain = NULL;

	// reset surfaces visframe number to default
/*	msurface_t* surfaces = world->surfaces;
	for (i = 0; i < world->numsurfaces; i++)
		surfaces[i].visframe = 1;

	// on brush entities too
	// (uhh.. isn't it just a pointers to surfs in world->surfaces array?)
	for (k = 0; k < numrenderents; k++)
	{
		model_t *clmodel = renderents[k]->model;
		msurface_t *psurf = &clmodel->surfaces[clmodel->firstmodelsurface];
		for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
			psurf->visframe = 1;
	}*/

//	if (numrenderents == 0)
//		CONPRINT("%d brush ents drawn\n", numrenderents);
}


int MsgCustomDlight(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	vec3_t pos;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	float radius = (float)READ_BYTE() * 10;
	float life = (float)READ_BYTE() / 10;
	float decay = (float)READ_BYTE() * 10;

	DynamicLight *dl = MY_AllocDlight (0);
	dl->origin.x = pos.x;
	dl->origin.y = pos.y;
	dl->origin.z = pos.z;
	dl->radius = radius;
	dl->die = gEngfuncs.GetClientTime() + life;
	dl->decay = decay;
	dl->color = Vector(0.7f, 0.6f, 0.5f);

	return 1;
}


// prevent engine from drawing sky in gl mode
void KillSky()
{
	Log("Killing sky faces\n");
	model_t *world = gEngfuncs.GetEntityByIndex(0)->model;
	msurface_t* surfaces = world->surfaces;
	for (int i = 0; i < world->numsurfaces; i++)
	{
		if (surfaces[i].flags & SURF_DRAWSKY)
		{
			glpoly_t *p = surfaces[i].polys;
			p->numverts = -p->numverts;
		}
	}
}


// debug command
// usage: makelight [R G B] [radius] [life]\n" );
void MakeLight()
{
	DynamicLight *dl = MY_AllocDlight (0);
	dl->origin.x = render_origin[0];
	dl->origin.y = render_origin[1];
	dl->origin.z = render_origin[2];
	
	if (gEngfuncs.Cmd_Argc() >= 4)
	{
		dl->color[0] = atof(gEngfuncs.Cmd_Argv(1));
		dl->color[1] = atof(gEngfuncs.Cmd_Argv(2));
		dl->color[2] = atof(gEngfuncs.Cmd_Argv(3));
	}
	else
	{
		dl->color = Vector(0.7f, 0.6f, 0.5f);
	}

	if (gEngfuncs.Cmd_Argc() >= 5)
		dl->radius = atof(gEngfuncs.Cmd_Argv(4));
	else
		dl->radius = 128;

	if (gEngfuncs.Cmd_Argc() >= 6)
		dl->die = gEngfuncs.GetClientTime() + atof(gEngfuncs.Cmd_Argv(5));
	else
		dl->die = gEngfuncs.GetClientTime() + 2;
}


// ======================
// GL_MtexStateSaver class
//
// manages storing and restoring some opengl parameters
// so our renderer will not affect engine's drawing
// ======================
class GL_MtexStateSaver
{
public:
	GL_MtexStateSaver()
	{
		if (gl.IsGLAllowed() && gl.ARB_multitexture_supported)
		{
			gl.glPushAttrib(GL_ALL_ATTRIB_BITS);

			do_this = TRUE;
			gl.glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTU);
			gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE_ARB, &clactiveTU);
		//	CONPRINT("active: %d\n", activeTU);

			if (gl.glIsEnabled(GL_BLEND)) blendactive = TRUE;
			else blendactive = FALSE;

			gl.glActiveTextureARB( GL_TEXTURE0_ARB );
			if (gl.glIsEnabled(GL_TEXTURE_2D)) TU1tex2dEnable = TRUE;
			else TU1tex2dEnable = FALSE;
			gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &TU1bind);
			gl.glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &TU1BlendMode);

			gl.glActiveTextureARB( GL_TEXTURE1_ARB );
			if (gl.glIsEnabled(GL_TEXTURE_2D)) TU2tex2dEnable = TRUE;
			else TU2tex2dEnable = FALSE;
			gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &TU2bind);
			gl.glDisable(GL_TEXTURE_2D); // disable texturing at 2nd TU

			if (gl.MAX_TU_supported > 2)
			{
				gl.glActiveTextureARB( GL_TEXTURE2_ARB );
				if (gl.glIsEnabled(GL_TEXTURE_2D)) TU3tex2dEnable = TRUE;
				else TU3tex2dEnable = FALSE;
				gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &TU3bind);
				gl.glDisable(GL_TEXTURE_2D); // disable texturing at 3rd TU
			//	CONPRINT("tex3 enabled: %d, bind %d\n", TU3tex2dEnable, TU3bind);

				// steam version sets texture matrix for detail textures, we must save it
				gl.glMatrixMode(GL_TEXTURE);
				gl.glPushMatrix();
				gl.glLoadIdentity();
				gl.glMatrixMode(GL_MODELVIEW);

				if (gl.MAX_TU_supported > 3)
				{
					gl.glActiveTextureARB( GL_TEXTURE3_ARB );
					if (gl.glIsEnabled(GL_TEXTURE_2D)) TU4tex2dEnable = TRUE;
					else TU4tex2dEnable = FALSE;
					gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &TU4bind);
					gl.glDisable(GL_TEXTURE_2D); // disable texturing at 4th TU
				//	CONPRINT("tex4 enabled: %d, bind %d\n", TU4tex2dEnable, TU4bind);
				}
			}

			gl.glActiveTextureARB( GL_TEXTURE0_ARB );

			gl.glGetIntegerv(GL_ALPHA_TEST_FUNC, &alphafunc);
			gl.glGetFloatv(GL_ALPHA_TEST_REF, &alphaval);
			if (gl.glIsEnabled(GL_ALPHA_TEST)) alphatestenabled = TRUE;
			else alphatestenabled = FALSE;
		}
		else
			do_this = FALSE;		
	}

	~GL_MtexStateSaver()
	{
		if (do_this)
		{
			gl.glPopAttrib();

			gl.glActiveTextureARB( GL_TEXTURE0_ARB );
			if (TU1tex2dEnable) gl.glEnable(GL_TEXTURE_2D);
			else gl.glDisable(GL_TEXTURE_2D);
			gl.glBindTexture(GL_TEXTURE_2D, TU1bind);
			gl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, TU1BlendMode);

			gl.glActiveTextureARB( GL_TEXTURE1_ARB );
			if (TU2tex2dEnable) gl.glEnable(GL_TEXTURE_2D);
			else gl.glDisable(GL_TEXTURE_2D);
			gl.glBindTexture(GL_TEXTURE_2D, TU2bind);

			if (gl.MAX_TU_supported > 2)
			{
				gl.glActiveTextureARB( GL_TEXTURE2_ARB );
				if (TU3tex2dEnable) gl.glEnable(GL_TEXTURE_2D);
				else gl.glDisable(GL_TEXTURE_2D);
				gl.glBindTexture(GL_TEXTURE_2D, TU3bind);

				// this must be set for steam version, so it will render detail textures correctly
				if (gl.ARB_dot3_supported)
				{
					gl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
					gl.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
					gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
					gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
					gl.glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
				}

				// load saved matrix for steam version
				gl.glMatrixMode(GL_TEXTURE);
				gl.glPopMatrix();
				gl.glMatrixMode(GL_MODELVIEW);

				if (gl.MAX_TU_supported > 3)
				{
					gl.glActiveTextureARB( GL_TEXTURE3_ARB );
					if (TU4tex2dEnable) gl.glEnable(GL_TEXTURE_2D);
					else gl.glDisable(GL_TEXTURE_2D);
					gl.glBindTexture(GL_TEXTURE_2D, TU4bind);
				}
			}

			gl.glActiveTextureARB( activeTU );
			gl.glClientActiveTextureARB( clactiveTU );

			if (blendactive) gl.glEnable(GL_BLEND);
			else gl.glDisable(GL_BLEND);

			if (alphatestenabled) gl.glEnable(GL_ALPHA_TEST);
			else gl.glDisable(GL_ALPHA_TEST);

			gl.glAlphaFunc(alphafunc, alphaval);
		}
	}

private:
	int do_this;
	GLint blendactive;
	GLint TU1bind;
	GLint TU2bind;
	GLint TU3bind;
	GLint TU4bind;
	GLint TU1tex2dEnable;
	GLint TU2tex2dEnable;
	GLint TU3tex2dEnable;
	GLint TU4tex2dEnable;
	GLint activeTU;
	GLint clactiveTU;
	GLint TU1BlendMode;

	GLint alphafunc;
	GLclampf alphaval;
	GLint alphatestenabled;
};


//===============================
// LINK TO WORLD
//===============================


void RendererCreateEntities()
{
	// ztrick is not allowed
	float ztrick = CVAR_GET_FLOAT( "gl_ztrick" );
	if (ztrick)
	{
		ConLog("gl_ztrick 1 is not allowed\n");
		gEngfuncs.Cvar_SetValue( "gl_ztrick", 0 );
	}

	if (needkillsky)
	{
		KillSky();
		needkillsky = 0;
	}
}


void RendererTentsUpdate()
{
//	if (!addedents) numrenderents = 0;
//	else addedents = 0;
}


void RendererUpdateEntityList()
{
	numrenderents = 0;

	if (!cv_renderer->value || !cv_drawbrushents->value)
		return;

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	if (!localPlayer)
	{
		CONPRINT("ERROR: UpdateEntityList - cant get local player!\n");
		return;
	}

	cl_entity_t *viewent = gEngfuncs.GetViewModel();

	int curmsgnum = localPlayer->curstate.messagenum;
	
	for (int ic=1; ic<MAX_EDICTS; ic++)
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex( ic );
		if (!ent)
			break;

		if (!ent->model)
			continue;

		if (ent->curstate.messagenum != curmsgnum)
			continue;

		if (ent->curstate.effects & EF_NODRAW)
			continue;

		if (ent == viewent)
			continue;

		if (ent->model->type == mod_brush && numrenderents < MAX_CLRENDER_ENTS)
		{
			if (!IsEntityTransparent(ent))
			{
				renderents[numrenderents] = ent;
				numrenderents++;
			}
		}
	}
//	CONPRINT("%d ents added\n", numrenderents);
}


int RendererFilterEntities( int type, struct cl_entity_s *ent, const char *modelname )
{
	if (ent->curstate.renderfx == 71) // dynamic light
	{
		DynamicLight *dl = MY_AllocDlight(ent->index);
		dl->origin = ent->curstate.origin;
		dl->angles = ent->curstate.angles;
		dl->radius = ent->curstate.renderamt * 8;
		dl->color[0] = (float)ent->curstate.rendercolor.r / 128;
		dl->color[1] = (float)ent->curstate.rendercolor.g / 128;
		dl->color[2] = (float)ent->curstate.rendercolor.b / 128;
		if (ent->curstate.scale) // spotlight
		{
			dl->cone_hor = ent->curstate.scale;
			dl->cone_ver = ent->curstate.scale;
			sprintf(dl->spot_texture, "gfx/spotlight%d.tga", ent->curstate.rendermode);
		}		
		dl->die = gEngfuncs.GetClientTime() + 0.01;
		return 0;
	}

	if (ent->curstate.renderfx == 70 && ent->model->type == mod_sprite)
	{
		StoreSkyboxEntity(ent);
		return 0;
	}

	if (cv_renderer->value && cv_drawbrushents->value)
	{
		if (ent->model->type == mod_brush)
		{
			if (!IsEntityTransparent(ent))
			{
				// tell engine dont draw non-transparent brush ents
				return 0;
			}
		}
	}

	return 1;
}


void RendererCalcRefDef( ref_params_t *pparams )
{
//	CONPRINT("calcrefdef\n");
	no_sky_visible = 0;
	newframe = 1;
	framecount++;
	PrepareRenderer( pparams );
}


void RendererDrawNormal()
{
//	CONPRINT("RendererDrawNormal\n");
	if (newframe)
	{
		GL_MtexStateSaver stateSaver;
	//	CONPRINT("newframe!\n");

		DrawBump();
		MY_DecayLights();

		if (gl.IsGLAllowed() && gl.Paranoia_hacks)
		{
			ResetRenderState();
			DrawSky();
		}

		numskyents = 0;
//		numrenderents = 0;
		newframe = 0;
	}
}


void RendererDrawTransparent()
{
//	GL_MtexStateSaver stateSaver;
	DrawTextureVecs();
	DrawDebugFrustumPlane();
}


void RendererVidInit()
{
	DeleteDecals();
	LoadDecals();
	ResetDynamicLights();
	ResetSky();
	reloaded = 1;
	framecount = 0;
//	maxmsgnum = -1;
//	addedents = 0;

	if (gl.IsGLAllowed() && gl.Paranoia_hacks)
		needkillsky = 1;
}


void RendererInit()
{
	SetLightmapBaseIndex();
	InitDecals();
	InitSky();

	cv_renderer = gEngfuncs.pfnRegisterVariable( "gl_renderer","1", 0 );
	cv_bump = gEngfuncs.pfnRegisterVariable( "gl_bump","1", FCVAR_ARCHIVE );
	cv_detailtex = gEngfuncs.pfnRegisterVariable( "gl_detailtex","1", FCVAR_ARCHIVE );
	cv_bumpvecs = gEngfuncs.pfnRegisterVariable( "bump_vecs","0", 0 );
	cv_bumpdebug = gEngfuncs.pfnRegisterVariable( "bump_debug","0", 0 );
	cv_specular = gEngfuncs.pfnRegisterVariable( "gl_specular","1", FCVAR_ARCHIVE );
	cv_dynamiclight = gEngfuncs.pfnRegisterVariable( "gl_dynlight","1", 0 );
	cv_entdynamiclight = gEngfuncs.pfnRegisterVariable( "gl_dynlight_ent","1", 0 );
	cv_specular_nocombiners = gEngfuncs.pfnRegisterVariable( "gl_nocombs","0", 0 );
	cv_specular_noshaders = gEngfuncs.pfnRegisterVariable( "gl_noshaders","0", 0 );
	cv_highspecular = gEngfuncs.pfnRegisterVariable( "gl_highspecular","1", FCVAR_ARCHIVE );
	cv_gamma = gEngfuncs.pfnRegisterVariable( "gl_gamma","1", FCVAR_ARCHIVE );
	cv_brightness = gEngfuncs.pfnRegisterVariable( "gl_brightness","0", FCVAR_ARCHIVE );
	cv_contrast = gEngfuncs.pfnRegisterVariable( "gl_contrast","1", FCVAR_ARCHIVE );
	cv_speculartwopass = gEngfuncs.pfnRegisterVariable( "gl_twopassspecular","0", 0 );
	cv_dyntwopass = gEngfuncs.pfnRegisterVariable( "gl_twopassdyn","0", 0 );
	cv_blurtest = gEngfuncs.pfnRegisterVariable( "gl_blurtest","0", 0 );
	cv_nsr = gEngfuncs.pfnRegisterVariable( "gl_modelrenderer","1", 0 );
	cv_lambert = gEngfuncs.pfnRegisterVariable( "gl_lambert","2", 0 );
	cv_drawbrushents = gEngfuncs.pfnRegisterVariable( "gl_brushents","1", 0 );
	cv_clipplanes = gEngfuncs.pfnRegisterVariable( "gl_clipplanes","0", 0 );
	cv_showplane = gEngfuncs.pfnRegisterVariable( "gl_showplane","0", 0 );

	gEngfuncs.pfnAddCommand ("dumptextures", DumpTextures);
	gEngfuncs.pfnAddCommand ("dumplevel", DumpLevel);
	gEngfuncs.pfnAddCommand ("makelight", MakeLight);
	gEngfuncs.pfnAddCommand ("dumpleaf", DumpLeafInfo);

	gEngfuncs.pfnHookUserMsg("mydlight", MsgCustomDlight);
}


void RendererCleanup()
{
	DeleteExtTextures();
	DecalsShutdown();
	FreeBuffer();
}


void RendererDrawHud()
{
	DecalsPrintDebugInfo();
	PrintOtherDebugInfo();
	PrintBumpDebugInfo();
}