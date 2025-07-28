#include "Stdafx.h"
#include "PhysicsObject.h"

CPhysicsObject::CPhysicsObject()
{
	Reset();
}

CPhysicsObject::~CPhysicsObject()
{
	Reset();
}

void CPhysicsObject::Reset()
{
	m_WorldTranslation.SetPosition(SVector3Df(0.0f, 0.0f, 0.0f));
	m_v3Velocity.SetToZero();
	m_v3Acceleration.SetToZero();
	m_fMass = 1.0f;
	m_fFriction = 0.5f;
	m_fGravityVal = 9.81f;
	m_bUseGravity = false;
	m_bIsCollidable = true;
	m_bIsOnGround = true;
	m_ePhysicsType = EObjectTypes::OBJECT_TYPE_NONE;
	m_v3AngularVelocity.SetToZero();
	m_v3Torque.SetToZero();
	m_fMomentOfInertia = 1.0f; // Default moment of inertia for simplicity
	m_fRestitution = 0.5f; // 0 = no bounce, 1 = perfect bounce
	m_pTerrainMap = nullptr;
}

const SVector3Df& CPhysicsObject::GetPosition() const
{
	return (m_WorldTranslation.GetPosition());
}

void CPhysicsObject::SetPosition(const SVector3Df& v3Pos)
{
	 m_WorldTranslation.SetPosition(v3Pos);
}

const SVector3Df& CPhysicsObject::GetRotation() const
{
	return (m_WorldTranslation.GetRotation());
}

void CPhysicsObject::SetRotation(const SVector3Df& v3Rot)
{
	m_WorldTranslation.SetRotation(v3Rot);
}

const SVector3Df& CPhysicsObject::GetScale() const
{
	return (m_WorldTranslation.GetScale());
}

void CPhysicsObject::SetScale(const SVector3Df& v3Scale)
{
	m_WorldTranslation.SetScale(v3Scale);
}

const CWorldTranslation& CPhysicsObject::GetWorldTranslation() const
{
	return (m_WorldTranslation);
}

void CPhysicsObject::SetWorldTranslation(const CWorldTranslation& worldT)
{
	m_WorldTranslation = worldT;
}

const SVector3Df& CPhysicsObject::GetVelocity() const
{
	return (m_v3Velocity);
}

void CPhysicsObject::SetVelocity(const SVector3Df& v3Veloc)
{
	m_v3Velocity = v3Veloc;
}

const SVector3Df& CPhysicsObject::GetAcceleration() const
{
	return (m_v3Acceleration);
}

void CPhysicsObject::SetAcceleration(const SVector3Df& v3Accel)
{
	m_v3Acceleration = v3Accel;
}

GLfloat CPhysicsObject::GetMass() const
{
	return (m_fMass);
}

void CPhysicsObject::SetMass(GLfloat fMass)
{
	m_fMass = fMass;
}

GLfloat CPhysicsObject::GetFriction() const
{
	return (m_fFriction);
}

void CPhysicsObject::SetFriction(GLfloat fFriction)
{
	m_fFriction = fFriction;
}

GLfloat CPhysicsObject::GetGravity() const
{
	return (m_fGravityVal);
}

void CPhysicsObject::SetGravity(GLfloat fGravity)
{
	m_fGravityVal = fGravity;
}

bool CPhysicsObject::UsesGravity() const
{
	return (m_bUseGravity);
}

void CPhysicsObject::SetUseGravity(bool bUseGravity)
{
	m_bUseGravity = bUseGravity;
}

bool CPhysicsObject::IsCollidable() const
{
	return (m_bIsCollidable);
}

void CPhysicsObject::SetCollidable(bool bCollidable)
{
	m_bIsCollidable = bCollidable;
}

bool CPhysicsObject::IsOnGround() const
{
	return (m_bIsOnGround);
}

void CPhysicsObject::SetOnGround(bool bOnGround)
{
	m_bIsOnGround = bOnGround;
}

EObjectTypes CPhysicsObject::GetType() const
{
	return (m_ePhysicsType);
}

void CPhysicsObject::SetType(EObjectTypes eType)
{
	m_ePhysicsType = eType;
}

const SVector3Df& CPhysicsObject::GetAngularVelocity() const
{
	return m_v3AngularVelocity;
}

void CPhysicsObject::SetAngularVelocity(const SVector3Df& v3AngVel)
{
	m_v3AngularVelocity = v3AngVel;
}

const SVector3Df& CPhysicsObject::GetTorque() const
{
	return (m_v3Torque);
}

void CPhysicsObject::SetTorque(const SVector3Df& v3Torque)
{
	m_v3Torque = v3Torque;
}

GLfloat CPhysicsObject::GetMomentOfInertia() const
{
	return (m_fMomentOfInertia);
}

void CPhysicsObject::SetMomentOfInertiay(GLfloat fMOIVal)
{
	m_fMomentOfInertia = fMOIVal;
}

void CPhysicsObject::SetRestitution(GLfloat fRestitution)
{
	m_fRestitution = fRestitution;
}

GLfloat CPhysicsObject::GetRestitution() const
{
	return m_fRestitution;
}

void CPhysicsObject::SetTerrainMap(CTerrainMap* pTerrainMap)
{
	m_pTerrainMap = pTerrainMap;
}

void CPhysicsObject::SetBoundingBoxLocal(const TBoundingBox& boundBox)
{
	m_BoundingBoxLocal = boundBox;
}

void CPhysicsObject::Update(float fDeltaTime)
{
	if (GetType() == OBJECT_TYPE_NONE || GetType() == OBJECT_TYPE_STATIC)
	{
		return;
	}

	// Apply gravity if enabled and not on ground
	if (UsesGravity() && !IsOnGround())
	{
		m_v3Acceleration.y -= m_fGravityVal; // Apply gravity in the negative Y direction
	}

	// Integrate acceleration to velocity
	m_v3Velocity += GetAcceleration() * fDeltaTime; // Update velocity based on acceleration

	// Apply friction (simple linear damping)
	m_v3Velocity *= (1.0f - GetFriction() * fDeltaTime);

	// Integrate velocity to position
	SVector3Df newPos = m_WorldTranslation.GetPosition() + GetVelocity() * fDeltaTime;

	// --- Terrain collision detection ---
	if (m_pTerrainMap)
	{
		// Query terrain height at (x, z)
		GLfloat fGroundY = m_pTerrainMap->GetHeight(newPos.x, newPos.z);

		if (newPos.y <= fGroundY)
		{
			// Clamp to terrain, stop vertical movement, mark as on ground
			newPos.y = fGroundY;

			// If falling, bounce
			if (m_v3Velocity.y < 0.0f)
			{
				m_v3Velocity.y = -m_v3Velocity.y * m_fRestitution;

				// Apply ground friction to horizontal velocity
				float groundFriction = 1.0f; // 1.0 = no friction, <1.0 = more friction
				m_v3Velocity.x *= groundFriction;
				m_v3Velocity.z *= groundFriction;

				// Stop bouncing if velocity is very small
				if (std::abs(m_v3Velocity.y) < 0.1f)
				{
					m_v3Velocity.y = 0.0f;
					m_bIsOnGround = true;
				}
				else
				{
					m_bIsOnGround = false;
				}
			}
			else
			{
				m_bIsOnGround = true;
			}

			// When on ground, apply additional friction to horizontal velocity
			if (m_bIsOnGround)
			{
				float slideFriction = 1.0f; // 1.0 = no friction, <1.0 = more friction
				m_v3Velocity.x *= slideFriction;
				m_v3Velocity.z *= slideFriction;

				// Stop completely if both horizontal and vertical velocities are very small
				if (std::abs(m_v3Velocity.x) < 0.05f && std::abs(m_v3Velocity.z) < 0.05f)
				{
					m_v3Velocity.x = 0.0f;
					m_v3Velocity.z = 0.0f;
				}
			}
		}
		else
		{
			m_bIsOnGround = false;
		}
	}

	m_WorldTranslation.SetPosition(newPos);

	// Reset acceleration for next frame
	m_v3Acceleration.SetToZero();

	// --- Rotational physics ---
	if (GetMomentOfInertia() > 0.0f)
	{
		// Calculate angular acceleration
		SVector3Df v3AngularAcceleration = GetTorque() / GetMomentOfInertia();

		// Integrate angular acceleration to angular velocity
		m_v3AngularVelocity += v3AngularAcceleration * fDeltaTime;

		// Optionally, apply angular damping (rotational friction)
		GLfloat fAngularDamping = 0.98f; // or (1.0f - friction * dt)
		m_v3AngularVelocity *= fAngularDamping;

		// Integrate angular velocity to rotation (Euler angles)
		SVector3Df newRot = m_WorldTranslation.GetRotation() + GetAngularVelocity() * fDeltaTime;
		m_WorldTranslation.SetRotation(newRot);
	}

	// Reset torque for next frame
	m_v3Torque.SetToZero();

	m_BoundingBoxWorld = m_BoundingBoxLocal.Transform(m_WorldTranslation.GetMatrix());
}

void CPhysicsObject::ApplyForce(const SVector3Df& v3Force)
{
	if (GetType() != OBJECT_TYPE_DYNAMIC || m_fMass <= 0.0f)
	{
		return;
	}

	m_v3Acceleration += v3Force / m_fMass;
}

void CPhysicsObject::ApplyImpulse(const SVector3Df& v3Impulse)
{
	if (GetType() != OBJECT_TYPE_DYNAMIC || m_fMass <= 0.0f)
	{
		return;
	}

	m_v3Velocity += v3Impulse / m_fMass;
}

void CPhysicsObject::Stop()
{
	m_v3Velocity.SetToZero();
	m_v3Acceleration.SetToZero();
}

void CPhysicsObject::EnableGravity(bool enable)
{
	m_bUseGravity = enable;
}

bool CPhysicsObject::IsMoving() const
{
	return !m_v3Velocity.IsZero();
}

void CPhysicsObject::ApplyForceAtPoint(const SVector3Df& v3Force, const SVector3Df& v3Point)
{
	// For a simple physics object (no rotation), just apply the force to the center of mass
	ApplyForce(v3Force);

	// If we add angular velocity/rotation, we would also compute torque here:
	SVector3Df r = v3Point - m_WorldTranslation.GetPosition();
	ApplyTorque(r.cross(v3Force));

}

// Apply a torque to the object (rotational force)
// For a simple object, you may not have angular velocity, so this is a stub
void CPhysicsObject::ApplyTorque(const SVector3Df& v3Torque)
{
	// For a real system, divide by moment of inertia
	m_v3Torque += v3Torque;
}

// Simple AABB collision check (expand as needed)
bool CPhysicsObject::IsCollidingWith(const CPhysicsObject& other) const
{
	// Use AABB intersection
	return m_BoundingBoxWorld.Intersects(other.m_BoundingBoxWorld);
}

// Simple elastic collision resolution (for point masses)
void CPhysicsObject::ResolveCollision(CPhysicsObject& other)
{
	if (!IsCollidable() || !other.IsCollidable())
	{
		sys_log("CPhysicsObject::ResolveCollision: Not Collidable");
		return;
	}

	if (!IsCollidingWith(other))
	{
		return;
	}

	// Only resolve if moving towards each other
	SVector3Df relVel = m_v3Velocity - other.m_v3Velocity;
	if (relVel.y < 0.0f) 
	{
		float restitution = 0.5f * (m_fRestitution + other.m_fRestitution);
		float v1 = m_v3Velocity.y;
		float v2 = other.m_v3Velocity.y;
		float m1 = m_fMass;
		float m2 = other.m_fMass;

		float newV1 = ((m1 - restitution * m2) * v1 + (1 + restitution) * m2 * v2) / (m1 + m2);
		float newV2 = ((m2 - restitution * m1) * v2 + (1 + restitution) * m1 * v1) / (m1 + m2);

		if (std::abs(newV1) < 0.05f)
		{
			newV1 = 0.0f;
		}

		if (std::abs(newV2) < 0.05f)
		{
			newV2 = 0.0f;
		}

		m_v3Velocity.y = newV1;
		other.m_v3Velocity.y = newV2;

		if (std::abs(m_v3Velocity.y) < 0.05f)
		{
			m_v3Velocity.y = 0.0f;
		}
		if (std::abs(other.m_v3Velocity.y) < 0.05f)
		{
			other.m_v3Velocity.y = 0.0f;
		}


		// Dampen horizontal velocity
		m_v3Velocity.x *= 0.8f;
		m_v3Velocity.z *= 0.8f;
		other.m_v3Velocity.x *= 0.8f;
		other.m_v3Velocity.z *= 0.8f;
	}

	// Simple horizontal damping
	SVector3Df delta = m_WorldTranslation.GetPosition() - other.m_WorldTranslation.GetPosition();
	if (!delta.IsZero())
	{
		SVector3Df normal = delta.normalize();

		// Relative velocity along the normal
		float relVelNorm = (m_v3Velocity - other.m_v3Velocity).dot(normal);

		// Only resolve if moving toward each other
		if (relVelNorm < 0.0f)
		{
			float restitution = MyMath::fmin(m_fRestitution, other.m_fRestitution);
			float m1 = m_fMass;
			float m2 = other.m_fMass;
			float impulse = -(1.0f + restitution) * relVelNorm / (1.0f / m1 + 1.0f / m2);

			SVector3Df impulseVec = impulse * normal;
			m_v3Velocity += impulseVec / m1;
			other.m_v3Velocity -= impulseVec / m2;
		}

		// Separate objects to avoid overlap
		float minSeparation = 0.00f;
		m_WorldTranslation.SetPosition(m_WorldTranslation.GetPosition() + normal * minSeparation);
		other.m_WorldTranslation.SetPosition(other.m_WorldTranslation.GetPosition() - normal * minSeparation);
	}

	// If both vertical velocities are very small, stop bouncing
	if (std::abs(m_v3Velocity.y) < 0.1f && std::abs(other.m_v3Velocity.y) < 0.1f)
	{
		m_v3Velocity.y = 0.0f;
		other.m_v3Velocity.y = 0.0f;
		m_bIsOnGround = true;
		other.m_bIsOnGround = true;
	}
}

void CPhysicsObject::Launch(GLfloat fSpeed, GLfloat fElevationDeg, GLfloat fAzimuthDeg)
{
	// Convert degrees to radians
	GLfloat fPI = static_cast<GLfloat>(M_PI);
	GLfloat fElevationRad = fElevationDeg * (fPI / 180.0f);
	GLfloat fAzimuthRad = fAzimuthDeg * (fPI / 180.0f);

	// Calculate velocity components
	GLfloat vx = fSpeed * std::cos(fElevationRad) * std::cos(fAzimuthRad);
	GLfloat vy = fSpeed * std::sin(fElevationRad);
	GLfloat vz = fSpeed * std::cos(fElevationRad) * std::sin(fAzimuthRad);

	SetVelocity(SVector3Df(vx, vy, vz));
	SetOnGround(false);
}
