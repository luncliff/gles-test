#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "testbed.hpp"
#include "rendVect.hpp"
#include "rendSkeleton.hpp"

#define VERBOSE_READ	0

namespace testbed
{

template <>
class scoped_functor< FILE >
{
public:

	void operator()(FILE* arg)
	{
		fclose(arg);
	}
};

} // namespace testbed

namespace rend
{

void
initBoneMatx(
	const unsigned count,
	matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);
	assert(bone_idx < count);

	if (bone[bone_idx].matx_valid)
		return;

#if 1
	bone[bone_idx].to_model = matx4(bone[bone_idx].orientation);

	vect3& row0 = vect3::castU(bone[bone_idx].to_model.dekast()[0]);
	vect3& row1 = vect3::castU(bone[bone_idx].to_model.dekast()[1]);
	vect3& row2 = vect3::castU(bone[bone_idx].to_model.dekast()[2]);

	row0.mul(bone[bone_idx].scale);
	row1.mul(bone[bone_idx].scale);
	row2.mul(bone[bone_idx].scale);

	bone[bone_idx].to_model.setColS(3, bone[bone_idx].position);
#else
	bone[bone_idx].to_model = matx4(bone[bone_idx].orientation);
	bone[bone_idx].to_model.mul(matx4().translate(
		bone[bone_idx].position[0],
		bone[bone_idx].position[1],
		bone[bone_idx].position[2]));
	bone[bone_idx].to_model.mur(matx4().scale(
		bone[bone_idx].scale[0],
		bone[bone_idx].scale[1],
		bone[bone_idx].scale[2]));
#endif

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx)
	{
		assert(parent_idx < bone_idx);
		assert(bone[parent_idx].matx_valid);

		bone[bone_idx].to_model.mul(bone[parent_idx].to_model);
	}

	bone[bone_idx].to_local.invert(bone[bone_idx].to_model);
	bone[bone_idx].matx_valid = true;

	bone_mat[bone_idx].identity();
}


void
updateBoneMatx(
	const unsigned count,
	matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);
	assert(bone_idx < count);

	if (bone[bone_idx].matx_valid)
		return;

#if 1
	bone[bone_idx].to_model = matx4(bone[bone_idx].orientation);

	vect3& row0 = vect3::castU(bone[bone_idx].to_model.dekast()[0]);
	vect3& row1 = vect3::castU(bone[bone_idx].to_model.dekast()[1]);
	vect3& row2 = vect3::castU(bone[bone_idx].to_model.dekast()[2]);

	row0.mul(bone[bone_idx].scale);
	row1.mul(bone[bone_idx].scale);
	row2.mul(bone[bone_idx].scale);

	bone[bone_idx].to_model.setColS(3, bone[bone_idx].position);
#else
	bone[bone_idx].to_model = matx4(bone[bone_idx].orientation);
	bone[bone_idx].to_model.mul(matx4().translate(
		bone[bone_idx].position[0],
		bone[bone_idx].position[1],
		bone[bone_idx].position[2]));
	bone[bone_idx].to_model.mur(matx4().scale(
		bone[bone_idx].scale[0],
		bone[bone_idx].scale[1],
		bone[bone_idx].scale[2]));
#endif

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx)
	{
		assert(parent_idx < bone_idx);
		assert(bone[parent_idx].matx_valid);

		bone[bone_idx].to_model.mul(bone[parent_idx].to_model);
	}

	bone[bone_idx].matx_valid = true;

	bone_mat[bone_idx].mul(bone[bone_idx].to_model, bone[bone_idx].to_local);
	bone_mat[bone_idx].transpose(); // ES does not allow trasposing uniforms on the fly
}


void
updateRoot(
	Bone* root)
{
	assert(root);

	if (root->matx_valid)
		return;

#if 0
	root->to_model = matx4(root->orientation);

	vect3& row0 = vect3::castU(root->to_model.dekast()[0]);
	vect3& row1 = vect3::castU(root->to_model.dekast()[1]);
	vect3& row2 = vect3::castU(root->to_model.dekast()[2]);

	row0.mul(root->scale);
	row1.mul(root->scale);
	row2.mul(root->scale);

	root->to_model.setColS(3, root->position);
#else
	root->to_model = matx4(root->orientation);
	root->to_model.mul(matx4().translate(
		root->position[0],
		root->position[1],
		root->position[2]));
	root->to_model.mur(matx4().scale(
		root->scale[0],
		root->scale[1],
		root->scale[2]));
#endif

	root->matx_valid = true;
}


void
invalidateBoneMatx(
	const unsigned bone_count,
	Bone* bone,
	const unsigned bone_idx)
{
	assert(256 > bone_count);
	assert(bone);
	assert(bone_idx < bone_count);

	// unless already invalid, a bone is as valid as its parent
	if (!bone[bone_idx].matx_valid)
		return;

	const unsigned parent_idx = bone[bone_idx].parent_idx;

	if (255 != parent_idx)
	{
		assert(parent_idx < bone_idx);

		bone[bone_idx].matx_valid = bone[parent_idx].matx_valid;
	}
}


void
animateSkeleton(
	const unsigned count,
	matx4* bone_mat,
	Bone* bone,
	const std::vector< Track >& skeletal_animation,
	const float anim_time,
	Bone* root)
{
	assert(256 > count);
	assert(bone_mat);
	assert(bone);

	if (skeletal_animation.empty())
		return;

	bool updates = false;

	for (std::vector< Track >::const_iterator it = skeletal_animation.begin(); it != skeletal_animation.end(); ++it)
	{
		if (0 == root && 255 == it->bone_idx)
			continue;

		Bone* const bone_alias = bone;
		Bone& bone = 255 == it->bone_idx
			? *root
			: bone_alias[it->bone_idx];

		std::vector< Track::BonePositionKey >::const_iterator jt0 = it->position_key.begin() + it->position_last_key_idx;
		std::vector< Track::BoneOrientationKey >::const_iterator jt1 = it->orientation_key.begin() + it->orientation_last_key_idx;
		std::vector< Track::BoneScaleKey >::const_iterator jt2 = it->scale_key.begin() + it->scale_last_key_idx;

		for (std::vector< Track::BonePositionKey >::const_iterator kt = it->position_key.end();
			jt0 != it->position_key.end(); ++jt0)
		{
			if (jt0->time < anim_time)
			{
				kt = jt0;
				it->position_last_key_idx = jt0 - it->position_key.begin();

				continue;
			}

			if (jt0->time > anim_time)
			{
				if (kt == it->position_key.end())
				{
					jt0 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt0->time - kt->time);
				const float w0 = 1.f - w1;

				bone.position.wsum(kt->value, jt0->value, w0, w1);
			}
			else
				bone.position = jt0->value;

			break;
		}

		for (std::vector< Track::BoneOrientationKey >::const_iterator kt = it->orientation_key.end();
			jt1 != it->orientation_key.end(); ++jt1)
		{
			if (jt1->time < anim_time)
			{
				kt = jt1;
				it->orientation_last_key_idx = jt1 - it->orientation_key.begin();

				continue;
			}

			if (jt1->time > anim_time)
			{
				if (kt == it->orientation_key.end())
				{
					jt1 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt1->time - kt->time);
				const float w0 = 0.f > kt->value.dot(jt1->value) ? w1 - 1.f : 1.f - w1;

				bone.orientation.wsum(kt->value, jt1->value, w0, w1);
				bone.orientation.normalise();
			}
			else
				bone.orientation = jt1->value;

			break;
		}

		for (std::vector< Track::BoneScaleKey >::const_iterator kt = it->scale_key.end();
			jt2 != it->scale_key.end(); ++jt2)
		{
			if (jt2->time < anim_time)
			{
				kt = jt2;
				it->scale_last_key_idx = jt2 - it->scale_key.begin();

				continue;
			}

			if (jt2->time > anim_time)
			{
				if (kt == it->scale_key.end())
				{
					jt2 = kt;
					break;
				}

				const float w1 = (anim_time - kt->time) / (jt2->time - kt->time);
				const float w0 = 1.f - w1;

				bone.scale.wsum(kt->value, jt2->value, w0, w1);
			}
			else
				bone.scale = jt2->value;

			break;
		}

		if (jt0 != it->position_key.end() ||
			jt1 != it->orientation_key.end() ||
			jt2 != it->scale_key.end())
		{
			bone.matx_valid = false;
			updates = true;
		}
	}

	if (updates)
	{
		for (unsigned i = 0; i < count; ++i)
			invalidateBoneMatx(count, bone, i);

		for (unsigned i = 0; i < count; ++i)
			updateBoneMatx(count, bone_mat, bone, i);

		if (0 != root)
			updateRoot(root);
	}
}


void
resetSkeletonAnimProgress(
	const std::vector< Track >& skeletal_animation)
{
	for (std::vector< Track >::const_iterator it = skeletal_animation.begin(); it != skeletal_animation.end(); ++it)
	{
		it->position_last_key_idx = 0;
		it->orientation_last_key_idx = 0;
		it->scale_last_key_idx = 0;
	}
}


bool
loadSkeletonAnimationAge(
	const char* const filename,
	unsigned* count,
	matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations)
{
	assert(filename);
	assert(count);
	assert(bone_mat);
	assert(bone);

	testbed::scoped_ptr< FILE, testbed::scoped_functor > file(fopen(filename, "rb"));

	if (0 == file())
	{
		std::cerr << __FUNCTION__ << " failed at fopen " << std::endl;
		return false;
	}

	uint32_t magic;
	uint32_t version;

	if (1 != fread(&magic, sizeof(magic), 1, file()) ||
		1 != fread(&version, sizeof(version), 1, file()))
	{
		return false;
	}

	std::cout << "skeleton magic, version: 0x" << std::hex << std::setw(8) << std::setfill('0') <<
		magic << std::dec << ", " << version << std::endl;

	static const uint32_t sMagic = 0x6c656b53;
	static const uint32_t sVersion = 100;

	if (magic != sMagic || version != sVersion)
		return false;

	uint16_t n_bones;

	if (1 != fread(&n_bones, sizeof(n_bones), 1, file()))
		return false;

	if (*count < n_bones)
	{
		std::cerr << __FUNCTION__ << " failed to load a skeleton with too many bones" << std::endl;
		return false;
	}

	for (uint16_t i = 0; i < n_bones; ++i)
	{
		vect4 ori_swizz;

		if (1 != fread(&bone[i].position, sizeof(bone[0].position), 1, file()) ||
			1 != fread(&ori_swizz, sizeof(ori_swizz), 1, file()) ||
			1 != fread(&bone[i].scale, sizeof(bone[0].scale), 1, file()) ||
			1 != fread(&bone[i].parent_idx, sizeof(bone[0].parent_idx), 1, file()))
		{
			return false;
		}

#if 1
		bone[i].orientation.set(0, -1.f * ori_swizz[1]);
		bone[i].orientation.set(1, -1.f * ori_swizz[2]);
		bone[i].orientation.set(2, -1.f * ori_swizz[3]);
		bone[i].orientation.set(3, ori_swizz[0]);
#else
		bone[i].orientation.set(0, ori_swizz[1]);
		bone[i].orientation.set(1, ori_swizz[2]);
		bone[i].orientation.set(2, ori_swizz[3]);
		bone[i].orientation.set(3, ori_swizz[0]);
#endif

		uint16_t name_len;

		if (1 != fread(&name_len, sizeof(name_len), 1, file()))
			return false;

		char name[1024];

		if (1 != fread(name, sizeof(name[0]) * name_len, 1, file()))
			return false;

		name[name_len] = '\0';
		bone[i].name = name;

#if VERBOSE_READ
		std::cout <<
			"bone " << i << ": " << name <<
			"\n\tpos: " <<
			bone[i].position[0] << ", " <<
			bone[i].position[1] << ", " <<
			bone[i].position[2] <<
			"\n\tori: " <<
			bone[i].orientation[0] << ", " <<
			bone[i].orientation[1] << ", " <<
			bone[i].orientation[2] << ", " <<
			bone[i].orientation[3] <<
			"\n\tsca: " <<
			bone[i].scale[0] << ", " <<
			bone[i].scale[1] << ", " <<
			bone[i].scale[2] <<
			"\n\tpar: " <<
			unsigned(bone[i].parent_idx) << std::endl;
#endif
	}

	uint16_t n_anims;

	if (1 != fread(&n_anims, sizeof(n_anims), 1, file()))
		return false;

#if VERBOSE_READ
	std::cout << "animations: " << n_anims << std::endl;
#endif

	for (uint16_t i = 0; i < n_anims; ++i)
	{
		std::vector< Track >& skeletal_animation = *animations.insert(animations.end(), std::vector< Track >());

		uint16_t name_len;

		if (1 != fread(&name_len, sizeof(name_len), 1, file()))
			return false;

		char name[1024];

		if (1 != fread(name, sizeof(name[0]) * name_len, 1, file()))
			return false;

		name[name_len] = '\0';

		uint32_t dummy[2];

		if (1 != fread(dummy, sizeof(dummy), 1, file()))
			return false;

		uint16_t n_tracks;

		if (1 != fread(&n_tracks, sizeof(n_tracks), 1, file()))
			return false;

#if VERBOSE_READ
		std::cout << "animation, tracks: " << name << ", " << unsigned(n_tracks) << std::endl;
#endif

		for (uint16_t i = 0; i < n_tracks; ++i)
		{
			uint8_t bone_idx_and_stuff[2];

			if (1 != fread(bone_idx_and_stuff, sizeof(bone_idx_and_stuff), 1, file()))
				return false;

			assert(!bone_idx_and_stuff[1]);

#if VERBOSE_READ
			std::cout << "track, bone: " << unsigned(i) << ", " << unsigned(bone_idx_and_stuff[0]) << std::endl;
#endif

			Track& track = *skeletal_animation.insert(skeletal_animation.end(), Track());

			track.bone_idx = bone_idx_and_stuff[0];

			uint32_t n_pos_keys;

			if (1 != fread(&n_pos_keys, sizeof(n_pos_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\tposition keys: " << n_pos_keys << std::endl;
#endif

			for (uint32_t i = 0; i < n_pos_keys; ++i)
			{
				Track::BonePositionKey& pos = *track.position_key.insert(track.position_key.end(), Track::BonePositionKey());

				if (1 != fread(&pos.time, sizeof(pos.time), 1, file()) ||
					1 != fread(&pos.value, sizeof(pos.value), 1, file()))
				{
					return false;
				}
			}

			uint32_t n_ori_keys;

			if (1 != fread(&n_ori_keys, sizeof(n_ori_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\torientation keys: " << n_ori_keys << std::endl;
#endif

			for (uint32_t i = 0; i < n_ori_keys; ++i)
			{
				Track::BoneOrientationKey& ori = *track.orientation_key.insert(track.orientation_key.end(), Track::BoneOrientationKey());
				vect4 ori_swizz;

				if (1 != fread(&ori.time, sizeof(ori.time), 1, file()) ||
					1 != fread(&ori_swizz, sizeof(ori_swizz), 1, file()))
				{
					return false;
				}

#if 1
				ori.value.set(0, -1.f * ori_swizz[1]);
				ori.value.set(1, -1.f * ori_swizz[2]);
				ori.value.set(2, -1.f * ori_swizz[3]);
				ori.value.set(3, ori_swizz[0]);
#else
				ori.value.set(0, ori_swizz[1]);
				ori.value.set(1, ori_swizz[2]);
				ori.value.set(2, ori_swizz[3]);
				ori.value.set(3, ori_swizz[0]);
#endif
			}

			uint32_t n_sca_keys;

			if (1 != fread(&n_sca_keys, sizeof(n_sca_keys), 1, file()))
				return false;

#if VERBOSE_READ
			std::cout << "\tscale keys: " << n_sca_keys << std::endl;
#endif

			for (uint32_t i = 0; i < n_sca_keys; ++i)
			{
				Track::BoneScaleKey& sca = *track.scale_key.insert(track.scale_key.end(), Track::BoneScaleKey());

				if (1 != fread(&sca.time, sizeof(sca.time), 1, file()) ||
					1 != fread(&sca.value, sizeof(sca.value), 1, file()))
				{
					return false;
				}
			}

			track.position_last_key_idx = 0;
			track.orientation_last_key_idx = 0;
			track.scale_last_key_idx = 0;
		}
	}

	*count = n_bones;

	for (uint16_t i = 0; i < n_bones; ++i)
		initBoneMatx(*count, bone_mat, bone, unsigned(i));

	return true;
}

} // namespace rend
