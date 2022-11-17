////////////////////////////////////////////////////////////////////////////////////////////////////////////
// depth-only with lambertian offset for slope, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

#define in_qualifier varying

#else

#define in_qualifier in

#endif

in_qualifier vec3 n_obj_i;	// vertex normal in object space
in_qualifier vec3 l_obj_i;	// to-light-source vector in object space

void main()
{
	vec3 n = normalize(n_obj_i);
	vec3 l = normalize(l_obj_i);

	float lambertian = max(dot(n, l), 0.0);
	const float epsilon = 1.0 / 64.0;

	gl_FragDepth = gl_FragCoord.z + (1.0 - lambertian) * epsilon;
}
