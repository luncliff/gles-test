////////////////////////////////////////////////////////////////////////////////////////////////////////////
// texture mapping from an external sampler, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 0
#error Shader uses GLES-specific functionality
#endif

#extension GL_OES_EGL_image_external : require

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

uniform samplerExternalOES albedo_map;

varying vec2 tcoord_i;

void main()
{
	gl_FragColor = texture2D(albedo_map, tcoord_i);
}
