/*
* Game-Deception Blank Wrapper v2
* Copyright (c) Crusader 2002
*/

/*
* Useful ogl functions for half-life including hooked extensions
*/

#include "opengl32.h"

/*
//booleans 
bool wall=true;
bool lambert=true;
bool smoke=true;
bool bSmoke=true;
bool flash=true;
bool bFlash=true;
bool modelviewport=false;
bool cross=false;
bool ch=true;


GLuint base; // for bitmap font
HDC hDC;

GLvoid BuildFont(GLvoid) // loads the opengl font into memory
{
	hDC=wglGetCurrentDC();
	HFONT	font;										
	HFONT	oldfont;									
	base = (*orig_glGenLists)(96);								
	font = CreateFont(-10,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,
	"Verdana");
	oldfont = (HFONT)SelectObject(hDC, font);           
	wglUseFontBitmaps(hDC, 32, 96, base);				
	SelectObject(hDC, oldfont);							
	DeleteObject(font);									
}

//////////////////////////////////////////////////////////////////////////////////////////////////


GLvoid KillFont(GLvoid)									
{
	(*orig_glDeleteLists)(base, 96);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void DrawText(float x, float y,float r, float g, float b, const char *fmt, ...)
{
	char		text[256];								
	va_list		ap;										
	
	if (fmt == NULL)									
		return;											
	
	va_start(ap, fmt);									
	vsprintf(text, fmt, ap);						
	va_end(ap);

	GLfloat  curcolor[4], position[4];
	(*orig_glPushAttrib)(GL_ALL_ATTRIB_BITS);
	(*orig_glGetFloatv)(GL_CURRENT_COLOR, curcolor);
	(*orig_glGetFloatv)(GL_CURRENT_RASTER_POSITION, position);
	(*orig_glDisable)(GL_TEXTURE_2D); 
	(*orig_glColor4f)(0.0f,0.0f,0.0f,1.0f);
	(*orig_glRasterPos2f)(x+1,y+1);

	//glPrint(text) - shadow
	(*orig_glPushAttrib)(GL_LIST_BIT);							
	(*orig_glListBase)(base - 32);								
	(*orig_glCallLists)(strlen(text), GL_UNSIGNED_BYTE, text);	
	(*orig_glPopAttrib)();										
	(*orig_glEnable)(GL_TEXTURE_2D); 

	(*orig_glDisable)(GL_TEXTURE_2D); 
	(*orig_glColor4f)(r,g,b,1.0f);
	(*orig_glRasterPos2f)(x,y);
	(*orig_glColor4f)(r,g,b,1.0f);

	//glPrint(text);
	(*orig_glPushAttrib)(GL_LIST_BIT);							
	(*orig_glListBase)(base - 32);								
	(*orig_glCallLists)(strlen(text), GL_UNSIGNED_BYTE, text);	
	(*orig_glPopAttrib)();										
	(*orig_glEnable)(GL_TEXTURE_2D); 

    //restore ogl 
    (*orig_glPopAttrib)();
    (*orig_glColor4fv)(curcolor);
    (*orig_glRasterPos2f)(position[0],position[1]);
}
*/

void sys_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
	orig_glMultiTexCoord2fARB(target,s,t);
}

void sys_glActiveTextureARB(GLenum target)
{
	orig_glActiveTextureARB(target);
}

void sys_BindTextureEXT(GLenum target, GLuint texture)
{
	orig_BindTextureEXT(target,texture);
}

void sys_glAlphaFunc (GLenum func,  GLclampf ref)
{
	(*orig_glAlphaFunc) (func, ref);
}

void sys_glBegin (GLenum mode)
{   // tells HL not to draw a wall when depth testing is disabled.
/*	if(wall)
	if((mode == GL_TRIANGLE_STRIP) || mode == GL_TRIANGLE_FAN) 
		{
			glDisable(GL_DEPTH_TEST);
	}

		if(smoke)
		{
			GLfloat smokecol[4];
			(*orig_glGetFloatv)(GL_CURRENT_COLOR, smokecol);
			if((smokecol[0]==smokecol[1]) && (smokecol[0]==smokecol[2]) && (smokecol[0]!=0.0) && (smokecol[0]!=1.0))
				bSmoke=true;
			else 
				bSmoke=false;
		}
		if(flash)
		{
			GLfloat flashcol[4];
			(*orig_glGetFloatv)(GL_CURRENT_COLOR, flashcol);
			if(flashcol[0]==1.0 && flashcol[1]==1.0 && flashcol[2]==1.0)
				bFlash=true;
			else
				bFlash=false;
		}
*/
//	if(mode == GL_TRIANGLE_STRIP)
//	{
//		glDisable(GL_DEPTH_TEST);
//	}

	(*orig_glBegin) (mode);
}

void sys_glBitmap (GLsizei width,  GLsizei height,  GLfloat xorig,  GLfloat yorig,  GLfloat xmove,  GLfloat ymove,  const GLubyte *bitmap)
{
	(*orig_glBitmap) (width, height, xorig, yorig, xmove, ymove, bitmap);
}

void sys_glBlendFunc (GLenum sfactor,  GLenum dfactor)
{
	(*orig_glBlendFunc) (sfactor, dfactor);
}

void sys_glClear (GLbitfield mask)
{
	// buz: clear also stencil buffer
/*	int hasStencil;
	glGetIntegerv(GL_STENCIL_BITS, &hasStencil);
	if (hasStencil)
	{
		mask |= GL_STENCIL_BUFFER_BIT;
		glClearStencil(0);
	//	glStencilMask(0); // dont write stencil by default
	}*/

	(*orig_glClear)(mask); 
}

void sys_glClearAccum (GLfloat red,  GLfloat green,  GLfloat blue,  GLfloat alpha)
{
	(*orig_glClearAccum) (red, green, blue, alpha);
}

void sys_glClearColor (GLclampf red,  GLclampf green,  GLclampf blue,  GLclampf alpha)
{
	(*orig_glClearColor) (red, green, blue, alpha);
}

void sys_glColor3f (GLfloat red,  GLfloat green,  GLfloat blue)
{
	(*orig_glColor3f) (red, green, blue);
}

void sys_glColor3ub (GLubyte red,  GLubyte green,  GLubyte blue)
{
	(*orig_glColor3ub) (red, green, blue);
}

void sys_glColor3ubv (const GLubyte *v)
{
	(*orig_glColor3ubv) (v);
}

void sys_glColor4f (GLfloat red,  GLfloat green,  GLfloat blue,  GLfloat alpha)
{
	(*orig_glColor4f) (red, green, blue, alpha);
}

void sys_glColor4ub (GLubyte red,  GLubyte green,  GLubyte blue,  GLubyte alpha)
{
	(*orig_glColor4ub) (red, green, blue, alpha);
}

void sys_glCullFace (GLenum mode)
{
	(*orig_glCullFace) (mode);
}

void sys_glDeleteTextures (GLsizei n,  const GLuint *textures)
{
	(*orig_glDeleteTextures) (n, textures);
}

void sys_glDepthFunc (GLenum func)
{
	(*orig_glDepthFunc) (func);
}

void sys_glDepthMask (GLboolean flag)
{
	(*orig_glDepthMask) (flag);
}

void sys_glDepthRange (GLclampd zNear,  GLclampd zFar)
{
	// buz
	if (zNear == 0 && (zFar == 0.5 || zFar == 1))
		zFar = 0.8;
	(*orig_glDepthRange) (zNear, zFar);
}

void sys_glDisable (GLenum cap)
{
	(*orig_glDisable) (cap);
}

void sys_glEnable (GLenum cap)
{
/*	if ((cross) && (!ch))
	{
		(*orig_glPushMatrix)(); 
		(*orig_glLoadIdentity)(); 
		(*orig_glDisable)(GL_TEXTURE_2D);
		(*orig_glEnable)(GL_BLEND);

		GLint iDim[4];

		(*orig_glGetIntegerv)(GL_VIEWPORT, iDim); 
		(*orig_glColor4f)(1.0f, 1.0f, 0.0f, 1.0f); 
		(*orig_glLineWidth)(1.0f); 

		(*orig_glBegin)(GL_LINES); 
		(*orig_glVertex2i)(iDim[2]/2, (iDim[3]/2)-22); 
		(*orig_glVertex2i)(iDim[2]/2, (iDim[3]/2)-2); 

		(*orig_glVertex2i)(iDim[2]/2, (iDim[3]/2)+2); 
		(*orig_glVertex2i)(iDim[2]/2, (iDim[3]/2)+22); 

		(*orig_glVertex2i)((iDim[2]/2)-22, iDim[3]/2); 
		(*orig_glVertex2i)((iDim[2]/2)-2, iDim[3]/2); 

		(*orig_glVertex2i)((iDim[2]/2)+2, iDim[3]/2); 
		(*orig_glVertex2i)((iDim[2]/2)+22, iDim[3]/2); 
		(*orig_glEnd)(); 

		(*orig_glDisable)(GL_BLEND);
		(*orig_glEnable)(GL_TEXTURE_2D); 
		(*orig_glPopMatrix)();
	}

	ch=true;
*/
	(*orig_glEnable) (cap);
}

void sys_glEnd (void)
{
	(*orig_glEnd) ();
}

void sys_glFrustum (GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble zNear,  GLdouble zFar)
{
	(*orig_glFrustum) (left, right, bottom, top, zNear, zFar);
}

void sys_glOrtho (GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble zNear,  GLdouble zFar)
{
	(*orig_glOrtho) (left, right, bottom, top, zNear, zFar);
}

void sys_glPopMatrix (void)
{
	(*orig_glPopMatrix) ();
}

void sys_glPopName (void)
{
	(*orig_glPopName) ();
}

void sys_glPrioritizeTextures (GLsizei n,  const GLuint *textures,  const GLclampf *priorities)
{
	(*orig_glPrioritizeTextures) (n, textures, priorities);
}

void sys_glPushAttrib (GLbitfield mask)
{
	(*orig_glPushAttrib) (mask);
}

void sys_glPushClientAttrib (GLbitfield mask)
{
	(*orig_glPushClientAttrib) (mask);
}

void sys_glPushMatrix (void)
{
	(*orig_glPushMatrix) ();
}

void sys_glRotatef (GLfloat angle,  GLfloat x,  GLfloat y,  GLfloat z)
{
	(*orig_glRotatef) (angle, x, y, z);
}

void sys_glShadeModel (GLenum mode)
{
	(*orig_glShadeModel) (mode);
}

void sys_glTexCoord2f (GLfloat s,  GLfloat t)
{
	(*orig_glTexCoord2f) (s, t);
}

void sys_glTexEnvf (GLenum target,  GLenum pname,  GLfloat param)
{
	(*orig_glTexEnvf) (target, pname, param);
}

void sys_glTexImage2D (GLenum target,  GLint level,  GLint internalformat,  GLsizei width,  GLsizei height,  GLint border,  GLenum format,  GLenum type,  const GLvoid *pixels)
{
	(*orig_glTexImage2D) (target, level, internalformat, width, height, border, format, type, pixels);
}

void sys_glTexParameterf (GLenum target,  GLenum pname,  GLfloat param)
{
	(*orig_glTexParameterf) (target, pname, param);
}

void sys_glTranslated (GLdouble x,  GLdouble y,  GLdouble z)
{
	(*orig_glTranslated) (x, y, z);
}

void sys_glTranslatef (GLfloat x,  GLfloat y,  GLfloat z)
{
	(*orig_glTranslatef) (x, y, z);
}

void sys_glVertex2f (GLfloat x,  GLfloat y)
{
/*	if(bFlash && flash)
	{
			GLfloat flashcol[4]; 
			(*orig_glGetFloatv)(GL_CURRENT_COLOR, flashcol); // we store the color and ...
			(*orig_glColor4f)(flashcol[0],flashcol[1],flashcol[2],0.01f);	// call the color but with very low alpha (trans)
			bFlash=false;
		}
*/
	(*orig_glVertex2f) (x, y);
}

void sys_glVertex3f (GLfloat x,  GLfloat y,  GLfloat z)
{
/*	if(lambert)
	{
		glColor3f(1.0f,1.0f,1.0f);
	}

*/
	(*orig_glVertex3f) (x, y, z);
}

void sys_glVertex3fv (const GLfloat *v)
{
/*	modelviewport=true;

	if(bSmoke && smoke) // leave this function if hl draws smoke
	{
		return;
	}

	
*/
	(*orig_glVertex3fv) (v);
}

void sys_glViewport (GLint x,  GLint y,  GLsizei width,  GLsizei height)
{
	(*orig_glViewport) (x, y, width, height);
}

PROC sys_wglGetProcAddress(LPCSTR ProcName)
{
	if (!strcmp(ProcName,"glMultiTexCoord2fARB"))
	{
		orig_glMultiTexCoord2fARB=(func_glMultiTexCoord2fARB_t)orig_wglGetProcAddress(ProcName);
		return (FARPROC)sys_glMultiTexCoord2fARB;
	}
	else if (!strcmp(ProcName,"glBindTextureEXT"))
	{
		orig_BindTextureEXT=(func_BindTextureEXT_t)orig_wglGetProcAddress(ProcName);
		return (FARPROC)sys_BindTextureEXT;
	}
	else if(!strcmp(ProcName,"glActiveTextureARB"))
	{
		orig_glActiveTextureARB=(func_glActiveTextureARB_t)orig_wglGetProcAddress(ProcName);
		return (FARPROC)sys_glActiveTextureARB;
	}
	return orig_wglGetProcAddress(ProcName);
}

void sys_wglSwapBuffers(HDC hDC)
{
	(*orig_wglSwapBuffers) (hDC);
}



void __cdecl add_log (const char * fmt, ...);


#pragma warning(disable:4100)
BOOL __stdcall DllMain (HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls (hOriginalDll);
		//	add_log("opengl32 loaded\n");
			return Init();

		case DLL_PROCESS_DETACH:
			if ( hOriginalDll != NULL )
			{
				FreeLibrary(hOriginalDll);
				hOriginalDll = NULL;
			}
			break;
	}
	return TRUE;
}
#pragma warning(default:4100)


void __cdecl add_log (const char * fmt, ...)
{
	va_list va_alist;
	char logbuf[256] = "";
    FILE * fp;
   
	va_start (va_alist, fmt);
	_vsnprintf (logbuf+strlen(logbuf), sizeof(logbuf) - strlen(logbuf), fmt, va_alist);
	va_end (va_alist);

	if ( (fp = fopen ("opengl32.log", "ab")) != NULL )
	{
		fprintf ( fp, "%s\n", logbuf );
		fclose (fp);
	}
}