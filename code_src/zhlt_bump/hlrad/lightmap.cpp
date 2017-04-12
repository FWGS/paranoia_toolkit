#include "qrad.h"

edgeshare_t     g_edgeshare[MAX_MAP_EDGES];
vec3_t          g_face_centroids[MAX_MAP_EDGES];
bool            g_sky_lighting_fix = DEFAULT_SKY_LIGHTING_FIX;

#define TEXTURE_STEP   16.0

// buz
#define LIGHTFLAG_NOT_NORMAL	2
#define LIGHTFLAG_NOT_RENDERER	4
//#define LIGHTFLAG_NO_BUMP		8

// =====================================================================================
//  PairEdges
// =====================================================================================
void            PairEdges()
{
    int             i, j, k;
    dface_t*        f;
    edgeshare_t*    e;

    memset(&g_edgeshare, 0, sizeof(g_edgeshare));

    f = g_dfaces;
    for (i = 0; i < g_numfaces; i++, f++)
    {
        for (j = 0; j < f->numedges; j++)
        {
            k = g_dsurfedges[f->firstedge + j];
            if (k < 0)
            {
                e = &g_edgeshare[-k];

                hlassert(e->faces[1] == NULL);
                e->faces[1] = f;
            }
            else
            {
                e = &g_edgeshare[k];

                hlassert(e->faces[0] == NULL);
                e->faces[0] = f;
            }

            if (e->faces[0] && e->faces[1]) {
            // determine if coplanar
            if (e->faces[0]->planenum == e->faces[1]->planenum) {
                    e->coplanar = true;
            } else {
                    // see if they fall into a "smoothing group" based on angle of the normals
                    vec3_t          normals[2];

                    VectorCopy(getPlaneFromFace(e->faces[0])->normal, normals[0]);
                    VectorCopy(getPlaneFromFace(e->faces[1])->normal, normals[1]);

                    e->cos_normals_angle = DotProduct(normals[0], normals[1]);

                    if (e->cos_normals_angle > (1.0 - NORMAL_EPSILON))
                    {
                        e->coplanar = true;
                    }
                    else if (g_smoothing_threshold > 0.0)
                    {
                        if (e->cos_normals_angle >= g_smoothing_threshold)
                        {
                            VectorAdd(normals[0], normals[1], e->interface_normal);
                            VectorNormalize(e->interface_normal);
                        }
                    }
                }
            }
        }
    }
}

#define	MAX_SINGLEMAP	(18*18*4)

typedef struct
{
    vec_t*          light;
    vec_t           facedist;
    vec3_t          facenormal;

    int             numsurfpt;
    vec3_t          surfpt[MAX_SINGLEMAP];

    vec3_t          texorg;
    vec3_t          worldtotex[2];                         // s = (world - texorg) . worldtotex[0]
    vec3_t          textoworld[2];                         // world = texorg + s * textoworld[0]

    vec_t           exactmins[2], exactmaxs[2];

    int             texmins[2], texsize[2];
    int             lightstyles[256];
    int             surfnum;
    dface_t*        face;
}
lightinfo_t;

// =====================================================================================
//  TextureNameFromFace
// =====================================================================================
static const char* TextureNameFromFace(const dface_t* const f)
{
    texinfo_t*      tx;
    miptex_t*       mt;
    int             ofs;

    //
    // check for light emited by texture
    //
    tx = &g_texinfo[f->texinfo];

    ofs = ((dmiptexlump_t*)g_dtexdata)->dataofs[tx->miptex];
    mt = (miptex_t*)((byte*) g_dtexdata + ofs);

	return mt->name;
}

// =====================================================================================
//  CalcFaceExtents
//      Fills in s->texmins[] and s->texsize[]
//      also sets exactmins[] and exactmaxs[]
// =====================================================================================
static void     CalcFaceExtents(lightinfo_t* l)
{
    const int       facenum = l->surfnum;
    dface_t*        s;
    vec_t           mins[2], maxs[2], val;
    int             i, j, e;
    dvertex_t*      v;
    texinfo_t*      tex;

    s = l->face;

    mins[0] = mins[1] = 999999;
    maxs[0] = maxs[1] = -99999;

    tex = &g_texinfo[s->texinfo];

    for (i = 0; i < s->numedges; i++)
    {
        e = g_dsurfedges[s->firstedge + i];
        if (e >= 0)
        {
            v = g_dvertexes + g_dedges[e].v[0];
        }
        else
        {
            v = g_dvertexes + g_dedges[-e].v[1];
        }

        for (j = 0; j < 2; j++)
        {
            val = v->point[0] * tex->vecs[j][0] +
                v->point[1] * tex->vecs[j][1] + v->point[2] * tex->vecs[j][2] + tex->vecs[j][3];
            if (val < mins[j])
            {
                mins[j] = val;
            }
            if (val > maxs[j])
            {
                maxs[j] = val;
            }
        }
    }

    for (i = 0; i < 2; i++)
    {
        l->exactmins[i] = mins[i];
        l->exactmaxs[i] = maxs[i];

        mins[i] = floor(mins[i] / 16.0);
        maxs[i] = ceil(maxs[i] / 16.0);

		l->texmins[i] = mins[i];
        l->texsize[i] = maxs[i] - mins[i];
	}

	if (!(tex->flags & TEX_SPECIAL))
	{
		if ((l->texsize[0] > 16) || (l->texsize[1] > 16))
		{
			ThreadLock();
			PrintOnce("\nfor Face %d (texture %s) at ", s - g_dfaces, TextureNameFromFace(s));

			for (i = 0; i < s->numedges; i++)
			{
				e = g_dsurfedges[s->firstedge + i];
				if (e >= 0)
                {
					v = g_dvertexes + g_dedges[e].v[0];
                }
				else
                {
					v = g_dvertexes + g_dedges[-e].v[1];
                }
                VectorAdd(v->point, g_face_offset[facenum], v->point);
				Log("(%4.3f %4.3f %4.3f) ", v->point[0], v->point[1], v->point[2]);
			}
			Log("\n");

			Error( "Bad surface extents (%d x %d)\nCheck the file ZHLTProblems.html for a detailed explanation of this problem", l->texsize[0], l->texsize[1]);
		}
	}
}

// =====================================================================================
//  CalcFaceVectors
//      Fills in texorg, worldtotex. and textoworld
// =====================================================================================
static void     CalcFaceVectors(lightinfo_t* l)
{
    texinfo_t*      tex;
    int             i, j;
    vec3_t          texnormal;
    vec_t           distscale;
    vec_t           dist, len;

    tex = &g_texinfo[l->face->texinfo];

    // convert from float to double
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 3; j++)
        {
            l->worldtotex[i][j] = tex->vecs[i][j];
        }
    }

    // calculate a normal to the texture axis.  points can be moved along this
    // without changing their S/T
    CrossProduct(tex->vecs[1], tex->vecs[0], texnormal);
    VectorNormalize(texnormal);

    // flip it towards plane normal
    distscale = DotProduct(texnormal, l->facenormal);
    if (distscale == 0.0)
    {
        const unsigned facenum = l->face - g_dfaces;
    
        ThreadLock();
        Log("Malformed face (%d) normal @ \n", facenum);
        Winding* w = new Winding(*l->face);
        {
            const unsigned numpoints = w->m_NumPoints;
            unsigned x;
            for (x=0; x<numpoints; x++)
            {
                VectorAdd(w->m_Points[x], g_face_offset[facenum], w->m_Points[x]);
            }
        }
        w->Print();
        delete w;
        ThreadUnlock();

        hlassume(false, assume_MalformedTextureFace);
    }

    if (distscale < 0)
    {
        distscale = -distscale;
        VectorSubtract(vec3_origin, texnormal, texnormal);
    }

    // distscale is the ratio of the distance along the texture normal to
    // the distance along the plane normal
    distscale = 1.0 / distscale;

    for (i = 0; i < 2; i++)
    {
        len = (float)VectorLength(l->worldtotex[i]);
        dist = DotProduct(l->worldtotex[i], l->facenormal);
        dist *= distscale;
        VectorMA(l->worldtotex[i], -dist, texnormal, l->textoworld[i]);
        VectorScale(l->textoworld[i], (1 / len) * (1 / len), l->textoworld[i]);
    }

    // calculate texorg on the texture plane
    for (i = 0; i < 3; i++)
    {
        l->texorg[i] = -tex->vecs[0][3] * l->textoworld[0][i] - tex->vecs[1][3] * l->textoworld[1][i];
    }

    // project back to the face plane
    dist = DotProduct(l->texorg, l->facenormal) - l->facedist - DEFAULT_HUNT_OFFSET;
    dist *= distscale;
    VectorMA(l->texorg, -dist, texnormal, l->texorg);

}

// =====================================================================================
//  SetSurfFromST
// =====================================================================================
static void     SetSurfFromST(const lightinfo_t* const l, vec_t* surf, const vec_t s, const vec_t t)
{
    const int       facenum = l->surfnum;
    int             j;

    for (j = 0; j < 3; j++)
    {
        surf[j] = l->texorg[j] + l->textoworld[0][j] * s + l->textoworld[1][j] * t;
    }

    // Adjust for origin-based models
    VectorAdd(surf, g_face_offset[facenum], surf);
}

// =====================================================================================
//  FindSurfaceMidpoint
// =====================================================================================
static dleaf_t* FindSurfaceMidpoint(const lightinfo_t* const l, vec_t* midpoint)
{
    int             s, t;
    int             w, h;
    vec_t           starts, startt;
    vec_t           us, ut;

    vec3_t          broken_midpoint;
    vec3_t          surface_midpoint;
    int             inside_point_count;

    dleaf_t*        last_valid_leaf = NULL;
    dleaf_t*        leaf_mid;

    const int       facenum = l->surfnum;
    const dface_t*  f = g_dfaces + facenum;
    const dplane_t* p = getPlaneFromFace(f);

    const vec_t*    face_delta = g_face_offset[facenum];

    h = l->texsize[1] + 1;
    w = l->texsize[0] + 1;
    starts = (float)l->texmins[0] * 16;
    startt = (float)l->texmins[1] * 16;

    // General case
    inside_point_count = 0;
    VectorClear(surface_midpoint);
    for (t = 0; t < h; t++)
    {
        for (s = 0; s < w; s++)
        {
            us = starts + s * TEXTURE_STEP;
            ut = startt + t * TEXTURE_STEP;

            SetSurfFromST(l, midpoint, us, ut);
            if ((leaf_mid = PointInLeaf(midpoint)) != g_dleafs)
            {
                if ((leaf_mid->contents != CONTENTS_SKY) && (leaf_mid->contents != CONTENTS_SOLID))
                {
                    last_valid_leaf = leaf_mid;
                    inside_point_count++;
                    VectorAdd(surface_midpoint, midpoint, surface_midpoint);
                }
            }
        }
    }

    if (inside_point_count > 1)
    {
        vec_t           tmp = 1.0 / inside_point_count;

        VectorScale(surface_midpoint, tmp, midpoint);

        //Verbose("Trying general at (%4.3f %4.3f %4.3f) %d\n", surface_midpoint[0], surface_midpoint[1], surface_midpoint[2], inside_point_count);
        if (
            (leaf_mid =
             HuntForWorld(midpoint, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET)))
        {
            //Verbose("general method succeeded at (%4.3f %4.3f %4.3f)\n", midpoint[0], midpoint[1], midpoint[2]);
            return leaf_mid;
        }
        //Verbose("Tried general , failed at (%4.3f %4.3f %4.3f)\n", midpoint[0], midpoint[1], midpoint[2]);
    }
    else if (inside_point_count == 1)
    {
        //Verbose("Returning single point from general\n");
        VectorCopy(surface_midpoint, midpoint);
        return last_valid_leaf;
    }
    else
    {
        //Verbose("general failed (no points)\n");
    }

    // Try harder
    inside_point_count = 0;
    VectorClear(surface_midpoint);
    for (t = 0; t < h; t++)
    {
        for (s = 0; s < w; s++)
        {
            us = starts + s * TEXTURE_STEP;
            ut = startt + t * TEXTURE_STEP;

            SetSurfFromST(l, midpoint, us, ut);
            leaf_mid =
                HuntForWorld(midpoint, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET);
            if (leaf_mid != g_dleafs)
            {
                last_valid_leaf = leaf_mid;
                inside_point_count++;
                VectorAdd(surface_midpoint, midpoint, surface_midpoint);
            }
        }
    }

    if (inside_point_count > 1)
    {
        vec_t           tmp = 1.0 / inside_point_count;

        VectorScale(surface_midpoint, tmp, midpoint);

        if (
            (leaf_mid =
             HuntForWorld(midpoint, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET)))
        {
            //Verbose("best method succeeded at (%4.3f %4.3f %4.3f)\n", midpoint[0], midpoint[1], midpoint[2]);
            return leaf_mid;
        }
        //Verbose("Tried best, failed at (%4.3f %4.3f %4.3f)\n", midpoint[0], midpoint[1], midpoint[2]);
    }
    else if (inside_point_count == 1)
    {
        //Verbose("Returning single point from best\n");
        VectorCopy(surface_midpoint, midpoint);
        return last_valid_leaf;
    }
    else
    {
        //Verbose("best failed (no points)\n");
    }

    // Original broken code
    {
        vec_t           mids = (l->exactmaxs[0] + l->exactmins[0]) / 2;
        vec_t           midt = (l->exactmaxs[1] + l->exactmins[1]) / 2;

        SetSurfFromST(l, midpoint, mids, midt);

        if ((leaf_mid = PointInLeaf(midpoint)) != g_dleafs)
        {
            if ((leaf_mid->contents != CONTENTS_SKY) && (leaf_mid->contents != CONTENTS_SOLID))
            {
                return leaf_mid;
            }
        }

        VectorCopy(midpoint, broken_midpoint);
        //Verbose("Tried original method, failed at (%4.3f %4.3f %4.3f)\n", midpoint[0], midpoint[1], midpoint[2]);
    }

    VectorCopy(broken_midpoint, midpoint);
    return HuntForWorld(midpoint, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET);
}

// =====================================================================================
//  SimpleNudge
//      Return vec_t in point only valid when function returns true
//      Use negative scales to push away from center instead
// =====================================================================================
static bool     SimpleNudge(vec_t* const point, const lightinfo_t* const l, vec_t* const s, vec_t* const t, const vec_t delta)
{
    const int       facenum = l->surfnum;
    const dface_t*  f = g_dfaces + facenum;
    const dplane_t* p = getPlaneFromFace(f);
    const vec_t*    face_delta = g_face_offset[facenum];
    const int       h = l->texsize[1] + 1;
    const int       w = l->texsize[0] + 1;
    const vec_t     half_w = (vec_t)(w - 1) / 2.0;
    const vec_t     half_h = (vec_t)(h - 1) / 2.0;
    const vec_t     s_vec = *s;
    const vec_t     t_vec = *t;
    vec_t           s1;
    vec_t           t1;

    if (s_vec > half_w)
    {
        s1 = s_vec - delta;
    }
    else
    {
        s1 = s_vec + delta;
    }

    SetSurfFromST(l, point, s1, t_vec);
    if (HuntForWorld(point, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET))
    {
        *s = s1;
        return true;
    }

    if (t_vec > half_h)
    {
        t1 = t_vec - delta;
    }
    else
    {
        t1 = t_vec + delta;
    }

    SetSurfFromST(l, point, s_vec, t1);
    if (HuntForWorld(point, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET))
    {
        *t = t1;
        return true;
    }

    return false;
}

typedef enum
{
    LightOutside,                                          // Not lit
    LightShifted,                                          // used HuntForWorld on 100% dark face
    LightShiftedInside,                                    // moved to neighbhor on 2nd cleanup pass
    LightNormal,                                           // Normally lit with no movement
    LightPulledInside,                                     // Pulled inside by bleed code adjustments
    LightSimpleNudge,                                      // A simple nudge 1/3 or 2/3 towards center along S or T axist
    LightSimpleNudgeEmbedded                               // A nudge either 1 full unit in each of S and T axis, or 1/3 or 2/3 AWAY from center
}
light_flag_t;

// =====================================================================================
//  CalcPoints
//      For each texture aligned grid point, back project onto the plane
//      to get the world xyz value of the sample point
// =====================================================================================
static void     CalcPoints(lightinfo_t* l)
{
    const int       facenum = l->surfnum;
    const dface_t*  f = g_dfaces + facenum;
    const dplane_t* p = getPlaneFromFace(f);

    const vec_t*    face_delta = g_face_offset[facenum];
    const eModelLightmodes lightmode = g_face_lightmode[facenum];

    const vec_t     mids = (l->exactmaxs[0] + l->exactmins[0]) / 2;
    const vec_t     midt = (l->exactmaxs[1] + l->exactmins[1]) / 2;

    const int       h = l->texsize[1] + 1;
    const int       w = l->texsize[0] + 1;

    const vec_t     starts = (l->texmins[0] * 16);
    const vec_t     startt = (l->texmins[1] * 16);

    light_flag_t    LuxelFlags[MAX_SINGLEMAP];
    light_flag_t*   pLuxelFlags;
    vec_t           us, ut;
    vec_t*          surf;
    vec3_t          surface_midpoint;
    dleaf_t*        leaf_mid;
    dleaf_t*        leaf_surf;
    int             s, t;
    int             i;

    l->numsurfpt = w * h;

    memset(LuxelFlags, 0, sizeof(LuxelFlags));

    leaf_mid = FindSurfaceMidpoint(l, surface_midpoint);
#if 0
    if (!leaf_mid)
    {
        Developer(DEVELOPER_LEVEL_FLUFF, "CalcPoints [face %d] (%4.3f %4.3f %4.3f) midpoint outside world\n",
                  facenum, surface_midpoint[0], surface_midpoint[1], surface_midpoint[2]);
    }
    else
    {
        Developer(DEVELOPER_LEVEL_FLUFF, "FindSurfaceMidpoint [face %d] @ (%4.3f %4.3f %4.3f)\n",
                  facenum, surface_midpoint[0], surface_midpoint[1], surface_midpoint[2]);
    }
#endif

    // First pass, light normally, and pull any faces toward the center for bleed adjustment

    surf = l->surfpt[0];
    pLuxelFlags = LuxelFlags;
    for (t = 0; t < h; t++)
    {
        for (s = 0; s < w; s++, surf += 3, pLuxelFlags++)
        {
            vec_t           original_s = us = starts + s * TEXTURE_STEP;
            vec_t           original_t = ut = startt + t * TEXTURE_STEP;

            SetSurfFromST(l, surf, us, ut);
            leaf_surf = HuntForWorld(surf, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET);

            if (!leaf_surf)
            {
                // At first try a 1/3 and 2/3 distance to nearest in each S and T axis towards the face midpoint
                if (SimpleNudge(surf, l, &us, &ut, TEXTURE_STEP * (1.0 / 3.0)))
                {
                    *pLuxelFlags = LightSimpleNudge;
                }
                else if (SimpleNudge(surf, l, &us, &ut, -TEXTURE_STEP * (1.0 / 3.0)))
                {
                    *pLuxelFlags = LightSimpleNudge;
                }
                else if (SimpleNudge(surf, l, &us, &ut, TEXTURE_STEP * (2.0 / 3.0)))
                {
                    *pLuxelFlags = LightSimpleNudge;
                }
                else if (SimpleNudge(surf, l, &us, &ut, -TEXTURE_STEP * (2.0 / 3.0)))
                {
                    *pLuxelFlags = LightSimpleNudge;
                }
                // Next, if this is a model flagged with the 'Embedded' mode, try away from the facemid too
                else if (lightmode & eModelLightmodeEmbedded)
                {
                    SetSurfFromST(l, surf, us, ut);
                    if (SimpleNudge(surf, l, &us, &ut, TEXTURE_STEP))
                    {
                        *pLuxelFlags = LightSimpleNudgeEmbedded;
                        continue;
                    }
                    if (SimpleNudge(surf, l, &us, &ut, -TEXTURE_STEP))
                    {
                        *pLuxelFlags = LightSimpleNudgeEmbedded;
                        continue;
                    }

                    SetSurfFromST(l, surf, original_s, original_t);
                    *pLuxelFlags = LightOutside;
                    continue;
                }
            }

            if (!(lightmode & eModelLightmodeEmbedded))
            {
                // Pull the sample points towards the facemid if visibility is blocked
                // and the facemid is inside the world
                int             nudge_divisor = max(max(w, h), 4);
                int             max_nudge = nudge_divisor + 1;
                bool            nudge_succeeded = false;

                vec_t           nudge_s = (mids - us) / (vec_t)nudge_divisor;
                vec_t           nudge_t = (midt - ut) / (vec_t)nudge_divisor;

                // if a line can be traced from surf to facemid, the point is good
                for (i = 0; i < max_nudge; i++)
                {
                    // Make sure we are "in the world"(Not the zero leaf)
                    if (leaf_mid)
                    {
                        SetSurfFromST(l, surf, us, ut);
                        leaf_surf =
                            HuntForWorld(surf, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE,
                                         DEFAULT_HUNT_OFFSET);

                        if (leaf_surf)
                        {
                            if (TestLine(surface_midpoint, surf) == CONTENTS_EMPTY)
                            {
                                if (lightmode & eModelLightmodeConcave)
                                {
#ifdef HLRAD_HULLU
                                    vec3_t transparency = { 1.0, 1.0, 1.0 };
                                    if (TestSegmentAgainstOpaqueList(surface_midpoint, surf, transparency))
#else
                                    if (TestSegmentAgainstOpaqueList(surface_midpoint, surf))
#endif
                                    {
                                        Log("SDF::4\n");
                                        us += nudge_s;
                                        ut += nudge_t;
                                        continue;   // Try nudge again, we hit an opaque face
                                    }
                                }
                                if (i)
                                {
                                    *pLuxelFlags = LightPulledInside;
                                }
                                else
                                {
                                    *pLuxelFlags = LightNormal;
                                }
                                nudge_succeeded = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        leaf_surf = PointInLeaf(surf);
                        if (leaf_surf != g_dleafs)
                        {
                            if ((leaf_surf->contents != CONTENTS_SKY) && (leaf_surf->contents != CONTENTS_SOLID))
                            {
                                *pLuxelFlags = LightNormal;
                                nudge_succeeded = true;
                                break;
                            }
                        }
                    }

                    us += nudge_s;
                    ut += nudge_t;
                }

                if (!nudge_succeeded)
                {
                    SetSurfFromST(l, surf, original_s, original_t);
                    *pLuxelFlags = LightOutside;
                }
            }
        }
    }

    // 2nd Pass, find units that are not lit and try to move them one half or unit worth 
    // in each direction and see if that is lit.
    // This handles 1 x N lightmaps which are all dark everywhere and have no frame of refernece
    // for a good center or directly lit areas
    surf = l->surfpt[0];
    pLuxelFlags = LuxelFlags;
#if 0
    Developer(DEVELOPER_LEVEL_SPAM,
              "w (%d) h (%d) dim (%d) leafmid (%4.3f %4.3f %4.3f) plane normal (%4.3f) (%4.3f) (%4.3f) dist (%f)\n", w,
              h, w * h, surface_midpoint[0], surface_midpoint[1], surface_midpoint[2], p->normal[0], p->normal[1],
              p->normal[2], p->dist);
#endif
    {
        int             total_dark = 0;
        int             total_adjusted = 0;

        for (t = 0; t < h; t++)
        {
            for (s = 0; s < w; s++, surf += 3, pLuxelFlags++)
            {
                if (!*pLuxelFlags)
                {
#if 0
                    Developer(DEVELOPER_LEVEL_FLUFF, "Dark (%d %d) (%4.3f %4.3f %4.3f)\n",
                              s, t, surf[0], surf[1], surf[2]);
#endif
                    total_dark++;
                    if (HuntForWorld(surf, face_delta, p, DEFAULT_HUNT_SIZE, DEFAULT_HUNT_SCALE, DEFAULT_HUNT_OFFSET))
                    {
#if 0
                        Developer(DEVELOPER_LEVEL_FLUFF, "Shifted %d %d to (%4.3f %4.3f %4.3f)\n", s, t, surf[0],
                                  surf[1], surf[2]);
#endif
                        *pLuxelFlags = LightShifted;
                        total_adjusted++;
                    }
                    else if (HuntForWorld(surf, face_delta, p, 101, 0.5, DEFAULT_HUNT_OFFSET))
                    {
#if 0
                        Developer(DEVELOPER_LEVEL_FLUFF, "Shifted %d %d to (%4.3f %4.3f %4.3f)\n", s, t, surf[0],
                                  surf[1], surf[2]);
#endif
                        *pLuxelFlags = LightShifted;
                        total_adjusted++;
                    }
                }
            }
        }
#if 0
        if (total_dark)
        {
            Developer(DEVELOPER_LEVEL_FLUFF, "Pass 2 : %d dark, %d corrected\n", total_dark, total_adjusted);
        }
#endif
    }

    // 3rd Pass, find units that are not lit and move them towards neighbhors who are
    // Currently finds the first lit neighbhor and uses its data
    surf = l->surfpt[0];
    pLuxelFlags = LuxelFlags;
    {
        int             total_dark = 0;
        int             total_adjusted = 0;

        for (t = 0; t < h; t++)
        {
            for (s = 0; s < w; s++, surf += 3, pLuxelFlags++)
            {
                if (!*pLuxelFlags)
                {
                    int             x_min = max(0, s - 1);
                    int             x_max = min(w, s + 1);
                    int             y_min = max(0, t - 1);
                    int             y_max = min(t, t + 1);

                    int             x, y;

#if 0
                    Developer(DEVELOPER_LEVEL_FLUFF, "Point outside (%d %d) (%4.3f %4.3f %4.3f)\n",
                              s, t, surf[0], surf[1], surf[2]);
#endif

                    total_dark++;

                    for (x = x_min; x < x_max; x++)
                    {
                        for (y = y_min; y < y_max; y++)
                        {
                            if (*pLuxelFlags >= LightNormal)
                            {
                                dleaf_t*        leaf;
                                vec_t*          other_surf = l->surfpt[0];

                                other_surf += ((y * w) + x) * 3;

                                leaf = PointInLeaf(other_surf);
                                if ((leaf->contents != CONTENTS_SKY && leaf->contents != CONTENTS_SOLID))
                                {
                                    *pLuxelFlags = LightShiftedInside;
#if 0
                                    Developer(DEVELOPER_LEVEL_MESSAGE,
                                              "Nudged (%d %d) (%4.3f %4.3f %4.3f) to (%d %d) (%4.3f %4.3f %4.3f) \n",
                                              s, t, surf[0], surf[1], surf[2], x, y, other_surf[0], other_surf[1],
                                              other_surf[2]);
#endif
                                    VectorCopy(other_surf, surf);
                                    total_adjusted++;
                                    goto found_it;
                                }
                            }
                        }
                    }
                }
              found_it:;
            }
        }
#if 0
        if (total_dark)
        {
            Developer(DEVELOPER_LEVEL_FLUFF, "Pass 2 : %d dark, %d corrected\n", total_dark, total_adjusted);
        }
#endif
    }
}

//==============================================================

typedef struct
{
    vec3_t          pos;
    vec3_t          light;
}
sample_t;

typedef struct
{
    int             numsamples;
    sample_t*       samples[MAXLIGHTMAPS];
}
facelight_t;

static directlight_t* directlights[MAX_MAP_LEAFS];
static facelight_t facelight[MAX_MAP_FACES];
static int      numdlights;

#define	DIRECT_SCALE	0.1f

// =====================================================================================
//  CreateDirectLights
// =====================================================================================
void            CreateDirectLights()
{
    unsigned        i;
    patch_t*        p;
    directlight_t*  dl;
    dleaf_t*        leaf;
    int             leafnum;
    entity_t*       e;
    entity_t*       e2;
    const char*     name;
    const char*     target;
    float           angle;
    vec3_t          dest;

    // AJM: coplaner lighting
    vec3_t          temp_normal;

    numdlights = 0;

    //
    // surfaces
    //
    for (i = 0, p = g_patches; i < g_num_patches; i++, p++)
    {
        if (VectorAvg(p->totallight) >= g_dlight_threshold)
        {
            numdlights++;
            dl = (directlight_t*)calloc(1, sizeof(directlight_t));

            VectorCopy(p->origin, dl->origin);

            leaf = PointInLeaf(dl->origin);
            leafnum = leaf - g_dleafs;

            dl->next = directlights[leafnum];
            directlights[leafnum] = dl;

            dl->type = emit_surface;
            VectorCopy(getPlaneFromFaceNumber(p->faceNumber)->normal, dl->normal);
            VectorCopy(p->totallight, dl->intensity);
            VectorScale(dl->intensity, p->area, dl->intensity);
            VectorScale(dl->intensity, DIRECT_SCALE, dl->intensity);
        
        	// --------------------------------------------------------------
	        // Changes by Adam Foster - afoster@compsoc.man.ac.uk
	        // mazemaster's l33t backwards lighting (I still haven't a clue
	        // what it's supposed to be for) :-)
#ifdef HLRAD_WHOME

	        if (g_softlight_hack[0] || g_softlight_hack[1] || g_softlight_hack[2]) 
            {
		        numdlights++;
		        dl = (directlight_t *) calloc(1, sizeof(directlight_t));

		        VectorCopy(p->origin, dl->origin);

		        leaf = PointInLeaf(dl->origin);
		        leafnum = leaf - g_dleafs;

		        dl->next = directlights[leafnum];
		        directlights[leafnum] = dl;

		        dl->type = emit_surface;
		        VectorCopy(getPlaneFromFaceNumber(p->faceNumber)->normal, dl->normal);
		        VectorScale(dl->normal, g_softlight_hack_distance, temp_normal);
		        VectorAdd(dl->origin, temp_normal, dl->origin);
		        VectorScale(dl->normal, -1, dl->normal);

		        VectorCopy(p->totallight, dl->intensity);

		        VectorScale(dl->intensity, p->area, dl->intensity);
		        VectorScale(dl->intensity, DIRECT_SCALE, dl->intensity);

		        dl->intensity[0] *= g_softlight_hack[0];
		        dl->intensity[1] *= g_softlight_hack[1];
		        dl->intensity[2] *= g_softlight_hack[2];
	        }

#endif
	        // --------------------------------------------------------------
        }
        VectorClear(p->totallight);
    }

    //
    // entities
    //
    for (i = 0; i < (unsigned)g_numentities; i++)
    {
        const char*     pLight;
        double          r, g, b, scaler;
        float           l1;
        int             argCnt;

        e = &g_entities[i];
        name = ValueForKey(e, "classname");
        if (strncmp(name, "light", 5))
            continue;
		
		//buz:	Dynamic light is a victim of bump-mapping.
		//		Ignore all lights with nonzero styles...
		int st = IntForKey(e, "style");
		vec3_t orig;
		GetVectorForKey(e, "origin", orig);
		if (st)
		{
			Log("Warning: light with style %d at position {%f, %f, %f} is deleted\n", st, orig[0], orig[1], orig[2]);
			continue;
		}

        numdlights++;
        dl = (directlight_t*)calloc(1, sizeof(directlight_t));

       // GetVectorForKey(e, "origin", dl->origin);
		VectorCopy(orig, dl->origin); // buz

        leaf = PointInLeaf(dl->origin);
        leafnum = leaf - g_dleafs;

        dl->next = directlights[leafnum];
        directlights[leafnum] = dl;

	//	dl->style = IntForKey(e, "style");
		dl->style = 0; // buz

        pLight = ValueForKey(e, "_light");
        // scanf into doubles, then assign, so it is vec_t size independent
        r = g = b = scaler = 0;
        argCnt = sscanf(pLight, "%lf %lf %lf %lf", &r, &g, &b, &scaler);
        dl->intensity[0] = (float)r;
        if (argCnt == 1)
        {
            // The R,G,B values are all equal.
            dl->intensity[1] = dl->intensity[2] = (float)r;
        }
        else if (argCnt == 3 || argCnt == 4)
        {
            // Save the other two G,B values.
            dl->intensity[1] = (float)g;
            dl->intensity[2] = (float)b;

            // Did we also get an "intensity" scaler value too?
            if (argCnt == 4)
            {
                // Scale the normalized 0-255 R,G,B values by the intensity scaler
                dl->intensity[0] = dl->intensity[0] / 255 * (float)scaler;
                dl->intensity[1] = dl->intensity[1] / 255 * (float)scaler;
                dl->intensity[2] = dl->intensity[2] / 255 * (float)scaler;
            }
        }
        else
        {
            Log("light at (%f,%f,%f) has bad or missing '_light' value : '%s'\n",
                dl->origin[0], dl->origin[1], dl->origin[2], pLight);
            continue;
        }

        dl->fade = FloatForKey(e, "_fade");
        if (dl->fade == 0.0)
        {
            dl->fade = g_fade;
        }

        dl->falloff = IntForKey(e, "_falloff");
        if (dl->falloff == 0)
        {
            dl->falloff = g_falloff;
        }

		dl->flags = IntForKey(e, "spawnflags"); // buz

        target = ValueForKey(e, "target");

        if (!strcmp(name, "light_spot") || !strcmp(name, "light_environment") || target[0])
        {
            if (!VectorAvg(dl->intensity))
            {
                VectorFill(dl->intensity, 500);
            }
            dl->type = emit_spotlight;
            dl->stopdot = FloatForKey(e, "_cone");
            if (!dl->stopdot)
            {
                dl->stopdot = 10;
            }
            dl->stopdot2 = FloatForKey(e, "_cone2");
            if (!dl->stopdot2)
            {
                dl->stopdot2 = dl->stopdot;
            }
            if (dl->stopdot2 < dl->stopdot)
            {
                dl->stopdot2 = dl->stopdot;
            }
            dl->stopdot2 = (float)cos(dl->stopdot2 / 180 * Q_PI);
            dl->stopdot = (float)cos(dl->stopdot / 180 * Q_PI);

            if (target[0])
            {                                              // point towards target
                e2 = FindTargetEntity(target);
                if (!e2)
                {
                    Warning("light at (%i %i %i) has missing target",
                            (int)dl->origin[0], (int)dl->origin[1], (int)dl->origin[2]);
                }
                else
                {
                    GetVectorForKey(e2, "origin", dest);
                    VectorSubtract(dest, dl->origin, dl->normal);
                    VectorNormalize(dl->normal);
                }
            }
            else
            {                                              // point down angle
                vec3_t          vAngles;

                GetVectorForKey(e, "angles", vAngles);

                angle = (float)FloatForKey(e, "angle");
                if (angle == ANGLE_UP)
                {
                    dl->normal[0] = dl->normal[1] = 0;
                    dl->normal[2] = 1;
                }
                else if (angle == ANGLE_DOWN)
                {
                    dl->normal[0] = dl->normal[1] = 0;
                    dl->normal[2] = -1;
                }
                else
                {
                    // if we don't have a specific "angle" use the "angles" YAW
                    if (!angle)
                    {
                        angle = vAngles[1];
                    }

                    dl->normal[2] = 0;
                    dl->normal[0] = (float)cos(angle / 180 * Q_PI);
                    dl->normal[1] = (float)sin(angle / 180 * Q_PI);
                }

                angle = FloatForKey(e, "pitch");
                if (!angle)
                {
                    // if we don't have a specific "pitch" use the "angles" PITCH
                    angle = vAngles[0];
                }

                dl->normal[2] = (float)sin(angle / 180 * Q_PI);
                dl->normal[0] *= (float)cos(angle / 180 * Q_PI);
                dl->normal[1] *= (float)cos(angle / 180 * Q_PI);
            }

            if (FloatForKey(e, "_sky") || !strcmp(name, "light_environment"))
            {
				// -----------------------------------------------------------------------------------
				// Changes by Adam Foster - afoster@compsoc.man.ac.uk
				// diffuse lighting hack - most of the following code nicked from earlier
				// need to get diffuse intensity from new _diffuse_light key
				//
				// What does _sky do for spotlights, anyway?
				// -----------------------------------------------------------------------------------
#ifdef HLRAD_WHOME
				pLight = ValueForKey(e, "_diffuse_light");
        		r = g = b = scaler = 0;
        		argCnt = sscanf(pLight, "%lf %lf %lf %lf", &r, &g, &b, &scaler);
        		dl->diffuse_intensity[0] = (float)r;
        		if (argCnt == 1)
        		{
            		// The R,G,B values are all equal.
            		dl->diffuse_intensity[1] = dl->diffuse_intensity[2] = (float)r;
        		}
        		else if (argCnt == 3 || argCnt == 4)
        		{
            		// Save the other two G,B values.
            		dl->diffuse_intensity[1] = (float)g;
            		dl->diffuse_intensity[2] = (float)b;

            		// Did we also get an "intensity" scaler value too?
      		    	if (argCnt == 4)
     	       		{
                		// Scale the normalized 0-255 R,G,B values by the intensity scaler
                		dl->diffuse_intensity[0] = dl->diffuse_intensity[0] / 255 * (float)scaler;
                		dl->diffuse_intensity[1] = dl->diffuse_intensity[1] / 255 * (float)scaler;
                		dl->diffuse_intensity[2] = dl->diffuse_intensity[2] / 255 * (float)scaler;
            		}
        		}
				else
        		{
					// backwards compatibility with maps without _diffuse_light

					dl->diffuse_intensity[0] = dl->intensity[0];
					dl->diffuse_intensity[1] = dl->intensity[1];
					dl->diffuse_intensity[2] = dl->intensity[2];
        		}
#endif
				// -----------------------------------------------------------------------------------

                dl->type = emit_skylight;
                dl->stopdot2 = FloatForKey(e, "_sky");     // hack stopdot2 to a sky key number
            }
        }
        else
        {
            if (!VectorAvg(dl->intensity))
                VectorFill(dl->intensity, 300);
            dl->type = emit_point;
        }

        if (dl->type != emit_skylight)
        {
            l1 = max(dl->intensity[0], max(dl->intensity[1], dl->intensity[2]));
            l1 = l1 * l1 / 10;

            dl->intensity[0] *= l1;
            dl->intensity[1] *= l1;
            dl->intensity[2] *= l1;
        }

	//	Log("Light: %f, %f, %f\n", dl->intensity[0], dl->intensity[1], dl->intensity[2]);
    }

    hlassume(numdlights, assume_NoLights);
    Log("%i direct lights\n", numdlights);
}

// =====================================================================================
//  DeleteDirectLights
// =====================================================================================
void            DeleteDirectLights()
{
    int             l;
    directlight_t*  dl;

    for (l = 0; l < g_numleafs; l++)
    {
        dl = directlights[l];
        while (dl)
        {
            directlights[l] = dl->next;
            free(dl);
            dl = directlights[l];
        }
    }

    // AJM: todo: strip light entities out at this point
}


// buz
void GetRotationMatrix( const vec3_t src, const vec3_t dst, float *matrix )
{
	float identity[9] = {
		1, 0, 0,
		0, 1, 0,
		0, 0, 1
	};

	vec3_t up, src_rt, dst_rt;
	CrossProduct(src, dst, up);
	if (VectorNormalize(up) < NORMAL_EPSILON)
	{
		for(int i = 0; i < 9; i++, matrix++)
			*matrix = identity[i];
		return;
	}
	CrossProduct(src, up, src_rt);
	VectorNormalize(src_rt);
	CrossProduct(dst, up, dst_rt);
	VectorNormalize(dst_rt);

	float *cur = identity;
	for( int i = 0; i < 3; i++, cur+=3, matrix+=3 )
	{
		vec3_t temp;
		temp[0] = DotProduct(cur, src);
		temp[1] = DotProduct(cur, src_rt);
		temp[2] = DotProduct(cur, up);
		
		matrix[0] = temp[0]*dst[0] + temp[1]*dst_rt[0] + temp[2]*up[0];
		matrix[1] = temp[0]*dst[1] + temp[1]*dst_rt[1] + temp[2]*up[1];
		matrix[2] = temp[0]*dst[2] + temp[1]*dst_rt[2] + temp[2]*up[2];
	}
}

// buz
void VTransform( vec3_t src, float *matrix, vec3_t dst )
{
	dst[0] = src[0]*matrix[0] + src[1]*matrix[3] + src[2]*matrix[6];
	dst[1] = src[0]*matrix[1] + src[1]*matrix[4] + src[2]*matrix[7];
	dst[2] = src[0]*matrix[2] + src[1]*matrix[5] + src[2]*matrix[8];
}

// =====================================================================================
//  GatherSampleLight
// =====================================================================================
// buz
enum {
	BUMP_BASELIGHT_STYLE	= 61,
	BUMP_ADDLIGHT_STYLE		= 62,
	BUMP_LIGHTVECS_STYLE	= 63,

	BUMP_BASELIGHT_MAP		= 1,
	BUMP_ADDLIGHT_MAP		= 3,
	BUMP_LIGHTVECS_MAP		= 2,
};

#define NUMVERTEXNORMALS	162
double          r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "../common/anorms.h"
};
static int GatherSampleLight(const vec3_t pos, const byte* const pvs, const vec3_t normal, vec3_t* sample, byte* styles, float *lightmatrix)
{
    int             i;
    directlight_t*  l;
    vec3_t          add;
    vec3_t          delta;
    float           dot, dot2;
    float           dist;
    float           ratio;
#ifdef HLRAD_OPACITY // AJM
    float           l_opacity;
#endif
    int             style_index;
    directlight_t*  sky_used = NULL;
	vec3_t lighttransformed; // buz

	// buz
	vec3_t lightdir;
	VectorClear(lightdir);

	int skiprenderer = 0; // will be true if we have here some lights with LIGHTFLAG_NOT_RENDERER

    for (i = 1; i < g_numleafs; i++)
    {
        l = directlights[i];
        if (l)
        {
            if (((l->type == emit_skylight) && (g_sky_lighting_fix)) || (pvs[(i - 1) >> 3] & (1 << ((i - 1) & 7))))
            {
                for (; l; l = l->next)
                {
                    // skylights work fundamentally differently than normal lights
                    if (l->type == emit_skylight)
                    {
                        // only allow one of each sky type to hit any given point
                        if (sky_used)
                        {
                            continue;
                        }
                        sky_used = l;

						VTransform(l->normal, lightmatrix, lighttransformed); // buz
					//	VectorCopy(l->normal, lighttransformed);

                        // make sure the angle is okay
					//	dot = -DotProduct(normal, l->normal);
						dot = -DotProduct(normal, lighttransformed);
                        if (dot <= ON_EPSILON / 10)
                        {
                            continue;
                        }

                        // search back to see if we can hit a sky brush
                        VectorScale(l->normal, -10000, delta);
                        VectorAdd(pos, delta, delta);
                        if (TestLine(pos, delta) != CONTENTS_SKY)
                        {
                            continue;                      // occluded
                        }

			            if (TestSegmentAgainstOpaqueList(pos, delta))
                        {
                            continue;
                        }

                        VectorScale(l->intensity, dot, add);
					//	VectorScale(l->normal, -1, delta); // buz - put in delta direction to light
						VectorScale(lighttransformed, -1, delta); // buz - put in delta direction to light
                    }
                    else
                    {
                        float           denominator;

                        VectorSubtract(l->origin, pos, delta);
                        dist = VectorNormalize(delta);

						VTransform(delta, lightmatrix, lighttransformed); // buz
					//	VectorCopy(delta, lighttransformed);

					//	dot = DotProduct(delta, normal);
						dot = DotProduct(lighttransformed, normal);
                        //                        if (dot <= 0.0)
                        //                            continue;
                        if (dot <= ON_EPSILON / 10)
                        {
                            continue;                      // behind sample surface
                        }

                        if (dist < 1.0)
                        {
                            dist = 1.0;
                        }

                        // Variable power falloff (1 = inverse linear, 2 = inverse square
                        denominator = dist * l->fade;
                        if (l->falloff == 2)
                        {
                            denominator *= dist;
                        }

                        switch (l->type)
                        {
                        case emit_point:
                        {
                            // Variable power falloff (1 = inverse linear, 2 = inverse square
                            vec_t           denominator = dist * l->fade;

                            if (l->falloff == 2)
                            {
                                denominator *= dist;
                            }
							ratio = dot / denominator;
                            VectorScale(l->intensity, ratio, add);							
							break;
                        }

                        case emit_surface:
                        {
                            dot2 = -DotProduct(delta, l->normal);
                            if (dot2 <= ON_EPSILON / 10)
                            {
                                continue;                  // behind light surface
                            }

                            // Variable power falloff (1 = inverse linear, 2 = inverse square
                            vec_t           denominator = dist * g_fade;
                            if (g_falloff == 2)
                            {
                                denominator *= dist;
                            }
                            ratio = dot * dot2 / denominator;

                            VectorScale(l->intensity, ratio, add);
                            break;
                        }

                        case emit_spotlight:
                        {
                            dot2 = -DotProduct(delta, l->normal);
                            if (dot2 <= l->stopdot2)
                            {
                                continue;                  // outside light cone
                            }

                            // Variable power falloff (1 = inverse linear, 2 = inverse square
                            vec_t           denominator = dist * l->fade;
                            if (l->falloff == 2)
                            {
                                denominator *= dist;
                            }
                            ratio = dot * dot2 / denominator;

                            if (dot2 <= l->stopdot)
                            {
                                ratio *= (dot2 - l->stopdot2) / (l->stopdot - l->stopdot2);
                            }
                            VectorScale(l->intensity, ratio, add);
                            break;
                        }

                        default:
                        {
                            hlassume(false, assume_BadLightType);
                            break;
                        }
                        }

						VectorCopy(lighttransformed, delta); // buz: put in delta direction to light
                    }

					// buz: here adding collected light to sample
                    if (VectorMaximum(add))
                    {
                        if (l->type != emit_skylight && TestLine(pos, l->origin) != CONTENTS_EMPTY)
							continue;                      // occluded

                        if (l->type != emit_skylight)
                        {                                  // Don't test from light_environment entities to face, the special sky code occludes correctly
                            if (TestSegmentAgainstOpaqueList(pos, l->origin))
                            {
                                continue;
                            }
                        }

						// buz: add intensity to lightdir vector
						// delta must contain direction to light
						if (!(l->flags & LIGHTFLAG_NOT_RENDERER))
						{
							vec_t maxlight = VectorMaximum(add);
							VectorScale(delta, maxlight, delta);
							VectorAdd(delta, lightdir, lightdir);
						}
						else
							skiprenderer = 1;

						if (!(l->flags & LIGHTFLAG_NOT_NORMAL))
						{
							VectorAdd(sample[0], add, sample[0]);
						}
					}
                }
            }
        }
    }

//	Log("sample: %f, %f, %f\n", sample[0][0], sample[0][1], sample[0][2]);

//============== buz: make second pass, create special lightmaps ==========

	if (lightdir[0] || lightdir[1] || lightdir[2])
	{
		VectorNormalize(lightdir);
		sky_used = NULL;

		for (i = 1; i < g_numleafs; i++)
		{
			l = directlights[i];
			if (l)
			{
				if (((l->type == emit_skylight) && (g_sky_lighting_fix)) || (pvs[(i - 1) >> 3] & (1 << ((i - 1) & 7))))
				{
					for (; l; l = l->next)
					{
						if (l->flags & LIGHTFLAG_NOT_RENDERER)
							continue;

						// buz: separate light into ambient and diffuse components
						vec3_t add_ambient;
						vec3_t add_diffuse;

						// skylights work fundamentally differently than normal lights
						if (l->type == emit_skylight)
						{
							// only allow one of each sky type to hit any given point
							if (sky_used)
							{
								continue;
							}
							sky_used = l;

							VTransform(l->normal, lightmatrix, lighttransformed); // buz
						//	VectorCopy(l->normal, lighttransformed);

							// make sure the angle is okay
						//	dot = -DotProduct(normal, l->normal);
							dot = -DotProduct(normal, lighttransformed);
							if (dot <= ON_EPSILON / 10)
							{
								continue;
							}

							// search back to see if we can hit a sky brush
							VectorScale(l->normal, -10000, delta);
							VectorAdd(pos, delta, delta);
							if (TestLine(pos, delta) != CONTENTS_SKY)
							{
								continue;                      // occluded
							}

							if (TestSegmentAgainstOpaqueList(pos, delta))
							{
								continue;
							}

							// buz
						//	VectorScale(l->normal, -1, delta);
							VectorScale(lighttransformed, -1, delta);

							float dotn = DotProduct(lightdir, delta);
							if (dotn < 0) dotn = 0;
							dotn = pow(dotn, 16); // test
							VectorScale(l->intensity, dotn, add_diffuse);
							VectorScale(l->intensity, (1 - dotn)*dot, add_ambient);						
						}
						else
						{
							float           denominator;

							VectorSubtract(l->origin, pos, delta);
							dist = VectorNormalize(delta);

							VTransform(delta, lightmatrix, lighttransformed); // buz
						//	VectorCopy(delta, lighttransformed);

						//	dot = DotProduct(delta, normal);
							dot = DotProduct(lighttransformed, normal);

							// buz
						//	float dotn = DotProduct(lightdir, delta);
							float dotn = DotProduct(lightdir, lighttransformed);
							if (dotn < 0) dotn = 0;
							dotn = pow(dotn, 16); // test

							//                        if (dot <= 0.0)
							//                            continue;
							if (dot <= ON_EPSILON / 10)
							{
								continue;                      // behind sample surface
							}

							if (dist < 1.0)
							{
								dist = 1.0;
							}

							// Variable power falloff (1 = inverse linear, 2 = inverse square
							denominator = dist * l->fade;
							if (l->falloff == 2)
							{
								denominator *= dist;
							}

							switch (l->type)
							{
							case emit_point:
							{
								// Variable power falloff (1 = inverse linear, 2 = inverse square
								vec_t           denominator = dist * l->fade;

								if (l->falloff == 2)
								{
									denominator *= dist;
								}
								ratio = dot / denominator;
								float ratio2 = 1 / denominator;

								VectorScale(l->intensity, dotn*ratio2, add_diffuse);
								VectorScale(l->intensity, (1 - dotn)*ratio, add_ambient);
							//	VectorScale(l->intensity, ratio, add);
								break;
							}

							case emit_surface:
							{
								dot2 = -DotProduct(delta, l->normal);
								if (dot2 <= ON_EPSILON / 10)
								{
									continue;                  // behind light surface
								}

								// Variable power falloff (1 = inverse linear, 2 = inverse square
								vec_t           denominator = dist * g_fade;
								if (g_falloff == 2)
								{
									denominator *= dist;
								}
								ratio = dot * dot2 / denominator;
								float ratio2 = dot2 / denominator;

								VectorScale(l->intensity, dotn*ratio2, add_diffuse);
								VectorScale(l->intensity, (1 - dotn)*ratio, add_ambient);
							//	VectorScale(l->intensity, ratio, add);
								break;
							}

							case emit_spotlight:
							{
								dot2 = -DotProduct(delta, l->normal);
								if (dot2 <= l->stopdot2)
								{
									continue;                  // outside light cone
								}

								// Variable power falloff (1 = inverse linear, 2 = inverse square
								vec_t           denominator = dist * l->fade;
								if (l->falloff == 2)
								{
									denominator *= dist;
								}
								ratio = dot * dot2 / denominator;
								float ratio2 = dot2 / denominator;

								if (dot2 <= l->stopdot)
								{
									ratio *= (dot2 - l->stopdot2) / (l->stopdot - l->stopdot2);
									ratio2 *= (dot2 - l->stopdot2) / (l->stopdot - l->stopdot2);
								}

								VectorScale(l->intensity, dotn*ratio2, add_diffuse);
								VectorScale(l->intensity, (1 - dotn)*ratio, add_ambient);
							//	VectorScale(l->intensity, ratio, add);
								break;
							}

							default:
							{
								hlassume(false, assume_BadLightType);
								break;
							}
							}
						}

						// buz: here adding collected light to sample
						if (VectorMaximum(add))
						{
							if (l->type != emit_skylight && TestLine(pos, l->origin) != CONTENTS_EMPTY)
								continue;                      // occluded

							if (l->type != emit_skylight)
							{                                  // Don't test from light_environment entities to face, the special sky code occludes correctly
								if (TestSegmentAgainstOpaqueList(pos, l->origin))
								{
									continue;
								}
							}
							
						//	float dotnormal = 1 / DotProduct(lightdir, normal);
						//	VectorScale(add_diffuse, dotn, add_diffuse);

							VectorAdd(sample[BUMP_BASELIGHT_MAP], add_ambient, sample[BUMP_BASELIGHT_MAP]);
							VectorAdd(sample[BUMP_ADDLIGHT_MAP], add_diffuse, sample[BUMP_ADDLIGHT_MAP]);
						}
					}
				}
			}
		}
	}
//=========================== buz: end second pass ========================

	VectorCopy(lightdir, sample[BUMP_LIGHTVECS_MAP]); // buz

    if (sky_used && g_indirect_sun != 0.0)
    {
        vec3_t          total;
        int             j;
		vec3_t          sky_intensity;

		// -----------------------------------------------------------------------------------
		// Changes by Adam Foster - afoster@compsoc.man.ac.uk
		// Instead of using intensity from sky_used->intensity, get it from the new sky_used->diffuse_intensity
#ifdef HLRAD_WHOME
		VectorScale(sky_used->diffuse_intensity, g_indirect_sun / (NUMVERTEXNORMALS * 2), sky_intensity);
#else
        VectorScale(sky_used->intensity, g_indirect_sun / (NUMVERTEXNORMALS * 2), sky_intensity);
#endif
		// That should be it. Who knows - it might actually work!
        // AJM: It DOES actually work. Havent you ever heard of beta testing....
		// -----------------------------------------------------------------------------------

        total[0] = total[1] = total[2] = 0.0;
        for (j = 0; j < NUMVERTEXNORMALS; j++)
        {
            // make sure the angle is okay
            dot = -DotProduct(normal, r_avertexnormals[j]);
            if (dot <= ON_EPSILON / 10)
            {
                continue;
            }

            // search back to see if we can hit a sky brush
            VectorScale(r_avertexnormals[j], -10000, delta);
            VectorAdd(pos, delta, delta);
            if (TestLine(pos, delta) != CONTENTS_SKY)
            {
                continue;                                  // occluded
            }

            VectorScale(sky_intensity, dot, add);
            VectorAdd(total, add, total);
        }
        if (VectorMaximum(total) > 0)
        {
            for (style_index = 0; style_index < MAXLIGHTMAPS; style_index++)
            {
				// buz: add indirect sun lighting to ambient lightmap too
                if (styles[style_index] == 0 || styles[style_index] == BUMP_BASELIGHT_STYLE)
                {
                    VectorAdd(sample[style_index], total, sample[style_index]);
                }
            }
        }
    }

	return skiprenderer;
}

// =====================================================================================
//  AddSampleToPatch
//      Take the sample's collected light and add it back into the apropriate patch for the radiosity pass.
// =====================================================================================
static void     AddSampleToPatch(const sample_t* const s, const int facenum)

{
    patch_t*        patch;
    BoundingBox     bounds;
    int             i;

    if (g_numbounce == 0)
    {
        return;
    }

    for (patch = g_face_patches[facenum]; patch; patch = patch->next)
    {
        // see if the point is in this patch (roughly)
        patch->winding->getBounds(bounds);
        for (i = 0; i < 3; i++)
        {
            if (bounds.m_Mins[i] > s->pos[i] + 16)
            {
                goto nextpatch;
            }
            if (bounds.m_Maxs[i] < s->pos[i] - 16)
            {
                goto nextpatch;
            }
        }

        // add the sample to the patch
        patch->samples++;
        VectorAdd(patch->samplelight, s->light, patch->samplelight);
        //return;

      nextpatch:;
    }

    // don't worry if some samples don't find a patch
}

// =====================================================================================
//  GetPhongNormal
// =====================================================================================
void            GetPhongNormal(int facenum, vec3_t spot, vec3_t phongnormal)
{
    int             j;
    const dface_t*  f = g_dfaces + facenum;
    const dplane_t* p = getPlaneFromFace(f);
    vec3_t          facenormal;

    VectorCopy(p->normal, facenormal);
    VectorCopy(facenormal, phongnormal);

    if (g_smoothing_threshold > 0.0)
    {
        // Calculate modified point normal for surface
        // Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
        // Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
        // Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
        // Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

        for (j = 0; j < f->numedges; j++)
        {
            vec3_t          p1;
            vec3_t          p2;
            vec3_t          v1;
            vec3_t          v2;
            vec3_t          vspot;
            unsigned        prev_edge;
            unsigned        next_edge;
            int             e;
            int             e1;
            int             e2;
            edgeshare_t*    es;
            edgeshare_t*    es1;
            edgeshare_t*    es2;
            float           a1;
            float           a2;
            float           aa;
            float           bb;
            float           ab;

            if (j)
            {
                prev_edge = f->firstedge + ((j - 1) % f->numedges);
            }
            else
            {
                prev_edge = f->firstedge + f->numedges - 1;
            }

            if ((j + 1) != f->numedges)
            {
                next_edge = f->firstedge + ((j + 1) % f->numedges);
            }
            else
            {
                next_edge = f->firstedge;
            }

            e = g_dsurfedges[f->firstedge + j];
            e1 = g_dsurfedges[prev_edge];
            e2 = g_dsurfedges[next_edge];

            es = &g_edgeshare[abs(e)];
            es1 = &g_edgeshare[abs(e1)];
            es2 = &g_edgeshare[abs(e2)];

            if (
                (es->coplanar && es1->coplanar && es2->coplanar)
                ||
                (VectorCompare(es->interface_normal, vec3_origin) &&
                 VectorCompare(es1->interface_normal, vec3_origin) &&
                 VectorCompare(es2->interface_normal, vec3_origin)))
            {
                continue;
            }

            if (e > 0)
            {
                VectorCopy(g_dvertexes[g_dedges[e].v[0]].point, p1);
                VectorCopy(g_dvertexes[g_dedges[e].v[1]].point, p2);
            }
            else
            {
                VectorCopy(g_dvertexes[g_dedges[-e].v[1]].point, p1);
                VectorCopy(g_dvertexes[g_dedges[-e].v[0]].point, p2);
            }

            // Adjust for origin-based models
            VectorAdd(p1, g_face_offset[facenum], p1);
            VectorAdd(p2, g_face_offset[facenum], p2);

            // Build vectors from the middle of the face to the edge vertexes and the sample pos.
            VectorSubtract(p1, g_face_centroids[facenum], v1);
            VectorSubtract(p2, g_face_centroids[facenum], v2);
            VectorSubtract(spot, g_face_centroids[facenum], vspot);

            aa = DotProduct(v1, v1);
            bb = DotProduct(v2, v2);
            ab = DotProduct(v1, v2);
            a1 = (bb * DotProduct(v1, vspot) - ab * DotProduct(vspot, v2)) / (aa * bb - ab * ab);
            a2 = (DotProduct(vspot, v2) - a1 * ab) / bb;

            // Test center to sample vector for inclusion between center to vertex vectors (Use dot product of vectors)
            if (a1 >= 0.0 && a2 >= 0.0)
            {
                // calculate distance from edge to pos
                vec3_t          n1, n2;
                vec3_t          temp;

                VectorAdd(es->interface_normal, es1->interface_normal, n1);

                if (VectorCompare(n1, vec3_origin))
                {
                    VectorCopy(facenormal, n1);
                }
                VectorNormalize(n1);

                VectorAdd(es->interface_normal, es2->interface_normal, n2);

                if (VectorCompare(n2, vec3_origin))
                {
                    VectorCopy(facenormal, n2);
                }
                VectorNormalize(n2);

                // Interpolate between the center and edge normals based on sample position
                VectorScale(facenormal, 1.0 - a1 - a2, phongnormal);
                VectorScale(n1, a1, temp);
                VectorAdd(phongnormal, temp, phongnormal);
                VectorScale(n2, a2, temp);
                VectorAdd(phongnormal, temp, phongnormal);
                VectorNormalize(phongnormal);
                break;
            }
        }
    }
}

const vec3_t    s_circuscolors[] = {
    {100000.0,  100000.0,   100000.0},                              // white
    {100000.0,  0.0,        0.0     },                              // red
    {0.0,       100000.0,   0.0     },                              // green
    {0.0,       0.0,        100000.0},                              // blue
    {0.0,       100000.0,   100000.0},                              // cyan
    {100000.0,  0.0,        100000.0},                              // magenta
    {100000.0,  100000.0,   0.0     }                               // yellow
};



// =====================================================================================
//  BuildFacelights
//
// buz:
// So, dynamic lights are now ignored. This means that we have 3 free light styles.
// lightmap 0 is basic lightmap, used when not bump-mapped mode enabled.
// lightmap 1 (LIGHTVECS) is light direction vectors.
// lightmap 2 (BASELIGHT) is ambient light style.
// lightmap 3 (ADDLIGHT) is diffuse light style (added by direct light).
// =====================================================================================

int c_faces_with_dirlight = 0; // affected by direct light (4 lightmaps)
int c_faces_without_dirlight = 0; // only ambient light (1 lightmap - same for both renderer modes)
int c_faces_without_dirlight_unique = 0;//  no direct light in bump mode, but have it in normal renderer
										// (2 lightmaps - for normal and ambient for bump)

void            BuildFacelights(const int facenum)
{
    dface_t*        f;
    vec3_t          sampled[MAXLIGHTMAPS];
    lightinfo_t     l;
    int             i;
    int             j;
    int             k;
    sample_t*       s;
    vec_t*          spot;
    patch_t*        patch;
    const dplane_t* plane;
    byte            pvs[(MAX_MAP_LEAFS + 7) / 8];
    int             thisoffset = -1, lastoffset = -1;
    int             lightmapwidth;
    int             lightmapheight;
    int             size;

#ifdef HLRAD_HULLU
    bool            b_transparency_loss = false;
    vec_t           light_left_for_facelight = 1.0;
#endif

    f = &g_dfaces[facenum];

    //
    // some surfaces don't need lightmaps
    //
    f->lightofs = -1;
    for (j = 0; j < MAXLIGHTMAPS; j++)
    {
        f->styles[j] = 255;
    }

    if (g_texinfo[f->texinfo].flags & TEX_SPECIAL)
    {
        return;                                            // non-lit texture
    }

    f->styles[0] = 0;                                      // Everyone gets the style zero map.
	
	// buz
	f->styles[BUMP_LIGHTVECS_MAP] = BUMP_LIGHTVECS_STYLE;
	f->styles[BUMP_BASELIGHT_MAP] = BUMP_BASELIGHT_STYLE;
	f->styles[BUMP_ADDLIGHT_MAP] = BUMP_ADDLIGHT_STYLE;

    memset(&l, 0, sizeof(l));

    l.surfnum = facenum;
    l.face = f;

    //
    // rotate plane
    //
    plane = getPlaneFromFace(f);
    VectorCopy(plane->normal, l.facenormal);
    l.facedist = plane->dist;

    CalcFaceVectors(&l);
    CalcFaceExtents(&l);
    CalcPoints(&l);

    lightmapwidth = l.texsize[0] + 1;
    lightmapheight = l.texsize[1] + 1;

    size = lightmapwidth * lightmapheight;
    hlassume(size <= MAX_SINGLEMAP, assume_MAX_SINGLEMAP);

    facelight[facenum].numsamples = l.numsurfpt;

    for (k = 0; k < MAXLIGHTMAPS; k++)
    {
        facelight[facenum].samples[k] = (sample_t*)calloc(l.numsurfpt, sizeof(sample_t));
    }

	const dplane_t* p = getPlaneFromFace(f); // buz

	int got_direct_light = 0; // buz
	int got_diflights = 0; // buz
    spot = l.surfpt[0];
    for (i = 0; i < l.numsurfpt; i++, spot += 3)
    {
        vec3_t          pointnormal = { 0, 0, 0 };

        for (k = 0; k < MAXLIGHTMAPS; k++)
        {
            VectorCopy(spot, facelight[facenum].samples[k][i].pos);
        }

        // get the PVS for the pos to limit the number of checks
        if (!g_visdatasize)
        {
            memset(pvs, 255, (g_numleafs + 7) / 8);
            lastoffset = -1;
        }
        else
        {
            dleaf_t*        leaf = PointInLeaf(spot);

            thisoffset = leaf->visofs;
            if (i == 0 || thisoffset != lastoffset)
            {
                hlassert(thisoffset != -1);
                DecompressVis(&g_dvisdata[leaf->visofs], pvs, sizeof(pvs));
            }
            lastoffset = thisoffset;
        }

        memset(sampled, 0, sizeof(sampled));

        // If we are doing "extra" samples, oversample the direct light around the point.
        if (g_extra)
        {
            int             weighting[3][3] = { {5, 9, 5}, {9, 16, 9}, {5, 9, 5} };
            vec3_t          pos;
            int             s, t, subsamples = 0;

            for (t = -1; t <= 1; t++)
            {
                for (s = -1; s <= 1; s++)
                {
                    int             subsample = i + t * lightmapwidth + s;
                    int             sample_s = i % lightmapwidth;
                    int sample_t = i / lightmapwidth;

                    if ((0 <= s + sample_s) && (s + sample_s < lightmapwidth)
                        && (0 <= t + sample_t)&&(t + sample_t <lightmapheight))
                    {
                        vec3_t          subsampled[MAXLIGHTMAPS];

                        for (j = 0; j < MAXLIGHTMAPS; j++)
                        {
                            VectorFill(subsampled[j], 0);
                        }

                        // Calculate the point one third of the way toward the "subsample point"
                        VectorCopy(l.surfpt[i], pos);
                        VectorAdd(pos, l.surfpt[i], pos);
                        VectorAdd(pos, l.surfpt[subsample], pos);
                        VectorScale(pos, 1.0 / 3.0, pos);

						float lightmatrix[9];
                        GetPhongNormal(facenum, pos, pointnormal);
						GetRotationMatrix(pointnormal, p->normal, lightmatrix);
                        if (GatherSampleLight(pos, pvs, /*pointnormal*/ p->normal, subsampled, f->styles, lightmatrix))
							got_diflights = 1;

						for (j = 0; j < MAXLIGHTMAPS; j++)
						{
							VectorScale(subsampled[j], weighting[s + 1][t + 1], subsampled[j]);
							VectorAdd(sampled[j], subsampled[j], sampled[j]);
						}
                        subsamples += weighting[s + 1][t + 1];
                    }
                }
            }
			for (j = 0; j < MAXLIGHTMAPS; j++)
			{
				if (j == BUMP_LIGHTVECS_MAP)
					VectorNormalize(sampled[BUMP_LIGHTVECS_MAP]); // buz
				else
					VectorScale(sampled[j], 1.0 / subsamples, sampled[j]);
			}
        }
        else
        {
			float lightmatrix[9];			
            GetPhongNormal(facenum, spot, pointnormal);
			GetRotationMatrix(pointnormal, p->normal, lightmatrix);
            if (GatherSampleLight(spot, pvs, /*pointnormal*/ p->normal, sampled, f->styles, lightmatrix))
				got_diflights = 1;
        }

		// buz: light vector is zero if no direct light received
		if (sampled[BUMP_LIGHTVECS_MAP][0] || sampled[BUMP_LIGHTVECS_MAP][1] || sampled[BUMP_LIGHTVECS_MAP][2])
		{
			got_direct_light = 1;

			// buz: transform light direction vector to surface's tangent space
			vec3_t tmp;
			texinfo_t *tx = &g_texinfo[f->texinfo];
			const dplane_t* p = getPlaneFromFace(f);
			vec3_t tex_x, tex_y;
			VectorCopy(tx->vecs[0], tex_x);
			VectorCopy(tx->vecs[1], tex_y);
			VectorNormalize(tex_x);
			VectorNormalize(tex_y);
			tmp[0] = DotProduct(tex_x, sampled[BUMP_LIGHTVECS_MAP]);
			tmp[1] = DotProduct(tex_y, sampled[BUMP_LIGHTVECS_MAP]);
			tmp[2] = DotProduct(p->normal, sampled[BUMP_LIGHTVECS_MAP]);
			VectorCopy(tmp, sampled[BUMP_LIGHTVECS_MAP]);
		}
		else
		{
			sampled[BUMP_LIGHTVECS_MAP][0] = 0;
			sampled[BUMP_LIGHTVECS_MAP][1] = 0;
			sampled[BUMP_LIGHTVECS_MAP][2] = 0.5; // 0 makes dark, 1 makes bright.. take 0.5?
		//	VectorCopy(sampled[0], sampled[BUMP_BASELIGHT_MAP]);
		}

		sampled[BUMP_LIGHTVECS_MAP][0] = (sampled[BUMP_LIGHTVECS_MAP][0] + 1) * 127;
		sampled[BUMP_LIGHTVECS_MAP][1] = (sampled[BUMP_LIGHTVECS_MAP][1] + 1) * 127;
		sampled[BUMP_LIGHTVECS_MAP][2] = (sampled[BUMP_LIGHTVECS_MAP][2] + 1) * 127;

        for (j = 0; j < MAXLIGHTMAPS && (f->styles[j] != 255); j++)
        {
            VectorCopy(sampled[j], facelight[facenum].samples[j][i].light);
/*
#ifdef HLRAD_HULLU            
            if(b_transparency_loss && (f->styles[j] != BUMP_LIGHTVECS_STYLE)) // buz
            {
				VectorScale(facelight[facenum].samples[j][i].light, light_left_for_facelight, facelight[facenum].samples[j][i].light);
            }
#endif*/
            if (j == 0) // buz: dont spread around special lightmaps
            {
                AddSampleToPatch(&facelight[facenum].samples[j][i], facenum);
            }
        }

	//	Log("%f, %f, %f\n", sampled[BUMP_ADDLIGHT_MAP][0], sampled[BUMP_ADDLIGHT_MAP][1], sampled[BUMP_ADDLIGHT_MAP][2]);
    }

	// buz: special bump-mapping lightmaps should be saved only if
	// surface was affected by direct light
	if (!got_direct_light)
	{
		if (!got_diflights) f->styles[BUMP_BASELIGHT_MAP] = 255;
		f->styles[BUMP_ADDLIGHT_MAP] = 255;
		f->styles[BUMP_LIGHTVECS_MAP] = 255;
		c_faces_without_dirlight++;
	}
	else
		c_faces_with_dirlight++;

    // average up the direct light on each patch for radiosity
    if (g_numbounce > 0)
    {
        for (patch = g_face_patches[facenum]; patch; patch = patch->next)
        {
            if (patch->samples)
            {
                vec3_t          v;  // BUGBUG: Use a weighted average instead?

                VectorScale(patch->samplelight, (1.0f / patch->samples), v);
                VectorAdd(patch->totallight, v, patch->totallight);
                VectorAdd(patch->directlight, v, patch->directlight);
            }
        }
    }

    // add an ambient term if desired
    if (g_ambient[0] || g_ambient[1] || g_ambient[2])
    {
        for (j = 0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++)
        {
			// buz: add ambient highlight to ambient lightmap
            if ((f->styles[j] == 0) || (f->styles[j] == BUMP_BASELIGHT_STYLE))
            {
                s = facelight[facenum].samples[j];
                for (i = 0; i < l.numsurfpt; i++, s++)
                {
                    VectorAdd(s->light, g_ambient, s->light);
                }
            }
        }
    }

    for (j = 0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++)
    {
        if (f->styles[j] == 0 || (f->styles[j] == BUMP_BASELIGHT_STYLE))
        {
            if (g_face_patches[facenum])
            {
                s = facelight[facenum].samples[j];
                for (i = 0; i < l.numsurfpt; i++, s++)
                {
                    VectorAdd(s->light, g_face_patches[facenum]->baselight, s->light);
                }
            }
        }
    }
}

void BUZ_Report()
{
	Log("%d faces total, %d faces has bump, %d faces has no direct lighting\n",
		c_faces_with_dirlight + c_faces_without_dirlight,
		c_faces_with_dirlight,
		c_faces_without_dirlight);
}

// =====================================================================================
//  PrecompLightmapOffsets
// =====================================================================================
void            PrecompLightmapOffsets()
{
    int             facenum;
    dface_t*        f;
    facelight_t*    fl;
    int             lightstyles;

    g_lightdatasize = 0;

    for (facenum = 0; facenum < g_numfaces; facenum++)
    {
        f = &g_dfaces[facenum];
        fl = &facelight[facenum];

        if (g_texinfo[f->texinfo].flags & TEX_SPECIAL)
        {
            continue;                                      // non-lit texture
        }

        for (lightstyles = 0; lightstyles < MAXLIGHTMAPS; lightstyles++)
        {
            if (f->styles[lightstyles] == 255)
            {
                break;
            }
        }

        if (!lightstyles)
        {
            continue;
        }

        f->lightofs = g_lightdatasize;
        g_lightdatasize += fl->numsamples * 3 * lightstyles;
    }
}


// buz
#define ClampVector(lb) \
{ \
	vec_t max = VectorMaximum(lb); \
	if (max > 255.0) \
{ \
		vec_t scale = 255.0 / max; \
		lb[0] *= scale; \
		lb[1] *= scale; \
		lb[2] *= scale; \
	} \
}

// buz
void PackColor(vec3_t lb, int facenum, int style, int sample)
{
	dface_t *f = &g_dfaces[facenum];
    facelight_t* fl = &facelight[facenum];
	unsigned char* colors;
	colors = &g_dlightdata[f->lightofs + style * fl->numsamples * 3 + sample * 3];

	// check
	for (int i = 0; i < 3; i++)
	{
		if (lb[i] < 0) Log("WARNING: component %d on style %d is too low: %f\n", i, style, lb[i]);
		if (lb[i] > 256.0) Log("WARNING: component %d on style %d is too high: %f\n", i, style, lb[i]);
	}
	
	colors[0] = (unsigned char)lb[0];
	colors[1] = (unsigned char)lb[1];
	colors[2] = (unsigned char)lb[2];
}



// =====================================================================================
//  FinalLightFace
//      Add the indirect lighting on top of the direct lighting and save into final map format
// =====================================================================================
void            FinalLightFace(const int facenum)
{
    int             i, j;
    vec3_t          v;
    facelight_t*    fl;
    float           minlight;
    int             lightstyles;
    dface_t*        f;
    lerpTriangulation_t* trian = NULL;

    // ------------------------------------------------------------------------
    // Changes by Adam Foster - afoster@compsoc.man.ac.uk
#ifdef HLRAD_WHOME
//    float           temp_rand;
#endif
    // ------------------------------------------------------------------------

    f = &g_dfaces[facenum];
    fl = &facelight[facenum];

    if (g_texinfo[f->texinfo].flags & TEX_SPECIAL)
    {
        return;                                            // non-lit texture
    }

    for (lightstyles = 0; lightstyles < MAXLIGHTMAPS; lightstyles++)
    {
        if (f->styles[lightstyles] == 255)
        {
            break;
        }
    }

    if (!lightstyles)
    {
        return;
    }

    //
    // set up the triangulation
    //
    if (g_numbounce)
    {
        trian = CreateTriangulation(facenum);
    }
    //
    // sample the triangulation
    //
    minlight = FloatForKey(g_face_entity[facenum], "_minlight") * 128;

	// buz: бля, хотел все упростить, но потом полезли хаки и прочее, и как всегда
	// все засралось непонять чем.

	sample_t *samp_orighlight = fl->samples[0];
	sample_t *samp_baselight = NULL;
	sample_t *samp_lightvecs = NULL;
	sample_t *samp_addlight = NULL;

    if (lightstyles == 4) // all bump styles present
    {
		samp_baselight = fl->samples[BUMP_BASELIGHT_MAP];
		samp_addlight = fl->samples[BUMP_ADDLIGHT_MAP];
		samp_lightvecs = fl->samples[BUMP_LIGHTVECS_MAP];

		// check styles
		if (f->styles[BUMP_BASELIGHT_MAP] != BUMP_BASELIGHT_STYLE ||
			f->styles[BUMP_ADDLIGHT_MAP] != BUMP_ADDLIGHT_STYLE ||
			f->styles[BUMP_LIGHTVECS_MAP] != BUMP_LIGHTVECS_STYLE)
			Log("WARNING: styles mismatch!!!\n");
	}
	else if (lightstyles == 2)
	{
		samp_baselight = fl->samples[BUMP_BASELIGHT_MAP];
		if (f->styles[BUMP_BASELIGHT_MAP] != BUMP_BASELIGHT_STYLE)
			Log("WARNING: styles mismatch 2!!!\n");
	}
	else if (lightstyles != 1)
	{
		Log("WARNING: bad lightstyles number: %d!!\n", lightstyles);
	}

    for (j = 0; j < fl->numsamples; j++, samp_orighlight++, samp_baselight++, samp_lightvecs++, samp_addlight++)
    {
		vec3_t origlight, baselight, lightvecs, addlight;

        // Should be a VectorCopy, but we scale by 2 to compensate for an earlier lighting flaw
        // Specifically, the directlight contribution was included in the bounced light AND the directlight
        // Since many of the levels were built with this assumption, this "fudge factor" compensates for it.

		VectorScale(samp_orighlight->light, g_direct_scale, origlight);
		if (lightstyles == 4)
		{
			VectorScale(samp_addlight->light, g_direct_scale, addlight);
			VectorScale(samp_baselight->light, g_direct_scale, baselight);
			VectorCopy(samp_lightvecs->light, lightvecs);
		}
		else if (lightstyles == 2)
		{
			VectorScale(samp_baselight->light, g_direct_scale, baselight);
		}

        if (g_numbounce)
        {
            SampleTriangulation(trian, samp_orighlight->pos, v);

            if (isPointFinite(v))
            {
                VectorAdd(origlight, v, origlight);
				if (lightstyles == 4 || lightstyles == 2)
					VectorAdd(baselight, v, baselight);
            }
            else
            {
                Warning("point (%4.3f %4.3f %4.3f) infinite v (%4.3f %4.3f %4.3f)\n",
                        samp_orighlight->pos[0], samp_orighlight->pos[1], samp_orighlight->pos[2], v[0], v[1], v[2]);
            }
        }

		VectorMultiply(origlight, g_colour_lightscale, origlight);
		if (lightstyles == 4)
		{
			VectorMultiply(baselight, g_colour_lightscale, baselight);
			VectorMultiply(addlight, g_colour_lightscale, addlight);
		}
		else if (lightstyles == 2)
		{
			VectorMultiply(baselight, g_colour_lightscale, baselight);
		}

		// buz - minlight and maxlight are ignored

		// buz - gamma calculations for zero style and bump styles are independent
		if ( g_colour_qgamma[0] != 1.0 )
		{
			if (gammamode == 0)
			{
				for (i = 0; i < 3; i++)
				{
					origlight[i] = (float) pow(origlight[i] / 256.0f, g_colour_qgamma[i]) * 256.0f;
				}

				if (lightstyles == 2)
				{
					for (i = 0; i < 3; i++)
					{
						baselight[i] = (float) pow(baselight[i] / 256.0f, g_colour_qgamma[i]) * 256.0f;
					}
				}
			}
			else 
			{
				float avg;
				if (gammamode == 2)
				{
					avg = VectorMaximum(origlight);
				}
				else
				{
					avg = VectorAvg(origlight);
				}
				float avgbefore = avg;
				if (avgbefore)
				{
					avg = (float) pow(avg / 256.0f, g_colour_qgamma[0]) * 256.0f;
					avg = avg / avgbefore;
					VectorScale(origlight, avg, origlight);
				}

				if (lightstyles == 2)
				{
					if (gammamode == 2)
					{
						avg = VectorMaximum(baselight);
					}
					else
					{
						avg = VectorAvg(baselight);
					}
					float avgbefore = avg;
					if (avgbefore)
					{
						avg = (float) pow(avg / 256.0f, g_colour_qgamma[0]) * 256.0f;
						avg = avg / avgbefore;
						VectorScale(baselight, avg, baselight);
					}
				}
			}
		}

		// gamma for bump styles, more complex
		if (lightstyles == 4)
		{
			// get average bumped value
			vec3_t bumplight, scale;
			VectorClear(scale);

			float dot = (lightvecs[2] / 127) - 1;
			if (dot < 0) dot = 0;
			VectorScale(addlight, dot, bumplight);
			VectorAdd(bumplight, baselight, bumplight);

			// calc scale depending on gammamode
			if (gammamode == 0)
			{
				for (i = 0; i < 3; i++)
				{
					if (bumplight[i])
					{
						if ( g_colour_qgamma[i] != 1.0 ) 
							scale[i] = (float) pow(bumplight[i] / 256.0f, g_colour_qgamma[i]) * 256.0f;
						scale[i] = scale[i] / bumplight[i];
					}
				}
			}
			else
			{
				float avg;
				if (gammamode == 2)
				{
					avg = VectorMaximum(bumplight);
				}
				else
				{
					avg = VectorAvg(bumplight);
				}
				float avgbefore = avg;
				if (avgbefore)
				{
					avg = (float) pow(avg / 256.0f, g_colour_qgamma[0]) * 256.0f;
					avg = avg / avgbefore;
					scale[0] = scale[1] = scale[2] = avg;
				}
			}

			VectorMultiply(baselight, scale, baselight);
			VectorMultiply(addlight, scale, addlight);

			// buz - divide by two bump styles.
			// this decreases precision, but increases range
			VectorScale(baselight, 0.5, baselight);
			VectorScale(addlight, 0.5, addlight);

			ClampVector(baselight);
			ClampVector(addlight);
		}

		ClampVector(origlight);

		PackColor(origlight, facenum, 0, j);
		if (lightstyles == 4)
		{
			// ACHTUNG! Hardcoded styles placement!!!
		//	PackColor(lightvecs, facenum, 1, j);
		//	PackColor(baselight, facenum, 2, j);
		//	PackColor(addlight,  facenum, 3, j);
			PackColor(baselight, facenum, 1, j);
			PackColor(lightvecs, facenum, 2, j);
			PackColor(addlight,  facenum, 3, j);
		}
		else if (lightstyles == 2)
		{
			VectorScale(baselight, 0.5, baselight);
			ClampVector(baselight);
			PackColor(baselight, facenum, 1, j);
		//	Log("packing two lightmaps!!!!\n");
		}
    }

    if (g_numbounce)
    {
        FreeTriangulation(trian);
    }
}


void BUZ_AddSpecialSurface()
{
	memset(&g_dfaces[g_numfaces], 0, sizeof(dface_t));
	g_dfaces[g_numfaces].firstedge = g_numsurfedges;
	g_dfaces[g_numfaces].numedges = 2;
	g_dfaces[g_numfaces].texinfo = g_numtexinfo;
	g_dfaces[g_numfaces].styles[0] = 0;
	g_dfaces[g_numfaces].styles[1] = 255;
	g_dfaces[g_numfaces].styles[2] = 255;
	g_dfaces[g_numfaces].styles[3] = 255;
	g_dfaces[g_numfaces].lightofs = g_lightdatasize;
	g_numfaces++;

	memset(&g_texinfo[g_numtexinfo], 0, sizeof(texinfo_t));
	g_texinfo[g_numtexinfo].vecs[0][0] = 1;
	g_texinfo[g_numtexinfo].vecs[1][1] = 1;
	g_numtexinfo++;

	g_dsurfedges[g_numsurfedges] = g_numedges;
	g_numsurfedges++;
	g_dsurfedges[g_numsurfedges] = g_numedges+1;
	g_numsurfedges++;

	g_dedges[g_numedges].v[0] = g_numvertexes;
	g_numedges++;
	g_dedges[g_numedges].v[0] = g_numvertexes+1;
	g_numedges++;

	VectorClear(g_dvertexes[g_numvertexes].point);
	g_numvertexes++;
	g_dvertexes[g_numvertexes].point[0] = 160;
	g_dvertexes[g_numvertexes].point[1] = 160;
	g_dvertexes[g_numvertexes].point[2] = 0;
	g_numvertexes++;

	const int size = 11 * 11 * 3;
	byte *map = &g_dlightdata[g_lightdatasize];
	g_lightdatasize += size;
	for (int i = 0; i < size; i++, map++)
	{
		if (i < 256)
			*map = (byte)i;
		else
			*map = 255;
	}
}

