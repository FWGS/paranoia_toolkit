#include "hud.h"
#include "cl_util.h"
#include "const.h"


// Triangle rendering apis are in gEngfuncs.pTriAPI
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "particlemgr.h"

// for AngleMatrix
#include "com_model.h"
#include "studio_util.h"

#include "triapiobjects.h"


CObjectsManager g_objmanager;
float g_viewMatrix[3][4];


void CalcViewMatrix( int reset = 0 )
{
	static int matrixCalculated = 0;

	if (reset) // new frame, matrix shold be recalculated
	{
		matrixCalculated = 0;
		return;
	}

	if (matrixCalculated) // already calculated in this frame
		return;

	// calculate view matrix
	vec3_t normal;
	gEngfuncs.GetViewAngles((float*)normal);
	AngleMatrix (normal, g_viewMatrix);

	matrixCalculated = 1;
}


void SetPoint( float x, float y, float z, float (*matrix)[4])
{
	vec3_t point, result;
	point[0] = x;
	point[1] = y;
	point[2] = z;

	VectorTransform(point, matrix, result);

	gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
}


CObjectsManager::CObjectsManager( void )
{
	lastDrawTime = 0;
	objectsHead = NULL;
}

CObjectsManager::~CObjectsManager( void )
{
	Reset();
}

void CObjectsManager::Reset( void )
{
	while (objectsHead)
	{
		CObject *next;
		next = objectsHead->next;
		delete objectsHead;
		objectsHead = next;
	}

	lastDrawTime = 0;
}

void CObjectsManager::AddObject( CObject *obj )
{
/*	CObject *savedObjectsHead;
	savedObjectsHead = 	objectsHead;
	objectsHead = obj;
	objectsHead->next = savedObjectsHead;*/
	obj->next = objectsHead;
	objectsHead = obj;
	obj->birthTime = gEngfuncs.GetClientTime();
}

void CObjectsManager::Draw( void )
{
	float time = gEngfuncs.GetClientTime();
	float timedelta = time - lastDrawTime;

	CObject *currentObject, *prevObject;
	currentObject = objectsHead;
	prevObject = NULL;

	CalcViewMatrix( 1 ); // reset view matrix
	
//	int counter = 0;
	
	while(currentObject)
	{
		if (currentObject->Draw(time, timedelta))
		{
			CObject *myNext = currentObject->next;
			delete currentObject;
			currentObject = myNext;
			
			if (prevObject)
				prevObject->next = myNext;
			else
				objectsHead = myNext;
		}
		else
		{
			prevObject = currentObject;
			currentObject = currentObject->next;
//			counter++;
		}
	}
//	gEngfuncs.Con_Printf("drawn %i objects\n", counter);

	lastDrawTime = time;
}


// черт, где-то я видел подобню функцию, но забыл где. Придется писать...
void MakeUpRight( const float *normal, float *up, float *right )
{
	vec3_t temp;
	temp[0] = 0; temp[1] = 0; temp[2] = 1;
	if (normal[0] != 0 || normal[1] != 0)
	{
		CrossProduct( normal, temp, right );
		CrossProduct( right, normal, up );
	}
	else // вертикальный вектор
	{
		right[0] = 0; right[1] = 1; right[2] = 0;
		up[0] = 1; up[1] = 0; up[2] = 0;
	}
}

///////////////////////////////////////////////////////////////////////////
// Custom triapi objects add here
///////////////////////////////////////////////////////////////////////////


CSmokePuff::CSmokePuff( vec3_t origin, vec3_t normal, float life, float alpha, float minspeed, float maxspeed, float minsize, float maxsize, float maxofs, float r, float g, float b)
{
	vec3_t up, right;
	VectorClear(right);
	VectorClear(up);
	MakeUpRight(normal, up, right);
	m_fAlpha = alpha;
	m_fLife = life;
	m_fr = r; m_fg = g; m_fb = b;

//	gEngfuncs.Con_Printf("normal: %f, %f, %f\n", normal[0], normal[1], normal[2]);
//	gEngfuncs.Con_Printf("up vec: %f, %f, %f\n", up[0], up[1], up[2]);
//	gEngfuncs.Con_Printf("right vec: %f, %f, %f\n", right[0], right[1], right[2]);
	
	for(int i = 0; i < 5; i++)
	{
		VectorCopy(origin, m_puffsArray[i].origin);
		m_puffsArray[i].size = gEngfuncs.pfnRandomFloat(minsize, maxsize);
		VectorClear(m_puffsArray[i].velocity);
		VectorMA(m_puffsArray[i].velocity, gEngfuncs.pfnRandomFloat(minspeed, maxspeed), normal, m_puffsArray[i].velocity);
		VectorMA(m_puffsArray[i].velocity, gEngfuncs.pfnRandomFloat(-maxofs, maxofs), right, m_puffsArray[i].velocity);
		VectorMA(m_puffsArray[i].velocity, gEngfuncs.pfnRandomFloat(-maxofs, maxofs), up, m_puffsArray[i].velocity);
	}

//	gEngfuncs.Con_Printf("creating puff\n");
}

int CSmokePuff::Draw(float time, float frametime)
{
	float frac = (time - birthTime) / m_fLife;
	if (frac > 1)
		return 1; // delete object

	HSPRITE spr = SPR_Load("sprites/smokeball.spr");
	if (!spr)
	{
		gEngfuncs.Con_Printf("error: cant load sprite sprites/smokeball.spr\n");
		return 1;
	}

	CalcViewMatrix();

	struct model_s *pModel = (struct model_s *)gEngfuncs.GetSpritePointer( spr );
	int frame = (int)(pModel->numframes * frac);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->SpriteTexture( pModel, frame );
	gEngfuncs.pTriAPI->Color4f( m_fr, m_fg, m_fb, m_fAlpha );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	
	for(int i = 0; i < 5; i++)
	{
		float size = m_puffsArray[i].size * frac;
		VectorMA(m_puffsArray[i].origin, frametime, m_puffsArray[i].velocity, m_puffsArray[i].origin);
		
		g_viewMatrix[0][3] = m_puffsArray[i].origin[0]; // write origin to matrix
		g_viewMatrix[1][3] = m_puffsArray[i].origin[1];
		g_viewMatrix[2][3] = m_puffsArray[i].origin[2];

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		SetPoint(0, size ,size, g_viewMatrix);

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		SetPoint(0, size ,-size, g_viewMatrix);

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		SetPoint(0, -size ,-size, g_viewMatrix);

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		SetPoint(0, -size ,size, g_viewMatrix);
	}
	
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	return 0;
}