#include "csg.h"

#define MAXWADNAME 16
#define MAX_TEXFILES 128

//  FindMiptex
//  TEX_InitFromWad
//  FindTexture
//  LoadLump
//  AddAnimatingTextures


typedef struct
{
    char            identification[4];                     // should be WAD2/WAD3
    int             numlumps;
    int             infotableofs;
} wadinfo_t;

typedef struct
{
    int             filepos;
    int             disksize;
    int             size;                                  // uncompressed
    char            type;
    char            compression;
    char            pad1, pad2;
    char            name[MAXWADNAME];                      // must be null terminated

    int             iTexFile;                              // index of the wad this texture is located in

} lumpinfo_t;

std::deque< std::string > g_WadInclude;
std::map< int, bool > s_WadIncludeMap;

static int      nummiptex = 0;
static lumpinfo_t miptex[MAX_MAP_TEXTURES];
static int      nTexLumps = 0;
static lumpinfo_t* lumpinfo = NULL;
static int      nTexFiles = 0;
static FILE*    texfiles[MAX_TEXFILES];

// fix for 64 bit machines
#if SIZEOF_CHARP == 8
    static char* texmap64[MAX_MAP_BRUSHES];
    static int   tex_max64=0;

    static inline int texmap64_store(char *texname)
    {
        int curr_tex;
        ThreadLock();
        if (tex_max64 >= MAX_MAP_BRUSHES)   // no assert?
        {
            printf("MAX_MAP_BRUSHES exceeded!\n");
            exit(-1);
        }
        curr_tex = tex_max64;
        texmap64[tex_max64] = texname;
        tex_max64++;
        ThreadUnlock();
        return curr_tex;
    }

    static inline char* texmap64_retrieve( int index)
    {
        if(index > tex_max64)
        {
            printf("retrieving bogus texture index %d\n", index);
            exit(-1);
        }
        return texmap64[index];
    }

#else // SIZEOF_CHARP != 8, almost certainly 4
    #define texmap64_store( A ) ( (int) A)
    #define texmap64_retrieve( A ) ( (char*) A)
#endif // SIZEOF_CHARP

// =====================================================================================
//  CleanupName
// =====================================================================================
static void     CleanupName(const char* const in, char* out)
{
    int             i;

    for (i = 0; i < MAXWADNAME; i++)
    {
        if (!in[i])
        {
            break;
        }

        out[i] = toupper(in[i]);
    }

    for (; i < MAXWADNAME; i++)
    {
        out[i] = 0;
    }
}

// =====================================================================================
//  lump_sorters
// =====================================================================================

static int CDECL lump_sorter_by_wad_and_name(const void* lump1, const void* lump2)
{
    lumpinfo_t*     plump1 = (lumpinfo_t*)lump1;
    lumpinfo_t*     plump2 = (lumpinfo_t*)lump2;

    if (plump1->iTexFile == plump2->iTexFile)
    {
        return strcmp(plump1->name, plump2->name);
    }
    else
    {
        return plump1->iTexFile - plump2->iTexFile;
    }
}

static int CDECL lump_sorter_by_name(const void* lump1, const void* lump2)
{
    lumpinfo_t*     plump1 = (lumpinfo_t*)lump1;
    lumpinfo_t*     plump2 = (lumpinfo_t*)lump2;

    return strcmp(plump1->name, plump2->name);
}

// =====================================================================================
//  FindMiptex
//      Find and allocate a texture into the lump data
// =====================================================================================
static int      FindMiptex(const char* const name)
{
    int             i;

    ThreadLock();
    for (i = 0; i < nummiptex; i++)
    {
        if (!strcmp(name, miptex[i].name))
        {
            ThreadUnlock();
            return i;
        }
    }

    hlassume(nummiptex < MAX_MAP_TEXTURES, assume_MAX_MAP_TEXTURES);
    safe_strncpy(miptex[i].name, name, MAXWADNAME);
    nummiptex++;
    ThreadUnlock();
    return i;
}

// =====================================================================================
//  TEX_InitFromWad
// =====================================================================================
bool            TEX_InitFromWad()
{
    int             i, j;
    wadinfo_t       wadinfo;
    char            szTmpWad[1024]; // arbitrary, but needs to be large.
    char*           pszWadFile;
    const char*     pszWadroot;
    wadpath_t*      currentwad;

    Log("\n"); // looks cleaner

    szTmpWad[0] = 0;
    pszWadroot = getenv("WADROOT");

#ifdef HLCSG_AUTOWAD
    autowad_UpdateUsedWads();
#endif

    // for eachwadpath
    for (i = 0; i < g_iNumWadPaths; i++)
    {
        FILE*           texfile;                           // temporary used in this loop
        bool            bExcludeThisWad = false;

        currentwad = g_pWadPaths[i];
        pszWadFile = currentwad->path;        

        
#ifdef HLCSG_AUTOWAD
        #ifdef _DEBUG
        Log("[dbg] Attempting to parse wad: '%s'\n", pszWadFile);
        #endif

        if (g_bWadAutoDetect && !currentwad->usedtextures)
            continue;

        #ifdef _DEBUG
        Log("[dbg] Parsing wad\n");
        #endif
#endif

        texfiles[nTexFiles] = fopen(pszWadFile, "rb");

        #ifdef SYSTEM_WIN32
        if (!texfiles[nTexFiles])
        {
            // cant find it, maybe this wad file has a hard code drive
            if (pszWadFile[1] == ':')
            {
                pszWadFile += 2;                           // skip past the drive
                texfiles[nTexFiles] = fopen(pszWadFile, "rb");
            }
        }
        #endif

        if (!texfiles[nTexFiles] && pszWadroot)
        {
            char            szTmp[_MAX_PATH];
            char            szFile[_MAX_PATH];
            char            szSubdir[_MAX_PATH];

            ExtractFile(pszWadFile, szFile);
            
            ExtractFilePath(pszWadFile, szTmp);
            ExtractFile(szTmp, szSubdir);

            // szSubdir will have a trailing separator
            safe_snprintf(szTmp, _MAX_PATH, "%s" SYSTEM_SLASH_STR "%s%s", pszWadroot, szSubdir, szFile);
            texfiles[nTexFiles] = fopen(szTmp, "rb");
            
            #ifdef SYSTEM_POSIX
            if (!texfiles[nTexFiles])
            {
                // if we cant find it, Convert to lower case and try again
                strlwr(szTmp);
                texfiles[nTexFiles] = fopen(szTmp, "rb");
            }
            #endif
        }

        if (!texfiles[nTexFiles])
        {
            // still cant find it, error out
            Fatal(assume_COULD_NOT_FIND_WAD, "Could not open wad file %s", pszWadFile);
            continue;
        }

        // look and see if we're supposed to include the textures from this WAD in the bsp.
        WadInclude_i it;
        for (it = g_WadInclude.begin(); it != g_WadInclude.end(); it++)
        {
            if (stristr(pszWadFile, it->c_str()))
            {
                Log("Including Wadfile: %s\n", pszWadFile);
                bExcludeThisWad = true;             // wadincluding this one
                s_WadIncludeMap[nTexFiles] = true;
                break;
            }
        }

        if (!bExcludeThisWad)
        {
            Log("Using Wadfile: %s\n", pszWadFile);
            safe_snprintf(szTmpWad, 1024, "%s%s;", szTmpWad, pszWadFile);
        }

        // temp assignment to make things cleaner:
        texfile = texfiles[nTexFiles];

        // read in this wadfiles information
        SafeRead(texfile, &wadinfo, sizeof(wadinfo));

        // make sure its a valid format
        if (strncmp(wadinfo.identification, "WAD2", 4) && strncmp(wadinfo.identification, "WAD3", 4))
        {
            Log(" - ");
            Error("%s isn't a Wadfile!", pszWadFile);
        }

        wadinfo.numlumps        = LittleLong(wadinfo.numlumps);
        wadinfo.infotableofs    = LittleLong(wadinfo.infotableofs);

        // read in lump
        if (fseek(texfile, wadinfo.infotableofs, SEEK_SET))
            Warning("fseek to %d in wadfile %s failed\n", wadinfo.infotableofs, pszWadFile);

        // memalloc for this lump
        lumpinfo = (lumpinfo_t*)realloc(lumpinfo, (nTexLumps + wadinfo.numlumps) * sizeof(lumpinfo_t));

        // for each texlump
        for (j = 0; j < wadinfo.numlumps; j++, nTexLumps++)
        {
            SafeRead(texfile, &lumpinfo[nTexLumps], (sizeof(lumpinfo_t) - sizeof(int)) );  // iTexFile is NOT read from file

            if (!TerminatedString(lumpinfo[nTexLumps].name, MAXWADNAME))
            {
                lumpinfo[nTexLumps].name[MAXWADNAME - 1] = 0;
                Log(" - ");
                Warning("Unterminated texture name : wad[%s] texture[%d] name[%s]\n", pszWadFile, nTexLumps, lumpinfo[nTexLumps].name);
            }

            CleanupName(lumpinfo[nTexLumps].name, lumpinfo[nTexLumps].name);

            lumpinfo[nTexLumps].filepos = LittleLong(lumpinfo[nTexLumps].filepos);
            lumpinfo[nTexLumps].disksize = LittleLong(lumpinfo[nTexLumps].disksize);
            lumpinfo[nTexLumps].iTexFile = nTexFiles;
            
            if (lumpinfo[nTexLumps].disksize > MAX_TEXTURE_SIZE)
            {
                Log(" - ");
                Warning("Larger than expected texture (%d bytes): '%s'", 
                    lumpinfo[nTexLumps].disksize, lumpinfo[nTexLumps].name);
            }

        }

        // AJM: this feature is dependant on autowad. :(
        // CONSIDER: making it standard?
#ifdef HLCSG_AUTOWAD
        {
            double percused = ((float)(currentwad->usedtextures) / (float)(g_numUsedTextures)) * 100;
            Log(" - Contains %i used texture%s, %2.2f percent of map (%d textures in wad)\n", 
                currentwad->usedtextures, currentwad->usedtextures == 1 ? "" : "s", percused, wadinfo.numlumps);
        }
#endif

        nTexFiles++;
        hlassume(nTexFiles < MAX_TEXFILES, assume_MAX_TEXFILES);
    }

    //Log("num of used textures: %i\n", g_numUsedTextures);

    // AJM: Tommy suggested i add this warning message in, and  it certianly doesnt 
    //  hurt to be cautious. Especially one of the possible side effects he mentioned was svc_bad
    if (nTexFiles > 8) 
    {
        Log("\n");
        Warning("More than 8 wadfiles are in use. (%i)\n"
                "This may be harmless, and if no strange side effects are occurring, then\n"
                "it can safely be ignored. However, if your map starts exhibiting strange\n"
                "or obscure errors, consider this as suspect.\n"
                , nTexFiles);
    }

    // sort texlumps in memory by name
    qsort((void*)lumpinfo, (size_t) nTexLumps, sizeof(lumpinfo[0]), lump_sorter_by_name);

    SetKeyValue(&g_entities[0], "wad", szTmpWad);

    Log("\n");
    CheckFatal();
    return true;
}

// =====================================================================================
//  FindTexture
// =====================================================================================
lumpinfo_t*     FindTexture(const lumpinfo_t* const source)
{
    //Log("** PnFNFUNC: FindTexture\n");

    lumpinfo_t*     found = NULL;

    found = (lumpinfo_t*)bsearch(source, (void*)lumpinfo, (size_t) nTexLumps, sizeof(lumpinfo[0]), lump_sorter_by_name);
    if (!found)
    {
        Warning("::FindTexture() texture %s not found!", source->name);
        if (!strcmp(source->name, "NULL"))
        {
            Log("Are you sure you included zhlt.wad in your wadpath list?\n");
        }
    }

    return found;
}

// =====================================================================================
//  LoadLump
// =====================================================================================
int             LoadLump(const lumpinfo_t* const source, byte* dest, int* texsize)
{
    //Log("** PnFNFUNC: LoadLump\n");

    *texsize = 0;
    if (source->filepos)
    {
        if (fseek(texfiles[source->iTexFile], source->filepos, SEEK_SET))
        {
            Warning("fseek to %d failed\n", source->filepos);
        }
        *texsize = source->disksize;

        bool wadinclude = false;
        std::map< int, bool >::iterator it;
        it = s_WadIncludeMap.find(source->iTexFile);
        if (it != s_WadIncludeMap.end())
        {
            wadinclude = it->second;
        }

        // Should we just load the texture header w/o the palette & bitmap?
        if (g_wadtextures && !wadinclude)
        {
            // Just read the miptex header and zero out the data offsets.
            // We will load the entire texture from the WAD at engine runtime
            int             i;
            miptex_t*       miptex = (miptex_t*)dest;
            SafeRead(texfiles[source->iTexFile], dest, sizeof(miptex_t));

            for (i = 0; i < MIPLEVELS; i++)
                miptex->offsets[i] = 0;
            return sizeof(miptex_t);
        }
        else
        {
            // Load the entire texture here so the BSP contains the texture
            SafeRead(texfiles[source->iTexFile], dest, source->disksize);
            return source->disksize;
        }
    }

    Warning("::LoadLump() texture %s not found!", source->name);
    return 0;
}

// =====================================================================================
//  AddAnimatingTextures
// =====================================================================================
void            AddAnimatingTextures()
{
    int             base;
    int             i, j, k;
    char            name[MAXWADNAME];

    base = nummiptex;

    for (i = 0; i < base; i++)
    {
        if ((miptex[i].name[0] != '+') && (miptex[i].name[0] != '-'))
        {
            continue;
        }

        safe_strncpy(name, miptex[i].name, MAXWADNAME);

        for (j = 0; j < 20; j++)
        {
            if (j < 10)
            {
                name[1] = '0' + j;
            }
            else
            {
                name[1] = 'A' + j - 10;                    // alternate animation
            }

            // see if this name exists in the wadfile
            for (k = 0; k < nTexLumps; k++)
            {
                if (!strcmp(name, lumpinfo[k].name))
                {
                    FindMiptex(name);                      // add to the miptex list
                    break;
                }
            }
        }
    }

    if (nummiptex - base)
    {
        Log("added %i additional animating textures.\n", nummiptex - base);
    }
}

// =====================================================================================
//  GetWadPath
// AJM: this sub is obsolete
// =====================================================================================
char*           GetWadPath()
{
    const char*     path = ValueForKey(&g_entities[0], "_wad");

    if (!path || !path[0])
    {
        path = ValueForKey(&g_entities[0], "wad");
        if (!path || !path[0])
        {
            Warning("no wadfile specified");
            return strdup("");
        }
    }
   
    return strdup(path);
}

// =====================================================================================
//  WriteMiptex
// =====================================================================================
void            WriteMiptex()
{
    int             len, texsize, totaltexsize = 0;
    byte*           data;
    dmiptexlump_t*  l;
    double          start, end;

    g_texdatasize = 0;

    start = I_FloatTime();
    {
        if (!TEX_InitFromWad())
            return;

        AddAnimatingTextures();
    }
    end = I_FloatTime();
    Verbose("TEX_InitFromWad & AddAnimatingTextures elapsed time = %ldms\n", (long)(end - start));

    start = I_FloatTime();
    {
        int             i;

        for (i = 0; i < nummiptex; i++)
        {
            lumpinfo_t*     found;

            found = FindTexture(miptex + i);
            if (found)
            {
                miptex[i] = *found;
            }
            else
            {
                miptex[i].iTexFile = miptex[i].filepos = miptex[i].disksize = 0;
            }
        }
    }
    end = I_FloatTime();
    Verbose("FindTextures elapsed time = %ldms\n", (long)(end - start));

    start = I_FloatTime();
    {
        int             i;
        texinfo_t*      tx = g_texinfo;

        // Sort them FIRST by wadfile and THEN by name for most efficient loading in the engine.
        qsort((void*)miptex, (size_t) nummiptex, sizeof(miptex[0]), lump_sorter_by_wad_and_name);

        // Sleazy Hack 104 Pt 2 - After sorting the miptex array, reset the texinfos to point to the right miptexs
        for (i = 0; i < g_numtexinfo; i++, tx++)
        {
            char*          miptex_name = texmap64_retrieve(tx->miptex);

            tx->miptex = FindMiptex(miptex_name);

            // Free up the originally strdup()'ed miptex_name
            free(miptex_name);
        }
    }
    end = I_FloatTime();
    Verbose("qsort(miptex) elapsed time = %ldms\n", (long)(end - start));

    start = I_FloatTime();
    {
        int             i;

        // Now setup to get the miptex data (or just the headers if using -wadtextures) from the wadfile
        l = (dmiptexlump_t*)g_dtexdata;
        data = (byte*) & l->dataofs[nummiptex];
        l->nummiptex = nummiptex;
        for (i = 0; i < nummiptex; i++)
        {
            l->dataofs[i] = data - (byte*) l;
            len = LoadLump(miptex + i, data, &texsize);

            if (!len)
            {
                l->dataofs[i] = -1;                        // didn't find the texture
            }
            else
            {
                totaltexsize += texsize;

                hlassume(totaltexsize < g_max_map_miptex, assume_MAX_MAP_MIPTEX);
            }
            data += len;
        }
        g_texdatasize = data - g_dtexdata;
    }
    end = I_FloatTime();
    Log("Texture usage is at %1.2f mb (of %1.2f mb MAX)\n", (float)totaltexsize / (1024 * 1024),
        (float)g_max_map_miptex / (1024 * 1024));
    Verbose("LoadLump() elapsed time = %ldms\n", (long)(end - start));
}

//==========================================================================

// =====================================================================================
//  TexinfoForBrushTexture
// =====================================================================================
int             TexinfoForBrushTexture(const plane_t* const plane, brush_texture_t* bt, const vec3_t origin)
{
    vec3_t          vecs[2];
    int             sv, tv;
    vec_t           ang, sinv, cosv;
    vec_t           ns, nt;
    texinfo_t       tx;
    texinfo_t*      tc;
    int             i, j, k;

    memset(&tx, 0, sizeof(tx));
    tx.miptex = FindMiptex(bt->name);

    // Note: FindMiptex() still needs to be called here to add it to the global miptex array

    // Very Sleazy Hack 104 - since the tx.miptex index will be bogus after we sort the miptex array later
    // Put the string name of the miptex in this "index" until after we are done sorting it in WriteMiptex().
    tx.miptex = texmap64_store(strdup(bt->name));

    // set the special flag
    if (bt->name[0] == '*'
        || !strncasecmp(bt->name, "sky", 3)
        || !strncasecmp(bt->name, "clip", 4)
        || !strncasecmp(bt->name, "origin", 6) 
#ifdef ZHLT_NULLTEX // AJM
        || !strncasecmp(bt->name, "null", 4) 
#endif
        || !strncasecmp(bt->name, "aaatrigger", 10)
       )
    {
        tx.flags |= TEX_SPECIAL;
    }

    if (bt->txcommand)
    {
        memcpy(tx.vecs, bt->vects.quark.vects, sizeof(tx.vecs));
        if (origin[0] || origin[1] || origin[2])
        {
            tx.vecs[0][3] += DotProduct(origin, tx.vecs[0]);
            tx.vecs[1][3] += DotProduct(origin, tx.vecs[1]);
        }
    }
    else
    {
        if (g_nMapFileVersion < 220)
        {
            TextureAxisFromPlane(plane, vecs[0], vecs[1]);
        }

        if (!bt->vects.valve.scale[0])
        {
            bt->vects.valve.scale[0] = 1;
        }
        if (!bt->vects.valve.scale[1])
        {
            bt->vects.valve.scale[1] = 1;
        }

        if (g_nMapFileVersion < 220)
        {
            // rotate axis
            if (bt->vects.valve.rotate == 0)
            {
                sinv = 0;
                cosv = 1;
            }
            else if (bt->vects.valve.rotate == 90)
            {
                sinv = 1;
                cosv = 0;
            }
            else if (bt->vects.valve.rotate == 180)
            {
                sinv = 0;
                cosv = -1;
            }
            else if (bt->vects.valve.rotate == 270)
            {
                sinv = -1;
                cosv = 0;
            }
            else
            {
                ang = bt->vects.valve.rotate / 180 * Q_PI;
                sinv = sin(ang);
                cosv = cos(ang);
            }

            if (vecs[0][0])
            {
                sv = 0;
            }
            else if (vecs[0][1])
            {
                sv = 1;
            }
            else
            {
                sv = 2;
            }

            if (vecs[1][0])
            {
                tv = 0;
            }
            else if (vecs[1][1])
            {
                tv = 1;
            }
            else
            {
                tv = 2;
            }

            for (i = 0; i < 2; i++)
            {
                ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
                nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
                vecs[i][sv] = ns;
                vecs[i][tv] = nt;
            }

            for (i = 0; i < 2; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    tx.vecs[i][j] = vecs[i][j] / bt->vects.valve.scale[i];
                }
            }
        }
        else
        {
            vec_t           scale;

            scale = 1 / bt->vects.valve.scale[0];
            VectorScale(bt->vects.valve.UAxis, scale, tx.vecs[0]);

            scale = 1 / bt->vects.valve.scale[1];
            VectorScale(bt->vects.valve.VAxis, scale, tx.vecs[1]);
        }

        tx.vecs[0][3] = bt->vects.valve.shift[0] + DotProduct(origin, tx.vecs[0]);
        tx.vecs[1][3] = bt->vects.valve.shift[1] + DotProduct(origin, tx.vecs[1]);
    }

    //
    // find the g_texinfo
    //
    ThreadLock();
    tc = g_texinfo;
    for (i = 0; i < g_numtexinfo; i++, tc++)
    {
        // Sleazy hack 104, Pt 3 - Use strcmp on names to avoid dups
        if (strcmp(texmap64_retrieve((tc->miptex)), texmap64_retrieve((tx.miptex))) != 0)
        {
            continue;
        }
        if (tc->flags != tx.flags)
        {
            continue;
        }
        for (j = 0; j < 2; j++)
        {
            for (k = 0; k < 4; k++)
            {
                if (tc->vecs[j][k] != tx.vecs[j][k])
                {
                    goto skip;
                }
            }
        }
        ThreadUnlock();
        return i;
skip:;
    }

    hlassume(g_numtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO);

    *tc = tx;
    g_numtexinfo++;
    ThreadUnlock();
    return i;
}

