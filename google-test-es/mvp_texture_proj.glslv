////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mvp vertex shader with projective texture mapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;

out_qualifier vec4 tcoord_i;

uniform mat4 mvp;
uniform mat4 mvp_lit;

void main()
{
	gl_Position = mvp * vec4(at_Vertex, 1.0);

	tcoord_i = mvp_lit * vec4(at_Vertex, 1.0);
}
