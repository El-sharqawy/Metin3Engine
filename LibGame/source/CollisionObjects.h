#pragma once

class CCollisionObject
{
public:
	CCollisionObject() = default;
	~CCollisionObject() = default;

    bool IntersectTriangle(const SVector3Df& c_orig,
        const SVector3Df& c_dir,
        const SVector3Df& c_v0,
        const SVector3Df& c_v1,
        const SVector3Df& c_v2,
        float* pu,
        float* pv,
        float* pt)
    {
        // Compute the two edges sharing c_v0
        SVector3Df edge1 = c_v1 - c_v0;
        SVector3Df edge2 = c_v2 - c_v0;

        // Begin calculating determinant - also used to calculate U parameter
        SVector3Df pvec = c_dir.cross(edge2);
        float det = edge1.dot(pvec);

        // If the determinant is near zero, the ray lies in the plane of the triangle
        // (or the triangle is degenerate)
        if (fabs(det) < 0.0001f)
        {
            return false;
        }

        float invDet = 1.0f / det;

        // Calculate distance from c_v0 to ray origin
        SVector3Df tvec = c_orig - c_v0;

        // Calculate U parameter and test bounds
        float u = tvec.dot(pvec) * invDet;

        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }

        // Prepare to test V parameter
        SVector3Df qvec = tvec.cross(edge1);

        // Calculate V parameter and test bounds
        float v = c_dir.dot(qvec) * invDet;

        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        // Calculate t, the ray parameter at the intersection
        float t = edge2.dot(qvec) * invDet;

        if (t < 0) // Optional: ensure intersection is in front of the ray
            return false;

        // Return the barycentrics and the ray parameter t
        *pu = u;
        *pv = v;
        *pt = t;

        return true;
    }
};