#include "Stdafx.h"
#include "TerrainManager.h"
#include "TerrainMap.h"

CTerrainManager::CTerrainManager()
{
	Initialize();
}

CTerrainManager::~CTerrainManager()
{
	Destroy();
}

void CTerrainManager::Initialize()
{
	Clear();
}

void CTerrainManager::Destroy()
{
	if (m_pTerrainMap)
	{
		m_pTerrainMap->Clear();
		safe_delete(m_pTerrainMap);
	}
}

void CTerrainManager::Clear()
{
	if (m_pTerrainMap)
	{
		m_pTerrainMap->Clear();
	}

	ClearEditor();
}

void CTerrainManager::Create()
{
	assert(!m_pTerrainMap);
	if (m_pTerrainMap)
	{
		sys_err("CTerrainManager::Create: Map Already Exists!");
		Clear();
		return;
	}

	m_pTerrainMap = AllocMap();
	assert(m_pTerrainMap);
}

CTerrainMap* CTerrainManager::AllocMap()
{
	m_pTerrainMap = new CTerrainMap;
	return (m_pTerrainMap);
}

bool CTerrainManager::LoadMap(const std::string& stMapName)
{
	assert(m_pTerrainMap);

	m_pTerrainMap->Clear();
	m_pTerrainMap->SetMapReady(false);
	m_pTerrainMap->SetMapName(stMapName);

	if (!m_pTerrainMap->LoadMap(0.0f))
	{
		sys_err("CTerrainManager::LoadMap: Failed to Load The Map %s", stMapName.c_str());
		return (false);
	}

	m_pTerrainMap->SetMapReady(true);
	return (true);
}

bool CTerrainManager::LoadMap(const std::string& stMapName, const SVector3Df& v3PlayerPos)
{
	assert(m_pTerrainMap);

	m_pTerrainMap->Clear();
	m_pTerrainMap->SetMapReady(false);
	m_pTerrainMap->SetMapName(stMapName);

	if (!m_pTerrainMap->LoadMap(v3PlayerPos))
	{
		sys_err("CTerrainManager::LoadMap: Failed to Load The Map %s", stMapName.c_str());
		return (false);
	}

	m_pTerrainMap->SetMapReady(true);
	return (true);
}

CTerrainMap& CTerrainManager::GetMapRef()
{
	assert(m_pTerrainMap);
	return (*m_pTerrainMap);
}

bool CTerrainManager::IsMapReady() const
{
	if (!m_pTerrainMap)
	{
		return (false);
	}

	return (m_pTerrainMap->IsMapReady());
}

bool CTerrainManager::UpdateMap(const SVector3Df& v3Pos)
{
	if (!m_pTerrainMap)
	{
		sys_err("CTerrainManager::UpdateMap: Map Is not Initialized");
		return (false);
	}

	CTerrainMap& rMap = GetMapRef();
	rMap.UpdateMap(v3Pos);
	return (true);
}

