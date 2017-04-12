
// ==============================
// grass.cpp
// written by BUzer for HL: Paranoia modification
// ==============================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "pm_defs.h"
#include "event_api.h"
#include "com_model.h"
#include "studio_util.h"
#include "r_studioint.h"
#include "triangleapi.h"
#include "log.h"

#include "quake_bsp.h"


extern engine_studio_api_t IEngineStudio;

#define MAX_GRASS_SPRITES	1000
#define MAX_GRASS_MODELS	100
#define MAX_GRASS_TYPES		32
#define MAX_TYPE_TEXTURES	8
#define FADE_RANGE	0.7

enum {
	MODE_SCREEN_PARALLEL = 0,
	MODE_FACING_PLAYER,
	MODE_FIXED,
	MODE_STUDIO,
};


typedef struct grass_sprite_s {
	vec3_t		pos;
	vec3_t		lightcolor;
	int			visible;
	float		hscale;
	float		vscale;
	vec3_t		vecside; // for fixed sprites
} grass_sprite_t;

typedef struct grass_type_s {
	float		radius;
	float		baseheight;
	model_t*	sprite; // model or sprite pointer
	int			modelindex; // for models
	float		sprheight;  // height for sprites, scale for models
	float		sprhalfwidth; // for sprites
	int			mode;
	int			startindex;
	int			numents;
	int			state; // see enum below
	int			numtextures;
	char	worldtextures[MAX_TYPE_TEXTURES][16];
} grass_type_t;


enum {
	TYPE_STATE_NORMAL = 0,
	TYPE_STATE_UNINITIALIZED,
};


grass_sprite_t	grass_sprites[MAX_GRASS_SPRITES];
grass_type_t	grass_types[MAX_GRASS_TYPES];
cl_entity_t		grass_models[MAX_GRASS_MODELS]; // omg, what a giant waste of memory..

static int sprites_count = 0;
static int models_count = 0;
static int types_count = 0;


// grass info msg
int MsgGrassInfo(const char *pszName, int iSize, void *pbuf)
{
	// got new grass type:
	//		coord radius
	//		coord height
	//		short numents
	//		short (sprite scale)*256
	//		byte  mode
	//		byte  numtextures
	//			strings[numtextures] texture names
	//		string sprite name

	if (types_count == MAX_GRASS_TYPES)
	{
		ConLog("Warning: new grass type ignored (MAX_GRASS_TYPES exceeded)\n");
		return 0;
	}

	grass_type_t *t = &grass_types[types_count];
	BEGIN_READ( pbuf, iSize );

	t->state = TYPE_STATE_UNINITIALIZED;
	t->radius = READ_COORD();
	t->baseheight = READ_COORD();
	t->numents = READ_SHORT();
	float sprscale = (float)READ_SHORT() / 256.0f;
	t->mode = READ_BYTE();
	t->numtextures = READ_BYTE();
	for (int i = 0; i < t->numtextures; i++)
		strcpy( t->worldtextures[i], READ_STRING() );
	char *sprname = READ_STRING();

	if ( !IEngineStudio.IsHardware() )
		return 1; // only hardware..

	if (t->mode == MODE_STUDIO)
	{
		t->startindex = models_count;
		t->sprheight = sprscale;
		if ((t->startindex + t->numents) > MAX_GRASS_MODELS)
		{
			ConLog("Warning: new grass type ignored (MAX_GRASS_MODELS exceeded)\n");
			return 1;
		}

		// load model
		t->sprite = gEngfuncs.CL_LoadModel( sprname, &t->modelindex );
		if (!t->sprite || !t->modelindex)
		{
			ConLog("Warning: new grass type ignored (cannot load model %s)\n", sprname);
			return 1;
		}
		models_count += t->numents;
	}
	else
	{
		t->startindex = sprites_count;
		if ((t->startindex + t->numents) > MAX_GRASS_SPRITES)
		{
			ConLog("Warning: new grass type ignored (MAX_GRASS_SPRITES exceeded)\n");
			return 1;
		}

		// load sprite
		HSPRITE hSpr = SPR_Load( sprname );
		if (!hSpr)
		{
			ConLog("Warning: new grass type ignored (cannot load sprite %s)\n", sprname);
			return 1;
		}

		t->sprhalfwidth = (float)SPR_Width( hSpr, 0 ) * sprscale * 0.5f;
		t->sprheight = (float)SPR_Height( hSpr, 0 ) * sprscale;
		t->sprite = (struct model_s *)gEngfuncs.GetSpritePointer( hSpr );
		if (!t->sprite)
		{
			ConLog("Warning: new grass type ignored (cannot get sprptr for %s)\n", sprname);
			return 1;
		}
		sprites_count += t->numents;
	}

	types_count++;	
	return 1;
}

// create cvars here, hook messages, etc..
void GrassInit()
{
	gEngfuncs.pfnHookUserMsg("GrassInfo", MsgGrassInfo);
}

// reset
void GrassVidInit()
{
	sprites_count = 0;
	types_count = 0;
	models_count = 0;
	memset(grass_models, 0, sizeof(grass_models));
}

float AlphaDist( const vec3_t &sprpos, const vec3_t &playerpos, float maxdist )
{
	float dist_x = playerpos[0] - sprpos[0];
	float dist_y = playerpos[1] - sprpos[1];
	if (dist_x < 0) dist_x = -dist_x;
	if (dist_y < 0) dist_y = -dist_y;
	float fract = dist_x > dist_y ? dist_x : dist_y;
	fract /= maxdist;
	if (fract >= 1.0) return 0;
	if (fract <= FADE_RANGE) return 1.0;
	return (1 - ((fract - FADE_RANGE) * (1/(1-FADE_RANGE))));
}


// =========================================================
//		sprites
// =========================================================

void GetSpriteRandomVec( vec3_t &vec )
{
	float angle = gEngfuncs.pfnRandomFloat(0.0f, M_PI*2);
	vec[0] = cos(angle);
	vec[1] = sin(angle);
	vec[2] = 0;
}

// corrects pos[2], checks world texture, sets lighting and other settings
void PlaceGrassSprite( grass_type_t *t, grass_sprite_t *s )
{
	vec3_t temp;
	s->visible = 0;
	s->pos[2] = t->baseheight;
	VectorCopy(s->pos, temp);
	temp[2] -= 8192;

	pmtrace_t ptr;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( s->pos, temp, PM_STUDIO_IGNORE, -1, &ptr );
	if (ptr.startsolid || ptr.allsolid || ptr.fraction == 1.0f)
		return;

	const char *texname = gEngfuncs.pEventAPI->EV_TraceTexture( ptr.ent, s->pos, temp );
	if (!texname)
		return;

	for (int cht = 0; cht < t->numtextures; cht++)
	{
		if (!strcmp( t->worldtextures[cht], texname))
		{
			s->visible = 1;
			s->vscale = gEngfuncs.pfnRandomFloat(0.7f, 1.0f);
			s->hscale = gEngfuncs.pfnRandomLong( 0, 1 ) ? s->vscale : -s->vscale;
			R_LightPoint (s->pos, s->lightcolor);
			VectorCopy(ptr.endpos, s->pos);
			if (t->mode == MODE_FIXED) GetSpriteRandomVec(s->vecside);
			break;
		}
	}
}

void UpdateGrassSpritePosition( grass_type_t *t, grass_sprite_t *s, const vec3_t &mins, const vec3_t &maxs )
{
	int shift_x = 0, shift_y = 0;
	float radx2 = maxs[0] - mins[0];

	if (s->pos[0] > maxs[0]) {
		do {s->pos[0] -= radx2;} while (s->pos[0] > maxs[0]);
		shift_x = 1;
	}
	else if (s->pos[0] < mins[0]) {
		do {s->pos[0] += radx2;} while (s->pos[0] < maxs[0]);
		shift_x = 1;
	}
	if (s->pos[1] > maxs[1]) {
		do {s->pos[1] -= radx2;} while (s->pos[1] > maxs[1]);
		shift_y = 1;
	}
	else if (s->pos[1] < mins[1]) {
		do {s->pos[1] += radx2;} while (s->pos[1] < maxs[1]);
		shift_y = 1;
	}

	if (shift_x) {
		if (!shift_y)
			s->pos[1] = gEngfuncs.pfnRandomFloat(mins[1], maxs[1]);
		PlaceGrassSprite(t, s);
	}
	else if (shift_y) {
		s->pos[0] = gEngfuncs.pfnRandomFloat(mins[0], maxs[0]);
		PlaceGrassSprite(t, s);
	}
}

void DrawSpriteFacingPlayer( grass_sprite_t *s, grass_type_t *t, const vec3_t &playerpos, float alpha )
{
	vec3_t toPlayer; 
	toPlayer[0] = playerpos[0] - s->pos[0];
	toPlayer[1] = playerpos[1] - s->pos[1];
	toPlayer[2] = 0;
	VectorNormalize(toPlayer);
	toPlayer[0] *= t->sprhalfwidth * s->hscale;
	toPlayer[1] *= t->sprhalfwidth * s->hscale;
	float sprheight = t->sprheight * s->vscale;

	gEngfuncs.pTriAPI->Color4f( s->lightcolor[0], s->lightcolor[1], s->lightcolor[2], alpha );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]-toPlayer[1], s->pos[1]+toPlayer[0], s->pos[2]+sprheight );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]-toPlayer[1], s->pos[1]+toPlayer[0], s->pos[2] );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]+toPlayer[1], s->pos[1]-toPlayer[0], s->pos[2] );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]+toPlayer[1], s->pos[1]-toPlayer[0], s->pos[2]+sprheight );
	
	gEngfuncs.pTriAPI->End();
}

void DrawSpriteParallel( grass_sprite_t *s, grass_type_t *t, const vec3_t &vecside, float alpha )
{
	vec3_t temp = vecside * (t->sprhalfwidth * s->hscale);
	float sprheight = t->sprheight * s->vscale;

	gEngfuncs.pTriAPI->Color4f( s->lightcolor[0], s->lightcolor[1], s->lightcolor[2], alpha );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]-temp[0], s->pos[1]-temp[1], s->pos[2]+sprheight );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]-temp[0], s->pos[1]-temp[1], s->pos[2] );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]+temp[0], s->pos[1]+temp[1], s->pos[2] );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		gEngfuncs.pTriAPI->Vertex3f( s->pos[0]+temp[0], s->pos[1]+temp[1], s->pos[2]+sprheight );
	
	gEngfuncs.pTriAPI->End();
}

void GrassDraw()
{
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();

	grass_type_t *t = grass_types;
	for (int type = 0; type < types_count; type++, t++)
	{
		if (t->mode == MODE_STUDIO)
			continue;

		vec3_t mins, maxs;
		VectorCopy(player->origin, mins);
		VectorCopy(player->origin, maxs);
		mins[0] -= t->radius; mins[1] -= t->radius;
		maxs[0] += t->radius; maxs[1] += t->radius;

		if (t->state == TYPE_STATE_UNINITIALIZED)
		{
			grass_sprite_t *s = &grass_sprites[t->startindex];
			for (int sprite = 0; sprite < t->numents; sprite++, s++)
			{
				s->pos[0] = gEngfuncs.pfnRandomFloat(mins[0], maxs[0]);
				s->pos[1] = gEngfuncs.pfnRandomFloat(mins[1], maxs[1]);
				PlaceGrassSprite(t, s);
			}
			t->state = TYPE_STATE_NORMAL;
		}
		else
		{
			int sprite;
			vec3_t vec;
			gEngfuncs.pTriAPI->SpriteTexture( t->sprite, 0 );
		//	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );

			grass_sprite_t *s = &grass_sprites[t->startindex];

			switch(t->mode)
			{
			default:
			case MODE_SCREEN_PARALLEL:
				gEngfuncs.GetViewAngles((float*)vec);
				AngleVectors(vec, NULL, vec, NULL);
				for (sprite = 0; sprite < t->numents; sprite++, s++)
				{
					UpdateGrassSpritePosition( t, s, mins, maxs );
					if (s->visible)
						DrawSpriteParallel( s, t, vec, AlphaDist(s->pos, player->origin, t->radius) );
				}
				break;
			case MODE_FACING_PLAYER:
				for (sprite = 0; sprite < t->numents; sprite++, s++)
				{
					UpdateGrassSpritePosition( t, s, mins, maxs );
					if (s->visible)
						DrawSpriteFacingPlayer( s, t, player->origin, AlphaDist(s->pos, player->origin, t->radius) );
				}
				break;
			case MODE_FIXED:
				for (sprite = 0; sprite < t->numents; sprite++, s++)
				{
					UpdateGrassSpritePosition( t, s, mins, maxs );
					if (s->visible)
						DrawSpriteParallel( s, t, s->vecside, AlphaDist(s->pos, player->origin, t->radius) );
				}
				break;
			}
		}
	} // grass types
}


// =========================================================
//		models
// =========================================================

// origin, angles
// curstate.iuser1 - is visible
// curstate.scale - scale

void PlaceGrassModel( grass_type_t *t, cl_entity_t *s )
{
	vec3_t temp;
	s->curstate.iuser1 = 0;
	s->origin[2] = t->baseheight;
	VectorCopy(s->origin, temp);
	temp[2] -= 8192;

	pmtrace_t ptr;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( s->origin, temp, PM_STUDIO_IGNORE, -1, &ptr );
	if (ptr.startsolid || ptr.allsolid || ptr.fraction == 1.0f)
		return;

	const char *texname = gEngfuncs.pEventAPI->EV_TraceTexture( ptr.ent, s->origin, temp );
	if (!texname)
		return;

	for (int cht = 0; cht < t->numtextures; cht++)
	{
		if (!strcmp( t->worldtextures[cht], texname))
		{
			s->curstate.iuser1 = 1;
			s->curstate.scale = gEngfuncs.pfnRandomFloat(0.7f, 1.0f) * t->sprheight;
			VectorCopy(ptr.endpos, s->origin);
			break;
		}
	}
}

void UpdateGrassModelPosition( grass_type_t *t, cl_entity_t *s, const vec3_t &mins, const vec3_t &maxs )
{
	int shift_x = 0, shift_y = 0;
	float radx2 = maxs[0] - mins[0];

	if (s->origin[0] > maxs[0]) {
		do {s->origin[0] -= radx2;} while (s->origin[0] > maxs[0]);
		shift_x = 1;
	}
	else if (s->origin[0] < mins[0]) {
		do {s->origin[0] += radx2;} while (s->origin[0] < maxs[0]);
		shift_x = 1;
	}
	if (s->origin[1] > maxs[1]) {
		do {s->origin[1] -= radx2;} while (s->origin[1] > maxs[1]);
		shift_y = 1;
	}
	else if (s->origin[1] < mins[1]) {
		do {s->origin[1] += radx2;} while (s->origin[1] < maxs[1]);
		shift_y = 1;
	}

	if (shift_x) {
		if (!shift_y)
			s->origin[1] = gEngfuncs.pfnRandomFloat(mins[1], maxs[1]);
		PlaceGrassModel(t, s);
	}
	else if (shift_y) {
		s->origin[0] = gEngfuncs.pfnRandomFloat(mins[0], maxs[0]);
		PlaceGrassModel(t, s);
	}
}

void GrassCreateEntities()
{
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();

	grass_type_t *t = grass_types;
	for (int type = 0; type < types_count; type++, t++)
	{
		if (t->mode != MODE_STUDIO)
			continue;

		vec3_t mins, maxs;
		VectorCopy(player->origin, mins);
		VectorCopy(player->origin, maxs);
		mins[0] -= t->radius; mins[1] -= t->radius;
		maxs[0] += t->radius; maxs[1] += t->radius;

		if (t->state == TYPE_STATE_UNINITIALIZED)
		{
			cl_entity_t *s = &grass_models[t->startindex];
			for (int model = 0; model < t->numents; model++, s++)
			{
				s->origin[0] = gEngfuncs.pfnRandomFloat(mins[0], maxs[0]);
				s->origin[1] = gEngfuncs.pfnRandomFloat(mins[1], maxs[1]);
				s->model = t->sprite;
				s->curstate.modelindex = t->modelindex;
				PlaceGrassModel(t, s);
			}
			t->state = TYPE_STATE_NORMAL;
		}
		else
		{
			cl_entity_t *s = &grass_models[t->startindex];
			for (int model = 0; model < t->numents; model++, s++)
			{
				UpdateGrassModelPosition( t, s, mins, maxs );
				if (s->curstate.iuser1)
				{
					float a = AlphaDist(s->origin, player->origin, t->radius);
					if (a < 1.0) {
						s->curstate.rendermode = kRenderTransAlpha;
						s->curstate.renderamt = a * 255;
					}
					else {
						s->curstate.rendermode = kRenderNormal;
						s->curstate.renderamt = 255;
					}

					gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, s );
				}
			} // models
		}
	} // grass types
}