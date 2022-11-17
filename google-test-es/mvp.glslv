////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mvp, vertex shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(at_Vertex, 1.0);
}
