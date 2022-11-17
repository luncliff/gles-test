////////////////////////////////////////////////////////////////////////////////////////////////////////////
// linear texture filtering, fragment shader
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
	const float albedo_res = 2.0;

	vec2 uv = tcoord_i * albedo_res;
	// the following statement is intended for bilinear samplers;
	// for nearest samplers change to: vec2 st = uv - 0.5;
	vec2 st = floor(uv) - 0.5 + step(0.5, fract(uv));
	vec2 w1 = fract(uv - 0.5);
	vec2 w0 = 1.0 - w1;

	st /= albedo_res;
	vec4 albedo_00 = texture2D(albedo_map, st + vec2(0.0, 0.0) / albedo_res);
	vec4 albedo_10 = texture2D(albedo_map, st + vec2(1.0, 0.0) / albedo_res);
	vec4 albedo_01 = texture2D(albedo_map, st + vec2(0.0, 1.0) / albedo_res);
	vec4 albedo_11 = texture2D(albedo_map, st + vec2(1.0, 1.0) / albedo_res);

	xx_FragColor =
		albedo_00 * w0.x * w0.y +
		albedo_10 * w1.x * w0.y +
		albedo_01 * w0.x * w1.y +
		albedo_11 * w1.x * w1.y;
}
