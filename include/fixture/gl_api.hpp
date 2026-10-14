#pragma once
// Minimal OpenGL 3.3 function declarations for dynamic loading. Windows SDK
// ships only gl.h, so do not require an external glext.h for the MSVC build.
#include <cstddef>
#ifdef _WIN32
#define FIXTURE_GL_CALL APIENTRY
#else
#define FIXTURE_GL_CALL
#endif
using GLsizeiptr=std::ptrdiff_t;
using GLchar=char;
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#endif
using PFNGLGENVERTEXARRAYSPROC=void(FIXTURE_GL_CALL*)(GLsizei,GLuint*);
using PFNGLBINDVERTEXARRAYPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC=void(FIXTURE_GL_CALL*)(GLsizei,const GLuint*);
using PFNGLGENBUFFERSPROC=void(FIXTURE_GL_CALL*)(GLsizei,GLuint*);
using PFNGLBINDBUFFERPROC=void(FIXTURE_GL_CALL*)(GLenum,GLuint);
using PFNGLBUFFERDATAPROC=void(FIXTURE_GL_CALL*)(GLenum,GLsizeiptr,const void*,GLenum);
using PFNGLDELETEBUFFERSPROC=void(FIXTURE_GL_CALL*)(GLsizei,const GLuint*);
using PFNGLCREATESHADERPROC=GLuint(FIXTURE_GL_CALL*)(GLenum);
using PFNGLSHADERSOURCEPROC=void(FIXTURE_GL_CALL*)(GLuint,GLsizei,const GLchar*const*,const GLint*);
using PFNGLCOMPILESHADERPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLGETSHADERIVPROC=void(FIXTURE_GL_CALL*)(GLuint,GLenum,GLint*);
using PFNGLGETSHADERINFOLOGPROC=void(FIXTURE_GL_CALL*)(GLuint,GLsizei,GLsizei*,GLchar*);
using PFNGLDELETESHADERPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLCREATEPROGRAMPROC=GLuint(FIXTURE_GL_CALL*)();
using PFNGLATTACHSHADERPROC=void(FIXTURE_GL_CALL*)(GLuint,GLuint);
using PFNGLLINKPROGRAMPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLGETPROGRAMIVPROC=void(FIXTURE_GL_CALL*)(GLuint,GLenum,GLint*);
using PFNGLGETPROGRAMINFOLOGPROC=void(FIXTURE_GL_CALL*)(GLuint,GLsizei,GLsizei*,GLchar*);
using PFNGLUSEPROGRAMPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLDELETEPROGRAMPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLVERTEXATTRIBPOINTERPROC=void(FIXTURE_GL_CALL*)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
using PFNGLENABLEVERTEXATTRIBARRAYPROC=void(FIXTURE_GL_CALL*)(GLuint);
using PFNGLGETUNIFORMLOCATIONPROC=GLint(FIXTURE_GL_CALL*)(GLuint,const GLchar*);
using PFNGLUNIFORMMATRIX3FVPROC=void(FIXTURE_GL_CALL*)(GLint,GLsizei,GLboolean,const GLfloat*);
#undef FIXTURE_GL_CALL
