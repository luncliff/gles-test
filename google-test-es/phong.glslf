////////////////////////////////////////////////////////////////////////////////////////////////////
// unshadowed, textured phong for one positional/directional light source, fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////

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

const vec4 scene_ambient	= vec4(0.2, 0.2, 0.2, 1.0);
const vec3 lprod_diffuse	= vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular	= vec3(0.7, 0.7, 0.5);
const float shininess		= 64.0;

in_qualifier vec3 n_obj_i;	// surface normal vector in object space
in_qualifier vec3 l_obj_i;	// light-source vector in object space, one per light
in_qualifier vec3 h_obj_i;	// half-direction vector in object space, one per light
in_qualifier vec2 tcoord_i;

uniform sampler2D albedo_map;

void main()
{
	vec4 tcolor = texture2D(albedo_map, tcoord_i);
	vec3 n = normalize(n_obj_i);
	vec3 l = normalize(l_obj_i);
	vec3 h = normalize(h_obj_i);

	vec3 d = lprod_diffuse * max(dot(n, l), 0.0);
	vec3 s = lprod_specular * pow(max(dot(n, h), 0.0), shininess);

	// be considerate of the older SIMD architectures and compilers
	xx_FragColor =
		vec4(s, 0.0) +
		vec4(d, 0.0) * tcolor +
		scene_ambient * tcolor;
}
