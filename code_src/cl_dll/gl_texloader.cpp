//
// written by BUzer for HL: Paranoia modification
//
//		2006

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "glmanager.h"
#include "gl_texloader.h"

#define BASE_EXT_TEXTURE_ID		(1<<25) // dont use zero

int current_ext_texture_id = BASE_EXT_TEXTURE_ID;


class DetailTexture
{
public:
	DetailTexture(char *_name, int _id);
	~DetailTexture();
	
	char	name[32];
	int		gl_id;
	DetailTexture *pnext;
};


DetailTexture *p_detailTexturesList = NULL;
ExtTextureData *p_extTexDataList = NULL;

// DetailTexture
DetailTexture::DetailTexture(char *_name, int _id) : gl_id(_id)
{
	strcpy(name, _name);
	pnext = p_detailTexturesList;
	p_detailTexturesList = this;
}

DetailTexture::~DetailTexture()
{
	if (pnext) delete pnext;
}

// ExtTextureData
ExtTextureData::ExtTextureData(char *_name)
{
	strcpy(name, _name);
	pnext = p_extTexDataList;
	p_extTexDataList = this;
}

ExtTextureData::~ExtTextureData()
{
	if (pnext) delete pnext;
}



void SortTexturesByDetailTexID();


// ===========================
// Mip maps creation
//
// ===========================

// test
int WriteTGA_24( color24 *pixels, unsigned short width, unsigned short height, const char *filename);

void GenerateMipMap (color24 *in, int width, int height)
{
	color24	*out = in;
	height >>= 1;
	for (int y = 0; y < height; y++, in+=width*2)
	{
		for (int x = 0; x < width/2; x++, out++)
		{
			out->r = (in[x*2].r + in[x*2+1].r + in[width+x*2].r + in[width+x*2+1].r) >> 2;
			out->g = (in[x*2].g + in[x*2+1].g + in[width+x*2].g + in[width+x*2+1].g) >> 2;
			out->b = (in[x*2].b + in[x*2+1].b + in[width+x*2].b + in[width+x*2+1].b) >> 2;
		//	out->r = (in[x*2].r/4 + in[x*2+1].r/4 + in[width+x*2].r/4 + in[width+x*2+1].r/4);
		//	out->g = (in[x*2].g/4 + in[x*2+1].g/4 + in[width+x*2].g/4 + in[width+x*2+1].g/4);
		//	out->b = (in[x*2].b/4 + in[x*2+1].b/4 + in[width+x*2].b/4 + in[width+x*2+1].b/4);
		}
	}
}


void NormalizeMipMap (color24 *buf, int width, int height)
{
	color24 *vec = buf;
	int numpixels = width * height;
	for (int i = 0; i < numpixels; i++)
	{
	/*	vec[i].r = 127;
		vec[i].g = 127;
		vec[i].b = 255;*/
		vec3_t flvec;
		flvec[0] = ((float)vec[i].r / 127.0) - 1;
		flvec[1] = ((float)vec[i].g / 127.0) - 1;
		flvec[2] = ((float)vec[i].b / 127.0) - 1;

		VectorNormalize(flvec);

		vec[i].r = (unsigned char)((flvec[0]+1)*127);
		vec[i].g = (unsigned char)((flvec[1]+1)*127);
		vec[i].b = (unsigned char)((flvec[2]+1)*127);
	}
}


//===================================
// CreateTexture
//
// loads 24bit TGAs
//	returns gl texture id, or 0 in error case
// TODO: compressed images support
//===================================
typedef struct tgaheader_s
{
	unsigned char	IdLength;
	unsigned char	ColorMap;
	unsigned char	DataType;
	unsigned char	unused[5];
	unsigned short	OriginX;
	unsigned short	OriginY;
	unsigned short	Width;
	unsigned short	Height;
	unsigned char	BPP;
	unsigned char	Description;
} TGAheader;

#define TEXBUFFER_SIZE	(512*512)

int CreateTexture(const char* filename, int mipmaps, int useid)
{
	color24 buf[TEXBUFFER_SIZE];

	if (!gl.IsGLAllowed())
	{
		ConLog("CreateTexture error: glmanager is not initialized!\n");
		return 0;
	}

	int length = 0;
	char *file = (char *)gEngfuncs.COM_LoadFile( (char*)filename, 5, &length );
	if (!file)
	{
		Log("CreateTexture failed to load file: %s\n", filename);
		return 0;
	}

	TGAheader *hdr = (TGAheader *)file;
	if (!(/*hdr->DataType == 10 ||*/ hdr->DataType == 2) || (hdr->BPP != 24))
	{
		ConLog("Error: texture %s uses unsupported data format -->\n(only 24-bit noncompressed TGA supported)\n", filename);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	int maxtexsize;
	gl.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
	if (hdr->Width > maxtexsize || hdr->Height > maxtexsize)
	{
		ConLog("Error: texture %s: sizes %d x %d are not supported by your hardware\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	 // lame :)
	if (!(hdr->Width == 16 || hdr->Width == 32 || hdr->Width == 64 || hdr->Width == 128 || hdr->Width == 256 || hdr->Width == 512 || hdr->Width == 1024) ||
		!(hdr->Height == 16 || hdr->Height == 32 || hdr->Height == 64 || hdr->Height == 128 || hdr->Height == 256 || hdr->Height == 512 || hdr->Height == 1024))
	{
		ConLog("Error: texture %s has bad dimensions (%d x %d)\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	int numpixels = hdr->Width * hdr->Height;
	if (numpixels > TEXBUFFER_SIZE)
	{
		ConLog("Error: texture %s (%d x %d) doesn't fit in static buffer\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	color24 *pixels = (color24*)(file + sizeof(TGAheader) + hdr->IdLength);
	color24 *dest = buf;
	
	// non-compressed image
	for (int i = 0; i < numpixels; i++, pixels++, dest++)
	{
		dest->r = pixels->b;
		dest->g = pixels->g;
		dest->b = pixels->r;
	}

	// flip image
	if (!(hdr->Description & 0x20))
	{
		color24 line[1024];
		for (int i = 0; i < hdr->Height / 2; i++)
		{
			int linelen = hdr->Width * sizeof(color24);
			memcpy(line, &buf[i*hdr->Width], linelen);
			memcpy(&buf[i*hdr->Width], &buf[(hdr->Height - i - 1)*hdr->Width], linelen);
			memcpy(&buf[(hdr->Height - i - 1)*hdr->Width], line, linelen);
		}
	}
	
	int minfilter = GL_NEAREST;
	if (mipmaps == MIPS_YES || mipmaps == MIPS_NORMALIZED)
		minfilter = GL_LINEAR_MIPMAP_NEAREST;

	// upload image
	if (useid)
		gl.glBindTexture(GL_TEXTURE_2D, useid);
	else
		gl.glBindTexture(GL_TEXTURE_2D, current_ext_texture_id);

	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.glTexImage2D (GL_TEXTURE_2D, 0, 3, hdr->Width, hdr->Height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);

//	static int counter = 0;
	if (mipmaps == MIPS_YES || mipmaps == MIPS_NORMALIZED)
	{
		int miplevel = 0;
		int width = hdr->Width;
		int height = hdr->Height;
		while (width > 1 || height > 1)
		{
			GenerateMipMap (buf, width, height);
			width >>= 1;
			height >>= 1;
			if (width < 1) width = 1;
			if (height < 1) height = 1;

			if (mipmaps == MIPS_NORMALIZED)
				NormalizeMipMap (buf, width, height);
			
			miplevel++;
			gl.glTexImage2D (GL_TEXTURE_2D, miplevel, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);

			// test
		/*	char tempname[256];
			sprintf(tempname, "%s/nmapdump/%d_%d.tga", gEngfuncs.pfnGetGameDirectory(), counter, miplevel);
			if (WriteTGA_24(buf, width, height, tempname))
				gEngfuncs.Con_Printf("written %d mipmap %d (%d x %d)\n", counter, miplevel, width, height);
			else
				gEngfuncs.Con_Printf("failed to write %d mipmap %d\n", counter, miplevel);*/
		}
	}
//	counter++;

	gEngfuncs.COM_FreeFile(file);

	const char *mipmsg = "bad mipmaps mode";
	switch (mipmaps)
	{
	case MIPS_NO: mipmsg = "no mips"; break;
	case MIPS_YES: mipmsg = "generated mips"; break;
	case MIPS_NORMALIZED: mipmsg = "normalized mips"; break;
	}
	Log("Loaded texture %s, %s\n", filename, mipmsg);

	if (useid)
		return useid;

	current_ext_texture_id++;
	return (current_ext_texture_id - 1);
}

//===================================
// Create_DSDT_Texture
//
// loads 24bit TGAs, and converts it to DSDT NV format
//	returns gl texture id, or 0 in error case
//===================================
typedef struct
{
	byte ds, dt;
} colorDSDT;

int Create_DSDT_Texture(const char* filename)
{
	colorDSDT buf[TEXBUFFER_SIZE];

	if (!gl.IsGLAllowed())
	{
		ConLog("Create_DSDT_Texture error: glmanager is not initialized!\n");
		return 0;
	}

	int length = 0;
	char *file = (char *)gEngfuncs.COM_LoadFile( (char*)filename, 5, &length );
	if (!file)
	{
		Log("Create_DSDT_Texture failed to load file: %s\n", filename);
		return 0;
	}

	TGAheader *hdr = (TGAheader *)file;
	if (!(/*hdr->DataType == 10 ||*/ hdr->DataType == 2) || (hdr->BPP != 24))
	{
		ConLog("Error: texture %s uses unsupported data format -->\n(only 24-bit noncompressed TGA supported)\n", filename);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	int maxtexsize;
	gl.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
	if (hdr->Width > maxtexsize || hdr->Height > maxtexsize)
	{
		ConLog("Error: texture %s: sizes %d x %d are not supported by your hardware\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	 // lame :)
	if (!(hdr->Width == 16 || hdr->Width == 32 || hdr->Width == 64 || hdr->Width == 128 || hdr->Width == 256 || hdr->Width == 512 || hdr->Width == 1024) ||
		!(hdr->Height == 16 || hdr->Height == 32 || hdr->Height == 64 || hdr->Height == 128 || hdr->Height == 256 || hdr->Height == 512 || hdr->Height == 1024))
	{
		ConLog("Error: texture %s has bad dimensions (%d x %d)\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	int numpixels = hdr->Width * hdr->Height;
	if (numpixels > TEXBUFFER_SIZE)
	{
		ConLog("Error: texture %s (%d x %d) doesn't fit in static buffer\n", filename, hdr->Width, hdr->Height);
		gEngfuncs.COM_FreeFile(file);
		return 0;
	}

	color24 *pixels = (color24*)(file + sizeof(TGAheader) + hdr->IdLength);
	colorDSDT *dest = buf;
	
	// non-compressed image
	for (int i = 0; i < numpixels; i++, pixels++, dest++)
	{
		dest->dt = pixels->g;
		dest->ds = pixels->r;
	}

	// flip image
	if (!(hdr->Description & 0x20))
	{
		colorDSDT line[1024];
		for (int i = 0; i < hdr->Height / 2; i++)
		{
			int linelen = hdr->Width * sizeof(colorDSDT);
			memcpy(line, &buf[i*hdr->Width], linelen);
			memcpy(&buf[i*hdr->Width], &buf[(hdr->Height - i - 1)*hdr->Width], linelen);
			memcpy(&buf[(hdr->Height - i - 1)*hdr->Width], line, linelen);
		}
	}
	
	// upload image
	gl.glBindTexture(GL_TEXTURE_2D, current_ext_texture_id);
	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	gl.glTexImage2D (GL_TEXTURE_2D, 0, GL_DSDT8_NV, hdr->Width, hdr->Height, 0, GL_DSDT_NV, GL_UNSIGNED_BYTE, buf);

	gEngfuncs.COM_FreeFile(file);

	Log("Loaded DSDT texture %s\n", filename);

	current_ext_texture_id++;
	return (current_ext_texture_id - 1);
}



// ===========================
// Detail textures uploading
//		(dont call from VidInit - we dont know the mapname at that point)
// ===========================

// parsed detail textures file data
#define MAX_DETAIL_FILE_ENTRIES		(512)

typedef struct detailtexFileEntry_s
{
	char	texname[16];
	char	detailtexname[32];
	int		xscale;
	int		yscale;
} detailtexFileEntry;

detailtexFileEntry parsedData[MAX_DETAIL_FILE_ENTRIES];
int numentries = 0;


int ParseDetailTextureDataFile()
{
	char levelname[256];
	numentries = 0;
	strcpy( levelname, gEngfuncs.pfnGetLevelName() );
	Log("\n>> loading detail textures\n");

	if ( strlen(levelname) == 0 )
	{
		ConLog("ERROR: ParseDetailTextureDataFile() cant get level name!\n");
		return FALSE;
	}

	levelname[strlen(levelname)-4] = 0;
	strcat(levelname, "_detail.txt");

	char *pfile = (char *)gEngfuncs.COM_LoadFile( levelname, 5, NULL);
	if (!pfile)
	{
		ConLog("No detail textures file %s\n", levelname);
		return FALSE;
	}

	numentries = 0;
	char *ptext = pfile;
	while(1)
	{
		char texture[256];
		char detailtexture[256];
		char sz_xscale[64];
		char sz_yscale[64];

		if (numentries >= MAX_DETAIL_FILE_ENTRIES)
		{
			ConLog("Too many entries in detail textures file %s\n", levelname);
			break;
		}

		ptext = gEngfuncs.COM_ParseFile(ptext, texture);
		if (!ptext) break;
		ptext = gEngfuncs.COM_ParseFile(ptext, detailtexture);
		if (!ptext) break;
		ptext = gEngfuncs.COM_ParseFile(ptext, sz_xscale);
		if (!ptext) break;
		ptext = gEngfuncs.COM_ParseFile(ptext, sz_yscale);
		if (!ptext) break;

		int i_xscale = atoi(sz_xscale);
		int i_yscale = atoi(sz_yscale);

		if (strlen(texture) > 16 || strlen(detailtexture) > 32)
		{
			ConLog("Error in detail textures file %s: token too large\n", levelname);
			ConLog("(entry %d: %s - %s)\n", numentries, texture, detailtexture);
			continue;
		}

	//	gEngfuncs.Con_Printf("%d: %s - %s (%s x %s)\n", numentries, texture, detailtexture, sz_xscale, sz_yscale);

		strcpy( parsedData[numentries].texname, texture );
		strcpy( parsedData[numentries].detailtexname, detailtexture );
		parsedData[numentries].xscale = i_xscale;
		parsedData[numentries].yscale = i_yscale;
		numentries++;
	}
	gEngfuncs.COM_FreeFile( pfile );
	return TRUE;
}


int GetDetailTextureData(char *_texname, int &glid, int &x_scale, int &y_scale)
{
	detailtexFileEntry *pentry = NULL;
	glid = 0;

	// get detail texture data from file
	for (int i = 0; i < numentries; i++)
	{
		if (!strcmp(parsedData[i].texname, _texname))
		{
			pentry = &parsedData[i];
			break;
		}
	}

	if (!pentry)
		return FALSE;

	x_scale = pentry->xscale;
	y_scale = pentry->yscale;

	// try to find detail texture in list
	DetailTexture *plist = p_detailTexturesList;
	while(plist)
	{
		if (!strcmp(plist->name, pentry->detailtexname))
		{
			glid = plist->gl_id;
			return TRUE;
		}
		plist = plist->pnext;
	}

	// load texture
	char path[256];
	sprintf(path, "gfx/%s.tga", pentry->detailtexname);

	glid = CreateTexture(path, MIPS_YES);
	if (!glid)
	{
		ConLog("Detail texture %s not found!\n", path);
		return FALSE;
	}

	new DetailTexture(pentry->detailtexname, glid);

	Log("Loaded detail texture %s\n", pentry->detailtexname);
	return TRUE;
}


// ===========================
// CreateExtDataForTextures()
//
// Loads all extra textures for base texture -
//		detail textures, normal maps, gloss maps
//
// Only non-animated textures support.
// pointer for extra data saved into anim_next texture_t pointer
// ===========================

ExtTextureData *FindTextureExtraData(char *texname)
{
	ExtTextureData *plist = p_extTexDataList;
	while(plist)
	{
		if (!strcmp(plist->name, texname))
			return plist;

		plist = plist->pnext;
	}
	return NULL; // nothing found
}


void CreateExtDataForTextures( )
{
	ParseDetailTextureDataFile();

	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;
	if (!world) return;

	Log("\n>> Creating ext data for textures\n");

	texture_t** tex = (texture_t**)world->textures;
	for (int i = 0; i < world->numtextures; i++)
	{
		if (tex[i]->anim_total || tex[i]->name[0] == 0 ||
			tex[i]->name[0] == '+' || tex[i]->name[0] == '-')
			continue;
		
		// get detail texture for it
		int det_texid = 0;
		int det_xscale = 0, det_yscale = 0;
		GetDetailTextureData(tex[i]->name, det_texid, det_xscale, det_yscale);

		ExtTextureData *pExtData;

		// data already allocated?
		pExtData = FindTextureExtraData(tex[i]->name);

		if (!pExtData)
		{
			// create new
			pExtData = new ExtTextureData(tex[i]->name);

			Log("Loaded texture data for %s\n", tex[i]->name);

			// load other special textures here

			// try to load normal map
			char filename[256];
			sprintf(filename, "gfx/bumpmaps/%s.tga", tex[i]->name);
			pExtData->gl_normalmap_id = CreateTexture(filename, MIPS_NORMALIZED);

			// try to load gloss map
			sprintf(filename, "gfx/bumpmaps/%s_gloss.tga", tex[i]->name);
			pExtData->gl_glossmap_id = CreateTexture(filename, MIPS_YES);

			// try to load gloss map for high quality specular
			if (pExtData->gl_normalmap_id)
			{
				sprintf(filename, "gfx/bumpmaps/%s_glosshi.tga", tex[i]->name);
				pExtData->gl_extra_glossmap_id = CreateTexture(filename, MIPS_YES);
			}
			else
				pExtData->gl_extra_glossmap_id = 0;
		}

		// store detail texture info (this may change between maps)
		pExtData->gl_detailtex_id = det_texid;
		pExtData->detail_xscale = det_xscale;
		pExtData->detail_yscale = det_yscale;

		tex[i]->anim_next = (struct texture_s *)pExtData; // HACK
		tex[i]->anim_min = -666;				
	}
	Log("Finished creating ext data for textures\n");

	SortTexturesByDetailTexID();
}


// ===========================
// Sorting textures pointers in world->textures array by detail texture id.
// This may allow us make less state switches during rendering
// ===========================

int CompareDetailIDs( texture_t *a, texture_t *b )
{
	int detail_id_a = 0;
	int detail_id_b = 0;
	ExtTextureData *pExtData;

	if (a->anim_min == -666)
	{
		pExtData = (ExtTextureData*)a->anim_next;
		if (pExtData)
			detail_id_a = pExtData->gl_detailtex_id;
	}

	if (b->anim_min == -666)
	{
		pExtData = (ExtTextureData*)b->anim_next;
		if (pExtData)
			detail_id_b = pExtData->gl_detailtex_id;
	}

	return (detail_id_a > detail_id_b);
}


void SortTexturesByDetailTexID()
{
	model_t* world = gEngfuncs.GetEntityByIndex(0)->model;
	if (!world) return;

	Log("Sorting textures by detail texture id...");

	// use simple insert sort
	texture_t** tex = (texture_t**)world->textures;
	texture_t *t;
    int i, j;
    for (i = 0 + 1; i < world->numtextures; i++)
	{
        t = tex[i];

        /* Сдвигаем элементы вниз, пока */
        /*  не найдем место вставки.    */
        for (j = i-1; j >= 0 && CompareDetailIDs(tex[j], t); j--)
            tex[j+1] = tex[j];

        /* вставка */
        tex[j+1] = t;
    }

	Log("ok\n");
}


void DeleteExtTextures()
{
	if (p_detailTexturesList) delete p_detailTexturesList;
	if (p_extTexDataList) delete p_extTexDataList;
}



//=====================================
// Cached textures
//
// damn, i should make thi a long time ago,
// and finally the time has come...
//=====================================
#define MAX_CACHED_TEXTURES 256

struct cache_tex {
	char name[64];
	int glid;
};

cache_tex cached_textures[MAX_CACHED_TEXTURES];
int num_cached_textures;

int LoadCacheTexture(const char* filename, int mipmaps, int clamped)
{
	int i;
	for (i = 0; i < num_cached_textures; i++)
	{
		if (!strcmp(cached_textures[i].name, filename))
			return cached_textures[i].glid;
	}

	if (i == MAX_CACHED_TEXTURES)
	{
		ONCE( ConLog("Error: LoadCacheTexture failed: MAX_CACHED_TEXTURES exceeded!\n"); );
		return 0;
	}

	cached_textures[i].glid = CreateTexture(filename, mipmaps);
	if (!cached_textures[i].glid)
		return 0;

	strcpy(cached_textures[i].name, filename);
	num_cached_textures++;	
	if(clamped)
	{
		gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}
	return cached_textures[i].glid;
}