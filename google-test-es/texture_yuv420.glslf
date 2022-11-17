////////////////////////////////////////////////////////////////////////////////////////////////////////////
// texture mapping with yuv420-to-rgb CSC, fragment shader
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

uniform sampler2D y_map;
uniform sampler2D u_map;
uniform sampler2D v_map;

void main()
{
/*	ITU-R BT.601 standard RGB-to-YPbPr:

         .299f,  .587f,  .114f,  0.f,
        -.169f, -.331f,  .499f,  .5f,
         .499f, -.418f, -.0813f, .5f,
        0.f,    0.f,    0.f,     1.f

	Inverse:

		1.000422	0.000302	1.404659	-0.702481
		0.999440	-0.344932	-0.715684	0.530308
		1.001776	1.775307	0.000994	-0.888151
		0.000000	0.000000	0.000000	1.000000
*/
	vec3 yuv;
	yuv.x = texture2D(y_map, tcoord_i).r;
	yuv.y = texture2D(u_map, tcoord_i).r;
	yuv.z = texture2D(v_map, tcoord_i).r;

	vec3 rgb_res = vec3(
		-0.702481,
		0.530308,
		-0.888151);
	rgb_res += yuv.x * vec3(
		1.000422,
		0.999440,
		1.001776);
	rgb_res += yuv.y * vec3(
		0.000302,
	 	-0.344932,
	 	1.775307);
	rgb_res += yuv.z * vec3(
		1.404659,
		-0.715684,
		0.000994);

	xx_FragColor = vec4(rgb_res, 1.0);
}
