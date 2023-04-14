//====== Copyright  1996-2005, Valve Corporation, All rights reserved. =======//
//
// glentrypoints.cpp
//
//=============================================================================//
// Immediately include gl.h, etc. here to avoid compilation warnings.

#include "togl/rendermechanism.h"

#include "appframework/AppFramework.h"
#include "appframework/IAppSystemGroup.h"
#include "tier0/dbg.h"
#include "tier0/icommandline.h"
#include "tier0/dynfunction.h"
#include "interface.h"
#include "filesystem.h"
#include "filesystem_init.h"
#include "tier1/convar.h"
#include "vstdlib/cvar.h"
#include "inputsystem/ButtonCode.h"
#include "tier1.h"
#include "tier2/tier2.h"

#ifdef _LINUX
#include <GL/glx.h>
#endif
// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#if !defined(DX_TO_GL_ABSTRACTION)
#error
#endif

#if defined( USE_SDL ) || defined(OSX) 
	#include "appframework/ilaunchermgr.h"
#endif

#define DEBUG_ALL_GLCALLS 0

#if DEBUG_ALL_GLCALLS
bool g_bDebugOpenGLCalls = true;
bool g_bPrintOpenGLCalls = false;

#define GL_EXT(x,glmajor,glminor)
#define GL_FUNC(ext,req,ret,fn,arg,call) \
	static ret (*fn##_gldebugptr) arg = NULL; \
	static ret fn##_gldebug arg { \
	if (!g_bDebugOpenGLCalls) { return fn##_gldebugptr call; } \
	if (g_bPrintOpenGLCalls) { \
	printf("Calling %s ... ", #fn); \
	fflush(stdout); \
	} \
	ret retval = fn##_gldebugptr call; \
	if (g_bPrintOpenGLCalls) { \
	printf("%s returned!\n", #fn); \
	fflush(stdout); \
	} \
	const GLenum err = glGetError_gldebugptr(); \
	if ( err == GL_INVALID_FRAMEBUFFER_OPERATION_EXT ) { \
	const GLenum fberr = gGL->glCheckFramebufferStatus( GL_FRAMEBUFFER_EXT ); \
	printf("%s triggered error GL_INVALID_FRAMEBUFFER_OPERATION_EXT! (0x%X)\n\n\n", #fn, (int) fberr); \
	fflush(stdout); \
	__asm__ __volatile__ ( "int $3\n\t" ); \
	} else if (err != GL_NO_ERROR) { \
	printf("%s triggered error 0x%X!\n\n\n", #fn, (int) err); \
	fflush(stdout); \
	__asm__ __volatile__ ( "int $3\n\t" ); \
	} \
	return retval; \
}

#define GL_FUNC_VOID(ext,req,fn,arg,call) \
	static void (*fn##_gldebugptr) arg = NULL; \
	static void fn##_gldebug arg { \
	if (!g_bDebugOpenGLCalls) { fn##_gldebugptr call; return; } \
	if (g_bPrintOpenGLCalls) { \
	printf("Calling %s ... ", #fn); \
	fflush(stdout); \
	} \
	fn##_gldebugptr call; \
	if (g_bPrintOpenGLCalls) { \
	printf("%s returned!\n", #fn); \
	fflush(stdout); \
	} \
	const GLenum err = glGetError_gldebugptr(); \
	if ( err == GL_INVALID_FRAMEBUFFER_OPERATION_EXT ) { \
	const GLenum fberr = gGL->glCheckFramebufferStatus( GL_FRAMEBUFFER_EXT ); \
	printf("%s triggered error GL_INVALID_FRAMEBUFFER_OPERATION_EXT! (0x%X)\n\n\n", #fn, (int) fberr); \
	fflush(stdout); \
	__asm__ __volatile__ ( "int $3\n\t" ); \
	} else if (err != GL_NO_ERROR) { \
	printf("%s triggered error 0x%X!\n\n\n", #fn, (int) err); \
	fflush(stdout); \
	__asm__ __volatile__ ( "int $3\n\t" ); \
	} \
}

#include "togl/glfuncs.inl"
#undef GL_FUNC_VOID
#undef GL_FUNC
#undef GL_EXT
#endif

GL_GetProcAddressCallbackFunc_t gGL_GetProcAddressCallback = NULL;

COpenGLEntryPoints *GetOpenGLEntryPoints(GL_GetProcAddressCallbackFunc_t callback)
{
	if (gGL == NULL)
	{
		gGL_GetProcAddressCallback = callback;
		gGL = new COpenGLEntryPoints(LIBGL_SONAME);
		if (!gGL->m_bHave_OpenGL)
			Error( "Missing basic required OpenGL functionality. %s", LIBGL_SONAME );
	}
	return gGL;
}

void ClearOpenGLEntryPoints()
{
	if ( gGL )
	{
		gGL->ClearEntryPoints();
	}
}
COpenGLEntryPoints *ToGLConnectLibraries( CreateInterfaceFn factory )
{
	ConnectTier1Libraries( &factory, 1 );
	ConVar_Register();
	ConnectTier2Libraries( &factory, 1 );

	if ( !g_pFullFileSystem )
	{
		Warning( "ToGL was unable to access the required interfaces!\n" );
	}

	// NOTE! : Overbright is 1.0 so that Hammer will work properly with the white bumped and unbumped lightmaps.
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	#if defined( USE_SDL )
		g_pLauncherMgr = (ILauncherMgr *)factory( SDLMGR_INTERFACE_VERSION, NULL );		
	#elif defined( OSX )
		g_pLauncherMgr = (ILauncherMgr *)factory( COCOAMGR_INTERFACE_VERSION, NULL );
	#endif

	return gGL;
}

void ToGLDisconnectLibraries()
{
	DisconnectTier2Libraries();
	ConVar_Unregister();
	DisconnectTier1Libraries();
}

#define GLVERNUM(Major, Minor, Patch) (((Major) * 100000) + ((Minor) * 1000) + (Patch))

static void GetOpenGLVersion( const char *libname, int *major, int *minor, int *patch)
{
	*major = *minor = *patch = 0;
	static CDynamicFunctionOpenGL< true, const GLubyte *( APIENTRY *)(GLenum name), const GLubyte * > glGetString( libname, "glGetString");
	if (glGetString)
	{
		const char *version = (const char *) glGetString(GL_VERSION);
		if (version)
		{
			sscanf( version, "%d.%d.%d", major, minor, patch );
		}
	}
}

static int GetOpenGLVersionMajor(const char *libname)
{
	int major, minor, patch;
	GetOpenGLVersion(libname, &major, &minor, &patch);
	return major;
}

static int GetOpenGLVersionMinor(const char *libname)
{
	int major, minor, patch;
	GetOpenGLVersion(libname, &major, &minor, &patch);
	return minor;
}

static int GetOpenGLVersionPatch(const char *libname)
{
	int major, minor, patch;
	GetOpenGLVersion(libname, &major, &minor, &patch);
	return patch;
}

static bool CheckBaseOpenGLVersion(const char *libname)
{
	const int NEED_MAJOR = 2;
	const int NEED_MINOR = 0;
	const int NEED_PATCH = 0;

	int major, minor, patch;
	GetOpenGLVersion(libname, &major, &minor, &patch);

	const int need = GLVERNUM(NEED_MAJOR, NEED_MINOR, NEED_PATCH);
	const int have = GLVERNUM(major, minor, patch);
	if (have < need)
	{
		fprintf(stderr, "PROBLEM: You appear to have OpenGL %d.%d.%d, but we need at least %d.%d.%d!\n",
			major, minor, patch, NEED_MAJOR, NEED_MINOR, NEED_PATCH);
		return false;
	}
	return true;
}

static bool CheckOpenGLExtension_internal(const char *libname, const char *ext, const int coremajor, const int coreminor)
{
	if ((coremajor >= 0) && (coreminor >= 0))  // we know that this extension is part of the base spec as of GL_VERSION coremajor.coreminor.
	{
		int major, minor, patch;
		GetOpenGLVersion(libname, &major, &minor, &patch);
		const int need = GLVERNUM(coremajor, coreminor, 0);
		const int have = GLVERNUM(major, minor, patch);
		if (have >= need)
			return true;  // we definitely have access to this "extension," as it is part of this version of the GL's core functionality.
	}

	// okay, see if the GL_EXTENSIONS string reports it.
	static CDynamicFunctionOpenGL< true, const GLubyte *( APIENTRY *)(GLenum name), const GLubyte * > glGetString(libname, "glGetString");
	if (!glGetString)
		return false;

	// hacky scanning of this string, because I don't want to spend time breaking it into a vector like I should have.
	const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
	const size_t extlen = strlen(ext);
	while ((extensions) && (*extensions))
	{
		const char *ptr = strstr(extensions, ext);
#if _WIN32
		if (!ptr)
		{
			static CDynamicFunctionOpenGL< true, const char *( APIENTRY *)( ), const char * > wglGetExtensionsStringEXT(NULL, "wglGetExtensionsStringEXT");
			if (wglGetExtensionsStringEXT) 
			{
				extensions = wglGetExtensionsStringEXT();
				ptr = strstr(extensions, ext);
			}

			if (!ptr) 
			{
				return false;
			}
		}
#elif defined (OSX)
		if (!ptr)
			return false;  // definitely not there.
#else
		if (!ptr)
		{
			static CDynamicFunctionOpenGL< true, Display *( APIENTRY *)( ), Display* > glXGetCurrentDisplay( NULL, "glXGetCurrentDisplay");
			static CDynamicFunctionOpenGL< true, const char *( APIENTRY *)( Display*, int ), const char * > glXQueryExtensionsString( NULL, "glXQueryExtensionsString");
			if (glXQueryExtensionsString && glXGetCurrentDisplay) 
			{
				extensions = glXQueryExtensionsString(glXGetCurrentDisplay(), 0);
				ptr = strstr(extensions, ext);
			}

			if (!ptr) 
			{
				return false;
			}
		}
#endif

		// make sure this matches the entire string, and isn't a substring match of some other extension.
		// if ( ( (string is at start of extension list) or (the char before the string is a space) ) and
		//      (the next char after the string is a space or a null terminator) )
		if ( ((ptr == extensions) || (ptr[-1] == ' ')) &&
			((ptr[extlen] == ' ') || (ptr[extlen] == '\0')) )
			return true;  // found it!

		extensions = ptr + extlen;  // skip ahead, search again.
	}
	return false;
}

static bool CheckOpenGLExtension(const char *libname, const char *ext, const int coremajor, const int coreminor)
{
	const bool retval = CheckOpenGLExtension_internal(libname, ext, coremajor, coreminor);
	printf("This system %s the OpenGL extension %s.\n", retval ? "supports" : "DOES NOT support", ext);
	return retval;
}

// The GL context you want entry points for must be current when you hit this constructor!
COpenGLEntryPoints::COpenGLEntryPoints(const char *libname)
	: m_nTotalGLCycles(0)
	, m_nTotalGLCalls(0)
    , m_strLibName(libname)
	, m_nOpenGLVersionMajor(GetOpenGLVersionMajor(m_strLibName))
	, m_nOpenGLVersionMinor(GetOpenGLVersionMinor(m_strLibName))
	, m_nOpenGLVersionPatch(GetOpenGLVersionPatch(m_strLibName))
	, m_bHave_OpenGL(CheckBaseOpenGLVersion(m_strLibName))  // may reset to false as these lookups happen.
#define GL_EXT(x,glmajor,glminor) , m_bHave_##x(CheckOpenGLExtension(m_strLibName, #x, glmajor, glminor))
#define GL_FUNC(ext,req,ret,fn,arg,call) , fn(m_strLibName, #fn, m_bHave_##ext)
#define GL_FUNC_VOID(ext,req,fn,arg,call) , fn(m_strLibName, #fn, m_bHave_##ext)
#include "togl/glfuncs.inl"
#undef GL_FUNC_VOID
#undef GL_FUNC
#undef GL_EXT
{
	// Locally cache the copy of the GL device strings, to avoid needing to call these glGet's (which can be extremely slow) more than once.
	const char *pszString = ( const char * )glGetString(GL_VENDOR);
	m_pGLDriverStrings[cGLVendorString] = strdup( pszString ? pszString : "" );

	m_nDriverProvider = cGLDriverProviderUnknown;
	if ( V_stristr( m_pGLDriverStrings[cGLVendorString], "nvidia" ) )
		m_nDriverProvider = cGLDriverProviderNVIDIA;
	else if ( V_stristr( m_pGLDriverStrings[cGLVendorString], "amd" ) || V_stristr( m_pGLDriverStrings[cGLVendorString], "ati" ) )
		m_nDriverProvider = cGLDriverProviderAMD;
	else if ( V_stristr( m_pGLDriverStrings[cGLVendorString], "intel" ) )
		m_nDriverProvider = cGLDriverProviderIntelOpenSource;
	else if ( V_stristr( m_pGLDriverStrings[cGLVendorString], "apple" ) )
		m_nDriverProvider = cGLDriverProviderApple;

	pszString = ( const char * )glGetString(GL_RENDERER);
	m_pGLDriverStrings[cGLRendererString] = strdup( pszString ? pszString : "" );

	pszString = ( const char * )glGetString(GL_VERSION);
	m_pGLDriverStrings[cGLVersionString] = strdup( pszString ? pszString : "" );

	pszString = ( const char * )glGetString(GL_EXTENSIONS);
	m_pGLDriverStrings[cGLExtensionsString] = strdup( pszString ? pszString : "" );

	// !!! FIXME: Alfred says the original GL_APPLE_fence code only exists to
	// !!! FIXME:  hint Apple's drivers and not because we rely on the
	// !!! FIXME:  functionality. If so, just remove this check (and the
	// !!! FIXME:  GL_NV_fence code entirely).
 	if ((m_bHave_OpenGL) && ((!m_bHave_GL_NV_fence) && (!m_bHave_GL_ARB_sync) && (!m_bHave_GL_APPLE_fence)))
 	{
 		Error( "Required OpenGL extension \"GL_NV_fence\", \"GL_ARB_sync\", or \"GL_APPLE_fence\" is not supported. Please upgrade your OpenGL driver." );
 	}

	// same extension, different name.
	if (m_bHave_GL_EXT_vertex_array_bgra || m_bHave_GL_ARB_vertex_array_bgra)
	{
		m_bHave_GL_EXT_vertex_array_bgra = m_bHave_GL_ARB_vertex_array_bgra = true;
	}

	// GL_ARB_framebuffer_object is a superset of GL_EXT_framebuffer_object,
	//  (etc) but if you don't call in through the ARB entry points, you won't
	//  get the relaxed restrictions on mismatched attachment dimensions.
	if (m_bHave_GL_ARB_framebuffer_object)
	{
		m_bHave_GL_EXT_framebuffer_object = true;
		m_bHave_GL_EXT_framebuffer_blit = true;
		m_bHave_GL_EXT_framebuffer_multisample = true;
		glBindFramebufferEXT.Force(glBindFramebuffer.Pointer());
		glBindRenderbufferEXT.Force(glBindRenderbuffer.Pointer());
		glCheckFramebufferStatusEXT.Force(glCheckFramebufferStatus.Pointer());
		glDeleteRenderbuffersEXT.Force(glDeleteRenderbuffers.Pointer());
		glFramebufferRenderbufferEXT.Force(glFramebufferRenderbuffer.Pointer());
		glFramebufferTexture2DEXT.Force(glFramebufferTexture2D.Pointer());
		glFramebufferTexture3DEXT.Force(glFramebufferTexture3D.Pointer());
		glGenFramebuffersEXT.Force(glGenFramebuffers.Pointer());
		glGenRenderbuffersEXT.Force(glGenRenderbuffers.Pointer());
		glDeleteFramebuffersEXT.Force(glDeleteFramebuffers.Pointer());
		glBlitFramebufferEXT.Force(glBlitFramebuffer.Pointer());
		glRenderbufferStorageMultisampleEXT.Force(glRenderbufferStorageMultisample.Pointer());
	}
		
#if DEBUG_ALL_GLCALLS
	// push all GL calls through the debug wrappers.
#define GL_EXT(x,glmajor,glminor)
#define GL_FUNC(ext,req,ret,fn,arg,call) \
	fn##_gldebugptr = this->fn; \
	this->fn.Force(fn##_gldebug);
#define GL_FUNC_VOID(ext,req,fn,arg,call) \
	fn##_gldebugptr = this->fn; \
	this->fn.Force(fn##_gldebug);
#include "togl/glfuncs.inl"
#undef GL_FUNC_VOID
#undef GL_FUNC
#undef GL_EXT
#endif

#ifdef OSX
    m_bHave_GL_NV_bindless_texture = false;
    m_bHave_GL_AMD_pinned_memory = false;
#else
	if ( ( m_bHave_GL_NV_bindless_texture ) && ( !CommandLine()->CheckParm( "-gl_nv_bindless_texturing" ) ) )
	{
		m_bHave_GL_NV_bindless_texture = false;
		glGetTextureHandleNV.Force( NULL );
		glGetTextureSamplerHandleNV.Force( NULL );
		glMakeTextureHandleResidentNV.Force( NULL );
		glMakeTextureHandleNonResidentNV.Force( NULL );
		glUniformHandleui64NV.Force( NULL );
		glUniformHandleui64vNV.Force( NULL );
		glProgramUniformHandleui64NV.Force( NULL );
		glProgramUniformHandleui64vNV.Force( NULL );
		glIsTextureHandleResidentNV.Force( NULL );
	}

	if ( ( m_bHave_GL_AMD_pinned_memory ) && ( !CommandLine()->CheckParm( "-gl_amd_pinned_memory" ) ) )
	{
		m_bHave_GL_AMD_pinned_memory = false;
	}
#endif

	if ( ( m_bHave_GL_ARB_buffer_storage ) && ( CommandLine()->CheckParm( "-gl_disable_arb_buffer_storage" ) ) )
	{
		m_bHave_GL_ARB_buffer_storage = false;
	}

	char buf[256];
	V_snprintf(buf, sizeof( buf ), "GL_NV_bindless_texture: %s\n", m_bHave_GL_NV_bindless_texture ? "ENABLED" : "DISABLED" );
	Plat_DebugString( buf );

	V_snprintf(buf, sizeof( buf ), "GL_AMD_pinned_memory: %s\n", m_bHave_GL_AMD_pinned_memory ? "ENABLED" : "DISABLED" );
	Plat_DebugString( buf );

	V_snprintf( buf, sizeof(buf), "GL_ARB_buffer_storage: %s\n", m_bHave_GL_ARB_buffer_storage ? "AVAILABLE" : "NOT AVAILABLE" );
	Plat_DebugString( buf );

	V_snprintf(buf, sizeof( buf ), "GL_EXT_texture_sRGB_decode: %s\n", m_bHave_GL_EXT_texture_sRGB_decode ? "AVAILABLE" : "NOT AVAILABLE" );
	Plat_DebugString( buf );

	bool bGLCanDecodeS3TCTextures = m_bHave_GL_EXT_texture_compression_s3tc || ( m_bHave_GL_EXT_texture_compression_dxt1 && m_bHave_GL_ANGLE_texture_compression_dxt3 && m_bHave_GL_ANGLE_texture_compression_dxt5 );
	if ( !bGLCanDecodeS3TCTextures )
	{
		Error( "This application requires either the GL_EXT_texture_compression_s3tc or the GL_EXT_texture_compression_dxt1 + GL_ANGLE_texture_compression_dxt3 + GL_ANGLE_texture_compression_dxt5 OpenGL extensions. Please install S3TC texture support.\n" );
	}

#ifndef OSX
	if ( !m_bHave_GL_EXT_texture_sRGB_decode )
 	{
 		Error( "Required OpenGL extension \"GL_EXT_texture_sRGB_decode\" is not supported. Please update your OpenGL driver.\n" );
 	}
#endif
}

COpenGLEntryPoints::~COpenGLEntryPoints()
{
	for ( uint i = 0; i < cGLTotalDriverProviders; ++i )
	{
		free( m_pGLDriverStrings[i] );
		m_pGLDriverStrings[i] = NULL;
	}
}

void COpenGLEntryPoints::ClearEntryPoints()
{
	#define GL_EXT(x,glmajor,glminor)
	#define GL_FUNC(ext,req,ret,fn,arg,call) fn.Force( NULL );
	#define GL_FUNC_VOID(ext,req,fn,arg,call) fn.Force( NULL );
	#include "togl/glfuncs.inl"
	#undef GL_FUNC_VOID
	#undef GL_FUNC
	#undef GL_EXT
}
// Turn off memdbg macros (turned on up top) since this is included like a header
#include "tier0/memdbgoff.h"



