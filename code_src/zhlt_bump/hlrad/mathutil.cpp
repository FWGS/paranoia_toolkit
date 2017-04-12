#include "qrad.h"

// =====================================================================================
//  point_in_winding
// =====================================================================================
bool            point_in_winding(const Winding& w, const dplane_t& plane, const vec_t* const point)
{
    unsigned        numpoints = w.m_NumPoints;
    int             x;

    for (x = 0; x < numpoints; x++)
    {
        vec3_t          A;
        vec3_t          B;
        vec3_t          normal;

        VectorSubtract(w.m_Points[(x + 1) % numpoints], point, A);
        VectorSubtract(w.m_Points[x], point, B);
        CrossProduct(A, B, normal);
        VectorNormalize(normal);

        if (DotProduct(normal, plane.normal) < 0.0)
        {
            return false;
        }
    }

    return true;
}

// =====================================================================================
//  point_in_wall
// =====================================================================================
bool            point_in_wall(const lerpWall_t* wall, vec3_t point)
{
    int             x;

    // Liberal use of the magic number '4' for the hardcoded winding count
    for (x = 0; x < 4; x++)
    {
        vec3_t          A;
        vec3_t          B;
        vec3_t          normal;

        VectorSubtract(wall->vertex[x], wall->vertex[(x + 1) % 4], A);
        VectorSubtract(wall->vertex[x], point, B);
        CrossProduct(A, B, normal);
        VectorNormalize(normal);

        if (DotProduct(normal, wall->plane.normal) < 0.0)
        {
            return false;
        }
    }
    return true;
}

// =====================================================================================
//  point_in_tri
// =====================================================================================
bool            point_in_tri(const vec3_t point, const dplane_t* const plane, const vec3_t p1, const vec3_t p2, const vec3_t p3)
{
    vec3_t          A;
    vec3_t          B;
    vec3_t          normal;

    VectorSubtract(p1, p2, A);
    VectorSubtract(p1, point, B);
    CrossProduct(A, B, normal);
    VectorNormalize(normal);

    if (DotProduct(normal, plane->normal) < 0.0)
    {
        return false;
    }

    VectorSubtract(p2, p3, A);
    VectorSubtract(p2, point, B);
    CrossProduct(A, B, normal);
    VectorNormalize(normal);

    if (DotProduct(normal, plane->normal) < 0.0)
    {
        return false;
    }

    VectorSubtract(p3, p1, A);
    VectorSubtract(p3, point, B);
    CrossProduct(A, B, normal);
    VectorNormalize(normal);

    if (DotProduct(normal, plane->normal) < 0.0)
    {
        return false;
    }
    return true;
}

// =====================================================================================
//  intersect_line_plane
//      returns true if line hits plane, and parameter 'point' is filled with where
// =====================================================================================
bool            intersect_line_plane(const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2, vec3_t point)
{
    vec3_t          pop;
    vec3_t          line_vector;                           // normalized vector for the line;
    vec3_t          tmp;
    vec3_t          scaledDir;
    vec_t           partial;
    vec_t           total;
    vec_t           perc;

    // Get a normalized vector for the ray
    VectorSubtract(p1, p2, line_vector);
    VectorNormalize(line_vector);

    VectorScale(plane->normal, plane->dist, pop);
    VectorSubtract(pop, p1, tmp);
    partial = DotProduct(tmp, plane->normal);
    total = DotProduct(line_vector, plane->normal);

    if (total == 0.0)
    {
        VectorClear(point);
        return false;
    }

    perc = partial / total;
    VectorScale(line_vector, perc, scaledDir);
    VectorAdd(p1, scaledDir, point);
    return true;
}

// =====================================================================================
//  intersect_linesegment_plane
//      returns true if line hits plane, and parameter 'point' is filled with where
// =====================================================================================
bool            intersect_linesegment_plane(const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2, vec3_t point)
{
    unsigned        count = 0;

    if (DotProduct(plane->normal, p1) <= plane->dist)
    {
        count++;
    }
    if (DotProduct(plane->normal, p2) <= plane->dist)
    {
        count++;
    }

    if (count == 1)
    {
        return intersect_line_plane(plane, p1, p2, point);
    }
    else
    {
        return false;
    }
}

// =====================================================================================
//  plane_from_points
// =====================================================================================
void            plane_from_points(const vec3_t p1, const vec3_t p2, const vec3_t p3, dplane_t* plane)
{
    vec3_t          delta1;
    vec3_t          delta2;
    vec3_t          normal;

    VectorSubtract(p3, p2, delta1);
    VectorSubtract(p1, p2, delta2);
    CrossProduct(delta1, delta2, normal);
    VectorNormalize(normal);
    plane->dist = DotProduct(normal, p1);
    VectorCopy(normal, plane->normal);
}

// =====================================================================================
//  TestSegmentAgainstOpaqueList
//      Returns true if the segment intersects an item in the opaque list
// =====================================================================================
#ifdef HLRAD_HULLU
bool            TestSegmentAgainstOpaqueList(const vec_t* p1, const vec_t* p2, vec3_t &scaleout)
#else
bool            TestSegmentAgainstOpaqueList(const vec_t* p1, const vec_t* p2)
#endif
{
    unsigned        x;
    vec3_t          point;
    const dplane_t* plane;
    const Winding*  winding;

#ifdef HLRAD_HULLU
    vec3_t	    scale = {1.0, 1.0, 1.0};
#endif

    for (x = 0; x < g_opaque_face_count; x++)
    {
        plane = &g_opaque_face_list[x].plane;
        winding = g_opaque_face_list[x].winding;

#ifdef HLRAD_OPACITY // AJM
        l_opacity = g_opaque_face_list[x].l_opacity;
#endif
        if (intersect_linesegment_plane(plane, p1, p2, point))
        {
#if 0
            Log
                ("Ray from (%4.3f %4.3f %4.3f) to (%4.3f %4.3f %4.3f) hits plane at (%4.3f %4.3f %4.3f)\n Plane (%4.3f %4.3f %4.3f) %4.3f\n",
                 p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], point[0], point[1], point[2], plane->normal[0],
                 plane->normal[1], plane->normal[2], plane->dist);
#endif
            if (point_in_winding(*winding, *plane, point))
            {
#if 0
                Log("Ray from (%4.3f %4.3f %4.3f) to (%4.3f %4.3f %4.3f) blocked by face %u @ (%4.3f %4.3f %4.3f)\n",
                    p1[0], p1[1], p1[2],
                    p2[0], p2[1], p2[2], g_opaque_face_list[x].facenum, point[0], point[1], point[2]);
#endif

#ifdef HLRAD_HULLU
		        if(g_opaque_face_list[x].transparency)
		        {
			        VectorMultiply(scale, g_opaque_face_list[x].transparency_scale, scale);
		        }
                else
                {
                    VectorCopy(scale, scaleout);
                	return true;
                }
#else
                return true;
#endif
            }
        }
    }

#ifdef HLRAD_HULLU
    VectorCopy(scale, scaleout);
    if(scaleout[0] < 0.01 && scaleout[1] < 0.01 && scaleout[2] < 0.01)
    {
    	return true; //so much shadowing that result is same as with normal opaque face
    }
#endif

    return false;
}

// =====================================================================================
//  ProjectionPoint
// =====================================================================================
void            ProjectionPoint(const vec_t* const v, const vec_t* const p, vec_t* rval)
{
    vec_t           val;
    vec_t           mag;

    mag = DotProduct(p, p);
#ifdef SYSTEM_POSIX
    if (mag == 0)
    {
        // division by zero seems to work just fine on x86;
        // it returns nan and the program still works!!
        // this causes a floating point exception on Alphas, so...
        mag = 0.00000001; 
    }
#endif
    val = DotProduct(v, p) / mag;

    VectorScale(p, val, rval);
}

// =====================================================================================
//  SnapToPlane
// =====================================================================================
void            SnapToPlane(const dplane_t* const plane, vec_t* const point, vec_t offset)
{
    vec3_t          delta;
    vec3_t          proj;
    vec3_t          pop;                                   // point on plane

    VectorScale(plane->normal, plane->dist + offset, pop);
    VectorSubtract(point, pop, delta);
    ProjectionPoint(delta, plane->normal, proj);
    VectorSubtract(delta, proj, delta);
    VectorAdd(delta, pop, point);
}
