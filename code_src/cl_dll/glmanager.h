#ifndef __GLMANAGER_H
#define __GLMANAGER_H

#include "windows.h"
#include "gl/gl.h"
#include "gl/glext.h"
#include "log.h"



enum {
	GMSTATE_NOINIT = 0, // should try to load at next changelevel
	GMSTATE_INITFAILED, // failed to load, dont try until video mode changes
	GMSTATE_GL, // loaded ok, we are in opengl mode
};

class GLManager
{
public:
	GLManager();
	~GLManager();
	void Init();
	void VidInit();	

	int  IsGLAllowed();
	bool IsExtensionSupported (const char *ext);

	// imported gl functions (initialization failed if not present)
	void (APIENTRY *glAccum)		(GLenum op, GLfloat value);
	void (APIENTRY *glAlphaFunc)	(GLenum func, GLclampf ref);
	void (APIENTRY *glBegin)		(GLenum mode);
	void (APIENTRY *glBindTexture)	(GLenum target, GLuint texture);
	void (APIENTRY *glBlendFunc)	(GLenum sfactor, GLenum dfactor);
	void (APIENTRY *glClipPlane)	(GLenum plane, const GLdouble *equation);
	void (APIENTRY *glClear)		(GLbitfield mask);
	void (APIENTRY *glClearColor)	(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void (APIENTRY *glColor3f)	(GLfloat red, GLfloat green, GLfloat blue);
	void (APIENTRY *glColor4f)	(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void (APIENTRY *glColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
	void (APIENTRY *glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void (APIENTRY *glCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void (APIENTRY *glCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void (APIENTRY *glCullFace)		(GLenum mode);
	void (APIENTRY *glDepthMask)	(GLboolean flag);
	void (APIENTRY *glDepthRange)	(GLclampd zNear, GLclampd zFar);
	void (APIENTRY *glDepthFunc)	(GLenum func);
	void (APIENTRY *glDisable)	(GLenum cap);
	void (APIENTRY *glDisableClientState)	(GLenum array);
	void (APIENTRY *glDrawArrays) (GLenum mode, GLint first, GLsizei count);
	void (APIENTRY *glDrawBuffer) (GLenum mode);
	void (APIENTRY *glEnable)	(GLenum cap);
	void (APIENTRY *glEnableClientState) (GLenum array);
	void (APIENTRY *glEnd)		(void);
	void (APIENTRY *glFrustum)	(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	const GLubyte* (APIENTRY *glGetString) (GLenum name);
	GLenum (APIENTRY *glGetError)	(void);
	void (APIENTRY *glGetDoublev) (GLenum pname, GLdouble *params);
	void (APIENTRY *glGetFloatv) (GLenum pname, GLfloat *params);
	void (APIENTRY *glGetIntegerv) (GLenum pname, GLint *params);
	void (APIENTRY *glGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
	void (APIENTRY *glGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void (APIENTRY *glGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
	GLboolean (APIENTRY *glIsEnabled)	(GLenum cap);
	void (APIENTRY *glLoadIdentity)	(void);
	void (APIENTRY *glLoadMatrixf)	(const GLfloat *m);
	void (APIENTRY *glMatrixMode)	(GLenum mode);
	void (APIENTRY *glMultMatrixd)	(const GLdouble *m);
	void (APIENTRY *glOrtho)	(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void (APIENTRY *glPopAttrib)	(void);
	void (APIENTRY *glPushAttrib)	(GLbitfield mask);
	void (APIENTRY *glPixelStorei)	(GLenum pname, GLint param);
	void (APIENTRY *glPolygonOffset)	(GLfloat factor, GLfloat units);
	void (APIENTRY *glPushMatrix)	(void);
	void (APIENTRY *glPopMatrix)	(void);
	void (APIENTRY *glReadBuffer)	(GLenum mode);
	void (APIENTRY *glRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glScalef)		(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glShadeModel)	(GLenum mode);
	void (APIENTRY *glStencilFunc)	(GLenum func, GLint ref, GLuint mask);
	void (APIENTRY *glStencilOp)	(GLenum fail, GLenum zfail, GLenum zpass);
	void (APIENTRY *glStencilMask) (GLuint mask);
	void (APIENTRY *glTexCoord2f)	(GLfloat s, GLfloat t);
	void (APIENTRY *glTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTexEnvf)	(GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTexEnvi) (GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTexGenfv)	(GLenum coord, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTexGeni)	(GLenum coord, GLenum pname, GLint param);
	void (APIENTRY *glTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTexParameteri)	(GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTexParameterf) (GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTranslated)	(GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glTranslatef)	(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glVertex3f) (GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glVertex3fv)	(const GLfloat *v);
	void (APIENTRY *glVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glViewport)	(GLint x, GLint y, GLsizei width, GLsizei height);
	PROC (APIENTRY *wglGetProcAddress)	(LPCSTR);


	// extension specific functions

	// ARB_multitexture
	int ARB_multitexture_supported;
	int MAX_TU_supported;
	PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB;
	PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTextureARB;
	PFNGLMULTITEXCOORD1FARBPROC		glMultiTexCoord1fARB;
	PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB;
	PFNGLMULTITEXCOORD3FVARBPROC	glMultiTexCoord3fvARB;

	// Diffuse bump-mapping extensions
	int ARB_dot3_supported;

	// NV_register_combiners
	int NV_combiners_supported;
	int MAX_NV_combiners;
	PFNGLCOMBINERPARAMETERINVPROC	glCombinerParameteriNV;
	PFNGLCOMBINERPARAMETERFVNVPROC	glCombinerParameterfvNV;
	PFNGLCOMBINERINPUTNVPROC		glCombinerInputNV;
	PFNGLCOMBINEROUTPUTNVPROC		glCombinerOutputNV;
	PFNGLFINALCOMBINERINPUTNVPROC	glFinalCombinerInputNV;

	// ARB_fragment_program
	int ARB_fragment_program_supported;
	int fp_max_image_units;
	int fp_max_texcoords;
	PFNGLGENPROGRAMSARBPROC			glGenProgramsARB;
	PFNGLBINDPROGRAMARBPROC			glBindProgramARB;
	PFNGLPROGRAMSTRINGARBPROC		glProgramStringARB;
	PFNGLGETPROGRAMIVARBPROC		glGetProgramivARB;

	// NV_fragment_program
	int NV_fragment_program_supported;
	int NV_fp_max_image_units;
	int NV_fp_max_texcoords;
	PFNGLGENPROGRAMSNVPROC			glGenProgramsNV;
	PFNGLBINDPROGRAMNVPROC			glBindProgramNV;
	PFNGLLOADPROGRAMNVPROC			glLoadProgramNV;

	// Cubemaps
	int ARB_texture_cube_map_supported;
	PFNGLTEXIMAGE3DEXTPROC			glTexImage3DEXT;

	// 3d textures
	int EXT_3Dtexture_supported;

	// VBO
	int ARB_VBO_supported;
	PFNGLBINDBUFFERARBPROC					glBindBufferARB;
	PFNGLGENBUFFERSARBPROC					glGenBuffersARB;
	PFNGLBUFFERDATAARBPROC					glBufferDataARB;
	PFNGLDELETEBUFFERSARBPROC				glDeleteBuffersARB;
	PFNGLGETBUFFERPARAMETERIVARBPROC		glGetBufferParameterivARB;

	// Non-power of two textures
	int texture_rectangle_supported;
	int texture_rectangle_max_size;

//	void (APIENTRY *glColorTableEXT) (GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);

	// Paranoia hacks
	int Paranoia_hacks;
	int max_texture_size;

	GLint alphabits;
	GLint stencilbits;

	// glu functions
	void gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
			GLdouble centerx, GLdouble centery, GLdouble centerz,
			GLdouble upx, GLdouble upy, GLdouble upz );

private:
	int LoadFunctions();
	int Load_ARB_multitexture();
	int Load_ARB_dot3();
	int Load_ARB_fragment_program();
	int Load_NV_fragment_program();
	int Load_ARB_texture_cube_map();
	int Load_NV_register_combiners();
	int Load_EXT_texture_3D();
	int Load_ARB_VBO();
	int Load_Rectangle_Textures();
	void LogDebugInfo();

	HMODULE		hOpengl32dll;
//	cvar_t		*v_GLAllowed;
	int			glstate;
};

extern GLManager gl;


#endif // __GLMANAGER_H