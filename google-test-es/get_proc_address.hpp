#ifndef get_proc_address_H__
#define get_proc_address_H__

#include <dlfcn.h>
#include <iostream>

#if defined(PLATFORM_GLX)
#define DEFAULT_GL_LIB "libGL.so"
#else
#define DEFAULT_GL_LIB "libGLESv2.so"
#endif

static void*
getProcAddress(
	const char* proc_name,
	const char* lib_name = DEFAULT_GL_LIB)
{
	void* library = dlopen(lib_name, RTLD_NOW | RTLD_GLOBAL);

	if (!library)
	{
		std::cerr << "error: " << dlerror() << std::endl;
		return 0;
	}

	void* proc = dlsym(library, proc_name);

	if (!proc)
		std::cerr << "error: " << dlerror() << std::endl;

	return proc;
}

#endif // get_proc_address_H__
