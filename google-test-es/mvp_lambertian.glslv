////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mvp and lambertian, vertex shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;
in_qualifier vec3 at_Normal;

out_qualifier vec3 n_obj_i;		// vertex normal in object space
out_qualifier vec3 l_obj_i;		// to-light-source vector in object space

uniform mat4 mvp;
uniform vec4 lp_obj;	// light-source position in object space

void main()
{
	gl_Position = mvp * vec4(at_Vertex, 1.0);

	n_obj_i = at_Normal;
	l_obj_i = normalize(lp_obj.xyz - at_Vertex * lp_obj.w);
}
