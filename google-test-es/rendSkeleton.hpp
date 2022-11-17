#ifndef	rend_skeleton_H__
#define	rend_skeleton_H__

#include "rendVect.hpp"

#include <stdint.h>
#include <string>
#include <vector>

namespace rend
{

struct Bone
{
	vect3			position;			// start position
	quat			orientation;		// start orientation
	vect3			scale;				// start scale
	uint8_t			parent_idx;			// 255 - no parent
	std::string		name;

	matx4			to_local;
	matx4			to_model;

	bool			matx_valid;

	Bone()
	: position(0.f, 0.f, 0.f)
	, orientation(0.f, 0.f, 0.f, 1.f)
	, scale(1.f, 1.f, 1.f)
	, parent_idx(255)
	, matx_valid(false)
	{
		to_local.identity();
		to_model.identity();
	}
};


struct Track
{
	uint8_t			bone_idx;

	struct BonePositionKey
	{
		float		time;
		vect3		value;
	};

	struct BoneOrientationKey
	{
		float		time;
		quat		value;
	};

	struct BoneScaleKey
	{
		float		time;
		vect3		value;
	};

	std::vector< BonePositionKey >		position_key;
	std::vector< BoneOrientationKey >	orientation_key;
	std::vector< BoneScaleKey >			scale_key;

	mutable unsigned position_last_key_idx;
	mutable unsigned orientation_last_key_idx;
	mutable unsigned scale_last_key_idx;
};


void
initBoneMatx(
	const unsigned bone_count,
	matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx);


void
updateBoneMatx(
	const unsigned bone_count,
	matx4* bone_mat,
	Bone* bone,
	const unsigned bone_idx);


void
updateRoot(
	Bone* root);


void
invalidateBoneMatx(
	const unsigned bone_count,
	Bone* bone,
	const unsigned bone_idx);


void
animateSkeleton(
	const unsigned bone_count,
	matx4* bone_mat,
	Bone* bone,
	const std::vector< Track >& skeletal_animation,
	const float anim_time,
	Bone* root = 0);


void
resetSkeletonAnimProgress(
	const std::vector< Track >& skeletal_animation);


bool
loadSkeletonAnimationAge(
	const char* const filename,
	unsigned* count,
	matx4* bone_mat,
	Bone* bone,
	std::vector< std::vector< Track > >& animations);

} // namespace rend

#endif // rend_skeleton_H__
