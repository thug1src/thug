// asset types for the asset manager

#ifndef __GEL_ASSETTYPES_H
#define __GEL_ASSETTYPES_H

namespace Ass
{

enum	EAssetType {
	ASSET_UNKNOWN,							// unknown, needs to be determined by inspection
	ASSET_BINARY,							// binary file
	ASSET_SCENE,							// world_geometry
	ASSET_TEXTURES,							// Texture file
	ASSET_COLLISION,						// collision file
	ASSET_ANIM,								// animation
    ASSET_SKELETON,                         // skeleton
	ASSET_SKIN,								// skin
	ASSET_CUTSCENE,							// cutscene
	ASSET_NODEARRAY,						// model node array
};

} // namespace Ass

#endif
