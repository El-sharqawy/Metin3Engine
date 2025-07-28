#include "Stdafx.h"
#include "Model.h"
#include "../../LibGL/source/texture.h"

void CModel::SetScale(const float& fScale)
{
	m_WorldTranslation.SetScale(fScale);
}

void CModel::SetPosition(const float& x, const float& y, const float& z)
{
	m_WorldTranslation.SetPosition(x, y, z);
}

void CModel::SetPosition(const SVector3Df& v3Pos)
{
	m_WorldTranslation.SetPosition(v3Pos);
}

void CModel::SetRotation(const float& x, const float& y, const float& z)
{
	m_WorldTranslation.SetRotation(x, y, z);
}

void CModel::SetRotation(const SVector3Df& v3Rot)
{
	m_WorldTranslation.SetRotation(v3Rot);
}

SVector3Df CModel::GetPosition() const
{
	return m_WorldTranslation.GetPosition();
}

SVector3Df CModel::GetRotation() const
{
	return m_WorldTranslation.GetRotation();
}

SVector3Df CModel::GetScale() const
{
	return m_WorldTranslation.GetScale();
}

CWorldTranslation& CModel::GetWorldTranslation()
{
	return m_WorldTranslation;
}

CMatrix4Df CModel::GetWorldMatrix()
{
	return m_WorldTranslation.GetMatrix();
}