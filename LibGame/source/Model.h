#pragma once

#include "../../LibMath/source/stdafx.h"

class CTexture;


class CModel
{
public:
	CModel() = default;
	~CModel() = default;

	void SetScale(const float& fScale);
	void SetPosition(const float& x, const float& y, const float& z);
	void SetPosition(const SVector3Df& v3Pos);
	void SetRotation(const float& x, const float& y, const float& z);
	void SetRotation(const SVector3Df& v3Rot);

	SVector3Df GetPosition() const;
	SVector3Df GetRotation() const;
	SVector3Df GetScale() const;
	CWorldTranslation& GetWorldTranslation();
	CMatrix4Df GetWorldMatrix();

protected:
	CWorldTranslation m_WorldTranslation;
};