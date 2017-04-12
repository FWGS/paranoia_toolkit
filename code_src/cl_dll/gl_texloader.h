#ifndef __TEXLOADER_H
#define __TEXLOADER_H

#define MAX_EXT_TEXTURE_NAME	16


class ExtTextureData
{
public:
	ExtTextureData(char *_name);
	~ExtTextureData();

	char	name[MAX_EXT_TEXTURE_NAME];
	int		gl_normalmap_id;
	int		gl_detailtex_id;
	int		gl_glossmap_id;
	int		gl_extra_glossmap_id;

	int		detail_xscale, detail_yscale;

	ExtTextureData *pnext;
};



enum {
	MIPS_NO		= 0,	// dont generate mipmaps
	MIPS_YES	= 1,	// generate mipmaps
	MIPS_NORMALIZED = 2 // generate and renormalize mipmaps (for normal maps)
};

int CreateTexture		(const char* filename, int mipmaps, int useid = 0);
//int Create_DSDT_Texture	(const char* filename);
int LoadCacheTexture	(const char* filename, int mipmaps, int clamped);
ExtTextureData* GetExtTexData (texture_t *tex);


void CreateExtDataForTextures();
void DeleteExtTextures();

extern int current_ext_texture_id;


#endif