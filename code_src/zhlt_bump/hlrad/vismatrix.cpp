#include "qrad.h"

////////////////////////////
// begin old vismat.c
//
#define	HALFBIT

// =====================================================================================
//
//      VISIBILITY MATRIX
//      Determine which patches can see each other
//      Use the PVS to accelerate if available
//
// =====================================================================================

static byte*    s_vismatrix;



#ifdef HLRAD_HULLU
// =====================================================================================
//      OPACITY ARRAY
// =====================================================================================

typedef struct {
	unsigned bitpos;
	vec3_t transparency;
} transparency_t;

static transparency_t *s_transparency_list = NULL;
static unsigned long s_transparency_count = 0;
static unsigned long s_max_transparency_count=0;

static void FindOpacity(const unsigned bitpos, vec3_t &out)
{
	for(unsigned long i = 0; i < s_transparency_count; i++)
	{
		if( s_transparency_list[i].bitpos == bitpos )
		{
			VectorCopy(s_transparency_list[i].transparency, out);
			return;
		}
	}
	VectorFill(out, 1.0);
}

#endif /*HLRAD_HULLU*/

// =====================================================================================
//  TestPatchToFace
//      Sets vis bits for all patches in the face
// =====================================================================================
static void     TestPatchToFace(const unsigned patchnum, const int facenum, const int head, const unsigned int bitpos)
{
    patch_t*        patch = &g_patches[patchnum];
    patch_t*        patch2 = g_face_patches[facenum];

    // if emitter is behind that face plane, skip all patches

    if (patch2)
    {
        const dplane_t* plane2 = getPlaneFromFaceNumber(facenum);

        if (DotProduct(patch->origin, plane2->normal) > (PatchPlaneDist(patch2) + MINIMUM_PATCH_DISTANCE))
        {
            // we need to do a real test
            const dplane_t* plane = getPlaneFromFaceNumber(patch->faceNumber);

            for (; patch2; patch2 = patch2->next)
            {
                unsigned        m = patch2 - g_patches;

#ifdef HLRAD_HULLU
		vec3_t 		transparency = {1.0, 1.0, 1.0};
#endif
		
                // check vis between patch and patch2
                // if bit has not already been set
                //  && v2 is not behind light plane
                //  && v2 is visible from v1
                if (m > patchnum
                    && (DotProduct(patch2->origin, plane->normal) > (PatchPlaneDist(patch) + MINIMUM_PATCH_DISTANCE))
                    && (TestLine_r(head, patch->origin, patch2->origin) == CONTENTS_EMPTY)
#ifdef HLRAD_HULLU
                    && (!TestSegmentAgainstOpaqueList(patch->origin, patch2->origin, transparency)))
#else
                    && (!TestSegmentAgainstOpaqueList(patch->origin, patch2->origin)))
#endif
                {
                    //Log("SDF::3\n");

                    // patchnum can see patch m
                    unsigned        bitset = bitpos + m;

#ifdef HLRAD_HULLU
                    // transparency face fix table
                    // TODO: this method makes MakeScale extreamly slow.. find new one

                    if(g_customshadow_with_bouncelight && fabs(VectorAvg(transparency) - 1.0) < 0.001)
                    {
                    	while(s_transparency_count >= s_max_transparency_count)
                    	{
                    	    //new size
                    	    unsigned long old_max = s_max_transparency_count;
                    	    s_max_transparency_count += 128;
                    	    
                    	    //realloc
                    	    s_transparency_list = (transparency_t*)realloc(s_transparency_list, s_max_transparency_count * sizeof(transparency_t));
                    	    
                    	    // clean new memory
                            memset(&s_transparency_list[old_max], 0, sizeof(transparency_t) * 128);
                    	}
                    	
                    	//add to array
                    	VectorCopy(transparency, s_transparency_list[s_transparency_count].transparency);
                    	s_transparency_list[s_transparency_count].bitpos = bitset;

                    	s_transparency_count++;
                    }
#endif /*HLRAD_HULLU*/

                    s_vismatrix[bitset >> 3] |= 1 << (bitset & 7);
                }
            }
        }
    }
}

// =====================================================================================
//  BuildVisRow
//      Calc vis bits from a single patch
// =====================================================================================
static void     BuildVisRow(const int patchnum, byte* pvs, const int head, const unsigned int bitpos)
{
    int             j, k, l;
    byte            face_tested[MAX_MAP_FACES];
    dleaf_t*        leaf;

    memset(face_tested, 0, g_numfaces);

    // leaf 0 is the solid leaf (skipped)
    for (j = 1, leaf = g_dleafs + 1; j < g_numleafs; j++, leaf++)
    {
        if (!(pvs[(j - 1) >> 3] & (1 << ((j - 1) & 7))))
            continue;                                      // not in pvs
        for (k = 0; k < leaf->nummarksurfaces; k++)
        {
            l = g_dmarksurfaces[leaf->firstmarksurface + k];

            // faces can be marksurfed by multiple leaves, but
            // don't bother testing again
            if (face_tested[l])
                continue;
            face_tested[l] = 1;

            TestPatchToFace(patchnum, l, head, bitpos);
        }
    }
}

// =====================================================================================
// BuildVisLeafs
//      This is run by multiple threads
// =====================================================================================
#ifdef SYSTEM_WIN32
#pragma warning(push)
#pragma warning(disable: 4100)                             // unreferenced formal parameter
#endif
static void     BuildVisLeafs(int threadnum)
{
    int             i;
    int             lface, facenum, facenum2;
    byte            pvs[(MAX_MAP_LEAFS + 7) / 8];
    dleaf_t*        srcleaf;
    dleaf_t*        leaf;
    patch_t*        patch;
    int             head;
    unsigned        bitpos;
    unsigned        patchnum;

    while (1)
    {
        //
        // build a minimal BSP tree that only
        // covers areas relevent to the PVS
        //
        i = GetThreadWork();
        if (i == -1)
            break;
        i++;                                               // skip leaf 0
        srcleaf = &g_dleafs[i];
        DecompressVis(&g_dvisdata[srcleaf->visofs], pvs, sizeof(pvs));
        head = 0;

        //
        // go through all the faces inside the
        // leaf, and process the patches that
        // actually have origins inside
        //
        for (lface = 0; lface < srcleaf->nummarksurfaces; lface++)
        {
            facenum = g_dmarksurfaces[srcleaf->firstmarksurface + lface];
            for (patch = g_face_patches[facenum]; patch; patch = patch->next)
            {
                leaf = PointInLeaf(patch->origin);
                if (leaf != srcleaf)
                    continue;

                patchnum = patch - g_patches;

#ifdef HALFBIT
                bitpos = patchnum * g_num_patches - (patchnum * (patchnum + 1)) / 2;
#else
                bitpos = patchnum * g_num_patches;
#endif
                // build to all other world leafs
                BuildVisRow(patchnum, pvs, head, bitpos);

                // build to bmodel faces
                if (g_nummodels < 2)
                    continue;
                for (facenum2 = g_dmodels[1].firstface; facenum2 < g_numfaces; facenum2++)
                    TestPatchToFace(patchnum, facenum2, head, bitpos);
            }
        }

    }
}

#ifdef SYSTEM_WIN32
#pragma warning(pop)
#endif

// =====================================================================================
// BuildVisMatrix
// =====================================================================================
static void     BuildVisMatrix()
{
    int             c;

#ifdef HALFBIT
    c = ((g_num_patches + 1) * (g_num_patches + 1)) / 16;
#else
    c = g_num_patches * ((g_num_patches + 7) / 8);
#endif

    Log("%-20s: %5.1f megs\n", "visibility matrix", c / (1024 * 1024.0));

    s_vismatrix = (byte*)AllocBlock(c);

    if (!s_vismatrix)
    {
        Log("Failed to allocate s_vismatrix");
        hlassume(s_vismatrix != NULL, assume_NoMemory);
    }

    NamedRunThreadsOn(g_numleafs - 1, g_estimate, BuildVisLeafs);
}

static void     FreeVisMatrix()
{
    if (s_vismatrix)
    {
        if (FreeBlock(s_vismatrix))
        {
            s_vismatrix = NULL;
        }
        else
        {
            Warning("Unable to free s_vismatrix");
        }
    }

#ifdef HLRAD_HULLU
    if(s_transparency_list)
    {
    	free(s_transparency_list);
    	s_transparency_list = NULL;
    }
    s_transparency_count = s_max_transparency_count = 0;
#endif

}

// =====================================================================================
// CheckVisBit
// =====================================================================================
#ifdef HLRAD_HULLU
static bool     CheckVisBitVismatrix(unsigned p1, unsigned p2, vec3_t &transparency_out)
#else
static bool     CheckVisBitVismatrix(unsigned p1, unsigned p2)
#endif
{
    unsigned        bitpos;

    if (p1 > p2)
    {
        const unsigned a = p1;
        const unsigned b = p2;
        p1 = b;
        p2 = a;
    }

    if (p1 > g_num_patches)
    {
        Warning("in CheckVisBit(), p1 > num_patches");
    }
    if (p2 > g_num_patches)
    {
        Warning("in CheckVisBit(), p2 > num_patches");
    }

#ifdef HALFBIT
    bitpos = p1 * g_num_patches - (p1 * (p1 + 1)) / 2 + p2;
#else
    bitpos = p1 * g_num_patches + p2;
#endif

    if (s_vismatrix[bitpos >> 3] & (1 << (bitpos & 7)))
    {
#ifdef HLRAD_HULLU    	
    	if(g_customshadow_with_bouncelight)
    	{
    	    vec3_t getvalue = {1.0, 1.0, 1.0};
    	    FindOpacity(bitpos, getvalue);
    	    VectorCopy(getvalue, transparency_out);
    	}
    	else
    	{
    	    VectorFill(transparency_out, 1.0);
    	}
#endif
        return true;
    }

#ifdef HLRAD_HULLU
    VectorFill(transparency_out, 0.0);
#endif

    return false;
}

//
// end old vismat.c
////////////////////////////

// =====================================================================================
// MakeScalesVismatrix
// =====================================================================================
void            MakeScalesVismatrix()
{
    char            transferfile[_MAX_PATH];

    hlassume(g_num_patches < MAX_VISMATRIX_PATCHES, assume_MAX_PATCHES);

    safe_strncpy(transferfile, g_source, _MAX_PATH);
    StripExtension(transferfile);
    DefaultExtension(transferfile, ".inc");

    if (!g_incremental || !readtransfers(transferfile, g_num_patches))
    {
        // determine visibility between g_patches
        BuildVisMatrix();
        g_CheckVisBit = CheckVisBitVismatrix;

#ifdef HLRAD_HULLU
        if((s_max_transparency_count*sizeof(transparency_t))>=(1024 * 1024))
        	Log("%-20s: %5.1f megs\n", "custom shadow array", (s_max_transparency_count*sizeof(transparency_t)) / (1024 * 1024.0));
        else if(s_transparency_count)
        	Log("%-20s: %5.1f kilos\n", "custom shadow array", (s_max_transparency_count*sizeof(transparency_t)) / 1024.0);
#endif

#ifndef HLRAD_HULLU
        NamedRunThreadsOn(g_num_patches, g_estimate, MakeScales);
#else
	if(g_rgb_transfers)
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeRGBScales);}
	else
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeScales);}
#endif
        FreeVisMatrix();

        // invert the transfers for gather vs scatter
#ifndef HLRAD_HULLU
        NamedRunThreadsOnIndividual(g_num_patches, g_estimate, SwapTransfers);
#else
	if(g_rgb_transfers)
		{NamedRunThreadsOnIndividual(g_num_patches, g_estimate, SwapRGBTransfers);}
	else
		{NamedRunThreadsOnIndividual(g_num_patches, g_estimate, SwapTransfers);}
#endif
        if (g_incremental)
            writetransfers(transferfile, g_num_patches);
        else
            unlink(transferfile);
        DumpTransfersMemoryUsage();
    }
}
