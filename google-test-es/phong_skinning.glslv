////////////////////////////////////////////////////////////////////////////////////////////////////
// unshadowed, textured, skinned phong for one positional/directional light source
////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#define in_qualifier attribute
#define out_qualifier varying

#else

#define in_qualifier in
#define out_qualifier out

#endif

in_qualifier vec3 at_Vertex;
in_qualifier vec3 at_Normal;
in_qualifier vec4 at_Weight;
in_qualifier vec2 at_MultiTexCoord0;

out_qualifier vec3 n_obj_i;
out_qualifier vec3 l_obj_i;
out_qualifier vec3 h_obj_i;
out_qualifier vec2 tcoord_i;

uniform mat4 bone[32];	// maximum 64 index-able
uniform mat4 mvp;		// mvp to clip space
uniform vec4 lp_obj;
uniform vec4 vp_obj;

void main()
{
	tcoord_i = at_MultiTexCoord0;

	vec4 weight = vec4(at_Weight.xyz, 1.0 - (at_Weight.x + at_Weight.y + at_Weight.z));

	vec4 fndex = mod(at_Weight.w * vec4(1.0, 1.0 / 64.0, 1.0 / 4096.0, 1.0 / 262144.0), vec4(64.0));
	ivec4 index = ivec4(fndex);

	vec3 p_obj = (bone[index[0]] * vec4(at_Vertex, 1.0)).xyz * weight[0];
#if 0
	vec3 n_obj = (mat3(bone[index[0]]) * at_Normal) * weight[0];
#else
	vec3 n_obj = (mat3(bone[index[0]][0].xyz,
					   bone[index[0]][1].xyz,
					   bone[index[0]][2].xyz) * at_Normal) * weight[0];
#endif

	for (int i = 1; i < 4; ++i)
	{
		p_obj += (bone[index[i]] * vec4(at_Vertex, 1.0)).xyz * weight[i];
#if 0
		n_obj += (mat3(bone[index[i]]) * at_Normal) * weight[i];
#else
		n_obj += (mat3(bone[index[i]][0].xyz,
					   bone[index[i]][1].xyz,
					   bone[index[i]][2].xyz) * at_Normal) * weight[i];
#endif
	}

	gl_Position = mvp * vec4(p_obj, 1.0);

	n_obj_i = normalize(n_obj);

	vec3 l_obj = normalize(lp_obj.xyz - p_obj * lp_obj.w);
	vec3 v_obj = normalize(vp_obj.xyz - p_obj * vp_obj.w);

	l_obj_i = l_obj;
	h_obj_i = l_obj + v_obj;
}
