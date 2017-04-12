//
// written by BUzer for HL: Paranoia modification
//
//		2006

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "cdll_int.h"
#include "com_model.h"
#include "glmanager.h"
#include "entity_types.h"
#include "gl_texloader.h"

#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "parsemsg.h"
#include "gl_renderer.h"


#define MAX_DECALTEXTURES	128
#define MAX_CUSTOMDECALS	1024
#define MAX_STATICDECALS	256
#define MAX_GROUPENTRIES	64

#define MAX_DECAL_MSG_CACHE	256


class DecalGroupEntry
{
public:
	int gl_texid;
	int xsize, ysize;
	float overlay;
};

class DecalGroup
{
public:
	DecalGroup(const char *_name, int numelems, DecalGroupEntry *source);
	~DecalGroup();

	DecalGroupEntry *GetEntry(int num);
	const DecalGroupEntry *GetRandomDecal();

	char name[16];

// private:
	DecalGroup *pnext;
	DecalGroupEntry *pEntryArray;
	int size;
};

DecalGroup *pDecalGroupList = NULL;


DecalGroup::DecalGroup(const char *_name, int numelems, DecalGroupEntry *source)
{
	strcpy(name, _name);
	pEntryArray = new DecalGroupEntry[numelems];
	size = numelems;
	memcpy(pEntryArray, source, sizeof(DecalGroupEntry)*size);
	pnext = pDecalGroupList;
	pDecalGroupList = this;
}

DecalGroup::~DecalGroup()
{
	delete[] pEntryArray;
	if (pnext) delete pnext;
}

DecalGroupEntry* DecalGroup::GetEntry(int num)
{
	return &pEntryArray[num];

}

const DecalGroupEntry* DecalGroup::GetRandomDecal()
{
	if (size == 0)
		return NULL;

	if (size == 1)
		return &pEntryArray[0];

	int idx = gEngfuncs.pfnRandomLong( 0, size-1 );
	return &pEntryArray[idx];
}



DecalGroup* FindGroup(const char *_name)
{
	DecalGroup *plist = pDecalGroupList;
	while(plist)
	{
		if (!strcmp(plist->name, _name))
			return plist;

		plist = plist->pnext;
	}
	return NULL; // nothing found
}



typedef struct decaltexture_s {
	char name[16];
	int gl_texid;
} decaltexture;

typedef struct customdecal_s {
	msurface_t *surface;
	vec3_t point;
	vec3_t normal;
	const DecalGroupEntry *texinfo;
} customdecal;


struct decal_msg_cache
{
	vec3_t	pos;
	vec3_t	normal;
	char	name[16];
	int		persistent;
};



decaltexture decaltextures[MAX_DECALTEXTURES];
customdecal decals[MAX_CUSTOMDECALS];
customdecal staticdecals[MAX_STATICDECALS];
decal_msg_cache msgcache[MAX_DECAL_MSG_CACHE];

int numdecaltextures = 0;
int numdecals = 0;
int numstaticdecals = 0;
int curdecal = 0;
int cachedecals = 0;

int decalrendercounter = 0;
int staticdecalrendercounter = 0;

cvar_t *cv_customdecals;
cvar_t *cv_decalsdebug;



// ===========================
// Math
// ===========================

// пересечение линии с плоскостью
void FindIntersectionPoint( const vec3_t &p1, const vec3_t &p2, const vec3_t &normal, const vec3_t &planepoint, vec3_t &newpoint )
{
	vec3_t planevec;
	vec3_t linevec;
	float planedist, linedist;

	VectorSubtract( planepoint, p1, planevec );
	VectorSubtract( p2, p1, linevec );
	planedist = DotProduct( normal, planevec );
	linedist = DotProduct( normal, linevec );

	if (linedist != 0)
	{
		VectorMA( p1, planedist/linedist, linevec, newpoint );
		return;
	}
	VectorClear( newpoint ); // error - прямая параллельна плоскости
}



// нормаль смотрит в сторону неотсекаемой части полигона
int ClipPolygonByPlane (const vec3_t *arrIn, int numpoints, vec3_t normal, vec3_t planepoint, vec3_t *arrOut)
{
	int i, cur, prev;
	int first = -1;
	int outCur = 0;
	float dots[64];
	for (i = 0; i < numpoints; i++)
	{
		vec3_t vecDir;
		VectorSubtract( arrIn[i], planepoint, vecDir );
		dots[i] = DotProduct( vecDir, normal );
		if (dots[i] > 0) first = i;
	}
	if (first == -1) return 0; // плоскость отсекла весь многоугольник

	VectorCopy( arrIn[first], arrOut[outCur] );
	outCur++;

	cur = first + 1;
	if (cur == numpoints) cur = 0;
	while (cur != first)
	{
		if (dots[cur] > 0)
		{
			VectorCopy( arrIn[cur], arrOut[outCur] );
			cur++; outCur++;
			if (cur == numpoints) cur = 0;
		}
		else
			break;
	}

	if (cur == first) return outCur; // ничего не отсекается этой плоскостью

	if (dots[cur] < 0)
	{
		vec3_t newpoint;
		if (cur > 0) prev = cur-1;
		else prev = numpoints - 1;
		FindIntersectionPoint( arrIn[prev], arrIn[cur], normal, planepoint, newpoint );
		VectorCopy( newpoint, arrOut[outCur] );
	}
	else
	{
		VectorCopy( arrIn[cur], arrOut[outCur] );
	}
	outCur++;
	cur++;
	if (cur == numpoints) cur = 0;

	while (dots[cur] < 0)
	{
		cur++;
		if (cur == numpoints) cur = 0;
	}

	if (cur > 0) prev = cur-1;
	else prev = numpoints - 1;
	if (dots[cur] > 0 && dots[prev] < 0)
	{
		vec3_t newpoint;
		FindIntersectionPoint( arrIn[prev], arrIn[cur], normal, planepoint, newpoint );
		VectorCopy( newpoint, arrOut[outCur] );
		outCur++;
	}

	while (cur != first)
	{
		VectorCopy( arrIn[cur], arrOut[outCur] );
		cur++; outCur++;
		if (cur == numpoints) cur = 0;
	}
	return outCur;
}

void GetUpRight(vec3_t forward, vec3_t &up, vec3_t &right)
{
	VectorClear(up);
	if (forward.x || forward.y)
		up.z = 1;
	else
		up.x = 1;

	right = CrossProduct(forward, up);
	VectorNormalize(right);
	up = CrossProduct(forward, right);
	VectorNormalize(up);
}



// ===========================
// Decals loading
// ===========================

int LoadDecalTexture(const char *texname)
{
	for (int i = 0; i < numdecaltextures; i++)
	{
		if (!strcmp(decaltextures[i].name, texname))
			return decaltextures[i].gl_texid;
	}

	if (numdecaltextures >= MAX_DECALTEXTURES)
	{
		ConLog("Too many entries in decal info file (%d max)\n", MAX_DECALTEXTURES);
		return 0;
	}

	char path[256];	
	sprintf(path, "gfx/decals/%s.tga", texname);
	int newid = CreateTexture(path, MIPS_YES);
	if (!newid)
	{
		ConLog("Missing decal texture %s!\n", path);
		return 0;
	}

//	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
//	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	strcpy(decaltextures[numdecaltextures].name, texname);
	decaltextures[numdecaltextures].gl_texid = newid;
	numdecaltextures++;
	return newid;
}

void LoadDecals()
{
	if (!cv_customdecals->value)
		return;

	if (!gl.IsGLAllowed())
		return;

	// only once
	static int called = 0;
	if (called) return;
	called = 1;

	Log("\n>> Loading decals\n");

	char *pfile = (char *)gEngfuncs.COM_LoadFile("gfx/decals/decalinfo.txt", 5, NULL);
	if (!pfile)
	{
		ConLog("Cannot open file \"gfx/decals/decalinfo.txt\"\n");
		return;
	}

	int counter = 0;
	char *ptext = pfile;
	while(1)
	{
		// store position where group names recorded
		char *groupnames = ptext;

		// loop until we'll find decal names
		int numgroups = 0;
		char token[256];
		while(1)
		{
			ptext = gEngfuncs.COM_ParseFile(ptext, token);
			if (!ptext) goto getout;
			if (token[0] == '{') break;
			numgroups++;
		}

		DecalGroupEntry tempentries[MAX_GROUPENTRIES];
		int numtemp = 0;
		while(1)
		{
			char sz_xsize[64];
			char sz_ysize[64];
			char sz_overlay[64];
			ptext = gEngfuncs.COM_ParseFile(ptext, token);
			if (!ptext) goto getout;
			if (token[0] == '}') break;

			if (numtemp >= MAX_GROUPENTRIES)
			{
				ConLog("Too many decals in group (%d max) - skipping %s\n", MAX_GROUPENTRIES, token);
				continue;
			}

			ptext = gEngfuncs.COM_ParseFile(ptext, sz_xsize);
			if (!ptext) goto getout;
			ptext = gEngfuncs.COM_ParseFile(ptext, sz_ysize);
			if (!ptext) goto getout;
			ptext = gEngfuncs.COM_ParseFile(ptext, sz_overlay);
			if (!ptext) goto getout;

			if (strlen(token) > 16)
			{
				ConLog("%s - got too large token when parsing decal info file\n", token);
				continue;
			}

			int id = LoadDecalTexture(token);
			if (!id)
				continue;

			tempentries[numtemp].gl_texid = id;
			tempentries[numtemp].xsize = atof(sz_xsize) / 2;
			tempentries[numtemp].ysize = atof(sz_ysize) / 2;
			tempentries[numtemp].overlay = atof(sz_overlay) * 2;
			numtemp++;
		}

		// get back to group names

		for (int i = 0; i < numgroups; i++)
		{
			groupnames = gEngfuncs.COM_ParseFile(groupnames, token);
			if (!numtemp)
			{
				ConLog("warning: got empty decal group: %s\n", token);
				continue;
			}

			new DecalGroup(token, numtemp, tempentries);
			counter++;
		}
	}

getout:

	gEngfuncs.COM_FreeFile( pfile );
	ConLog("%d decal groups created\n", counter);
}



// ===========================
// Decals creation
// ===========================
customdecal *AllocDecal()
{
	customdecal *ret = &decals[curdecal];

	if (numdecals < MAX_CUSTOMDECALS)
		numdecals++;

	curdecal++;
	if (curdecal == MAX_CUSTOMDECALS)
		curdecal = 0; // get decals from tail

	return ret;
}


// static decals, used for level detalization, placed by mapper
customdecal *AllocStaticDecal()
{
	if (numstaticdecals < MAX_STATICDECALS)
	{
		customdecal *ret = &staticdecals[numstaticdecals];
		numstaticdecals++;
		return ret;
	}
	return NULL;
}



void CreateDecal(vec3_t endpos, vec3_t pnormal, const char *name, int persistent = 0)
{
	if (!cv_customdecals->value)
		return;

	LoadDecals();

	model_t *world = gEngfuncs.GetEntityByIndex(0)->model;
	if (!world)
	{
		// CreateDecal often being called from server messages. Messages about static
		// decals are sent during initialization, and in some cases i cant access
		// world model when message received (this happens during level transitions).
		// So instead of creating decal now, i will add them to cache -
		// they will be created when drawing starts.

		if (cachedecals >= MAX_DECAL_MSG_CACHE)
		{
			ConLog("Too many decals in cache!\n");
			return;
		}

		strcpy(msgcache[cachedecals].name, name);
		msgcache[cachedecals].normal = pnormal;
		msgcache[cachedecals].pos = endpos;
		msgcache[cachedecals].persistent = persistent;
		cachedecals++;
	//	CONPRINT("decal cached\n");
		return;
	}

	DecalGroup *group = FindGroup(name);
	if (!group)
	{
	//	gEngfuncs.Con_Printf("decal group %s not found\n", name);
		// buz: this msg will flood in sw and d3d video modes
		return;
	}

	const DecalGroupEntry *texptr = group->GetRandomDecal();
	if (!texptr)
	{
	//	gEngfuncs.Con_Printf("no decals in group %s!\n", name);
		return;
	}

	int xsize = texptr->xsize;
	int ysize = texptr->ysize;

	vec3_t right, up;
	GetUpRight(pnormal, up, right);
	float radius = sqrt(xsize*xsize + ysize*ysize);

	float texc_orig_x = DotProduct(endpos, right);
	float texc_orig_y = DotProduct(endpos, up);

//	msurface_t* surfaces = world->surfaces;
	msurface_t* surfaces = &world->surfaces[world->firstmodelsurface];
//	for (int i = 0; i < world->numsurfaces; i++)
	for (int i = 0; i < world->nummodelsurfaces; i++)
	{
		vec3_t norm;
		float ndist = surfaces[i].plane->dist;
		VectorCopy(surfaces[i].plane->normal, norm);
		if (surfaces[i].flags & SURF_PLANEBACK)
		{
			VectorInverse(norm);
			ndist *= -1;
		}

		float normdot = DotProduct(pnormal, norm);
		if (normdot < 0.7)
			continue;

		float dist = DotProduct(norm, endpos);
		dist = dist - ndist;
		if (dist < 0) dist *= -1;
		if (dist > radius)
			continue;

		if (surfaces[i].flags & (SURF_DRAWSKY|SURF_DRAWTURB))
			continue;

		if (!surfaces[i].samples)
			continue;

		// execute part of drawing process to see if polygon completely clipped

		glpoly_t	*p = surfaces[i].polys;
		float		*v = p->verts[0];

		vec3_t array1[64];
		vec3_t array2[64];
		for (int j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
		{
			VectorCopy( v, array1[j] );
		}

		int nv;
		vec3_t planepoint;
		VectorMA(endpos, -xsize, right, planepoint);
		nv = ClipPolygonByPlane (array1, p->numverts, right, planepoint, array2);

		VectorMA(endpos, xsize, right, planepoint);
		nv = ClipPolygonByPlane (array2, nv, right*-1, planepoint, array1);

		VectorMA(endpos, -ysize, up, planepoint);
		nv = ClipPolygonByPlane (array1, nv, up, planepoint, array2);

		VectorMA(endpos, ysize, up, planepoint);
		nv = ClipPolygonByPlane (array2, nv, up*-1, planepoint, array1);

		if (!nv)
			continue; // no vertexes left after clipping

		customdecal *newdecal = NULL;

		if (persistent)
		{
			newdecal = AllocStaticDecal();
			if (!newdecal)
			{
				CONPRINT("Warning: too many static decals on level!\n");
				return;
			}
		}
		else
		{
			// look other decals on this surface - is someone too close?
			// if so, clear him

			for (int k = 0; k < numdecals; k++)
			{
				if ((decals[k].surface == &surfaces[i]) &&
					(decals[k].texinfo->xsize == texptr->xsize) &&
					(decals[k].texinfo->ysize == texptr->ysize))
				{
					float texc_x = DotProduct(decals[k].point, right) - texc_orig_x;
					float texc_y = DotProduct(decals[k].point, up) - texc_orig_y;
					if (texc_x < 0) texc_x *= -1;
					if (texc_y < 0) texc_y *= -1;
					if (texc_x < (float)texptr->xsize*texptr->overlay &&
						texc_y < (float)texptr->ysize*texptr->overlay)
					{
						newdecal = &decals[k];
					//	gEngfuncs.Con_Printf("paste on others place\n");
						break;
					}
				}
			}

			if (!newdecal)
				newdecal = AllocDecal();
		}

		newdecal->surface = &surfaces[i];
		VectorCopy(endpos, newdecal->point);
		VectorCopy(pnormal, newdecal->normal);
		newdecal->texinfo = texptr;
	//	gEngfuncs.Con_Printf("pasted decal %s\n", name);
	}
}


void CreateDecal(pmtrace_t &tr, const char *name, int persistent = 0)
{
	if (tr.ent)	return; // not on entites for now
	if (tr.allsolid || tr.fraction == 1.0) return;

	CreateDecal(tr.endpos, tr.plane.normal, name, persistent);
}

void CreateDecalTrace(vec3_t start, vec3_t end, const char *name, int persistent = 0)
{
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( start, end, PM_NORMAL, -1, &tr );
	CreateDecal(tr, name, persistent);
}


void CreateCachedDecals()
{
	if (!gEngfuncs.GetEntityByIndex(0)->model)
		return; // world model is inaccesible!
	// (heh, if it will happen during rendering, the paranoia renderer will crash much earlier :)

	for (int i = 0; i < cachedecals; i++)
	{
		CreateDecal(msgcache[i].pos, msgcache[i].normal, msgcache[i].name, msgcache[i].persistent);
	//	CONPRINT("decal created from cache\n");
	}

	cachedecals = 0;
}


// debugging feature
extern vec3_t	render_origin;
extern vec3_t	render_angles;

void PasteViewDecal()
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage: pastedecal <decal name>\n" );
		return;
	}

	vec3_t dir;
	AngleVectors(render_angles, dir, NULL, NULL);
	CreateDecalTrace(render_origin, render_origin + (dir * 1024), gEngfuncs.Cmd_Argv(1));
}



// ===========================
// Decals drawing
//
//		Uses surface visframe information from custom renderer's pass
// ===========================

int DrawSingleDecal(customdecal *decal)
{
	if (decal->surface->visframe != framecount)
		return FALSE;

	vec3_t right, up;
	GetUpRight(decal->normal, up, right);

	float texc_orig_x = DotProduct(decal->point, right);
	float texc_orig_y = DotProduct(decal->point, up);
	int xsize = decal->texinfo->xsize;
	int ysize = decal->texinfo->ysize;

	glpoly_t	*p = decal->surface->polys;
	float		*v = p->verts[0];

	vec3_t array1[64];
	vec3_t array2[64];
	for (int j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
	{
		VectorCopy( v, array1[j] );
	}

	int nv;
	vec3_t planepoint;
	VectorMA(decal->point, -xsize, right, planepoint);
	nv = ClipPolygonByPlane (array1, p->numverts, right, planepoint, array2);

	VectorMA(decal->point, xsize, right, planepoint);
	nv = ClipPolygonByPlane (array2, nv, right*-1, planepoint, array1);

	VectorMA(decal->point, -ysize, up, planepoint);
	nv = ClipPolygonByPlane (array1, nv, up, planepoint, array2);

	VectorMA(decal->point, ysize, up, planepoint);
	nv = ClipPolygonByPlane (array2, nv, up*-1, planepoint, array1);

	if (!nv)
		return FALSE; // shouldn't happen..
	
	Bind2DTexture(GL_TEXTURE0_ARB,  decal->texinfo->gl_texid);
	gl.glBegin(GL_POLYGON);
	for (int k=0; k<nv; k++, v+= VERTEXSIZE)
	{
		float texc_x = (DotProduct(array1[k], right) - texc_orig_x)/xsize;
		float texc_y = (DotProduct(array1[k], up) - texc_orig_y)/ysize;
		texc_x = (texc_x + 1) / 2;
		texc_y = (texc_y + 1) / 2;

		gl.glMultiTexCoord2fARB (GL_TEXTURE0_ARB, texc_x, texc_y);
		gl.glVertex3fv (array1[k]);
	}
	gl.glEnd ();
	return TRUE;
}


void DrawDecals()
{
	decalrendercounter = 0;
	staticdecalrendercounter = 0;

	CreateCachedDecals();

	if ((!numdecals && !numstaticdecals) || !cv_customdecals->value)
		return;

	gl.glEnable(GL_BLEND);
	gl.glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
	gl.glDepthMask(GL_FALSE);

	gl.glPolygonOffset ( -1, -1 );
	gl.glEnable ( GL_POLYGON_OFFSET_FILL );

	// disable texturing on all units except first
/*	gl.glActiveTextureARB( GL_TEXTURE0_ARB );
	gl.glEnable(GL_TEXTURE_2D);
	gl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	gl.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	gl.glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	
	gl.glActiveTextureARB( GL_TEXTURE1_ARB );
	gl.glDisable(GL_TEXTURE_2D);
	gl.glActiveTextureARB( GL_TEXTURE2_ARB );
	gl.glDisable(GL_TEXTURE_2D);
	gl.glActiveTextureARB( GL_TEXTURE3_ARB );
	gl.glDisable(GL_TEXTURE_2D);
	gl.glActiveTextureARB( GL_TEXTURE0_ARB );*/

	SetTexEnvs(ENVSTATE_REPLACE);

	for (int i = 0; i < numdecals; i++)
	{
		if (DrawSingleDecal(&decals[i]))
			decalrendercounter++;
	}

	for (i = 0; i < numstaticdecals; i++)
	{
		if (DrawSingleDecal(&staticdecals[i]))
			staticdecalrendercounter++;
	}

	gl.glDisable ( GL_POLYGON_OFFSET_FILL );
}


int MsgCustomDecal(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	vec3_t pos, normal;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	normal.x = READ_COORD();
	normal.y = READ_COORD();
	normal.z = READ_COORD();
	int persistent = READ_BYTE();
//	CONPRINT("fgdsfgdsd %d\n", persistent);

	CreateDecal(pos, normal, READ_STRING(), persistent);
	return 1;
}


void DeleteDecals()
{
	numdecals = 0;
	curdecal = 0;
	numstaticdecals = 0;
}
/*
void HowManyDecals()
{
	gEngfuncs.Con_Printf("%d decals in memory, %d rendered last time\n", numdecals, decalrendercounter);
}*/

void InitDecals()
{
	gEngfuncs.pfnAddCommand ("pastedecal", PasteViewDecal );
	gEngfuncs.pfnAddCommand ("decalsclear", DeleteDecals );
//	gEngfuncs.pfnAddCommand ("howmanydecals", HowManyDecals );
	cv_customdecals = gEngfuncs.pfnRegisterVariable("gl_customdecals","1",0);
	cv_decalsdebug = gEngfuncs.pfnRegisterVariable("gl_decalstatus","0",0);
	gEngfuncs.pfnHookUserMsg("customdecal", MsgCustomDecal);
}

void DecalsShutdown()
{
	if (pDecalGroupList) delete pDecalGroupList;
}

void DecalsPrintDebugInfo()
{
	if (cv_decalsdebug->value)
	{
		char msg[256];
		sprintf(msg, "%d decals in memory, %d rendered\n", numdecals, decalrendercounter);
		DrawConsoleString( XRES(10), YRES(100), msg );
		sprintf(msg, "%d static decals in memory, %d rendered\n", numstaticdecals, staticdecalrendercounter);
		DrawConsoleString( XRES(10), YRES(115), msg );
	}
}