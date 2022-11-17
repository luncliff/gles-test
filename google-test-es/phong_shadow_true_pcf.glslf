////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pcf shadowed phong with one light source (no clipping), fragment shader
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
	const float shadow_res = 2048.0;

	vec3 p_lit = p_lit_i.xyz / p_lit_i.w;
	vec2 uv = p_lit.xy * shadow_res;
	// the following statement is intended for bilinear samplers;
	// for nearest samplers change to: vec2 st = uv - 0.5;
	vec2 st = floor(uv) - 0.5 + step(0.5, fract(uv));
	vec2 w1 = fract(uv - 0.5);
	vec2 w0 = 1.0 - w1;

	st /= shadow_res;
	vec4 shadow_z = vec4(
		texture2D(shadow_map, st + vec2(0.0, 0.0) / shadow_res).x,
		texture2D(shadow_map, st + vec2(1.0, 0.0) / shadow_res).x,
		texture2D(shadow_map, st + vec2(0.0, 1.0) / shadow_res).x,
		texture2D(shadow_map, st + vec2(1.0, 1.0) / shadow_res).x);

	vec3 n = normalize(n_obj_i);
	vec3 l = normalize(l_obj_i);
	vec3 h = normalize(h_obj_i);

	float lambertian = max(dot(n, l), 0.0);
	float blinnian	 = max(dot(n, h), 0.0);

	vec3 d = lprod_diffuse  * lambertian;
	vec3 s = lprod_specular * pow(blinnian, shininess);

#if 1 // leave slope offsetting to shadow map generation
	float p_z_lit = p_lit.z;

#else
	const float epsilon = 1.0 / 128.0;

	float p_z_lit = p_lit.z - (1.0 - lambertian) * epsilon;

#endif

	vec4 lit = step(vec4(p_z_lit), shadow_z);
	float lit_weighed = dot(lit, vec4(
		w0.x * w0.y,
		w1.x * w0.y,
		w0.x * w1.y,
		w1.x * w1.y));

	d *= lit_weighed;
	s *= lit_weighed;

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
