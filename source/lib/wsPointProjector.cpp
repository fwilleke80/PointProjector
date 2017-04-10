#include "c4d.h"
#include "wsPointProjector.h"


Bool wsPointProjector::Init(PolygonObject *collisionObject, Bool force)
{
	// If no collisionObject was passed, abort initialization
	if (!collisionObject)
		goto InitUnsuccessful;
	
	// Check if RayCollider was allocated
	if (!_collider)
		goto InitUnsuccessful;
	
	// Initialize RayCollider
	_collisionObject = collisionObject;
	if (!_collider->Init(_collisionObject, force))
		goto InitUnsuccessful;
	
	// Everything went fine
	_initialized = true;
	return true;
	
InitUnsuccessful:
	// Something went wrong
	_initialized = false;
	return false;
}

Bool wsPointProjector::ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI, Float offset, Float blend)
{
	if (!_initialized || !_collider || !_collisionObject)
		return false;
	
	if (rayLength <= 0.0 || rayDirection == Vector())
		return false;

	Vector workPosition = collisionObjectMgI * position;            // Transform position to m_collop's local space
	GeRayColResult collisionResult;
	Vector rPos(workPosition);
	Vector rDir(collisionObjectMgI.TransformVector(rayDirection));  // Transform direction to m_collop's local space

	if (_collider->Intersect(rPos, rDir, rayLength, false))
	{
		// Get collision result
		// Return true if no intersection was found, as this is not a critical problem (the ray simply shot into the void, nothing happens)
		if (!_collider->GetNearestIntersection(&collisionResult))
			return true;
		
		workPosition = collisionResult.hitpos;
		
		// Apply offset
		if (offset != 0.0)
			workPosition += collisionResult.s_normal.GetNormalized() * offset;
		
		// Apply blend
		if (blend != 1.0)
			workPosition = Blend(rPos, workPosition, blend);
		
		// Transform position back to global space
		position = collisionObjectMg * workPosition;
	}

	return true;
}

Bool wsPointProjector::Project(PointObject *op, const wsPointProjectorParams &params, BaseThread *thread)
{
	if (!_initialized || !_collider || !_collisionObject || !op)
		return false;
	
	// Get point count
	const Int32 pointCount = op->GetPointCount();
	if (pointCount == 0)
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
	
	// Ray position in gobal space. Don't construct yet (DC), they will be receiving values soon enough.
	Vector rayPosition(DC);
	Vector originalRayPosition(DC);
	
	// Ray direction in global space Don't construct yet (DC), it will be receiving a value soon enough.
	Vector rayDirection(DC);
	
	// If using parallel projection, calculate rayDirection now, as it's the same for all points
	if (params._mode == PROJECTORMODE_PARALLEL)
	{
		// Direction points along the modifier's Z axis
		rayDirection = params._modifierMg.v3;
	}
	
	// Calculate a ray length.
	// The resulting length might be a bit too long, but with this we're on the safe side. No ray should ever be too short to reach the collision geometry.
	Float rayLength = (collisionObjectMg.off - opMg.off).GetLength() + _collisionObject->GetRad().GetSum()+ op->GetRad().GetSum();

	// Iterate points
	for (Int32 i = 0; i < pointCount; i++)
	{
		// Check if procesing should be cancelled
		if (thread && !(i & 63) && thread->TestBreak())
			break;

		// Transform point position to global space
		rayPosition = opMg * padr[i];
		originalRayPosition = rayPosition;

		// Calculate ray direction for spherical projection
		// This needs to be done inside the loop, as the direction is different for each point
		if (params._mode == PROJECTORMODE_SPHERICAL)
		{
			// Direction points from the modifier to the position of the point
			rayDirection = rayPosition - params._modifierMg.off;
		}
		
		// Project point, cancel if critical error occurred
		if (!ProjectPosition(rayPosition, rayDirection, rayLength, collisionObjectMg, collisionObjectMgI, params._offset, params._blend))
			return false;
		
		// Calculate geometry falloff
		if (params._geometryFalloffEnabled)
		{
			// Square the falloff distance
			Float maxDistSquared = params._geometryFalloffDist * params._geometryFalloffDist;
			
			// Get squared length vector from original ray position to resulting ray position
			// We're using squared distances here, to avoid calculate expensive square roots
			Float distanceSquared = (rayPosition - originalRayPosition).GetSquaredLength();
			
			// If within falloff range
			if (distanceSquared < maxDistSquared)
			{
				// Calculate blend value using a smooth step interpolation
				Float blendVal = Smoothstep(0.0, maxDistSquared, distanceSquared);
				rayPosition = Blend(rayPosition, originalRayPosition, blendVal);
			}
			else
			{
				rayPosition = originalRayPosition;
			}
		}

		// Evaluate falloff
		if (params._falloff)
		{
			// Sample falloff value at original position
			Float falloffResult = 1.0;
			params._falloff->Sample(originalRayPosition, &falloffResult);
			
			// Only perform blending if necessary
			if (falloffResult < 1.0)
				rayPosition = Blend(originalRayPosition, rayPosition, falloffResult);
		}
		
		// Evaluate eight map
		if (params._weightMap)
		{
			// Get weight value for this point form map
			Float32 weight = params._weightMap[i];
			
			// Only perform blending if necessary
			if (weight < 1.0)
				rayPosition = Blend(originalRayPosition, rayPosition, weight);
		}

		// Transform point position back to op's local space
		padr[i] = opMgI * rayPosition;
	}
	
	return true;
}
