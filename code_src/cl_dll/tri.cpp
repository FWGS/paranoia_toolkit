//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"
#include "wrect.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "particlemgr.h"

#include "grass.h" // buz
#include "rain.h" 
#include "custom_alloc.h"
#include "com_model.h"
#include "studio_util.h"

#include "triapiobjects.h"
#include "gl_renderer.h" // buz

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

extern float g_fFogColor[4];
extern float g_fStartDist;
extern float g_fEndDist;
extern int g_iWaterLevel;
extern vec3_t v_origin;

int UseTexture(HSPRITE &hsprSpr, char * str)
{
	if (hsprSpr == 0)
	{
		char sz[256];
		sprintf( sz, str );
		hsprSpr = SPR_Load( sz );
	}

	return gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( hsprSpr ), 0 );
}


// rain
void SetPoint( float x, float y, float z, float (*matrix)[4]);

//
//-----------------------------------------------------
//

//LRC - code for CShinySurface, declared in hud.h
CShinySurface::CShinySurface( float fScale, float fAlpha, float fMinX, float fMaxX, float fMinY, float fMaxY, float fZ, char *szSprite)
{
	m_fScale = fScale; m_fAlpha = fAlpha;
	m_fMinX = fMinX; m_fMinY = fMinY;
	m_fMaxX = fMaxX; m_fMaxY = fMaxY;
	m_fZ = fZ;
	m_hsprSprite = 0;
	sprintf( m_szSprite, szSprite );
	m_pNext = NULL;
}

CShinySurface::~CShinySurface()
{
	if (m_pNext)
		delete m_pNext;
}

void CShinySurface::DrawAll(const vec3_t &org)
{
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd ); //kRenderTransTexture );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	for(CShinySurface *pCurrent = this; pCurrent; pCurrent = pCurrent->m_pNext)
	{
		pCurrent->Draw(org);
	}

	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

void CShinySurface::Draw(const vec3_t &org)
{
	// add 5 to the view height, so that we don't get an ugly repeating texture as it approaches 0.
	float fHeight = org.z - m_fZ + 5;

	// only visible from above
//	if (fHeight < 0) return;
	if (fHeight < 5) return;

	// fade out if we're really close to the surface, so they don't see an ugly repeating texture
//	if (fHeight < 15)
//		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, (fHeight - 5)*0.1*m_fAlpha );
//	else
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, m_fAlpha );

	// check whether the texture is valid
	if (!UseTexture(m_hsprSprite, m_szSprite)) return;

//	gEngfuncs.Con_Printf("minx %f, maxx %f, miny %f, maxy %f\n", m_fMinX, m_fMaxX, m_fMinY, m_fMaxY);

	float fFactor = 1/(m_fScale*fHeight);
	float fMinTX = (org.x - m_fMinX)*fFactor;
	float fMaxTX = (org.x - m_fMaxX)*fFactor;
	float fMinTY = (org.y - m_fMinY)*fFactor;
	float fMaxTY = (org.y - m_fMaxY)*fFactor;
//	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, m_fAlpha );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->TexCoord2f(	fMinTX,		fMinTY					);
		gEngfuncs.pTriAPI->Vertex3f  (	m_fMinX,	m_fMinY,	m_fZ+0.02	); // add 0.02 to avoid z-buffer problems
		gEngfuncs.pTriAPI->TexCoord2f(	fMinTX,		fMaxTY					);
		gEngfuncs.pTriAPI->Vertex3f  (	m_fMinX,	m_fMaxY,	m_fZ+0.02	);
		gEngfuncs.pTriAPI->TexCoord2f(	fMaxTX,		fMaxTY					);
		gEngfuncs.pTriAPI->Vertex3f  (	m_fMaxX,	m_fMaxY,	m_fZ+0.02	);
		gEngfuncs.pTriAPI->TexCoord2f(	fMaxTX,		fMinTY					);
		gEngfuncs.pTriAPI->Vertex3f  (	m_fMaxX,	m_fMinY,	m_fZ+0.02	);
	gEngfuncs.pTriAPI->End();
}

//
//-----------------------------------------------------
//


//LRCT
//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
//LRCT		sprintf( sz, "sprites/cursor.spr" );
		sprintf( sz, "sprites/bubble.spr" ); //LRCT
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

void BlackFog ( void )
{
	//Not in water and we want fog.
	static float fColorBlack[3] = {0,0,0};
	bool bFog = g_iWaterLevel < 2 && g_fStartDist > 0 && g_fEndDist > 0;
	if (bFog)
		gEngfuncs.pTriAPI->Fog ( fColorBlack, g_fStartDist, g_fEndDist, bFog );
	else
		gEngfuncs.pTriAPI->Fog ( g_fFogColor, g_fStartDist, g_fEndDist, bFog );
}

void RenderFog ( void )
{
	//Not in water and we want fog.
	bool bFog = g_iWaterLevel < 2 && g_fStartDist > 0 && g_fEndDist > 0;
	if (bFog)
		gEngfuncs.pTriAPI->Fog ( g_fFogColor, g_fStartDist, g_fEndDist, bFog );
//	else
//		gEngfuncs.pTriAPI->Fog ( g_fFogColor, 10000, 10001, 0 );
}


/*
====================
buz:
Orthogonal polygons
====================
*/

// helper functions

void OrthoQuad(int x1, int y1, int x2, int y2)
{
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

		gEngfuncs.pTriAPI->TexCoord2f(0.01, 0.01);
		gEngfuncs.pTriAPI->Vertex3f(x1, y1, 0);

		gEngfuncs.pTriAPI->TexCoord2f(0.01, 0.99);
		gEngfuncs.pTriAPI->Vertex3f(x1, y2, 0);

		gEngfuncs.pTriAPI->TexCoord2f(0.99, 0.99);
		gEngfuncs.pTriAPI->Vertex3f(x2, y2, 0);

		gEngfuncs.pTriAPI->TexCoord2f(0.99f, 0.01);
		gEngfuncs.pTriAPI->Vertex3f(x2, y1, 0);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes

	
/*	vec3_t world1, screen1;
	vec3_t world2, screen2;
	vec3_t world3, screen3;
	vec3_t world4, screen4;
	screen1[0] = (x1 / (float)ScreenWidth)*2 - 1;
	screen1[1] = (y1 / (float)ScreenWidth)*2 - 1;
	screen1[2] = 0;
	screen2[0] = (x1 / (float)ScreenWidth)*2 - 1;
	screen2[1] = (y2 / (float)ScreenWidth)*2 - 1;
	screen2[2] = 0;
	screen3[0] = (x2 / (float)ScreenWidth)*2 - 1;
	screen3[1] = (y2 / (float)ScreenWidth)*2 - 1;
	screen3[2] = 0;
	screen4[0] = (x2 / (float)ScreenWidth)*2 - 1;
	screen4[1] = (y1 / (float)ScreenWidth)*2 - 1;
	screen4[2] = 0;

	gEngfuncs.pTriAPI->ScreenToWorld(screen1,world1);
	gEngfuncs.pTriAPI->ScreenToWorld(screen2,world2);
	gEngfuncs.pTriAPI->ScreenToWorld(screen3,world3);
	gEngfuncs.pTriAPI->ScreenToWorld(screen4,world4);

	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

		gEngfuncs.pTriAPI->TexCoord2f(0.01, 0.01);
		gEngfuncs.pTriAPI->Vertex3f(world1[0]+16, world1[1], world1[2]);

		gEngfuncs.pTriAPI->TexCoord2f(0.01, 0.99);
		gEngfuncs.pTriAPI->Vertex3f(world2[0]+16, world2[1], world2[2]);

		gEngfuncs.pTriAPI->TexCoord2f(0.99, 0.99);
		gEngfuncs.pTriAPI->Vertex3f(world3[0]+16, world3[1], world3[2]);

		gEngfuncs.pTriAPI->TexCoord2f(0.99f, 0.01);
		gEngfuncs.pTriAPI->Vertex3f(world4[0]+16, world4[1], world4[2]);

	gEngfuncs.pTriAPI->End();*/
}


// buz: use triapi to draw sprites on screen
void DrawSpriteAsPoly( HSPRITE hspr, wrect_t *rect, wrect_t *screenpos, int mode, float r, float g, float b, float a)
{
	if (!hspr) return;

	const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(hspr);
	gEngfuncs.pTriAPI->RenderMode(mode);
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f( r, g, b, a );

	float x = rect->left / (float)SPR_Width(hspr, 0) + 0.01;
	float x2 = rect->right / (float)SPR_Width(hspr, 0) - 0.01;
	float y = rect->top / (float)SPR_Height(hspr, 0) + 0.01;
	float y2 = rect->bottom / (float)SPR_Height(hspr, 0) - 0.01;

	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
		gEngfuncs.pTriAPI->TexCoord2f(x, y);
		gEngfuncs.pTriAPI->Vertex3f(screenpos->left, screenpos->top, 0);

		gEngfuncs.pTriAPI->TexCoord2f(x, y2);
		gEngfuncs.pTriAPI->Vertex3f(screenpos->left, screenpos->bottom, 0);

		gEngfuncs.pTriAPI->TexCoord2f(x2, y2);
		gEngfuncs.pTriAPI->Vertex3f(screenpos->right, screenpos->bottom, 0);

		gEngfuncs.pTriAPI->TexCoord2f(x2, y);
		gEngfuncs.pTriAPI->Vertex3f(screenpos->right, screenpos->top, 0);
	gEngfuncs.pTriAPI->End();
}

void OrthoDraw(HSPRITE spr, int mode, float r, float g, float b, float a)
{
	if (!spr) return;

	const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(spr);
	gEngfuncs.pTriAPI->RenderMode(mode);

	int frames = SPR_Frames(spr);

	switch (frames)
	{
	case 1:
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(0, 0, ScreenWidth, ScreenHeight);
		break;

	case 2:
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(0, 0, ScreenWidth/2, ScreenHeight);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 1);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(ScreenWidth/2, 0, ScreenWidth, ScreenHeight);
		break;

	case 4:
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(0, 0, ScreenWidth/2, ScreenHeight/2);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 1);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(ScreenWidth/2, 0, ScreenWidth, ScreenHeight/2);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 2);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(0, ScreenHeight/2, ScreenWidth/2, ScreenHeight);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 3);
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->Color4f( r, g, b, a );
		OrthoQuad(ScreenWidth/2, ScreenHeight/2, ScreenWidth, ScreenHeight);
		break;

	default:
		gEngfuncs.Con_Printf("ERROR: illegal number of frames in ortho sprite (%i)\n", frames);
		break;
	}
}


extern vec3_t gLighting;
extern float gLightLevel;

void OrthoGasMask()
{
	// mask base
	HSPRITE spr1 = SPR_Load("sprites/gasmask1.spr");
	if (!spr1)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/gasmask1.spr\n");
		return;
	}

	// dirt decal
	HSPRITE spr2 = SPR_Load("sprites/gasmask2.spr");
	if (!spr2)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/gasmask2.spr\n");
		return;
	}

	// transparent glass
	HSPRITE spr3 = SPR_Load("sprites/gasmask3.spr");
	if (!spr3)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/gasmask3.spr\n");
		return;
	}

	OrthoDraw(spr3, kRenderTransAdd, gLighting[0]*gLightLevel, gLighting[1]*gLightLevel, gLighting[2]*gLightLevel, 1.0);
	OrthoDraw(spr2, kRenderTransColor, gLighting[0]*gLightLevel*0.5, gLighting[1]*gLightLevel*0.5, gLighting[2]*gLightLevel*0.5, 1.0);
	OrthoDraw(spr1, kRenderNormal, gLighting[0]*gLightLevel, gLighting[1]*gLightLevel, gLighting[2]*gLightLevel, 1.0);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal
}



void OrthoScope(void)
{
	// scope base
	HSPRITE spr1 = SPR_Load("sprites/sniper1.spr");
	if (!spr1)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/sniper1.spr\n");
		return;
	}

	// dirt decal
	HSPRITE spr2 = SPR_Load("sprites/sniper2.spr");
	if (!spr2)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/sniper2.spr\n");
		return;
	}

	// transparent glass
	HSPRITE spr3 = SPR_Load("sprites/sniper3.spr");
	if (!spr3)
	{
		gEngfuncs.Con_Printf("ERROR: Can't load sprites/sniper3.spr\n");
		return;
	}

	OrthoDraw(spr3, kRenderTransAdd, gLighting[0]*gLightLevel, gLighting[1]*gLightLevel, gLighting[2]*gLightLevel, 1.0);
	OrthoDraw(spr2, kRenderTransColor, gLighting[0]*gLightLevel*0.5, gLighting[1]*gLightLevel*0.5, gLighting[2]*gLightLevel*0.5, 1.0);
	OrthoDraw(spr1, kRenderNormal, gLighting[0]*gLightLevel, gLighting[1]*gLightLevel, gLighting[2]*gLightLevel, 1.0);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal
}




/*
=================================
DrawRain

Рисование капель и снежинок.
=================================
*/
//extern cl_drip FirstChainDrip;
extern MemBlock<cl_drip>	g_dripsArray;
extern MemBlock<cl_rainfx>	g_fxArray;
extern rain_properties Rain;

void DrawRain( void )
{
//	if (FirstChainDrip.p_Next == NULL)
	if (g_dripsArray.IsClear())
		return; // no drips to draw

	HSPRITE hsprTexture;
	const model_s *pTexture;
	float visibleHeight = Rain.globalHeight - SNOWFADEDIST;

	if (Rain.weatherMode == 0)
		hsprTexture = LoadSprite( "sprites/hi_rain.spr" ); // load rain sprite
	else 
		hsprTexture = LoadSprite( "sprites/snowflake.spr" ); // load snow sprite

	if (!hsprTexture)
	{
		gEngfuncs.Con_Printf("ERROR: hi_rain.spr or snowflake.spr could not be loaded!\n");
		return;
	}

	// usual triapi stuff
	pTexture = gEngfuncs.GetSpritePointer( hsprTexture );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	
	// go through drips list
	g_dripsArray.StartPass();
//	cl_drip* Drip = FirstChainDrip.p_Next;
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();
	cl_drip* Drip = g_dripsArray.GetCurrent();

	if ( Rain.weatherMode == 0 )
	{ // draw rain
		while (Drip != NULL)
		{
		//	cl_drip* nextdDrip = Drip->p_Next;
					
			Vector2D toPlayer; 
			toPlayer.x = player->origin[0] - Drip->origin[0];
			toPlayer.y = player->origin[1] - Drip->origin[1];
			toPlayer = toPlayer.Normalize();
	
			toPlayer.x *= DRIP_SPRITE_HALFWIDTH;
			toPlayer.y *= DRIP_SPRITE_HALFWIDTH;

			float shiftX = (Drip->xDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
			float shiftY = (Drip->yDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;

		// --- draw triangle --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, Drip->alpha );
			gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0]-toPlayer.y - shiftX, Drip->origin[1]+toPlayer.x - shiftY,
					Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 0.5, 1 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0] + shiftX, Drip->origin[1] + shiftY,
					Drip->origin[2]-DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0]+toPlayer.y - shiftX, Drip->origin[1]-toPlayer.x - shiftY,
					Drip->origin[2]+DRIP_SPRITE_HALFHEIGHT);
	
			gEngfuncs.pTriAPI->End();
		// --- draw triangle end ----------------------

		//	Drip = nextdDrip;
			g_dripsArray.MoveNext();
			Drip = g_dripsArray.GetCurrent();
		}
	}
	else
	{ // draw snow
		vec3_t normal;
		gEngfuncs.GetViewAngles((float*)normal);
	
		float matrix[3][4];
		AngleMatrix (normal, matrix);	// calc view matrix

		while (Drip != NULL)
		{
		//	cl_drip* nextdDrip = Drip->p_Next;

			matrix[0][3] = Drip->origin[0]; // write origin to matrix
			matrix[1][3] = Drip->origin[1];
			matrix[2][3] = Drip->origin[2];

			// apply start fading effect
			float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha :
				((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;
					
		// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				SetPoint(0, SNOW_SPRITE_HALFSIZE ,SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				SetPoint(0, SNOW_SPRITE_HALFSIZE ,-SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE ,-SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE ,SNOW_SPRITE_HALFSIZE, matrix);
				
			gEngfuncs.pTriAPI->End();
		// --- draw quad end ----------------------

		//	Drip = nextdDrip;
			g_dripsArray.MoveNext();
			Drip = g_dripsArray.GetCurrent();
		}
	}
}

/* rain
=================================
DrawFXObjects

Рисование водяных кругов
=================================
*/
//extern cl_rainfx FirstChainFX;

void DrawFXObjects( void )
{
//	if (FirstChainFX.p_Next == NULL)
	if (g_fxArray.IsClear())
		return; // no objects to draw

	float curtime = gEngfuncs.GetClientTime();

	// usual triapi stuff
	HSPRITE hsprTexture;
	const model_s *pTexture;
	hsprTexture = LoadSprite( "sprites/waterring.spr" ); // load water ring sprite
	pTexture = gEngfuncs.GetSpritePointer( hsprTexture );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	
	// go through objects list
//	cl_rainfx* curFX = FirstChainFX.p_Next;
	g_fxArray.StartPass();
	cl_rainfx* curFX = g_fxArray.GetCurrent();

	while (curFX != NULL)
	{
	//	cl_rainfx* nextFX = curFX->p_Next;
					
		// fadeout
		float alpha = ((curFX->birthTime + curFX->life - curtime) / curFX->life) * curFX->alpha;
		float size = (curtime - curFX->birthTime) * MAXRINGHALFSIZE;

		// --- draw quad --------------------------
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] - size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] + size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] + size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] - size, curFX->origin[2]);
	
		gEngfuncs.pTriAPI->End();
		// --- draw quad end ----------------------

	//	curFX = nextFX;
		g_fxArray.MoveNext();
		curFX = g_fxArray.GetCurrent();
	}
}




/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/

extern void ExtDrawShield( void );
int g_DrawHeadShied;

void DLLEXPORT HUD_DrawNormalTriangles( void )
{
//	gHUD.m_Spectator.DrawOverview();

	if (g_DrawHeadShied)
	{
		ExtDrawShield();
		return; // buz: dont draw anything else in headshield's pass
	}

	RendererDrawNormal(); // buz

#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
//extern ParticleSystemManager* g_pParticleSystems; // LRC
extern ParticleSystemManager	g_pParticleSystems; // LRC // buz
void DrawGlows(); // buz

void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	BlackFog();

	if (g_DrawHeadShied)
		return; // buz: dont draw anything else in headshield's pass

	RendererDrawTransparent(); // buz

   	//22/03/03 LRC: shiny surfaces
	if (gHUD.m_pShinySurface)
		gHUD.m_pShinySurface->DrawAll(v_origin);

	// LRC: find out the time elapsed since the last redraw
	static float fOldTime, fTime;
	fOldTime = fTime;
	fTime = gEngfuncs.GetClientTime();

	// buz: catch map changes
	float timedelta = fTime - fOldTime;
	if (timedelta < 0) timedelta = 0;
	if (timedelta > 0.5) timedelta = 0.5;
//	CONPRINT("%f\n", fTime);

	// LRC: draw and update particle systems
//	g_pParticleSystems->UpdateSystems(fTime - fOldTime);
	g_pParticleSystems.UpdateSystems(timedelta, FALSE); // buz

	g_objmanager.Draw();

	GrassDraw();

	ProcessFXObjects();
	ProcessRain();
	DrawRain();
	DrawFXObjects();

	DrawGlows();

/*	HSPRITE spr = SPR_Load("sprites/test0.spr");
	const struct model_s *sprmodel = gEngfuncs.GetSpritePointer(spr);
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) sprmodel, 0);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 0.5 );
	OrthoQuad(0, 0, ScreenWidth, ScreenHeight/2);*/


#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}
