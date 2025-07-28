#include "Stdafx.h"
#include "ResourcesManager.h"
#include "Mesh.h"
#include "../../LibGL/source/Shader.h"
#include <nlohmann/json.hpp> // Requires JSON for Modern C++ library


CMesh* CResourcesManager::GetMesh(const std::string& stMeshPath)
{
	// 1. Check if the mesh is already loaded by searching for its path in the map.
	std::map<std::string, std::unique_ptr<CMesh>>::iterator it = m_mMeshes.find(stMeshPath);
	if (it != m_mMeshes.end())
	{
		// Found it! Return the existing pointer.
		return it->second.get();
	}

	// 2. If not found, we need to load it from disk.
	sys_log("CResourcesManager::GetMesh: Loading new mesh from %s", stMeshPath.c_str());

	// Create a new CMesh object managed by a unique_ptr.
	std::unique_ptr<CMesh> newMesh = std::make_unique<CMesh>();

	// Assume your CMesh class has a Load function that returns true on success.
	if (newMesh->LoadMesh(stMeshPath))
	{
		// 3. If loading was successful, get the raw pointer to return.
		CMesh* pMesh = newMesh.get();

		// 4. Move the unique_ptr into the map. The map now owns the mesh.
		m_mMeshes[stMeshPath] = std::move(newMesh);

		return (pMesh);
	}
	else
	{
		// Loading failed.
		sys_err("CResourcesManager::GetMesh: Failed to load mesh from %s", stMeshPath.c_str());
		return (nullptr);
	}

	return (nullptr);
}

CShader* CResourcesManager::GetShader(const std::string& stShaderName)
{
	// 1. Check if shader is already compiled and cached
	if (m_mShaders.count(stShaderName))
	{
		return m_mShaders[stShaderName].get();
	}

	// 2. Find the definition for the requested shader
	if (!m_mShaderDefinitions.count(stShaderName))
	{
		sys_err("CResourcesManager::GetShader: No shader definition found for '%s'", stShaderName.c_str());
		return nullptr;
	}

	const SShaderProgramDefinitions& def = m_mShaderDefinitions.at(stShaderName);

	// 3. Load the shader using the paths from the definition
	sys_log("CResourcesManager::GetShader: Compiling new shader '%s'", stShaderName.c_str());
	auto newShader = std::make_unique<CShader>(stShaderName);

	// Assume your CShader::Load function is updated to take a definition struct
	if (newShader->LoadFromDefinition(def))
	{
		CShader* pShader = newShader.get();
		m_mShaders[stShaderName] = std::move(newShader);
		return pShader;
	}

	return (nullptr);
}

bool CResourcesManager::LoadShaderDefinitions(const std::string& sDefinitionFilePath)
{
	try
	{
		std::ifstream file(sDefinitionFilePath);
		nlohmann::json root;

		file >> root;
		for (const auto& shaderJson : root["shaders"])
		{
			SShaderProgramDefinitions definition;
			definition.sName = shaderJson["name"].get<std::string>();

			const auto& stages = shaderJson["stages"];

			if (stages.contains("vertex"))
			{
				definition.sVertexPath = stages["vertex"].get<std::string>();
			}
			if (stages.contains("fragment"))
			{
				definition.sFragmentPath = stages["fragment"].get<std::string>();
			}
			if (stages.contains("geometry"))
			{
				definition.sGeometryPath = stages["geometry"].get<std::string>();
			}
			if (stages.contains("tess_control"))
			{
				definition.sTessControlPath = stages["tess_control"].get<std::string>();
			}
			if (stages.contains("tess_eval"))
			{
				definition.sTessEvalPath = stages["tess_eval"].get<std::string>();
			}

			m_mShaderDefinitions[definition.sName] = definition;
		}
	}
	catch (const std::exception& err)
	{
		sys_err("CResourcesManager::LoadShaderDefinitions: Failed to load shader definitions from %s. Error: %s", sDefinitionFilePath.c_str(), err.what());
		return false;
	}

	sys_log("CResourcesManager::LoadShaderDefinitions: Successfully loaded %zu shader definitions.", m_mShaderDefinitions.size());
	return true;
}