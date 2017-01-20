#include "c4d.h"
#include "wsPointProjector.h"


// Initialize Projector with op as collision geometry
// Caller owns the pointed object
Bool wsPointProjector::Init(PolygonObject *collisionObject, Bool force)
{
	if (!collisionObject)
		return false;
	
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

// Project a single point (expects all positions and directions in global space)
Bool wsPointProjector::ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI, Float offset, Float blend)
{
	if (!_initialized || !_collider || !_collisionObject)
		return false;
	
	if (rayLength <= 0.0 || rayDirection == Vector())
		return false;

	Vector workPosition = collisionObjectMgI * position;		// Transform position to m_collop's local space
	GeRayColResult collisionResult;
	Vector rPos(workPosition);
	Vector rDir(collisionObjectMgI.TransformVector(rayDirection));		// Transform direction to m_collop's local space

	if (_collider->Intersect(rPos, rDir, rayLength, false))
	{
		// Get collision result
		// Return true if no intersection was found, as this is not a critical problem (the ray simply shot into the void, nothing happens)
		if (!_collider->GetNearestIntersection(&collisionResult))
			return true;
		
		workPosition = collisionResult.hitpos;
		
		// Apply offset
		if (offset != 0.0)
		{
			workPosition += collisionResult.s_normal.GetNormalized() * offset;
		}
		
		// Apply blend
		if (blend != 1.0)
		{
			workPosition = Blend(rPos, workPosition, blend);
		}
		
		// Transform position back to global space
		position = collisionObjectMg * workPosition;
	}

	return true;
}

// Iterate all points of op and project them onto _collisionObject
// If wsPointProjectorParams::_falloff should be used, it must already be initialized!
Bool wsPointProjector::Project(PointObject *op, const wsPointProjectorParams &params)
{
	if (!_initialized || !_collider || !_collisionObject || !op)
		return false;
	
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
	
	// Ray position in gobal space
	Vector rayPosition(DC);
	Vector originalRayPosition(DC);
	
	// Ray direction in global space
	Vector rayDirection(DC);

	// Calculate a ray length.
	// The resulting length might be a bit too long, but with this we're on the safe side. No ray should ever be too shortto reach the collision geometry.
	Float rayLength = (collisionObjectMg.off - opMg.off).GetLength() + _collisionObject->GetRad().GetSum()+ op->GetRad().GetSum();

	// Iterate points
	for (Int32 i = 0; i < pointCount; i++)
	{
		// Transform point position to global space
		rayPosition = opMg * padr[i];
		originalRayPosition = rayPosition;

		// Falloff
		Float falloffResult = 0.0;
		if (params._falloff)
		{
			params._falloff->Sample(originalRayPosition, &falloffResult);
		}
		GePrint("Falloff sample at " + String::VectorToString(originalRayPosition) + ": " + String::FloatToString(falloffResult));
		
		// Calculate ray direction for spherical projection
		if (params._mode == PROJECTORMODE_SPHERICAL)
		{
			rayDirection = rayPosition - params._modifierMg.off;
		}
		else
		{
			rayDirection = params._modifierMg.TransformVector(Vector(0.0, 0.0, 1.0));
		}
		
		// Project point, cancel if critical error occurred
		if (!ProjectPosition(rayPosition, rayDirection, rayLength, collisionObjectMg, collisionObjectMgI, params._offset, params._blend))
			return false;
		
		// Geometry Falloff
		if (params._geometryFalloffEnabled)
		{
			Float maxDistSquared = params._geometryFalloffDist * params._geometryFalloffDist;
			Float distanceSquared = (rayPosition - originalRayPosition).GetSquaredLength();
			
			if (distanceSquared < maxDistSquared)
			{
				Float mixVal = Smoothstep(0.0, maxDistSquared, distanceSquared);
				rayPosition = Blend(rayPosition, originalRayPosition, mixVal);
			}
			else
			{
				rayPosition = originalRayPosition;
			}
		}

		// Transform point position back to op's local space
		padr[i] = opMgI * rayPosition;
	}
	
	return true;
}
