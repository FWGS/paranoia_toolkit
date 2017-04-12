
/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

// csg4.c

#include "ripent.h"

typedef enum
{
    hl_undefined = -1,
    hl_export = 0,
    hl_import = 1
}
hl_types;

static hl_types g_mode = hl_undefined;

static void     ReadBSP(const char* const name)
{
    char            filename[_MAX_PATH];

    safe_strncpy(filename, name, _MAX_PATH);
    StripExtension(filename);
    DefaultExtension(filename, ".bsp");

    LoadBSPFile(name);
}

static void     WriteBSP(const char* const name)
{
    char            filename[_MAX_PATH];

    safe_strncpy(filename, name, _MAX_PATH);
    StripExtension(filename);
    DefaultExtension(filename, ".bsp");

    WriteBSPFile(filename);
}

static void     WriteEntities(const char* const name)
{
    char            filename[_MAX_PATH];

    safe_strncpy(filename, name, _MAX_PATH);
    StripExtension(filename);
    DefaultExtension(filename, ".ent");
    unlink(filename);

    {
        FILE           *f = SafeOpenWrite(filename);

        SafeWrite(f, g_dentdata, g_entdatasize);
        fclose(f);
    }
}

static void     ReadEntities(const char* const name)
{
    char            filename[_MAX_PATH];

    safe_strncpy(filename, name, _MAX_PATH);
    StripExtension(filename);
    DefaultExtension(filename, ".ent");

    {
        FILE           *f = SafeOpenRead(filename);

        g_entdatasize = q_filelength(f);
        assume(g_entdatasize < sizeof(g_dentdata), "ent data filesize exceedes dentdata limit");

        SafeRead(f, g_dentdata, g_entdatasize);

        if (g_dentdata[g_entdatasize-1] != 0)
        {
//            Log("g_dentdata[g_entdatasize-1] = %d\n", g_dentdata[g_entdatasize-1]);
            g_dentdata[g_entdatasize] = 0;
            g_entdatasize++;
        }
        fclose(f);
    }
}

//======================================================================

static void     Usage(void)
{
    Log("%s " ZHLT_VERSIONSTRING "\n" MODIFICATIONS_STRING "\n", g_Program);
    Log("  Usage: ripent [-import|-export] [-texdata n] bspname\n");
    exit(1);
}

/*
 * ============
 * main
 * ============
 */
int             main(int argc, char** argv)
{
    int             i;

    g_Program = "ripent";

    if (argc == 1)
    {
        Usage();
    }

    for (i = 1; i < argc; i++)
    {
        if (!strcasecmp(argv[i], "-import"))
        {
            g_mode = hl_import;
        }
        else if (!strcasecmp(argv[i], "-export"))
        {
            g_mode = hl_export;
        }
        else if (!strcasecmp(argv[i], "-texdata"))
        {
            if (i < argc)
            {
                int             x = atoi(argv[++i]) * 1024;

                if (x > g_max_map_miptex)
                {
                    g_max_map_miptex = x;
                }
            }
            else
            {
                Usage();
            }
        }
        else
        {
            safe_strncpy(g_Mapname, argv[i], _MAX_PATH);
            StripExtension(g_Mapname);
            DefaultExtension(g_Mapname, ".bsp");
        }
    }

    if (g_mode == hl_undefined)
    {
        fprintf(stderr, "%s", "Must specify either -import or -export\n");
        Usage();
    }

    if (!q_exists(g_Mapname))
    {
        fprintf(stderr, "%s", "bspfile '%s' does not exist\n", g_Mapname);
        Usage();
    }

    dtexdata_init();
    atexit(dtexdata_free);

    switch (g_mode)
    {
    case hl_import:
        ReadBSP(g_Mapname);
        ReadEntities(g_Mapname);
        WriteBSP(g_Mapname);
        PrintBSPFileSizes();
        break;
    case hl_export:
        ReadBSP(g_Mapname);
        PrintBSPFileSizes();
        WriteEntities(g_Mapname);
        break;
    default:
        Usage();
        break;
    }

    return 0;
}
