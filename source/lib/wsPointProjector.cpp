#include "c4d.h"
#include "wsPointProjector.h"


// Initialize Projector with op as collision geometry
// Caller owns the pointed object
Bool wsPointProjector::Init(PolygonObject *collisionObject, Bool force)
{
	if (!collisionObject) return false;
	
	// Check if RayCollider was allocated
	if (!_collider)
		goto ErrorHandler;
	
	// Initialize RayCollider
	_collisionObject = collisionObject;
	if (!_collider->Init(_collisionObject, force))
		goto ErrorHandler;
	
	_initialized = true;
	return true;
	
ErrorHandler:
	_initialized = false;
	return false;
}

// Project a single point
Bool wsPointProjector::ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI)
{
	if (!_initialized || !_collider || !_collisionObject)
		return false;
	
	if (rayLength <= 0.0 || rayDirection == Vector())
		return false;

	GeRayColResult collisionResult;
	Vector rPos(collisionObjectMgI * position);				// Transform position to m_collop's local space
	Vector rDir(collisionObjectMgI.TransformVector(rayDirection));		// Transform direction to m_collop's local space

	if (_collider->Intersect(rPos, rDir, rayLength, false))
	{
		// Get collision result
		if (!_collider->GetNearestIntersection(&collisionResult)) return false;
		
		// Transform position back to global space
		position = collisionObjectMg * collisionResult.hitpos;
	}

	return true;
}

// Iterate all points of op and project them onto _collisionObject
Bool wsPointProjector::Project(PointObject *op, const wsPointProjectorParams &params)
{
	if (!_initialized || !_collider || !_collisionObject || !op)
		return FALSE;
	
	// Get point count
	const Int32 pointCount = op->GetPointCount();
	if (pointCount <= 0)
		return false;
	
	// Get writable point array
	Vector *padr = op->GetPointW();
	if (!padr)
		return false;
	
	// Get global Matrix of collision object and op (precalculated for better performance)
	const Matrix collisionObjectMg = _collisionObject->GetMg();
	const Matrix opMg = op->GetMg();
	
	// Also calculate the inversions of both matrices (precalculated for better performance)
	const Matrix collisionObjectMgI = ~collisionObjectMg;
	const Matrix opMgI = ~opMg;
	
	// Ray direction
	Vector rayPosition(DC);
	
	// Ray direction in global space
	Vector rayDirection(params._modifierMg.TransformVector(Vector(0.0, 0.0, 1.0)));

	// Calculate a ray length.
	// The resulting length might be a bit too long, but with this we're on the safe side. No ray should ever be too short.
	Float rayLength = (collisionObjectMg.off - opMg.off).GetLength() + _collisionObject->GetRad().GetSum()+ op->GetRad().GetSum();

	// Iterate points
	for (Int32 i = 0; i < pointCount; i++)
	{
		// Transform point position to global space
		rayPosition = opMg * padr[i];
		
		// Calculate ray direction for spherical projection
		if (params._mode == PROJECTORMODE_SPHERICAL)
		{
			rayDirection = rayPosition - params._modifierMg.off;
		}
		
		// Project point
		if (!ProjectPosition(rayPosition, rayDirection, rayLength, collisionObjectMg, collisionObjectMgI))
			return false;

		// Transform point position back to op's local space
		padr[i] = opMgI * rayPosition;
	}
		
	return true;
}
