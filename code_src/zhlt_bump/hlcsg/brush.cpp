#include "csg.h"

plane_t         g_mapplanes[MAX_MAP_PLANES];
int             g_nummapplanes;

#define DIST_EPSILON   0.01

/*
 * =============
 * FindIntPlane
 * 
 * Returns which plane number to use for a given integer defined plane.
 * 
 * =============
 */
int             FindIntPlane(const vec_t* const normal, const vec_t* const origin)
{
    int             i, j;
    plane_t*        p;
    plane_t         temp;
    vec_t           t;
    bool            locked;

    p = g_mapplanes;
    locked = false;
    i = 0;

    while (1)
    {
        if (i == g_nummapplanes)
        {
            if (!locked)
            {
                locked = true;
                ThreadLock();                              // make sure we don't race
            }
            if (i == g_nummapplanes)
            {
                break;                                     // we didn't race
            }
        }

        t = 0;                                             // Unrolled loop
        t += (origin[0] - p->origin[0]) * normal[0];
        t += (origin[1] - p->origin[1]) * normal[1];
        t += (origin[2] - p->origin[2]) * normal[2];

        if (fabs(t) < DIST_EPSILON)
        {                                                  // on plane
            // see if the normal is forward, backwards, or off
            for (j = 0; j < 3; j++)
            {
                if (fabs(normal[j] - p->normal[j]) > NORMAL_EPSILON)
                {
                    break;
                }
            }
            if (j == 3)
            {
                if (locked)
                {
                    ThreadUnlock();
                }
                return i;
            }
        }

        i++;
        p++;
    }

    hlassert(locked);

    // create a new plane
    p->origin[0] = origin[0];
    p->origin[1] = origin[1];
    p->origin[2] = origin[2];

    (p + 1)->origin[0] = origin[0];
    (p + 1)->origin[1] = origin[1];
    (p + 1)->origin[2] = origin[2];

    p->normal[0] = normal[0];
    p->normal[1] = normal[1];
    p->normal[2] = normal[2];

    (p + 1)->normal[0] = -normal[0];
    (p + 1)->normal[1] = -normal[1];
    (p + 1)->normal[2] = -normal[2];

    hlassume(g_nummapplanes < MAX_MAP_PLANES, assume_MAX_MAP_PLANES);

    VectorNormalize(p->normal);

    p->type = (p + 1)->type = PlaneTypeForNormal(p->normal);

    p->dist = DotProduct(origin, p->normal);
    VectorSubtract(vec3_origin, p->normal, (p + 1)->normal);
    (p + 1)->dist = -p->dist;

    // allways put axial planes facing positive first
    if (p->type <= last_axial)
    {
        if (normal[0] < 0 || normal[1] < 0 || normal[2] < 0)
        {
            // flip order
            temp = *p;
            *p = *(p + 1);
            *(p + 1) = temp;
            g_nummapplanes += 2;
            ThreadUnlock();
            return i + 1;
        }
    }

    g_nummapplanes += 2;
    ThreadUnlock();
    return i;
}

int             PlaneFromPoints(const vec_t* const p0, const vec_t* const p1, const vec_t* const p2)
{
    vec3_t          v1, v2;
    vec3_t          normal;

    VectorSubtract(p0, p1, v1);
    VectorSubtract(p2, p1, v2);
    CrossProduct(v1, v2, normal);
    if (VectorNormalize(normal))
    {
        return FindIntPlane(normal, p0);
    }
    return -1;
}

/*
 * =============================================================================
 * 
 * TURN BRUSHES INTO GROUPS OF FACES
 * 
 * =============================================================================
 */

void            ScaleUpIVector(int* iv, const int min)
{
    int             i;
    int             largest, scale;

    largest = 0;
    for (i = 0; i < 3; i++)
    {
        if (abs(iv[i]) > largest)
        {
            largest = abs(iv[i]);
        }
    }

    scale = (min + largest - 1) / largest;
    VectorScale(iv, scale, iv);
}

/*
 * ==============================================================================
 * 
 * BEVELED CLIPPING HULL GENERATION
 * 
 * This is done by brute force, and could easily get a lot faster if anyone cares.
 * ==============================================================================
 */

#define	MAX_HULL_POINTS	32
#define	MAX_HULL_EDGES	64

typedef struct
{
    brush_t*        b;
    int             hullnum;
    int             num_hull_points;
    vec3_t          hull_points[MAX_HULL_POINTS];
    vec3_t          hull_corners[MAX_HULL_POINTS * 8];
    int             num_hull_edges;
    int             hull_edges[MAX_HULL_EDGES][2];
} expand_t;

/*
 * =============
 * IPlaneEquiv
 * 
 * =============
 */
bool            IPlaneEquiv(const plane_t* const p1, const plane_t* const p2)
{
    vec_t           t;
    int             j;

    // see if origin is on plane
    t = 0;
    for (j = 0; j < 3; j++)
    {
        t += (p2->origin[j] - p1->origin[j]) * p2->normal[j];
    }
    if (fabs(t) > DIST_EPSILON)
    {
        return false;
    }

    // see if the normal is forward, backwards, or off
    for (j = 0; j < 3; j++)
    {
        if (fabs(p2->normal[j] - p1->normal[j]) > NORMAL_EPSILON)
        {
            break;
        }
    }
    if (j == 3)
    {
        return true;
    }

    for (j = 0; j < 3; j++)
    {
        if (fabs(p2->normal[j] - p1->normal[j]) > NORMAL_EPSILON)
        {
            break;
        }
    }
    if (j == 3)
    {
        return true;
    }

    return false;
}

/*
 * ============
 * AddBrushPlane
 * =============
 */
void            AddBrushPlane(const expand_t* const ex, const plane_t* const plane)
{
    plane_t*        pl;
    bface_t*        f;
    bface_t*        nf;
    brushhull_t*    h;

    h = &ex->b->hulls[ex->hullnum];
    // see if the plane has allready been added
    for (f = h->faces; f; f = f->next)
    {
        pl = f->plane;
        if (IPlaneEquiv(plane, pl))
        {
            return;
        }
    }

    nf = (bface_t*)Alloc(sizeof(*nf));                               // TODO: This leaks
    nf->planenum = FindIntPlane(plane->normal, plane->origin);
    nf->plane = &g_mapplanes[nf->planenum];
    nf->next = h->faces;
    nf->contents = CONTENTS_EMPTY;
    h->faces = nf;

    nf->texinfo = 0;                                       // all clip hulls have same texture
}

// =====================================================================================
//  ExpandBrush
// =====================================================================================
void            ExpandBrush(brush_t* b, const int hullnum)
{
    int             x;
    int             s;
    int             corner;
    bface_t*        brush_faces;
    bface_t*        f;
    bface_t*        nf;
    plane_t*        p;
    plane_t         plane;
    vec3_t          origin;
    vec3_t          normal;
    expand_t        ex;
    brushhull_t*    h;
    bool            axial;

    brush_faces = b->hulls[0].faces;
    h = &b->hulls[hullnum];

    ex.b = b;
    ex.hullnum = hullnum;
    ex.num_hull_points = 0;
    ex.num_hull_edges = 0;

    // expand all of the planes

    axial = true;

    // for each of this brushes faces
    for (f = brush_faces; f; f = f->next)
    {
        p = f->plane;
        if (p->type > last_axial) // ajm: last_axial == (planetypes enum)plane_z == (2)
        {
            axial = false;                                 // not an xyz axial plane
        }

        VectorCopy(p->origin, origin);
        VectorCopy(p->normal, normal);

        for (x = 0; x < 3; x++)
        {
            if (p->normal[x] > 0)
            {
                corner = g_hull_size[hullnum][1][x];
            }
            else if (p->normal[x] < 0)
            {
                corner = -g_hull_size[hullnum][0][x];
            }
            else
            {
                corner = 0;
            }
            origin[x] += p->normal[x] * corner;
        }
        nf = (bface_t*)Alloc(sizeof(*nf));                           // TODO: This leaks

        nf->planenum = FindIntPlane(normal, origin);
        nf->plane = &g_mapplanes[nf->planenum];
        nf->next = h->faces;
        nf->contents = CONTENTS_EMPTY;
        h->faces = nf;
        nf->texinfo = 0;                        // all clip hulls have same texture
//        nf->texinfo = f->texinfo;               // Hack to view clipping hull with textures (might crash halflife)
    }

    // if this was an axial brush, we are done
    if (axial)
    {
        return;
    }

    // add any axis planes not contained in the brush to bevel off corners
    for (x = 0; x < 3; x++)
    {
        for (s = -1; s <= 1; s += 2)
        {
            // add the plane
            VectorCopy(vec3_origin, plane.normal);
            plane.normal[x] = s;
            if (s == -1)
            {
                VectorAdd(b->hulls[0].bounds.m_Mins, g_hull_size[hullnum][0], plane.origin);
            }
            else
            {
                VectorAdd(b->hulls[0].bounds.m_Maxs, g_hull_size[hullnum][1], plane.origin);
            }
            AddBrushPlane(&ex, &plane);
        }
    }
}


// =====================================================================================
//  MakeHullFaces
// =====================================================================================
void            MakeHullFaces(const brush_t* const b, brushhull_t *h)
{
    bface_t*        f;
    bface_t*        f2;

restart:
    h->bounds.reset();

    // for each face in this brushes hull
    for (f = h->faces; f; f = f->next)
    {
        Winding* w = new Winding(f->plane->normal, f->plane->dist);
        for (f2 = h->faces; f2; f2 = f2->next)
        {
            if (f == f2)
            {
                continue;
            }
            const plane_t* p = &g_mapplanes[f2->planenum ^ 1];
            if (!w->Chop(p->normal, p->dist))   // Nothing left to chop (getArea will return 0 for us in this case for below)
            {
                break;
            }
        }
        if (w->getArea() < 0.1)
        {
            delete w;
            if (h->faces == f)
            {
                h->faces = f->next;
            }
            else
            {
                for (f2 = h->faces; f2->next != f; f2 = f2->next)
                {
                    ;
                }
                f2->next = f->next;
            }
            goto restart;
        }
        else
        {
            f->w = w;
            f->contents = CONTENTS_EMPTY;
            unsigned int    i;
            for (i = 0; i < w->m_NumPoints; i++)
            {
                h->bounds.add(w->m_Points[i]);
            }
        }
    }

    unsigned int    i;
    for (i = 0; i < 3; i++)
    {
        if (h->bounds.m_Mins[i] < -BOGUS_RANGE / 2 || h->bounds.m_Maxs[i] > BOGUS_RANGE / 2)
        {
            Fatal(assume_BRUSH_OUTSIDE_WORLD, "Entity %i, Brush %i: outside world(+/-%d): (%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)",
                  b->entitynum, b->brushnum,
                  BOGUS_RANGE / 2, 
                  h->bounds.m_Mins[0], h->bounds.m_Mins[1], h->bounds.m_Mins[2], 
                  h->bounds.m_Maxs[0], h->bounds.m_Maxs[1], h->bounds.m_Maxs[2]);
        }
    }
}

// =====================================================================================
//  MakeBrushPlanes
// =====================================================================================
bool            MakeBrushPlanes(brush_t* b)
{
    int             i;
    int             j;
    int             planenum;
    side_t*         s;
    bface_t*        f;
    vec3_t          origin;

    //
    // if the origin key is set (by an origin brush), offset all of the values
    //
    GetVectorForKey(&g_entities[b->entitynum], "origin", origin);

    //
    // convert to mapplanes
    //
    // for each side in this brush
    for (i = 0; i < b->numsides; i++)
    {
        s = &g_brushsides[b->firstside + i];
        for (j = 0; j < 3; j++)
        {
            VectorSubtract(s->planepts[j], origin, s->planepts[j]);
        }
        planenum = PlaneFromPoints(s->planepts[0], s->planepts[1], s->planepts[2]);
        if (planenum == -1)
        {
            Fatal(assume_PLANE_WITH_NO_NORMAL, "Entity %i, Brush %i, Side %i: plane with no normal", b->entitynum, b->brushnum, i);
        }

        //
        // see if the plane has been used already
        //
        for (f = b->hulls[0].faces; f; f = f->next)
        {
            if (f->planenum == planenum || f->planenum == (planenum ^ 1))
            {
                Fatal(assume_BRUSH_WITH_COPLANAR_FACES, "Entity %i, Brush %i, Side %i: has a coplanar plane at (%.0f, %.0f, %.0f), texture %s",
                      b->entitynum, b->brushnum, i, s->planepts[0][0] + origin[0], s->planepts[0][1] + origin[1],
                      s->planepts[0][2] + origin[2], s->td.name);
            }
        }

        f = (bface_t*)Alloc(sizeof(*f));                             // TODO: This leaks

        f->planenum = planenum;
        f->plane = &g_mapplanes[planenum];
        f->next = b->hulls[0].faces;
        b->hulls[0].faces = f;
        f->texinfo = g_onlyents ? 0 : TexinfoForBrushTexture(f->plane, &s->td, origin);
    }

    return true;
}


// =====================================================================================
//  TextureContents
// =====================================================================================
static contents_t TextureContents(const char* const name)
{
    if (!strncasecmp(name, "sky", 3))
        return CONTENTS_SKY;

    if (!strncasecmp(name + 1, "!lava", 5))
        return CONTENTS_LAVA;

    if (!strncasecmp(name + 1, "!slime", 6))
        return CONTENTS_SLIME;

    if (!strncasecmp(name, "!cur_90", 7))
        return CONTENTS_CURRENT_90;
    if (!strncasecmp(name, "!cur_0", 6))
        return CONTENTS_CURRENT_0;
    if (!strncasecmp(name, "!cur_270", 8))
        return CONTENTS_CURRENT_270;
    if (!strncasecmp(name, "!cur_180", 8))
        return CONTENTS_CURRENT_180;
    if (!strncasecmp(name, "!cur_up", 7))
        return CONTENTS_CURRENT_UP;
    if (!strncasecmp(name, "!cur_dwn", 8))
        return CONTENTS_CURRENT_DOWN;

    if (name[0] == '!')
        return CONTENTS_WATER;

    if (!strncasecmp(name, "origin", 6))
        return CONTENTS_ORIGIN;

    if (!strncasecmp(name, "clip", 4))
        return CONTENTS_CLIP;

    if (!strncasecmp(name, "hint", 4))
        return CONTENTS_HINT;
    if (!strncasecmp(name, "skip", 4))
        return CONTENTS_HINT;

    if (!strncasecmp(name, "translucent", 11))
        return CONTENTS_TRANSLUCENT;

    if (name[0] == '@')
        return CONTENTS_TRANSLUCENT;

#ifdef ZHLT_NULLTEX // AJM: 
	if (!strncasecmp(name, "null", 4))
        return CONTENTS_NULL;
#endif

    return CONTENTS_SOLID;
}

// =====================================================================================
//  ContentsToString
// =====================================================================================
const char*     ContentsToString(const contents_t type)
{
    switch (type)
    {
    case CONTENTS_EMPTY:
        return "EMPTY";
    case CONTENTS_SOLID:
        return "SOLID";
    case CONTENTS_WATER:
        return "WATER";
    case CONTENTS_SLIME:
        return "SLIME";
    case CONTENTS_LAVA:
        return "LAVA";
    case CONTENTS_SKY:
        return "SKY";
    case CONTENTS_ORIGIN:
        return "ORIGIN";
    case CONTENTS_CLIP:
        return "CLIP";
    case CONTENTS_CURRENT_0:
        return "CURRENT_0";
    case CONTENTS_CURRENT_90:
        return "CURRENT_90";
    case CONTENTS_CURRENT_180:
        return "CURRENT_180";
    case CONTENTS_CURRENT_270:
        return "CURRENT_270";
    case CONTENTS_CURRENT_UP:
        return "CURRENT_UP";
    case CONTENTS_CURRENT_DOWN:
        return "CURRENT_DOWN";
    case CONTENTS_TRANSLUCENT:
        return "TRANSLUCENT";
    case CONTENTS_HINT:
        return "HINT";

#ifdef ZHLT_NULLTEX // AJM
    case CONTENTS_NULL:
        return "NULL";
#endif

#ifdef ZHLT_DETAIL // AJM
    case CONTENTS_DETAIL:
        return "DETAIL";
#endif

    default:
        return "UNKNOWN";
    }
}

// =====================================================================================
//  CheckBrushContents
//      Perfoms abitrary checking on brush surfaces and states to try and catch errors
// =====================================================================================
contents_t      CheckBrushContents(const brush_t* const b)
{
    contents_t      best_contents;
    contents_t      contents;
    side_t*         s;
    int             i;

    s = &g_brushsides[b->firstside];

    // cycle though the sides of the brush and attempt to get our best side contents for 
    //  determining overall brush contents
    best_contents = TextureContents(s->td.name);
    s++;
    for (i = 1; i < b->numsides; i++, s++)
    {
        contents_t contents_consider = TextureContents(s->td.name);
        if (contents_consider > best_contents)
        {
            // if our current surface contents is better (larger) than our best, make it our best.
            best_contents = contents_consider;
        }
    }
    contents = best_contents;

    // attempt to pick up on mixed_face_contents errors
    s = &g_brushsides[b->firstside];
    s++;
    for (i = 1; i < b->numsides; i++, s++)
    {
        contents_t contents2 = TextureContents(s->td.name);

        // AJM: sky and null types are not to cause mixed face contents
        if (contents2 == CONTENTS_SKY)
            continue;

#ifdef ZHLT_NULLTEX
        if (contents2 == CONTENTS_NULL)
            continue;
#endif
 
        if (contents2 != best_contents)
        {
            Fatal(assume_MIXED_FACE_CONTENTS, "Entity %i, Brush %i: mixed face contents\n    Texture %s and %s",
                b->entitynum, b->brushnum, g_brushsides[b->firstside].td.name, s->td.name);
        }
    }

    // check to make sure we dont have an origin brush as part of worldspawn
    if ((b->entitynum == 0) || (strcmp("func_group", ValueForKey(&g_entities[b->entitynum], "classname"))==0))
    {
        if (contents == CONTENTS_ORIGIN)
        {
            Fatal(assume_BRUSH_NOT_ALLOWED_IN_WORLD, "Entity %i, Brush %i: %s brushes not allowed in world\n(did you forget to tie this origin brush to a rotating entity?)", b->entitynum, b->brushnum, ContentsToString(contents));
        }
    }
    else
    {
        // otherwise its not worldspawn, therefore its an entity. check to make sure this brush is allowed
        //  to be an entity.
        switch (contents)
        {
        case CONTENTS_SOLID:
        case CONTENTS_WATER:
        case CONTENTS_SLIME:
        case CONTENTS_LAVA:
        case CONTENTS_ORIGIN:
        case CONTENTS_CLIP:
#ifdef ZHLT_NULLTEX // AJM
        case CONTENTS_NULL:
            break; 
#endif
        default:
            Fatal(assume_BRUSH_NOT_ALLOWED_IN_ENTITY, "Entity %i, Brush %i: %s brushes not allowed in entity", b->entitynum, b->brushnum, ContentsToString(contents));
            break;
        }
    }

    return contents;
}


// =====================================================================================
//  CreateBrush
//      makes a brush!
// =====================================================================================
void            CreateBrush(const int brushnum)
{
    brush_t*        b;
    int             contents;
    int             h;

    b = &g_mapbrushes[brushnum];

    contents = b->contents;

    if (contents == CONTENTS_ORIGIN)
        return;

    //  HULL 0
    MakeBrushPlanes(b);
    MakeHullFaces(b, &b->hulls[0]);

    // these brush types do not need to be represented in the clipping hull
    switch (contents)
    {
        case CONTENTS_LAVA:
        case CONTENTS_SLIME:
        case CONTENTS_WATER:
        case CONTENTS_TRANSLUCENT:
        case CONTENTS_HINT:
            return;
    }

#ifdef HLCSG_CLIPECONOMY // AJM
    if (b->noclip)
        return;
#endif

    // HULLS 1-3
    if (!g_noclip)
    {
        for (h = 1; h < NUM_HULLS; h++)
        {
            ExpandBrush(b, h);
            MakeHullFaces(b, &b->hulls[h]);
        }
    }

    // clip brushes don't stay in the drawing hull
    if (contents == CONTENTS_CLIP)
    {
        b->hulls[0].faces = NULL;
        b->contents = CONTENTS_SOLID;
    }
}
