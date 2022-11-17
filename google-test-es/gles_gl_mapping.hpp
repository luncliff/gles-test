#ifndef gles_gl_mapping_H__
#define gles_gl_mapping_H__

#if !defined(PLATFORM_GLX)
#error Wrong side of the road.
#endif

#if !defined(glClearDepthf)
#define glClearDepthf glClearDepth
#endif

#if !defined(GL_DEPTH_STENCIL_OES)
#define GL_DEPTH_STENCIL_OES GL_DEPTH_STENCIL
#endif

#if !defined(GL_UNSIGNED_INT_24_8_OES)
#define GL_UNSIGNED_INT_24_8_OES GL_UNSIGNED_INT_24_8
#endif

#if !defined(GL_RGB565)
#define GL_RGB565 GL_RGB5
#endif

#endif // gles_gl_mapping_H__
