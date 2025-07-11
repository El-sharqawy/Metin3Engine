#pragma once

#include <glad/glad.h>
#include "../../LibGL/source/texture.h"
#include "../../LibMath/source/stdafx.h"
#include <nlohmann/json.hpp> // Requires JSON for Modern C++ library

using json = nlohmann::json;

enum ETerrainMains
{
	TERRAIN_SIZE = 256,
	TERRAIN_PATCHSIZE = 32,
	TERRAIN_PATCHCOUNT = TERRAIN_SIZE / TERRAIN_PATCHSIZE,
};

enum ETerrainData
{
	// Core terrain grid dimensions (in cells)
	XSIZE = TERRAIN_SIZE,									// Number of cells along X-axis (e.g., 128 cells)
	ZSIZE = TERRAIN_SIZE,									// Number of cells along Z-axis (matches X for square terrain)

	// Heightmap data (vertex-based)
	HEIGHTMAP_XSIZE = XSIZE + 1,							// Heightmap width: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_ZSIZE = ZSIZE + 1,							// Heightmap depth: cells + 1 (vertices per row) and This ensures the heightmap covers all vertex points needed to define the terrain’s surface.
	HEIGHTMAP_RAW_XSIZE = XSIZE + 3,						// Heightmap raw width: cells + 3 (vertices per row) (with padding for edge sampling)
	HEIGHTMAP_RAW_ZSIZE = ZSIZE + 3,						// Heightmap raw depth: cells + 3 (vertices per row) (prevents out-of-bounds access).

	// Tile maps (texture splatting)
	TILEMAP_XSIZE = XSIZE * 2,								// Tile map width (2 tiles per cell for texture variation)
	TILEMAP_ZSIZE = ZSIZE * 2,								// Tile map depth (e.g., 256x256 for 128-cell terrain)
	TILEMAP_RAW_XSIZE = XSIZE * 2 + 2,						// Raw tile map width (with padding for filtering)
	TILEMAP_RAW_ZSIZE = ZSIZE * 2 + 2,						// Raw tile map depth (avoids edge artifacts)

	// Texture splatting (blending)
	SPLATALPHA_RAW_XSIZE = XSIZE * 2 + 2,					// Splat alpha map width (with padding for blending)
	SPLATALPHA_RAW_ZSIZE = ZSIZE * 2 + 2,					// Splat alpha map depth (prevents edge artifacts)
	SPLATALPHA_COUNT = 7,									// Number of blendable texture layers

	// Tile-to-heightmap ratios
	HEIGHT_TILE_XRATIO = TILEMAP_XSIZE / XSIZE,				// Tiles per cell width (2:1)
	HEIGHT_TILE_ZRATIO = TILEMAP_ZSIZE / ZSIZE,				// Tiles per cell depth (2:1)

	// Patch system (LOD/culling units)
	PATCH_XSIZE = TERRAIN_PATCHSIZE,						// Patch width in cells(e.g., 16 cells)
	PATCH_ZSIZE = TERRAIN_PATCHSIZE,						// Patch depth in cells (square patches)

	PATCH_XCOUNT = TERRAIN_PATCHCOUNT,						// Number of patches along X-axis
	PATCH_ZCOUNT = TERRAIN_PATCHCOUNT,						// Number of patches along Z-axis

	// Spatial scaling
	CELL_SCALE = 200,										// Cell size in centimeters (e.g., 200cm = 2m)
	CELL_SCALE_METER = CELL_SCALE / 100,					// Cell size in meters (e.g 200cm = 2m)
	HALF_CELL_SCALE = CELL_SCALE / 2,						// Half-cell offset (for centering objects)
	HALF_CELL_SCALE_METER = HALF_CELL_SCALE / 100,			// Half-cell offset in meters (1m)

	// Terrain Actual size
	TERRAIN_XSIZE = XSIZE * CELL_SCALE_METER,				// Total terrain width in meters
	TERRAIN_ZSIZE = ZSIZE * CELL_SCALE_METER,				// Total terrain depth in meters
};

typedef struct STerrainTexture
{
	std::string m_stFileName;
	GLuint m_uiTextureID;
	CTexture* m_pTexture;
	GLfloat m_fUScale;
	GLfloat m_fVScale;
	GLfloat m_fUOffset;
	GLfloat m_fVOffset;
	bool m_bIsSplat;
	GLuint m_uiHeightMin;
	GLuint m_uiHeightMax;
	CMatrix4Df m_matTransform;

	STerrainTexture()
	{
		m_stFileName.clear();
		m_uiTextureID = 0;
		m_pTexture = nullptr;
		m_fUScale = 4.0f;
		m_fVScale = 4.0f;
		m_fUOffset = 0.0f;
		m_fVOffset = 0.0f;
		m_bIsSplat = true;
		m_uiHeightMin = 0;
		m_uiHeightMax = 65535; // Check Later
		m_matTransform.InitIdentity();
	}

	json ToJson() const
	{
		return {
			{"filename", m_stFileName},
			{"u_scale", m_fUScale},
			{"v_scale", m_fVScale},
			{"u_offset", m_fUOffset},
			{"v_offset", m_fVOffset},
			{"is_splat", m_bIsSplat},
			{"begin", m_uiHeightMin},
			{"end", m_uiHeightMax},
		};
	}

} TTerrainTexture;

#pragma pack(push)
#pragma pack(1)

typedef struct STerrainVertex
{
	SVector3Df m_v3Position;
	SVector2Df m_v2TexCoords;
	SVector3Df m_v3Normals;
} TTerrainVertex;

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
	BRUSH_SHAPE_MAX = 2,
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
