//
// written by BUzer for HL: Paranoia modification
//
//		2006

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "gl_renderer.h"
#include "gl_dlight.h"

extern engine_studio_api_t IEngineStudio;

//
// Custom model rendering, based on modelviewer's code
//

vec3_t	g_xformverts[MAXSTUDIOVERTS];	// transformed vertices
vec3_t	g_lightvalues[MAXSTUDIOVERTS];	// light surface normals
vec3_t	*g_pxformverts;
vec3_t	*g_pvlightvalues;

//vec3_t	g_lightvec;						// light vector in model reference frame
vec3_t	g_blightvec[MAXSTUDIOBONES];	// light vectors in bone reference frames
//int		g_ambientlight;					// ambient world light
//float	g_shadelight;					// direct world light
//vec3_t	g_lightcolor;
lighting_ext light; // buz

// buz: dynamic lighting
vec3_t	g_bdynlightvec[MAX_MODEL_DYNLIGHTS][MAXSTUDIOBONES]; // light vectors in bone reference frames
vec3_t	g_dynlightcolor[MAX_MODEL_DYNLIGHTS];
int		g_numdynlights;

int		g_chrome[MAXSTUDIOVERTS][2];	// texture coords for surface normals
int		g_chromeage[MAXSTUDIOBONES];	// last time chrome vectors were updated
vec3_t	g_chromeup[MAXSTUDIOBONES];		// chrome vector "up" in bone reference frames
vec3_t	g_chromeright[MAXSTUDIOBONES];	// chrome vector "right" in bone reference frames

//vec3_t	g_vright;		// needs to be set to viewer's right in order for chrome to work
float	alpha = 1; // buz: current alpha
int		useblending = FALSE;


//================================================
// GL_SetupTextureHeader
//
// gets access to external textures for model
// thanks to XaeroX
//================================================
void CStudioModelRenderer::GL_SetupTextureHeader()
{
	//Get texture header
	if (!m_pStudioHeader->numtextures || !m_pStudioHeader->textureindex)
	{
		char texturename[256];		

		strcpy( texturename, m_pRenderModel->name );
		strcpy( &texturename[strlen(texturename) - 4], "T.mdl" );

		model_t *textures = IEngineStudio.Mod_ForName(texturename, 0);
		if (!textures)
		{
			m_pTextureHeader = NULL;
			CONPRINT("Mod_ForName: failed for %s\n", texturename);
			return;
		}

		m_pTextureHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(textures);
		if (!m_pTextureHeader)
			CONPRINT("Mod_Extradata: failed for %s\n", texturename);
	}
	else
	{
		m_pTextureHeader = m_pStudioHeader;
	}
}



void CStudioModelRenderer::GL_SetupRenderer( int rendermode )
{
	g_pxformverts = &g_xformverts[0];
	g_pvlightvalues = &g_lightvalues[0];

/*	gl.glPushMatrix ();

    gl.glTranslatef (m_pCurrentEntity->origin[0], m_pCurrentEntity->origin[1], m_pCurrentEntity->origin[2]);

    gl.glRotatef (m_pCurrentEntity->angles[1],  0, 0, 1);
    gl.glRotatef (m_pCurrentEntity->angles[0],  0, 1, 0);
    gl.glRotatef (m_pCurrentEntity->angles[2],  1, 0, 0);*/

	gl.glShadeModel (GL_SMOOTH);
//	gl.glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	alpha = 1;
	useblending = FALSE;
	if (rendermode == kRenderTransAdd)
	{
		gl.glDepthMask(GL_FALSE);
		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		alpha = (float)m_pCurrentEntity->curstate.renderamt / 255.0;
		useblending = TRUE;
	}
	else if (rendermode == kRenderTransAlpha)
	{
		gl.glDepthMask(GL_FALSE);
		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		alpha = (float)m_pCurrentEntity->curstate.renderamt / 255.0;
		useblending = TRUE;
	}

	// buz: DAMN! another texture_env_combine bug! It works perfectly until
	// blending is enabled. When i turning on blending, the things becomes much darker!
	// Maybe driver bug, i dunno.
	// On register combiners everything works fine in any case, so let it use
	// register combiners if they supported..

	if (!gl.NV_combiners_supported)
	{
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);

	/*	gl.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);*/
	}
	else
	{
		gl.glEnable ( GL_REGISTER_COMBINERS_NV );
		gl.glCombinerParameteriNV ( GL_NUM_GENERAL_COMBINERS_NV, 1 );

		// RC 0 setup:
		//	spare0.rgb = (tex0.rgb * primary_color.rgb) * 2
		gl.glCombinerInputNV ( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB,
							   GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glCombinerInputNV ( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV,
							   GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glCombinerOutputNV ( GL_COMBINER0_NV, GL_RGB,
								GL_SPARE0_NV,          // AB output
								GL_DISCARD_NV,         // CD output
								GL_DISCARD_NV,         // sum output
								GL_SCALE_BY_TWO_NV, GL_NONE,
								GL_FALSE,               // AB = A * B
								GL_FALSE, GL_FALSE );

		//	spare0.a = tex0.a * primary_color.a
		gl.glCombinerInputNV ( GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB,
							   GL_UNSIGNED_IDENTITY_NV, GL_ALPHA );
		gl.glCombinerInputNV ( GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV,
							   GL_UNSIGNED_IDENTITY_NV, GL_ALPHA );
		gl.glCombinerOutputNV ( GL_COMBINER0_NV, GL_ALPHA,
								GL_SPARE0_NV,          // AB output
								GL_DISCARD_NV,         // CD output
								GL_DISCARD_NV,         // sum output
								GL_NONE, GL_NONE,
								GL_FALSE,
								GL_FALSE, GL_FALSE );

		// Final RC setup:
		//	out.rgb = spare0.rgb
		//	out.a = spare0.a
		gl.glFinalCombinerInputNV ( GL_VARIABLE_A_NV, GL_ZERO,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_B_NV, GL_ZERO,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_C_NV, GL_ZERO,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_D_NV, GL_SPARE0_NV,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_E_NV, GL_ZERO,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_F_NV, GL_ZERO,
									GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		gl.glFinalCombinerInputNV ( GL_VARIABLE_G_NV, GL_SPARE0_NV,
									GL_UNSIGNED_IDENTITY_NV, GL_ALPHA );
	}

	// buz special hack for transparent parts on head shield -
	// use less alpha
	if (!strcmp(m_pRenderModel->name, "models/v_headshield.mdl"))
		alpha = 0.7;
}

void CStudioModelRenderer::GL_RestoreRenderer()
{
//	gl.glPopMatrix ();

	if (!gl.NV_combiners_supported)
		gl.glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	else
		gl.glDisable ( GL_REGISTER_COMBINERS_NV );

	gl.glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	gl.glShadeModel (GL_FLAT);
//	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	if (useblending)
	{
		gl.glDepthMask(GL_TRUE);
		gl.glDisable(GL_BLEND);
	}
}


// =================================================================

void VectorRotate (const vec3_t &in1, const float in2[3][4], vec3_t &out)
{
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}


// rotate by the inverse of the matrix
void VectorIRotate (const vec3_t &in1, const float in2[3][4], vec3_t &out)
{
	out[0] = in1[0]*in2[0][0] + in1[1]*in2[1][0] + in1[2]*in2[2][0];
	out[1] = in1[0]*in2[0][1] + in1[1]*in2[1][1] + in1[2]*in2[2][1];
	out[2] = in1[0]*in2[0][2] + in1[1]*in2[1][2] + in1[2]*in2[2][2];
}


#define VectorMaximum(a) ( max( (a)[0], max( (a)[1], (a)[2] ) ) )


// =================================================================
/*
void CStudioModelRenderer::Lighting (float *lv, int bone, int flags, vec3_t normal)
{
	float 	illum;
	float	lightcos;

	illum = g_ambientlight;

	if (flags & STUDIO_NF_FLATSHADE)
	{
		illum += g_shadelight * 0.8;
	} 
	else 
	{
		float r;
		lightcos = DotProduct (normal, g_blightvec[bone]); // -1 colinear, 1 opposite

		if (lightcos > 1.0)
			lightcos = 1;

		illum += g_shadelight;

		r = g_lambert;
		if (r <= 1.0) r = 1.0;

		lightcos = (lightcos + (r - 1.0)) / r; 		// do modified hemispherical lighting
		if (lightcos > 0.0) 
		{
			illum -= g_shadelight * lightcos; 
		}
		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255) 
		illum = 255;
	*lv = illum / 255.0;	// Light from 0 to 1.0
}*/


// buz: colored
void CStudioModelRenderer::Lighting (vec3_t &lv, int bone, int flags, vec3_t normal)
{
	vec3_t 	illum;
	float	lightcos;

	illum = light.ambientlight;

	if (flags & STUDIO_NF_FLATSHADE)
	{
		VectorMA(illum, 0.8, light.addlight, illum);
	} 
	else 
	{
		float r;
		lightcos = DotProduct (normal, g_blightvec[bone]); // -1 colinear, 1 opposite

		if (lightcos > 1.0)
			lightcos = 1;

		VectorAdd(illum, light.addlight, illum);

		r = cv_lambert->value;
		if (r <= 1.0) r = 1.0;

		lightcos = (lightcos + (r - 1.0)) / r; 		// do modified hemispherical lighting
		if (lightcos > 0.0) 
		{
			VectorMA(illum, -lightcos, light.addlight, illum);
		}
		
		if (illum[0] <= 0) illum[0] = 0;
		if (illum[1] <= 0) illum[1] = 0;
		if (illum[2] <= 0) illum[2] = 0;

		// buz: now add all dynamic lights
		for (int i = 0; i < g_numdynlights; i++)
		{
			lightcos = -DotProduct (normal, g_bdynlightvec[i][bone]);
			if (lightcos > 0)
				VectorMA(illum, lightcos, g_dynlightcolor[i], illum);
		}
	}

	float max = VectorMaximum(illum);
	if (max > 1.0)
	{
		float scale = 1.0 / max;

		illum[0] *= scale;
		illum[1] *= scale;
		illum[2] *= scale;
	}

/*	if (illum[0] > 1.0) illum[0] = 1.0;
	if (illum[1] > 1.0) illum[1] = 1.0;
	if (illum[2] > 1.0) illum[2] = 1.0;*/

	lv = illum;
}


void CStudioModelRenderer::Chrome (int *pchrome, int bone, vec3_t normal)
{
	float n;

	if (g_chromeage[bone] != *m_pStudioModelCount)
	{
		// calculate vectors from the viewer to the bone. This roughly adjusts for position
		vec3_t chromeupvec;		// g_chrome t vector in world reference frame
		vec3_t chromerightvec;	// g_chrome s vector in world reference frame
		vec3_t tmp;				// vector pointing at bone in world reference frame
		VectorScale( m_pCurrentEntity->origin, -1, tmp );
		tmp[0] += (*m_pbonetransform)[bone][0][3];
		tmp[1] += (*m_pbonetransform)[bone][1][3];
		tmp[2] += (*m_pbonetransform)[bone][2][3];
		VectorNormalize( tmp );
		CrossProduct( tmp, m_vRight, chromeupvec );
		VectorNormalize( chromeupvec );
		CrossProduct( tmp, chromeupvec, chromerightvec );
		VectorNormalize( chromerightvec );

		VectorIRotate( chromeupvec, (*m_pbonetransform)[bone], g_chromeup[bone] );
		VectorIRotate( chromerightvec, (*m_pbonetransform)[bone], g_chromeright[bone] );

		g_chromeage[bone] = *m_pStudioModelCount;
	}

	// calc s coord
	n = DotProduct( normal, g_chromeright[bone] );
	pchrome[0] = (n + 1.0) * 32; // FIX: make this a float

	// calc t coord
	n = DotProduct( normal, g_chromeup[bone] );
	pchrome[1] = (n + 1.0) * 32; // FIX: make this a float
}


/*
================
StudioModel::SetupLighting
	set some global variables based on entity position
inputs:
outputs:
	g_ambientlight
	g_shadelight
================
*/
void CStudioModelRenderer::GL_SetupLighting ()
{
	int isheadshield = FALSE;

	if (m_pCurrentEntity->curstate.renderfx == 60) // light tester entity
		return;

	if (!strcmp(m_pRenderModel->name, "models/v_headshield.mdl"))
		isheadshield = TRUE;

	vec3_t center = (m_pCurrentEntity->curstate.mins + m_pCurrentEntity->curstate.maxs) / 2;
	vec3_t point = m_pCurrentEntity->origin + center;

/*	light.addlight = Vector(0.5, 0.5, 0.5);
	light.ambientlight = Vector(0.5, 0.5, 0.5);
	light.lightdir = Vector(0, 0, -1);*/

	EXT_LightPoint(point, &light, (m_pCurrentEntity->curstate.effects & EF_INVLIGHT));

	// use inverted light vector for head shield (hack)
	if (!isheadshield)
		VectorInverse(light.lightdir); // we need vector FROM light, not TO light

//	CONPRINT("got amblight: %f, %f, %f\n", light.ambientlight[0], light.ambientlight[1], light.ambientlight[2]);
//	CONPRINT("got diffuse: %f, %f, %f\n", light.addlight[0], light.addlight[1], light.addlight[2]);
//	CONPRINT("vector: %f, %f, %f\n", light.lightdir[0], light.lightdir[1], light.lightdir[2]);

//	light.ambientlight = Vector(0, 0, 0);
//	light.addlight = Vector(1, 1, 1);
//	light.lightdir = Vector(0, -1, 0);

	// TODO: only do it for bones that actually have textures
	for (int i = 0; i < m_pStudioHeader->numbones; i++)
	{
	//	VectorIRotate( g_lightvec, (*m_pbonetransform)[i], g_blightvec[i] );
		VectorIRotate( light.lightdir, (*m_pbonetransform)[i], g_blightvec[i] );
	}

	//
	// buz: Dynamic lighting
	//

//	CONPRINT("model: %s\n", m_pRenderModel->name);

	g_numdynlights = 0;
	DynamicLight* dlights[32];

	if (isheadshield) // player will get crazy of all this full-screen flickering when somebody shoots..
		return;

	int isViewModel = (m_pCurrentEntity == gEngfuncs.GetViewModel()) ? TRUE : FALSE;

	int numlights;
	numlights = GetDlightsForPoint(m_pCurrentEntity->origin, 
		m_pCurrentEntity->curstate.mins, m_pCurrentEntity->curstate.maxs, dlights, isViewModel);
	
	for (i = 0; i < numlights; i++)
	{
		if (g_numdynlights == MAX_MODEL_DYNLIGHTS)
			break;

		vec3_t forward; // light forward for spotlights check
		float spotcos;
		if (dlights[i]->spot_texture[0])
		{
			AngleVectors(dlights[i]->angles, forward, NULL, NULL);
			spotcos = cos((dlights[i]->cone_hor + dlights[i]->cone_ver)*0.3*(M_PI*2/360));
		}

		float r = dlights[i]->radius * dlights[i]->radius; // squared radius

		// use special hack view models when they lit by flashlight -
		// light should come from back of model, angles independent.
		// This just looks cool.
		int isViewModelHack = FALSE;
		vec3_t alternateOrigin;
		if (isViewModel && dlights[i]->key == -666)
		{
			isViewModelHack = TRUE;
			vec3_t forward, right, angles;
			angles = m_pCurrentEntity->angles;
			angles[0] = -angles[0];
			AngleVectors(angles, forward, right, NULL);
			alternateOrigin = m_pCurrentEntity->origin - (forward * 13) + (right * 5);
		}

		for (int b = 0; b < m_pStudioHeader->numbones; b++)
		{
			vec3_t vec;
			if (isViewModelHack)
			{
				vec[0] = (*m_pbonetransform)[b][0][3] - alternateOrigin[0];
				vec[1] = (*m_pbonetransform)[b][1][3] - alternateOrigin[1];
				vec[2] = (*m_pbonetransform)[b][2][3] - alternateOrigin[2];
			}
			else
			{
				vec[0] = (*m_pbonetransform)[b][0][3] - dlights[i]->origin[0];
				vec[1] = (*m_pbonetransform)[b][1][3] - dlights[i]->origin[1];
				vec[2] = (*m_pbonetransform)[b][2][3] - dlights[i]->origin[2];
			}
			
			float dist = DotProduct(vec, vec);
			float atten = (dist / r - 1) * -1;
			if (atten < 0) atten = 0;
			dist = sqrt(dist);
			if (dist)
			{
				dist = 1/dist;
				vec[0] *= dist;
				vec[1] *= dist;
				vec[2] *= dist;
			}

			VectorIRotate( vec, (*m_pbonetransform)[b], g_bdynlightvec[g_numdynlights][b] );
			VectorScale( g_bdynlightvec[g_numdynlights][b], atten, g_bdynlightvec[g_numdynlights][b] );

			if (dlights[i]->spot_texture[0])
			{
				// calc spotlight cone
				atten = DotProduct(forward, vec);
				if (atten < spotcos)
					VectorClear(g_bdynlightvec[g_numdynlights][b]);
				else
					VectorScale(g_bdynlightvec[g_numdynlights][b], (atten - spotcos)/(1 - spotcos), g_bdynlightvec[g_numdynlights][b]);
			}
		}

		g_dynlightcolor[g_numdynlights][0] = ApplyGamma(dlights[i]->color[0]) / 2;
		g_dynlightcolor[g_numdynlights][1] = ApplyGamma(dlights[i]->color[1]) / 2;
		g_dynlightcolor[g_numdynlights][2] = ApplyGamma(dlights[i]->color[2]) / 2;
		g_numdynlights++;
	}
}


/*
=================
StudioModel::SetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
	currententity
outputs:
	pstudiomesh
	pmdl
=================
*/
void CStudioModelRenderer::GL_SetupModel ( int bodypart )
{
	int index;

	if (bodypart > m_pStudioHeader->numbodyparts)
	{
		CONPRINT("SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	m_pBodyPart = (mstudiobodyparts_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bodypartindex) + bodypart;

	index = m_pCurrentEntity->curstate.body / m_pBodyPart->base;
	index = index % m_pBodyPart->nummodels;

	m_pSubModel = (mstudiomodel_t *)((byte *)m_pStudioHeader + m_pBodyPart->modelindex) + index;
}




void CStudioModelRenderer::GL_DrawPoints ( )
{
	byte *pvertbone = ((byte *)m_pStudioHeader + m_pSubModel->vertinfoindex);
	byte *pnormbone = ((byte *)m_pStudioHeader + m_pSubModel->norminfoindex);

	if (!m_pTextureHeader)
		return;

	if (m_pCurrentEntity->curstate.renderfx == 60) // light tester entity
		return;

	mstudiotexture_t *ptexture = (mstudiotexture_t *)((byte *)m_pTextureHeader + m_pTextureHeader->textureindex);

	mstudiomesh_t *pmesh = (mstudiomesh_t *)((byte *)m_pStudioHeader + m_pSubModel->meshindex);

	vec3_t *pstudioverts = (vec3_t *)((byte *)m_pStudioHeader + m_pSubModel->vertindex);
	vec3_t *pstudionorms = (vec3_t *)((byte *)m_pStudioHeader + m_pSubModel->normindex);

	int skinnum = m_pCurrentEntity->curstate.skin; // for short..

	short *pskinref = (short *)((byte *)m_pTextureHeader + m_pTextureHeader->skinindex);
	if (skinnum != 0 && skinnum < m_pTextureHeader->numskinfamilies)
		pskinref += (skinnum * m_pTextureHeader->numskinref);

	for (int i = 0; i < m_pSubModel->numverts; i++)
	{
		VectorTransform (pstudioverts[i], (*m_pbonetransform)[pvertbone[i]], g_pxformverts[i]);
	}

//
// clip and draw all triangles
//

	vec3_t *lv = g_pvlightvalues;
	for (int j = 0; j < m_pSubModel->nummesh; j++) 
	{
		int flags = ptexture[pskinref[pmesh[j].skinref]].flags;
		for (i = 0; i < pmesh[j].numnorms; i++, lv++, pstudionorms++, pnormbone++)
		{
		//	float lv_tmp;
		//	Lighting (&lv_tmp, *pnormbone, flags, (float *)pstudionorms);
			Lighting (*lv, *pnormbone, flags, (float *)pstudionorms); // buz

			// FIX: move this check out of the inner loop
			if (flags & STUDIO_NF_CHROME)
				Chrome( g_chrome[(float (*)[3])lv - (float (*)[3])g_pvlightvalues], *pnormbone, (float *)pstudionorms );

		//	lv[0] = lv_tmp * g_lightcolor[0];
		//	lv[1] = lv_tmp * g_lightcolor[1];
		//	lv[2] = lv_tmp * g_lightcolor[2];
		}
	}

	gl.glCullFace(GL_FRONT);

	for (j = 0; j < m_pSubModel->nummesh; j++) 
	{
		float s, t;		
		short		*ptricmds;

		pmesh = (mstudiomesh_t *)((byte *)m_pStudioHeader + m_pSubModel->meshindex) + j;
		ptricmds = (short *)((byte *)m_pStudioHeader + pmesh->triindex);

		s = 1.0/(float)ptexture[pskinref[pmesh->skinref]].width;
		t = 1.0/(float)ptexture[pskinref[pmesh->skinref]].height;

		mstudiotexture_t *ptex = &ptexture[pskinref[pmesh->skinref]];
		gl.glBindTexture( GL_TEXTURE_2D, (ptex->index & 0xFFFF));
	//	byte *pal = (byte *)m_pStudioHeader + ptex->width * ptex->height + ((ptex->index>>16) & 0xFFFF);
	//	gl.glColorTableEXT(0x81fb,GL_RGB,256,GL_RGB,GL_UNSIGNED_BYTE,pal);
	//	CONPRINT("texture %s (low: %d, high: %d), %d x %d\n", ptex->name,
	//		(ptex->index & 0xFFFF), ((ptex->index>>16) & 0xFFFF),
	//		ptex->width, ptex->height);
	//	CONPRINT("texture %s, flags %d\n", ptex->name, ptex->flags);

		if (ptex->flags & STUDIO_NF_ADDITIVE && !useblending)	// buz
		{
			gl.glEnable(GL_BLEND);
			gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		if (ptex->flags & STUDIO_NF_ALPHATEST && !useblending)	// buz
		{
			gl.glEnable(GL_ALPHA_TEST);
			gl.glAlphaFunc(GL_GREATER, 0.5);
		}
	
		if (ptexture[pskinref[pmesh->skinref]].flags & STUDIO_NF_CHROME)
		{
			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					gl.glBegin( GL_TRIANGLE_FAN );
					i = -i;
				}
				else
				{
					gl.glBegin( GL_TRIANGLE_STRIP );
				}

				for( ; i > 0; i--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					gl.glTexCoord2f(g_chrome[ptricmds[1]][0]*s, g_chrome[ptricmds[1]][1]*t);
					
					float *lv = (float*)&g_pvlightvalues[ptricmds[1]];
					gl.glColor4f( lv[0], lv[1], lv[2], alpha );

					float *av = g_pxformverts[ptricmds[0]];
					gl.glVertex3f(av[0], av[1], av[2]);
				}
				gl.glEnd( );
			}	
		} 
		else 
		{
			while (i = *(ptricmds++))
			{
				if (i < 0)
				{
					gl.glBegin( GL_TRIANGLE_FAN );
					i = -i;
				}
				else
				{
					gl.glBegin( GL_TRIANGLE_STRIP );
				}

				for( ; i > 0; i--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					gl.glTexCoord2f(ptricmds[2]*s, ptricmds[3]*t);
					
					float *lv = (float*)&g_pvlightvalues[ptricmds[1]];
					gl.glColor4f( lv[0], lv[1], lv[2], alpha );
				//	gl.glColor4f( 0.5, 0.5, 0.5, alpha );
				//	gl.glColor4f( 1, 1, 1, alpha );

					float *av = g_pxformverts[ptricmds[0]];
					gl.glVertex3f(av[0], av[1], av[2]);
				}
				gl.glEnd( );
			}
		}

		if (ptex->flags & STUDIO_NF_ADDITIVE && !useblending)	// buz
		{
			gl.glDisable(GL_BLEND);
		}

		if (ptex->flags & STUDIO_NF_ALPHATEST && !useblending)	// buz
		{
			gl.glDisable(GL_ALPHA_TEST);
			gl.glAlphaFunc(GL_GREATER, 0); // if leave 0.5, then grass sprites looks strange, i dunno why
		}
	}
}


//=============================
// EXT_LightPoint
//
// Gets extended lighting info from bump lightmaps.
// Light values should be multiplied by 2, if you want exact color value
//=============================
//int recursioncheck;
int EXT_RecursiveLightPoint (mnode_t *node, const vec3_t &start, const vec3_t &end, lighting_ext *lightinfo);

void EXT_LightPoint(vec3_t pos, lighting_ext *lightinfo, int invlight)
{
	vec3_t		end;
	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;
	memset(lightinfo, 0, sizeof(lighting_ext));
	
	if (!world->lightdata)
	{
		lightinfo->ambientlight[0] = 0.5;
		lightinfo->ambientlight[1] = 0.5;
		lightinfo->ambientlight[2] = 0.5;
		return;
	}

	end[0] = pos[0];
	end[1] = pos[1];

	if (invlight)
		end[2] = pos[2] + 256;
	else
		end[2] = pos[2] - 256;

//	recursioncheck = 0;
	if (!EXT_RecursiveLightPoint (world->nodes, pos, end, lightinfo))
	{
		// Object is too high from ground (for example, helicopter).
		// Use sky color and direction
		cvar_t *sv_skycolor_r = gEngfuncs.pfnGetCvarPointer("sv_skycolor_r");
		cvar_t *sv_skycolor_g = gEngfuncs.pfnGetCvarPointer("sv_skycolor_g");
		cvar_t *sv_skycolor_b = gEngfuncs.pfnGetCvarPointer("sv_skycolor_b");
		lightinfo->ambientlight[0] = lightinfo->addlight[0] = ApplyGamma(sv_skycolor_r->value/255) / 4;
		lightinfo->ambientlight[1] = lightinfo->addlight[1] = ApplyGamma(sv_skycolor_g->value/255) / 4;
		lightinfo->ambientlight[2] = lightinfo->addlight[2] = ApplyGamma(sv_skycolor_b->value/255) / 4;
		lightinfo->lightdir = Vector(0,0,1);
	}
//	CONPRINT("calls: %d\n", recursioncheck);
}


void PARSE_COLOR (vec3_t &out, color24 *lightmap)
{
	out[0] = (float)lightmap->r / 255.0f;
	out[1] = (float)lightmap->g / 255.0f;
	out[2] = (float)lightmap->b / 255.0f;
}


int EXT_RecursiveLightPoint (mnode_t *node, const vec3_t &start, const vec3_t &end, lighting_ext *light)
{
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	color24		*lightmap;

//	recursioncheck++;

	if (node->contents < 0)
		return FALSE;		// didn't hit anything
	
// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return EXT_RecursiveLightPoint (node->children[side], start, end, light);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	int r = EXT_RecursiveLightPoint (node->children[side], start, mid, light);
	if (r) return TRUE;		// hit something
		
	if ((back < 0) == side)
		return FALSE;		// didn't hit anuthing
		
// check for impact on this node
	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;

	surf = world->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTILED | SURF_DRAWSKY))
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
		t < surf->texturemins[1])
			continue;
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			continue;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		if (lightmap)
		{
			vec3_t origcolor = Vector(0, 0, 0);
			vec3_t ambientcolor = Vector(0, 0, 0);
			vec3_t diffusecolor = Vector(0, 0, 0);
			vec3_t lightvec;
			float dot;
			int not_normal = 0;

			lightmap += dt * ((surf->extents[0]>>4)+1) + ds;
			
			for (int style = 0; style < MAXLIGHTMAPS && surf->styles[style] != 255; style++)
			{
				if (surf->styles[style] == 0)
				{
					PARSE_COLOR(origcolor, lightmap);
				}
				else if (surf->styles[style] == BUMP_LIGHTVECS_STYLE)
				{
					vec3_t tmp;
					PARSE_COLOR(tmp, lightmap);

					vec3_t vec_x;
					vec3_t vec_y;
					VectorCopy(tex->vecs[0], vec_x);
					VectorCopy(tex->vecs[1], vec_y);
					VectorNormalize(vec_x);
					VectorNormalize(vec_y);

					vec3_t pnormal;
					VectorCopy(surf->plane->normal, pnormal);
					if (surf->flags & SURF_PLANEBACK)
					{
						VectorInverse(pnormal);
					}

					dot = tmp[2]*2 - 1;

					VectorClear(lightvec);
					VectorMA(lightvec, tmp[0]*2 - 1, vec_x, lightvec);
					VectorMA(lightvec, tmp[1]*2 - 1, vec_y, lightvec);
					VectorMA(lightvec, tmp[2]*2 - 1, pnormal, lightvec);
				}
				else if (surf->styles[style] == BUMP_ADDLIGHT_STYLE)
				{
					PARSE_COLOR(diffusecolor, lightmap);
				}
				else if (surf->styles[style] == BUMP_BASELIGHT_STYLE)
				{
					PARSE_COLOR(ambientcolor, lightmap);
					not_normal = 1;
				}

				lightmap += ((surf->extents[0]>>4)+1) *
						((surf->extents[1]>>4)+1);
			}

			// apply gamma and write to ext_lightinfo struct
		//	if (diffusecolor[0] || diffusecolor[1] || diffusecolor[2] ||
		//		ambientcolor[0] || ambientcolor[1] || ambientcolor[2])
			if (not_normal)
			{
				vec3_t temp;
				vec3_t scale = Vector(0,0,0);
				VectorScale(diffusecolor, dot, temp);
				VectorAdd(temp, ambientcolor, temp);
				VectorScale(temp, 2, temp);

				if (temp[0]) scale[0] = ApplyGamma(temp[0]) / temp[0];
				if (temp[1]) scale[1] = ApplyGamma(temp[1]) / temp[1];
				if (temp[2]) scale[2] = ApplyGamma(temp[2]) / temp[2];

				light->addlight[0] = diffusecolor[0] * scale[0];
				light->addlight[1] = diffusecolor[1] * scale[1];
				light->addlight[2] = diffusecolor[2] * scale[2];

				light->ambientlight[0] = ambientcolor[0] * scale[0];
				light->ambientlight[1] = ambientcolor[1] * scale[1];
				light->ambientlight[2] = ambientcolor[2] * scale[2];

				VectorCopy(lightvec, light->lightdir);
			}
			else
			{
				light->ambientlight[0] = ApplyGamma(origcolor[0])/2;
				light->ambientlight[1] = ApplyGamma(origcolor[1])/2;
				light->ambientlight[2] = ApplyGamma(origcolor[2])/2;
			}
		}		
		return TRUE;
	}

// go down back side
	return EXT_RecursiveLightPoint (node->children[!side], mid, end, light);
}