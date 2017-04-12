/*************************************
*	
*	
*
*
**************************************/



#ifndef __TRIAPIOBJECTS_H
#define __TRIAPIOBJECTS_H

// base class for all TriApi custom tempobjects
class CObject
{
friend class CObjectsManager;
public:
	virtual int Draw( float time, float frametime ) = 0; // return 1 to delete object
//	virtual ~CObject() {};
	float	birthTime;

protected:
	CObject *next;
};


class CObjectsManager
{
public:
	CObjectsManager();
	~CObjectsManager();

	void AddObject( CObject *obj );
	void Reset( void );
	void Draw( void );

private:
	CObject *objectsHead;
	float lastDrawTime;
};

extern CObjectsManager g_objmanager;
extern float g_viewMatrix[3][4];
extern void CalcViewMatrix( int reset );



///////////////////////////////////////////////////////////////////////////
// Custom triapi objects add here
///////////////////////////////////////////////////////////////////////////


class CSmokePuff : public CObject
{
public:
	// construction
	CSmokePuff( vec3_t origin, vec3_t normal, float life, float alpha, float minspeed, float maxspeed, float minsize, float maxsize, float maxofs, float r=1.0, float g=1.0, float b=1.0 );
	~CSmokePuff() {gEngfuncs.Con_Printf("deleting puff\n");};
	int Draw( float time, float frametime );

	struct puff_s {
		vec3_t	origin;
		vec3_t	velocity;
		float	size;
	} m_puffsArray[5];

	float m_fAlpha;
	float m_fLife;

	float m_fr, m_fg, m_fb;
};


#endif