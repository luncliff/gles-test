////////////////////////////////////////////////////////////////////////////////////////////////////////////
// shadowed phong with one light source, fragment shader
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

const vec4 scene_ambient	= vec4(0.2, 0.2, 0.2, 1.0);
const vec3 lprod_diffuse	= vec3(0.5, 0.5, 0.5);
const vec3 lprod_specular	= vec3(0.7, 0.7, 0.5);
const float shininess		= 64.0;

in_qualifier vec4 p_lit_i;	// vertex position in light projection space
in_qualifier vec3 n_obj_i;	// vertex normal in object space
in_qualifier vec3 l_obj_i;	// to-light-source vector in object space
in_qualifier vec3 h_obj_i;	// half-direction vector in object space
in_qualifier vec2 tcoord_i;

uniform sampler2D shadow_map;
uniform sampler2D albedo_map;

void main()
{
	vec3 l = normalize(l_obj_i);
	vec3 h = normalize(h_obj_i);
	vec3 n = normalize(n_obj_i);

	float lambertian = max(dot(n, l), 0.0);
	float blinnian	 = max(dot(n, h), 0.0);

	vec3 d = lprod_diffuse  * lambertian;
	vec3 s = lprod_specular * pow(blinnian, shininess);

	float clip = step(p_lit_i.z, p_lit_i.w); // standard [-1, 1] ndc was biased to [0, 1]

#if 1 // leave slope accomodations to shadow map generation
	float p_z = clip * (p_lit_i.z / p_lit_i.w);

#else
	const float epsilon = 1.0 / 128.0;

	float p_z = clip * (p_lit_i.z / p_lit_i.w - (1.0 - lambertian) * epsilon);

#endif

	float lit = step(p_z, texture2DProj(shadow_map, p_lit_i).x);

	d *= lit;
	s *= lit;

#if 1 // apply albedo map with alpha
	vec4 tcolor = texture2D(albedo_map, tcoord_i);

#else
	const vec4 tcolor = vec4(1.0);

#endif

	// be considerate of the older SIMD architectures and compilers
	xx_FragColor =
		vec4(s, 0.0) +
		vec4(d, 0.0) * tcolor +
		scene_ambient * tcolor;
}
