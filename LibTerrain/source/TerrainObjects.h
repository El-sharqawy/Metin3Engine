#pragma once

#include "../../LibMath/source/vectors.h"
#include <glad/glad.h>

class CTerrainMap;

enum EObjectRotation
{
	ROTATION_STEP_COUNT = 24,
	ROTATION_STEP_AMOUNT = 360 / ROTATION_STEP_COUNT,

	YAW_STEP_COUNT = 24,
	YAW_STEP_AMOUNT = 360 / YAW_STEP_COUNT,

	PITCH_STEP_COUNT = 24,
	PITCH_STEP_AMOUNT = 360 / PITCH_STEP_COUNT,
};

typedef struct SObjectData
{
	SVector3Df v3Position;
	GLuint uiObjCRC;

	// Rotation
	GLfloat m_fYaw;
	GLfloat m_fPitch;
	GLFloat m_fRoll;

} TObjectData;

class CTerrainAreaObjects
{
public:
	CTerrainAreaObjects();
	~CTerrainAreaObjects();

};