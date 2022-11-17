////////////////////////////////////////////////////////////////////////////////////////////////////////////
// matmul fragment shader
//
// routine computes a single matrix row C[i] of C = A * B, where A, B and C are 4x4 matrices. A multitude of
// source A & B matrices is packed in two maps: a_map and b_map. A-matrices are stored sequentially by rows:
//
// A1_row1, A1_row2, A1_row3, A1_row4, ...
//
// B-matrices are stored sequentially by rows as well:
//
// B1_row1, B1_row2, B1_row3, B1_row4, ...
//
// Resulting C-matrices are stored in the framebuffer in the same order as A-matrices.
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

uniform sampler2D a_map;
uniform sampler2D b_map;

void main()
{
	const vec2 tcoord_delta = vec2(1.0 / 512.0); // viewport, a_map & b_map share the same dimensions

	vec4 a = texture2D(a_map, tcoord_i);

	float modulus = mod(gl_FragCoord.x, 4.0) * tcoord_delta.x;

	vec4 b0 = texture2D(b_map, vec2(tcoord_i.x - modulus + 0.5 * tcoord_delta.x, tcoord_i.y));
	vec4 b1 = texture2D(b_map, vec2(tcoord_i.x - modulus + 1.5 * tcoord_delta.x, tcoord_i.y));
	vec4 b2 = texture2D(b_map, vec2(tcoord_i.x - modulus + 2.5 * tcoord_delta.x, tcoord_i.y));
	vec4 b3 = texture2D(b_map, vec2(tcoord_i.x - modulus + 3.5 * tcoord_delta.x, tcoord_i.y));

	xx_FragColor =
		a.x * b0 +
		a.y * b1 +
		a.z * b2 +
		a.w * b3;
}
