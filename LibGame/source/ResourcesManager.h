#pragma once

#include <map>
#include <memory>
#include <string>

#include "../../LibGL/source/Singleton.h"

class CShader;
class CMesh;

typedef struct SShaderProgramDefinitions
{
    std::string sName;
    std::string sVertexPath;
    std::string sFragmentPath;
    std::string sGeometryPath;
    std::string sTessControlPath;
    std::string sTessEvalPath;
} TShaderProgramDefinitions;

class CResourcesManager : public CSingleton<CResourcesManager>
{
public:
    CResourcesManager() = default;
    ~CResourcesManager() = default;

    // Prevents copying and assignment
    CResourcesManager(const CResourcesManager&) = delete;
    CResourcesManager& operator=(const CResourcesManager&) = delete;

    // Gets a pointer to a mesh.
    // It loads the mesh from the file path if it's not already in memory.
    CMesh* GetMesh(const std::string& stMeshPath);

    // Gets a pointer to a shader.
    // If "model_shader" is requested, this class knows how to find and load
    // "shaders/model_shader.vert" and "shaders/model_shader.frag".
    // It stores the loaded shader in a map so it's only loaded once.
    CShader* GetShader(const std::string& stShaderName);

    // New function to load the shader manifest file
    bool LoadShaderDefinitions(const std::string& sDefinitionFilePath);

private:
    // This map stores the definitions loaded from shaders.json
    std::map<std::string, SShaderProgramDefinitions> m_mShaderDefinitions;

    std::map<std::string, std::unique_ptr<CShader>> m_mShaders;
    std::map<std::string, std::unique_ptr<CMesh>> m_mMeshes;
};