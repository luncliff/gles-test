////////////////////////////////////////////////////////////////////////////////////////////////////
// shadowed, textured, skinned phong for one positional/directional light source
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

out_qualifier vec4 p_lit_i;	// vertex position in light projection space
out_qualifier vec3 n_obj_i;
out_qualifier vec3 l_obj_i;
out_qualifier vec3 h_obj_i;
out_qualifier vec2 tcoord_i;

uniform mat4 bone[32];	// maximum 64 index-able
uniform mat4 mvp;		// mvp to clip space
uniform mat4 mvp_lit;	// mvp to light clip space
uniform vec4 lp_obj;
uniform vec4 vp_obj;

void main()
{
	tcoord_i = at_MultiTexCoord0;

	vec4 weight = vec4(at_Weight.xyz, 1.0 - (at_Weight.x + at_Weight.y + at_Weight.z));

	vec4 fndex = mod(at_Weight.w * vec4(1.0, 1.0 / 64.0, 1.0 / 4096.0, 1.0 / 262144.0), vec4(64.0));
	ivec4 index = ivec4(fndex);

	mat4 bone_0 = bone[index.x];
	mat4 bone_1 = bone[index.y];
	mat4 bone_2 = bone[index.z];
	mat4 bone_3 = bone[index.w];

	vec3 p_obj = (bone_0 * vec4(at_Vertex, 1.0)).xyz * weight.x;
	vec3 n_obj = (mat3(bone_0[0].xyz,
					   bone_0[1].xyz,
					   bone_0[2].xyz) * at_Normal) * weight.x;

	p_obj += (bone_1 * vec4(at_Vertex, 1.0)).xyz * weight.y;
	n_obj += (mat3(bone_1[0].xyz,
				   bone_1[1].xyz,
				   bone_1[2].xyz) * at_Normal) * weight.y;

	p_obj += (bone_2 * vec4(at_Vertex, 1.0)).xyz * weight.z;
	n_obj += (mat3(bone_2[0].xyz,
				   bone_2[1].xyz,
				   bone_2[2].xyz) * at_Normal) * weight.z;

	p_obj += (bone_3 * vec4(at_Vertex, 1.0)).xyz * weight.w;
	n_obj += (mat3(bone_3[0].xyz,
				   bone_3[1].xyz,
				   bone_3[2].xyz) * at_Normal) * weight.w;

	gl_Position = mvp * vec4(p_obj, 1.0);

	p_lit_i = mvp_lit * vec4(p_obj, 1.0);
	n_obj_i = normalize(n_obj);

	vec3 l_obj = normalize(lp_obj.xyz - p_obj * lp_obj.w);
	vec3 v_obj = normalize(vp_obj.xyz - p_obj * vp_obj.w);

	l_obj_i = l_obj;
	h_obj_i = l_obj + v_obj;
}
