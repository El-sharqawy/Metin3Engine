#pragma once

#include "../../LibMath/source/stdafx.h"
#include "../../LibTerrain/source/TerrainMap.h"
#include "BoundingBox.h"

// Simple physics body for game objects
class CPhysicsObject
{
public:
	CPhysicsObject();
	~CPhysicsObject();

	// Reset the object's state (position, velocity, etc.)
	void Reset();

	// Accessors
	const SVector3Df& GetPosition() const;
	void SetPosition(const SVector3Df& v3Pos);

	const SVector3Df& GetRotation() const;
	void SetRotation(const SVector3Df& v3Rot);

	const SVector3Df& GetScale() const;
	void SetScale(const SVector3Df& v3Scale);

	const CWorldTranslation& GetWorldTranslation() const;
	void SetWorldTranslation(const CWorldTranslation& worldT);

	const SVector3Df& GetVelocity() const;
	void SetVelocity(const SVector3Df& v3Veloc);

	const SVector3Df& GetAcceleration() const;
	void SetAcceleration(const SVector3Df& v3Accel);

	GLfloat GetMass() const;
	void SetMass(GLfloat fMass);

	GLfloat GetFriction() const;
	void SetFriction(GLfloat fFriction);

	GLfloat GetGravity() const;
	void SetGravity(GLfloat fGravity);

	bool UsesGravity() const;
	void SetUseGravity(bool bUseGravity);

	bool IsCollidable() const;
	void SetCollidable(bool bCollidable);

	bool IsOnGround() const;
	void SetOnGround(bool bOnGround);

	EObjectTypes GetType() const;
	void SetType(EObjectTypes eType);

	const SVector3Df& GetAngularVelocity() const;
	void SetAngularVelocity(const SVector3Df& v3AngVel);

	const SVector3Df& GetTorque() const;
	void SetTorque(const SVector3Df& v3Torque);

	GLfloat GetMomentOfInertia() const;
	void SetMomentOfInertiay(GLfloat fMOIVal);

	void SetRestitution(GLfloat fRestitution);
	GLfloat GetRestitution() const;

	// Set the Map to intersect with
	void SetTerrainMap(CTerrainMap* pTerrainMap);
	CTerrainMap* GetTerrainMap();

	void SetBoundingBoxLocal(const TBoundingBox& boundBox);
	TBoundingBox GetBoundingBoxLocal() const;

	void SetBoundingBoxWorld(const TBoundingBox& boundBoxWorld);
	TBoundingBox GetBoundingBoxWorld() const;

	bool IsSelectedObject() const;
	void SetSelectedObject(bool bSelected);

	GLint64 GetObjectID() const;
	void SetObjectID(GLint64 lObjID);

public:
	// Physics step: update position and velocity based on acceleration, gravity, and friction
	void Update(float fDeltaTime);

	// Apply a force to the object (adds to acceleration)
	void ApplyForce(const SVector3Df& v3Force);

	// Instantly change velocity (e.g., for collisions)
	void ApplyImpulse(const SVector3Df& v3Impulse);

	// Set velocity directly
	void Stop();

	// Gravity setter for convenience
	void EnableGravity(bool enable);

	// Check if object is moving
	bool IsMoving() const;

	// Apply a force at a point (adds to acceleration)
	void ApplyForceAtPoint(const SVector3Df& v3Force, const SVector3Df& v3Point);

	// Apply a torque to the object (rotational force)
	void ApplyTorque(const SVector3Df& v3Torque);

	// Check if the object is colliding with another object
	bool IsCollidingWith(const CPhysicsObject& other) const;

	// Resolve collision with another object
	void ResolveCollision(CPhysicsObject& other);

	// launch your object at a specific angle and speed
	void Launch(GLfloat fSpeed, GLfloat fElevationDeg, GLfloat fAzimuthDeg = 0.0f);

private:
	CWorldTranslation m_WorldTranslation;	// World Translation of the object (position, scale, rotation, height)
	SVector3Df m_v3Velocity;				// Velocity vector
	SVector3Df m_v3Acceleration;			// Acceleration vector

	GLfloat m_fMass;						// Mass of the object
	GLfloat m_fFriction;					// Friction coefficient
	GLfloat m_fGravityVal;					// world's gravity

	bool m_bUseGravity;						// Whether the object is affected by gravity
	bool m_bIsCollidable;					// Whether the object can collide with other objects
	bool m_bIsOnGround;						// Whether the object is currently on the ground

	EObjectTypes m_ePhysicsType;			// Type of physics behavior (static, dynamic, kinematic)

	SVector3Df m_v3AngularVelocity;			// Angular velocity (radians/sec)
	SVector3Df m_v3Torque;					// Accumulated torque for this frame

	GLfloat m_fMomentOfInertia;				// For simplicity, same for all axes (can be a vector for per-axis)
	GLfloat m_fRestitution;					// 0 = no bounce, 1 = perfect bounce

	CTerrainMap* m_pTerrainMap;				// The current map the object within

	TBoundingBox m_BoundingBoxLocal;		// Model-space AABB
	TBoundingBox m_BoundingBoxWorld;		// World-space AABB (updated each frame)

	bool m_bSelectedObject;					// Is the object selected in the editor or game

	GLint64 m_lObjectID;					// Unique ID for the object, can be used for selection or identification
};