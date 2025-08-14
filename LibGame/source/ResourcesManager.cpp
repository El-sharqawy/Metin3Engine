#include "Stdafx.h"
#include "ResourcesManager.h"
#include "../../LibGL/source/Shader.h"
#include <nlohmann/json.hpp> // Requires JSON for Modern C++ library

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
#if defined(ENABLE_SHADER_LOGS)
	sys_log("CResourcesManager::GetShader: Compiling new shader '%s'", stShaderName.c_str());
#endif

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