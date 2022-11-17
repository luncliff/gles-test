////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pass-through vertex shader with texture mapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;
in_qualifier vec2 at_MultiTexCoord0;

out_qualifier vec2 tcoord_i;

void main()
{
	gl_Position = vec4(at_Vertex, 1.0);
	tcoord_i = at_MultiTexCoord0;
}
