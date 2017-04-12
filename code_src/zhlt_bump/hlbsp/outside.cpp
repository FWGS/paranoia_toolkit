#include "bsp5.h"

//  PointInLeaf
//  PlaceOccupant
//  MarkLeakTrail
//  RecursiveFillOutside
//  ClearOutFaces_r
//  isClassnameAllowableOutside
//  FreeAllowableOutsideList
//  LoadAllowableOutsideList
//  FillOutside

static int      outleafs;
static int      valid;
static int      c_falsenodes;
static int      c_free_faces;
static int      c_keep_faces;

// =====================================================================================
//  PointInLeaf
// =====================================================================================
static node_t*  PointInLeaf(node_t* node, const vec3_t point)
{
    vec_t           d;

    if (node->contents)
    {
        //Log("PointInLeaf::node->contents == %i\n", node->contents);
        return node;
    }

    d = DotProduct(g_dplanes[node->planenum].normal, point) - g_dplanes[node->planenum].dist;

    if (d > 0)
        return PointInLeaf(node->children[0], point);

    return PointInLeaf(node->children[1], point);
}

// =====================================================================================
//  PlaceOccupant
// =====================================================================================
static bool     PlaceOccupant(const int num, const vec3_t point, node_t* headnode)
{
    node_t*         n;

    n = PointInLeaf(headnode, point);
    if (n->contents == CONTENTS_SOLID)
    {
        return false;
    }
    //Log("PlaceOccupant::n->contents == %i\n", n->contents);

    n->occupied = num;
    return true;
}

// =====================================================================================
//  MarkLeakTrail
// =====================================================================================
static portal_t* prevleaknode;
static FILE*    pointfile;
static FILE*    linefile;

static void     MarkLeakTrail(portal_t* n2)
{
    int             i;
    vec3_t          p1, p2, dir;
    float           len;
    portal_t*       n1;

    n1 = prevleaknode;
    prevleaknode = n2;

    if (!n1)
    {
        return;
    }

    n2->winding->getCenter(p1);
    n1->winding->getCenter(p2);

    // Linefile
    fprintf(linefile, "%f %f %f - %f %f %f\n", p1[0], p1[1], p1[2], p2[0], p2[1], p2[2]);

    // Pointfile
    fprintf(pointfile, "%f %f %f\n", p1[0], p1[1], p1[2]);

    VectorSubtract(p2, p1, dir);
    len = VectorLength(dir);
    VectorNormalize(dir);

    while (len > 2)
    {
        fprintf(pointfile, "%f %f %f\n", p1[0], p1[1], p1[2]);
        for (i = 0; i < 3; i++)
            p1[i] += dir[i] * 2;
        len -= 2;
    }
}

// =====================================================================================
//  RecursiveFillOutside
//      Returns true if an occupied leaf is reached
//      If fill is false, just check, don't fill 
// =====================================================================================
static int      hit_occupied;
static int      backdraw;
static bool     RecursiveFillOutside(node_t* l, const bool fill)
{
    portal_t*       p;
    int             s;

    if ((l->contents == CONTENTS_SOLID) || (l->contents == CONTENTS_SKY) 
#ifdef ZHLT_DETAIL
        || (l->contents == CONTENTS_DETAIL)
#endif
        )
	{
        /*if (l->contents != CONTENTS_SOLID)
            Log("RecursiveFillOutside::l->contents == %i \n", l->contents);*/

        return false;
    }

    if (l->valid == valid)
    {
        return false;
    }

    if (l->occupied)
    {
        hit_occupied = l->occupied;
        backdraw = 1000;
        return true;
    }

    l->valid = valid;

    // fill it and it's neighbors
    if (fill)
    {
        l->contents = CONTENTS_SOLID;
        l->planenum = -1;
    }
    outleafs++;

    for (p = l->portals; p;)
    {
        s = (p->nodes[0] == l);

        if (RecursiveFillOutside(p->nodes[s], fill))
        {                                                  // leaked, so stop filling
            if (backdraw-- > 0)
            {
                MarkLeakTrail(p);
            }
            return true;
        }
        p = p->next[!s];
    }

    return false;
}

// =====================================================================================
//  ClearOutFaces_r
//      Removes unused nodes
// =====================================================================================
static node_t*  ClearOutFaces_r(node_t* node)
{
    face_t*         f;
    face_t*         fnext;
    face_t**        fp;
    portal_t*       p;

    // mark the node and all it's faces, so they
    // can be removed if no children use them

    node->valid = 0;                                       // will be set if any children touch it
    for (f = node->faces; f; f = f->next)
    {
        f->outputnumber = -1;
    }

    // go down the children
    if (node->planenum != -1)
    {
        //
        // decision node
        //
        node->children[0] = ClearOutFaces_r(node->children[0]);
        node->children[1] = ClearOutFaces_r(node->children[1]);

        // free any faces not in open child leafs
        f = node->faces;
        node->faces = NULL;

        for (; f; f = fnext)
        {
            fnext = f->next;
            if (f->outputnumber == -1)
            {                                              // never referenced, so free it
                c_free_faces++;
                FreeFace(f);
            }
            else
            {
                c_keep_faces++;
                f->next = node->faces;
                node->faces = f;
            }
        }

        if (!node->valid)
        {
            // this node does not touch any interior leafs

            // if both children are solid, just make this node solid
            if (node->children[0]->contents == CONTENTS_SOLID && node->children[1]->contents == CONTENTS_SOLID)
            {
                node->contents = CONTENTS_SOLID;
                node->planenum = -1;
                return node;
            }

            // if one child is solid, shortcut down the other side
            if (node->children[0]->contents == CONTENTS_SOLID)
            {
                return node->children[1];
            }
            if (node->children[1]->contents == CONTENTS_SOLID)
            {
                return node->children[0];
            }

            c_falsenodes++;
        }
        return node;
    }

    //
    // leaf node
    //
    if (node->contents != CONTENTS_SOLID)
    {
        // this node is still inside

        // mark all the nodes used as portals
        for (p = node->portals; p;)
        {
            if (p->onnode)
            {
                p->onnode->valid = 1;
            }
            if (p->nodes[0] == node)                       // only write out from first leaf
            {
                p = p->next[0];
            }
            else
            {
                p = p->next[1];
            }
        }

        // mark all of the faces to be drawn
        for (fp = node->markfaces; *fp; fp++)
        {
            (*fp)->outputnumber = 0;
        }

        return node;
    }

    // this was a filled in node, so free the markfaces
    if (node->planenum != -1)
    {
        free(node->markfaces);
    }

    return node;
}

// =====================================================================================
//  isClassnameAllowableOutside
// =====================================================================================
#define  MAX_ALLOWABLE_OUTSIDE_GROWTH_SIZE 64

unsigned        g_nAllowableOutside = 0;
unsigned        g_maxAllowableOutside = 0;
char**          g_strAllowableOutsideList;

bool            isClassnameAllowableOutside(const char* const classname)
{
    if (g_strAllowableOutsideList)
    {
        unsigned        x;
        char**          list = g_strAllowableOutsideList;

        for (x = 0; x < g_nAllowableOutside; x++, list++)
        {
            if (list)
            {
                if (!strcasecmp(classname, *list))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

// =====================================================================================
//  FreeAllowableOutsideList
// =====================================================================================
void            FreeAllowableOutsideList()
{
    if (g_strAllowableOutsideList)
    {
        free(g_strAllowableOutsideList);
        g_strAllowableOutsideList = NULL;
    }
}

// =====================================================================================
//  LoadAllowableOutsideList
// =====================================================================================
void            LoadAllowableOutsideList(const char* const filename)
{
    char*           fname;
    int             i, x, y;
    char*           pData;
    char*           pszData;

    if (!filename)
    {
        return;
    }
    else
    {
        unsigned        len = strlen(filename) + 5;

        fname = (char*)Alloc(len);
        safe_snprintf(fname, len, "%s", filename);
    }

    if (q_exists(fname))
    {
        if ((i = LoadFile(fname, &pData)))
        {
            Log("Reading allowable void entities from file '%s'\n", fname);
            g_nAllowableOutside = 0;
            for (pszData = pData, y = 0, x = 0; x < i; x++)
            {
                if ((pData[x] == '\n') || (pData[x] == '\r'))
                {
                    pData[x] = 0;
                    if (strlen(pszData))
                    {
                        if (g_nAllowableOutside == g_maxAllowableOutside)
                        {
                            g_maxAllowableOutside += MAX_ALLOWABLE_OUTSIDE_GROWTH_SIZE;
                            g_strAllowableOutsideList =

                                (char**)realloc(g_strAllowableOutsideList, sizeof(char*) * g_maxAllowableOutside);
                        }

                        g_strAllowableOutsideList[y++] = pszData;
                        g_nAllowableOutside++;

                        Verbose("Adding entity '%s' to the allowable void list\n", pszData);
                    }
                    pszData = pData + x + 1;
                }
            }
        }
    }
}

// =====================================================================================
//  FillOutside
// =====================================================================================
node_t*         FillOutside(node_t* node, const bool leakfile, const unsigned hullnum)
{
    int             s;
    int             i;
    bool            inside;
    bool            ret;
    vec3_t          origin;
    const char*     cl;

    Verbose("----- FillOutside ----\n");

    if (g_nofill)
    {
        Log("skipped\n");
        return node;
    }

    //
    // place markers for all entities so
    // we know if we leak inside
    //
    inside = false;
    for (i = 1; i < g_numentities; i++)
    {
        GetVectorForKey(&g_entities[i], "origin", origin);
        cl = ValueForKey(&g_entities[i], "classname");
        if (!isClassnameAllowableOutside(cl))
        {
            if (!VectorCompare(origin, vec3_origin))
            {
                origin[2] += 1;                            // so objects on floor are ok

                // nudge playerstart around if needed so clipping hulls allways
                // have a vlaid point
                if (!strcmp(cl, "info_player_start"))
                {
                    int             x, y;

                    for (x = -16; x <= 16; x += 16)
                    {
                        for (y = -16; y <= 16; y += 16)
                        {
                            origin[0] += x;
                            origin[1] += y;
                            if (PlaceOccupant(i, origin, node))
                            {
                                inside = true;
                                goto gotit;
                            }
                            origin[0] -= x;
                            origin[1] -= y;
                        }
                    }
                  gotit:;
                }
                else
                {
                    if (PlaceOccupant(i, origin, node))
                        inside = true;
                }
            }
        }
    }

    if (!inside)
    {
        Warning("No entities exist in hull %i, no filling performed for this hull", hullnum);
        return node;
    }

    s = !(g_outside_node.portals->nodes[1] == &g_outside_node);

    // first check to see if an occupied leaf is hit
    outleafs = 0;
    valid++;

    prevleaknode = NULL;

    if (leakfile)
    {
        pointfile = fopen(g_pointfilename, "w");
        if (!pointfile)
        {
            Error("Couldn't open pointfile %s\n", g_pointfilename);
        }

        linefile = fopen(g_linefilename, "w");
        if (!linefile)
        {
            Error("Couldn't open linefile %s\n", g_linefilename);
        }
    }

    ret = RecursiveFillOutside(g_outside_node.portals->nodes[s], false);

    if (leakfile)
    {
        fclose(pointfile);
        fclose(linefile);
    }

    if (ret)
    {
        GetVectorForKey(&g_entities[hit_occupied], "origin", origin);


        {
            Warning("=== LEAK in hull %i ===\nEntity %s @ (%4.0f,%4.0f,%4.0f)",
                 hullnum, ValueForKey(&g_entities[hit_occupied], "classname"), origin[0], origin[1], origin[2]);
            PrintOnce(
                "\n  A LEAK is a hole in the map, where the inside of it is exposed to the\n"
                "(unwanted) outside region.  The entity listed in the error is just a helpful\n"
                "indication of where the beginning of the leak pointfile starts, so the\n"
                "beginning of the line can be quickly found and traced to until reaching the\n"
                "outside. Unless this entity is accidentally on the outside of the map, it\n"
                "probably should not be deleted.  Some complex rotating objects entities need\n"
                "their origins outside the map.  To deal with these, just enclose the origin\n"
                "brush with a solid world brush\n");
        }

        if (!g_bLeaked)
        {
            // First leak spits this out
            Log("Leak pointfile generated\n\n");
        }

        if (g_bLeakOnly)
        {
            Error("Stopped by leak.");
        }

        g_bLeaked = true;
            
        return node;
    }

    // now go back and fill things in
    valid++;
    RecursiveFillOutside(g_outside_node.portals->nodes[s], true);

    // remove faces and nodes from filled in leafs  
    c_falsenodes = 0;
    c_free_faces = 0;
    c_keep_faces = 0;
    node = ClearOutFaces_r(node);

    Verbose("%5i outleafs\n", outleafs);
    Verbose("%5i freed faces\n", c_free_faces);
    Verbose("%5i keep faces\n", c_keep_faces);
    Verbose("%5i falsenodes\n", c_falsenodes);

    // save portal file for vis tracing
    if ((hullnum == 0) && leakfile)
    {
        WritePortalfile(node);
    }

    return node;
}
