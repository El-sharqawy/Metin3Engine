#pragma once

#include <glad/glad.h>
#include "../../LibMath/source/stdafx.h"

#include "BoundingBox.h"
#include "MeshTexture.h"

class CMeshTexture2D;

#define ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices |    \
                           aiProcess_Triangulate |              \
                           aiProcess_GenSmoothNormals |         \
                           aiProcess_LimitBoneWeights |         \
                           aiProcess_SplitLargeMeshes |         \
                           aiProcess_ImproveCacheLocality |     \
                           aiProcess_RemoveRedundantMaterials | \
                           aiProcess_FindDegenerates |          \
                           aiProcess_FindInvalidData |          \
                           aiProcess_GenUVCoords |              \
                           aiProcess_CalcTangentSpace)

constexpr GLuint INVALID_MATERIAL = 0xFFFFFFFF;
constexpr GLuint POSITION_LOCATION = 0;
constexpr GLuint NORMALS_LOCATION = 1;
constexpr GLuint TEX_COORDS_LOCATION = 2;
constexpr GLuint WVP_LOCATION = 3;
constexpr GLuint WORLD_LOCATION = 7;


#pragma pack(push)
#pragma pack(1)
typedef struct SMeshEntry
{
	SMeshEntry()
	{
		uiBaseVertex = 0;
		uiBaseIndex = 0;
		uiNumIndices = 0;
		uiMaterialIndex = INVALID_MATERIAL;
	}

	GLuint uiBaseVertex;
	GLuint uiBaseIndex;
	GLuint uiNumIndices;
	GLuint uiMaterialIndex;
} TMeshEntry;

typedef struct SMeshVertex
{
	SVector3Df v3Pos;
	SVector3Df v3Normals;
	SVector2Df v2Texture;

	SMeshVertex() = default;

	SMeshVertex(const SVector3Df& vPos, const SVector3Df& vNormals, const SVector2Df& vTexture)
	{
		v3Pos = vPos;
		v3Normals = vNormals;
		v2Texture = vTexture;
	}
} TMeshVertex;

typedef struct SPBRMaterial
{
	float m_fRoughness;
	bool m_bIsMetal;
	SVector3Df m_v3Color;
	CMeshTexture2D* m_pAlbedo;
	CMeshTexture2D* m_pRoughness;
	CMeshTexture2D* m_pMetallic;
	CMeshTexture2D* m_pNormalMap;

	SPBRMaterial()
	{
		m_fRoughness = 0.0f;
		m_bIsMetal = false;
		m_v3Color = SVector3Df(0.0f, 0.0f, 0.0f);
		m_pAlbedo = nullptr;
		m_pRoughness = nullptr;
		m_pMetallic = nullptr;
		m_pNormalMap = nullptr;
	}

} TPBRMaterial;

typedef struct SMaterial
{
	std::string m_stName;
	TPBRMaterial m_sPBRMaterial;

	SVector4Df m_v4AmbientColor;
	SVector4Df m_v4DiffuseColor;
	SVector4Df m_v4SpecularColor;

	CMeshTexture2D* m_pDiffuseMap;
	CMeshTexture2D* m_pSpecularMap;

	float m_fTransparency;
	float m_fAlpha;

	SMaterial()
	{
		m_stName = "Material";
		m_sPBRMaterial = {};
		m_v4AmbientColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);
		m_v4DiffuseColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);
		m_v4SpecularColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);

		m_pDiffuseMap = nullptr;
		m_pSpecularMap = nullptr;

		m_fTransparency = 1.0f;
		m_fAlpha = 0.0f;
	}

	~SMaterial()
	{
		if (m_pDiffuseMap)
		{
			safe_delete(m_pDiffuseMap);
		}

		if (m_pSpecularMap)
		{
			safe_delete(m_pSpecularMap);
		}
	}

} TMaterial;
#pragma pack(pop)

