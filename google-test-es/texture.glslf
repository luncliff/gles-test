////////////////////////////////////////////////////////////////////////////////////////////////////////////
// texture mapping, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

#define in_qualifier varying
#define xx_FragColor gl_FragColor

#else

#define in_qualifier in
out vec4 xx_FragColor;

#endif

in_qualifier vec2 tcoord_i;

uniform sampler2D albedo_map;

void main()
{
	xx_FragColor = texture2D(albedo_map, tcoord_i);
}
