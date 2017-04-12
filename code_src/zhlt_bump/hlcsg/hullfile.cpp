#include "csg.h"

vec3_t          g_hull_size[NUM_HULLS][2] = 
{
    {// 0x0x0
        {0, 0, 0},          {0, 0, 0}
    }
    ,
    {// 32x32x72
        {-16, -16, -36},    {16, 16, 36}
    }
    ,                                                      
    {// 64x64x64
        {-32, -32, -32},    {32, 32, 32}
    }
    ,                                                      
    {// 32x32x36
        {-16, -16, -18},    {16, 16, 18}
    }                                                     
};

void        LoadHullfile(const char* filename)
{
    if (filename == NULL)
    {
        return;
    }

    if (q_exists(filename))
    {
        Log("Loading hull definitions from '%s'\n", filename);
    }
    else
    {
        Error("Could not find hull definition file '%s'\n", filename);
        return;
    }

    float x,y,z;

    FILE* file = fopen(filename, "r");

    int count;
    int i;
    // Skip hull 0 (visibile polygons)
    for (i=1; i<NUM_HULLS; i++)
    {
        count = fscanf(file, "%f %f %f\n", &x, &y, &z);
        if (count != 3)
        {
            Error("Could not parse hull definition file '%s' (%d, %d)\n", filename, i, count);
        }
        x *= 0.5;
        y *= 0.5;
        z *= 0.5;

        g_hull_size[i][0][0] = -x;
        g_hull_size[i][0][1] = -y;
        g_hull_size[i][0][2] = -z;

        g_hull_size[i][1][0] = x;
        g_hull_size[i][1][1] = y;
        g_hull_size[i][1][2] = z;
    }

    fclose(file);
}