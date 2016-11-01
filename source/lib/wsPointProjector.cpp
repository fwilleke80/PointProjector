#include "c4d.h"
#include "wsPointProjector.h"

// Initialize Projector with op as collision geometry
// Caller owns the pointed object
Bool wsPointProjector::Init(PolygonObject *op, Bool bForce)
{
	if (!op) return FALSE;
	
	// Initialize RayCollider
	m_collop = op;
	if (!m_rc->Init(m_collop, bForce))
	{
		m_initialized = FALSE;
		return FALSE;
	}
	
	m_initialized = TRUE;

	return TRUE;
}

// Project a single point
Bool wsPointProjector::ProjectPosition(Vector &pos, const Vector &dir, Real dist)
{
	if (!m_initialized || !m_rc || !m_collop) return FALSE;
	if (dist == RCO 0.0 || dir == Vector(RCO 0.0)) return FALSE;

	GeRayColResult res;
	Vector ppos = pos * !m_collop->GetMg();			// Transform position to m_collop's local space

	if (m_rc->Intersect(ppos, dir, dist, FALSE))
	{
		// Get collision result
		if (!m_rc->GetNearestIntersection(&res)) return FALSE;

		// Transform position back to global space
		pos = res.hitpos * m_collop->GetMg();
	}

	return TRUE;
}

// Iterate all points of op and project them onto m_geom
Bool wsPointProjector::Project(PointObject *op, const wsPointProjectorParams &params)
{
	if (!m_initialized || !m_rc || !m_collop || !op) return FALSE;

	// Get point count
	LONG l_count = op->GetPointCount();
	if (l_count <= 0) return FALSE;
	
	// Get writable point array
	Vector *padr = op->GetPointW();
	if (!padr) return FALSE;
	
	// Subdivide, if desired
	/* TO DO */

	// Calculate a ray length.
	// The resulting "dist" might be a bit too long, but with this we're on the safe side
	Real dist = Len(m_collop->GetMg().off - op->GetMg().off) + VectorSum(m_collop->GetRad()) + VectorSum(op->GetRad());

	// Iterate points
	LONG i(DC);
	for (i = 0; i < l_count; i++)
	{
		// Transform point position to global space
		Vector pos = padr[i] * op->GetMg();

		// Project point
		if (!ProjectPosition(pos, params.vDir, dist))
			return FALSE;

		// Transform point position back to op's local space
		padr[i] = pos * !op->GetMg();
	}
		
	return TRUE;
}