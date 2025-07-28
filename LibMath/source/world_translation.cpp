#include "stdafx.h"

CWorldTranslation::CWorldTranslation()
{
	m_v3Scale = SVector3Df(1.0f, 1.0f, 1.0f);
	m_vPosition = SVector3Df(0.0f, 0.0f, 0.0f);
	m_vRotation = SVector3Df(0.0f, 0.0f, 0.0f);
}

/**
 * CWorldTranslation::SetScale - Sets the scale factors of the object
 * @fScale: A float value representing the scale along X, Y, and Z axes
 *
 * This function sets how much the object is scaled in each axis.
 * A scale of (1,1,1) means no scaling. Values less than 1 shrink the object;
 * values greater than 1 enlarge it.
 *
 * Use this when you need to make an object appear bigger or smaller,
 * such as enlarging a model or shrinking a UI element.
 */
void CWorldTranslation::SetScale(const GLfloat fScale)
{
	m_v3Scale = SVector3Df(fScale, fScale, fScale);
}

/**
 * CWorldTranslation::SetScale - Sets the scale factors of the object
 * @fScaleX: A float representing the scale along X Axiss
 * @fScaleY: A float representing the scale along Y Axiss
 * @fScaleZ: A float representing the scale along Z Axiss
 *
 * This function sets how much the object is scaled in each axis.
 * A scale of (1,1,1) means no scaling. Values less than 1 shrink the object;
 * values greater than 1 enlarge it.
 *
 * Use this when you need to make an object appear bigger or smaller,
 * such as enlarging a model or shrinking a UI element.
 */
void CWorldTranslation::SetScale(const GLfloat fScaleX, const GLfloat fScaleY, const GLfloat fScaleZ)
{
	m_v3Scale = SVector3Df(fScaleX, fScaleZ, fScaleZ);
}

/**
 * CWorldTranslation::SetScale - Sets the scale factors of the object
 * @v3Scale: A 3D vector representing the scale along X, Y, and Z axes
 *
 * This function sets how much the object is scaled in each axis.
 * A scale of (1,1,1) means no scaling. Values less than 1 shrink the object;
 * values greater than 1 enlarge it.
 *
 * Use this when you need to make an object appear bigger or smaller,
 * such as enlarging a model or shrinking a UI element.
 */
void CWorldTranslation::SetScale(const SVector3Df& v3Scale)
{
	m_v3Scale = v3Scale;
}

/**
 * CWorldTranslation::SetPosition - Sets the world position of the object
 * @fPosX: A float value representing the new world position on X Axis
 * @fPosY: A float value representing the new world position on Y Axis
 * @fPosZ: A float value representing the new world position on Z Axis
 *
 * This function assigns a new position to the object in world space.
 * It modifies the internal m_vPosition member to reflect this new position.
 *
 * Use this function when you want to move an object to a new location
 * in the 3D world without affecting its rotation or scale.
 */
void CWorldTranslation::SetPosition(const GLfloat fPosX, const GLfloat fPosY, const GLfloat fPosZ)
{
	m_vPosition.x = fPosX;
	m_vPosition.y = fPosY;
	m_vPosition.z = fPosZ;
}

/**
 * CWorldTranslation::SetPosition - Sets the world position of the object
 * @v3Pos: A 3D vector representing the new world position (x, y, z)
 *
 * This function assigns a new position to the object in world space.
 * It modifies the internal m_vPosition member to reflect this new position.
 *
 * Use this function when you want to move an object to a new location
 * in the 3D world without affecting its rotation or scale.
 */
void CWorldTranslation::SetPosition(const SVector3Df& v3Pos)
{
	m_vPosition = v3Pos;
}

/**
 * CWorldTranslation::SetRotation - Sets the world rotation of the object
 * @fRotX: A float value representing rotation angles in degrees on X Axis
 * @fRotY: A float value representing rotation angles in degrees on Y Axis
 * @fRotZ: A float value representing rotation angles in degrees on Z Axis
 *
 * This function sets the rotation of the object in world space.
 * The rotation is usually interpreted in Euler angles (ZYX order).
 *
 * Use this when you need to change the object's orientation,
 * such as rotating a character, prop, or camera.
 */
void CWorldTranslation::SetRotation(const GLfloat fRotX, const GLfloat fRotY, const GLfloat fRotZ)
{
	m_vRotation.x = fRotX;
	m_vRotation.y = fRotY;
	m_vRotation.z = fRotZ;
}

/**
 * CWorldTranslation::SetRotation - Sets the world rotation of the object
 * @v3Rotation: A 3D vector representing rotation angles in degrees (pitch, yaw, roll)
 *
 * This function sets the rotation of the object in world space.
 * The rotation is usually interpreted in Euler angles (ZYX order).
 *
 * Use this when you need to change the object's orientation,
 * such as rotating a character, prop, or camera.
 */
void CWorldTranslation::SetRotation(const SVector3Df& v3Rot)
{
	m_vRotation = v3Rot;
}

/**
 * CWorldTranslation::Rotate - Rotates the object by specified angles
 * @fRotX: Rotation to apply around the X-axis (in degrees)
 * @fRotY: Rotation to apply around the Y-axis (in degrees)
 * @fRotZ: Rotation to apply around the Z-axis (in degrees)
 *
 * This function adds the given Euler rotation values to the current rotation
 * of the object. It is useful for incremental rotations, such as turning
 * an object based on input or animation.
 */
void CWorldTranslation::Rotate(const GLfloat fRotX, const GLfloat fRotY, const GLfloat fRotZ)
{
	m_vRotation.x += fRotX;
	m_vRotation.y += fRotY;
	m_vRotation.z += fRotZ;
}

/**
 * CWorldTranslation::Rotate - Rotates the object by a vector of angles
 * @v3Rot: A 3D vector representing the rotation deltas (x, y, z in degrees)
 *
 * This overload applies a single vector of rotations to the current object.
 * It is useful for applying rotation deltas as a single vector operation.
 */
void CWorldTranslation::Rotate(const SVector3Df& v3Rot)
{
	m_vRotation += v3Rot;
}

/**
 * CWorldTranslation::GetScale - Gets the current scale of the object
 *
 * Return: A constant reference to the internal scale vector
 *
 * This function provides read-only access to the object's scale.
 * Use it when you need to query or debug an object's size in the world.
 */
const SVector3Df& CWorldTranslation::GetScale() const
{
	return m_v3Scale;
}

/**
 * CWorldTranslation::GetPosition - Gets the current world position
 *
 * Return: A constant reference to the internal position vector
 *
 * This function returns the object's position in world coordinates.
 * It can be used for placement logic, culling, physics, or debugging.
 */
const SVector3Df& CWorldTranslation::GetPosition() const
{
	return (m_vPosition);
}

/**
 * CWorldTranslation::GetRotation - Gets the current rotation of the object
 *
 * Return: A constant reference to the internal rotation vector (Euler angles)
 *
 * Use this function when you need to check or export an object's rotation.
 * The returned rotation is typically interpreted in ZYX Euler order.
 */
const SVector3Df& CWorldTranslation::GetRotation() const
{
	return (m_vRotation);
}

/**
 * GetMatrix - Computes the complete world transformation matrix
 *             from the position, rotation, and scale of the object.
 *
 * This function should be used when you want to transform local
 * object-space vertices to world-space (e.g. in rendering or physics).
 *
 * Return: A 4x4 matrix combining translation, rotation, and scaling
 *         in the order: Translation * Rotation * Scale.
 */
CMatrix4Df CWorldTranslation::GetMatrix() const
{
	CMatrix4Df matScale{}, matRotation{}, matTranslation{};

	matScale.InitScaleTransform(m_v3Scale.x, m_v3Scale.y, m_v3Scale.z);

	matRotation.InitRotateTransformZYX(m_vRotation.x, m_vRotation.y, m_vRotation.z);

	matTranslation.InitTranslationTransform(m_vPosition.x, m_vPosition.y, m_vPosition.z);

	CMatrix4Df matWorldTransformation = matTranslation * matRotation * matScale;

	return (matWorldTransformation);
}

/**
 * GetReversedTranslationMatrix - Builds a matrix that moves world
 *                                space into the object's local origin.
 *
 * This is useful for inverse transformations, such as converting
 * a world position into a position relative to the object.
 *
 * Return: A 4x4 matrix representing inverse of the translation.
 */
CMatrix4Df CWorldTranslation::GetReversedTranslationMatrix() const
{
	CMatrix4Df ReversedTranslationMatrix{};
	ReversedTranslationMatrix.InitTranslationTransform(GetPosition().negate());
	return (ReversedTranslationMatrix);
}

/**
 * GetReversedRotationMatrix - Builds a matrix that undoes the object's
 *                              rotation, converting world rotation
 *                              back to local space.
 *
 * Used for transforming world-space directions or positions into
 * the object's local orientation frame.
 *
 * Return: A 4x4 matrix representing the inverse of the rotation.
 */
CMatrix4Df CWorldTranslation::GetReversedRotationMatrix() const
{
	CMatrix4Df ReversedRotationMatrix{};
	ReversedRotationMatrix.InitRotateTransformZYX(-m_vRotation.x, -m_vRotation.y, -m_vRotation.z);
	return (ReversedRotationMatrix);
}

/**
 * WorldPosToLocalPos - Converts a position in world space to a position
 *                      relative to the object's local space.
 *
 * This is used when you want to determine where a global point is
 * with respect to a specific object’s local origin and orientation.
 *
 * @v3WorldPos: A 3D position in world space.
 *
 * Return: The position in local object space.
 */
SVector3Df CWorldTranslation::WorldPosToLocalPos(const SVector3Df& v3WorldPos) const
{
	CMatrix4Df WorldToLocalTranslation = GetReversedTranslationMatrix();
	CMatrix4Df WorldToLocalRotation = GetReversedRotationMatrix();
	CMatrix4Df WorldToLocalTransformation = WorldToLocalRotation * WorldToLocalTranslation;

	SVector4Df WorldPos4f = SVector4Df(v3WorldPos, 1.0f);
	SVector4Df LocalPos4f = WorldToLocalTransformation * WorldPos4f;
	SVector3Df LocalPos3f(LocalPos4f);
	return (LocalPos3f);
}

/**
 * WorldDirToLocalDir - Converts a world-space direction vector into
 *                      the local space of the object.
 *
 * This is useful for things like transforming movement or aiming
 * directions into an object’s local frame (e.g. camera or model).
 *
 * Assumes no shearing and uniform scale, so uses matrix transpose
 * as an inverse approximation for rotation part.
 *
 * @v3WorldDir: A normalized direction vector in world space.
 *
 * Return: A normalized direction vector in local space.
 */
SVector3Df CWorldTranslation::WorldDirToLocalDir(const SVector3Df& v3WorldDir) const
{
	CMatrix3Df World(GetMatrix());

	// Inverse Local-To-World Transformation using Transpose (assuming uniform scaling)
	CMatrix3Df WorldToLocal = World.Transpose();

	SVector3Df v3LocalDirection = WorldToLocal * v3WorldDir;
	v3LocalDirection = v3LocalDirection.normalize();

	return (v3LocalDirection);
}

