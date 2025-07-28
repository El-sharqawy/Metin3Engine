#pragma once

#include <glad/glad.h>
#include "../../LibGL/source/texture.h"
#include "../../LibMath/source/stdafx.h"
#include <nlohmann/json.hpp> // Requires JSON for Modern C++ library

using json = nlohmann::json;

enum ETerrainMains
{
	TERRAIN_SIZE = 128,
	TERRAIN_PATCHSIZE = 16,
	TERRAIN_PATCHCOUNT = TERRAIN_SIZE / TERRAIN_PATCHSIZE,
};

enum ETerrainData
{
	// Core terrain grid dimensions (in cells)
	XSIZE = TERRAIN_SIZE,											// Number of cells along X-axis (e.g., 128 cells)
	ZSIZE = TERRAIN_SIZE,											// Number of cells along Z-axis (matches X for square terrain)

	// Heightmap data (vertex-based)
	HEIGHTMAP_XSIZE = XSIZE + 1,									// Heightmap width: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_ZSIZE = ZSIZE + 1,									// Heightmap depth: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_RAW_XSIZE = XSIZE + 3,								// Heightmap raw width: cells + 3 (vertices per row) (with padding for edge sampling)
	HEIGHTMAP_RAW_ZSIZE = ZSIZE + 3,								// Heightmap raw depth: cells + 3 (vertices per row) (prevents out-of-bounds access).

	// Patch system (LOD/culling units)
	PATCH_XSIZE = TERRAIN_PATCHSIZE,								// Patch width in cells(e.g., 16 cells)
	PATCH_ZSIZE = TERRAIN_PATCHSIZE,								// Patch depth in cells (square patches)

	PATCH_XCOUNT = TERRAIN_PATCHCOUNT,								// Number of patches along X-axis
	PATCH_ZCOUNT = TERRAIN_PATCHCOUNT,								// Number of patches along Z-axis

	// Spatial scaling
	CELL_SCALE = 200,												// Cell size in centimeters (e.g., 200cm = 2m)
	CELL_SCALE_METER = CELL_SCALE / 100,							// Cell size in meters (e.g 200cm = 2m)
	HALF_CELL_SCALE = CELL_SCALE / 2,								// Half-cell offset (for centering objects)
	HALF_CELL_SCALE_METER = HALF_CELL_SCALE / 100,					// Half-cell offset in meters (1m)

	// Terrain Actual size
	TERRAIN_XSIZE = XSIZE * CELL_SCALE_METER,						// Total terrain width in meters
	TERRAIN_ZSIZE = ZSIZE * CELL_SCALE_METER,						// Total terrain depth in meters

	// Tile maps (texture splatting)
	TILEMAP_XSIZE = XSIZE * 2,								// Tile map width (2 tiles per cell for texture variation)
	TILEMAP_ZSIZE = ZSIZE * 2,								// Tile map depth (e.g., 256x256 for 128-cell terrain)
	TILEMAP_RAW_XSIZE = (XSIZE) * 2,						// Raw tile map width (with padding for filtering)
	TILEMAP_RAW_ZSIZE = (ZSIZE) * 2,						// Raw tile map depth (avoids edge artifacts)

	// Tile-to-heightmap ratios
	HEIGHT_TILE_XRATIO = TILEMAP_XSIZE / XSIZE,						// Tiles per cell width (2:1)
	HEIGHT_TILE_ZRATIO = TILEMAP_ZSIZE / ZSIZE,						// Tiles per cell depth (2:1)

	TILEMAP_BLEND_COUNT = 4,										// Number of blendable texture layers

	// Attribute maps (e.g., collision, material types)
	ATTRMAP_XSIZE = TILEMAP_XSIZE,									// Attribute map width (2x resolution for per-corner data)
	ATTRMAP_ZSIZE = TILEMAP_ZSIZE,									// Attribute map depth (stores data at cell corners)
	ATTRMAP_COUNT = 8,												// Number of attribute layers (e.g., 8 texture/material types)

	// Water Mapping
	MAX_WATER_NUM = 255,											// Maximum number of water bodies per terrain patch
	WATERMAP_XSIZE = XSIZE,											// Water patch width in cells (e.g., 256 cells)
	WATERMAP_ZSIZE = ZSIZE,											// Water patch depth in cells (e.g., 256 cells)

	HEIGHT_WATER_XRATIO = WATERMAP_XSIZE / XSIZE,					// Tiles per cell width (2:1)
	HEIGHT_WATER_ZRATIO = WATERMAP_ZSIZE / ZSIZE,					// Tiles per cell depth (2:1)
};

typedef struct STerrainTexture
{
	std::string m_stFileName;
	GLuint m_uiTextureID;
	CTexture* m_pTexture;

	STerrainTexture()
	{
		m_stFileName.clear();
		m_uiTextureID = 0;
		m_pTexture = nullptr;
	}

	json ToJson() const
	{
		return {
			{"filename", m_stFileName},
		};
	}

} TTerrainTexture;

#pragma pack(push)
#pragma pack(1)
typedef struct STerrainVertex
{
	SVector3Df m_v3Position;	// World position
	SVector2Df m_v2TexCoords;	// UVs (For Texturing)
	SVector3Df m_v3Normals;		// Normal
} TTerrainVertex;

typedef struct STerrainWaterVertex
{
	SVector3Df m_v3Position;	// World position
	SVector2Df m_v2TexCoords;	// UVs (for reflection/refraction maps or animated foam)
	SVector3Df m_v3Normals;		// Normal
	GLuint m_uiColor;			// RGBA tint/alpha
} TTerrainWaterVertex;
#pragma pack(pop)

enum ETerrainPatchData
{
	PATCH_TYPE_PLAIN,
	PATCH_TYPE_HILL,
	PATCH_TYPE_CLIFF,

	PATCH_VERTEX_COUNT = (PATCH_XSIZE + 1) * (PATCH_ZSIZE + 1),
};

enum ETerrainBrushShape
{
	BRUSH_SHAPE_NONE = 0,
	BRUSH_SHAPE_CIRCLE = 1 << 0,
	BRUSH_SHAPE_SQUARE = 1 << 1,
	BRUSH_SHAPE_MAX = 3,
};

enum ETerrainBrushType
{
	BRUSH_TYPE_NONE = 0,
	BRUSH_TYPE_UP = 1 << 0,
	BRUSH_TYPE_DOWN = 1 << 1,
	BRUSH_TYPE_FLATTEN = 1 << 2,
	BRUSH_TYPE_NOISE = 1 << 3,
	BRUSH_TYPE_SMOOTH = 1 << 4,
	BRUSH_TYPE_MAX = 5,
};

enum ETerrainAttributesTypes
{
	// Terrain attributes, used for collision, material types, etc.
	TERRAIN_ATTRIBUTE_NONE = 0,

	// Block, Where players can't walk
	TERRAIN_ATTRIBUTE_BLOCK = (1 << 0),

	// Safe Area, Where No damage can be taken or enemies spawn/enter
	TERRAIN_ATTRIBUTE_SAFE = (1 << 1),

	// Water, Where players can swim
	TERRAIN_ATTRIBUTE_WATER = (1 << 2),

	TERRAIN_ATTRIBUTE_MAX,
};

enum EObjectRotations
{
	// X Vertical direction Side-To-Side (Up/Down) Tilts the object’s nose up or down, Axis X rotation steps, like Facing +Z, Apply +30° pitch -> now looking upward.
	PITCH_STEP_COUNT = 24,
	PITCH_STEP_AMOUNT = 360 / PITCH_STEP_COUNT,

	// Y Horizontal direction (Left/Right) Turns the object to look left or right. Axis T rotation steps, like spinning on chair around yourself, if you're facing North and applied +90 you will face East
	YAW_STEP_COUNT = 24,
	YAW_STEP_AMOUNT = 360 / YAW_STEP_COUNT,

	// Z Orientation, not aim (Forward/Backwar) Tilts the object sideways like a plane turning or tilting its wings. Axis Z rotation steps, Facing +Z, upright, Apply +45° roll -> everything feels rotated like leaning sideways.
	ROLL_STEP_COUNT = 24,
	ROLL_STEP_AMOUNT = 360 / ROLL_STEP_COUNT,
};

enum EObjectTypes : GLubyte
{
	OBJECT_TYPE_NONE,		// No object type, used for uninitialized or invalid objects
	OBJECT_TYPE_STATIC,		// Static object, does not move or change
	OBJECT_TYPE_DYNAMIC,	// Dynamic object, can move or change
	OBJECT_TYPE_KINEMATIC,	// Kinematic object, controlled by physics but not affected by forces
	OBJECT_TYPE_PLAYER,		// Player controlled object
	OBJECT_TYPE_NPC,		// Non-player character object
	OBJECT_TYPE_VEHICLE,	// Vehicle object, can be driven or controlled
	OBJECT_TYPE_BUILDING,	// Building object, static structure
	OBJECT_TYPE_ITEM,		// Item object, can be picked up or interacted with
	OBJECT_TYPE_EFFECT,		// Visual or sound effect object
	OBJECT_TYPE_PROJECTILE, // Projectile object, like bullets or missiles
	OBJECT_TYPE_COUNT,		// Total count of object types
};
