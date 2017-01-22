#ifndef __WS_POINTPROJECTOR_H__
#define __WS_POINTPROJECTOR_H__


#include "c4d.h"
#include "lib_collider.h"
#include "c4d_falloffdata.h"


/// Modes of projection
enum PROJECTORMODE
{
	PROJECTORMODE_PARALLEL	= 1,
	PROJECTORMODE_SPHERICAL	= 2
} ENUM_END_LIST(PROJECTORMODE);


/// Parameters for projection
struct wsPointProjectorParams
{
	Matrix				_modifierMg;							///< Global matrix of modifier
	PROJECTORMODE	_mode;										///< Projector mode (parallel or spherical)
	Float					_offset;									///< Offset attribute
	Float					_blend;										///< Blend attribute
	Bool					_geometryFalloffEnabled;	///< Geometry falloff enabled attribute
	Float					_geometryFalloffDist;			///< Geometry falloff distance attribute
	Float32				*_weightMap;							///< Ptr to weight map
	C4D_Falloff		*_falloff;								///< Ptr to falloff
	
	/// Default constructor
	wsPointProjectorParams() : _mode(PROJECTORMODE_PARALLEL), _offset(0.0), _blend(0.0), _geometryFalloffEnabled(false), _geometryFalloffDist(0.0), _weightMap(nullptr), _falloff(nullptr)
	{ }
	
	/// Constructor with parameters
	wsPointProjectorParams(const Matrix &modifierMg, PROJECTORMODE mode, Float offset, Float blend, Bool geometryFalloffEnabled, Float geometryFalloffDist, Float32 *weightMap = nullptr, C4D_Falloff *falloff = nullptr) : _modifierMg(modifierMg), _mode(mode), _offset(offset), _blend(blend), _geometryFalloffEnabled(geometryFalloffEnabled), _geometryFalloffDist(geometryFalloffDist), _weightMap(weightMap), _falloff(falloff)
	{ }
};

/// Handy class that does all the ray shooting and calculations for us
class wsPointProjector
{
private:
	AutoAlloc<GeRayCollider>	_collider;					///< Used for shooting rays at the collision geometry
	PolygonObject							*_collisionObject;	///< Collision geometry
	Bool											_initialized;				///< Indicates if the class has been initialized
	
public:
	/// Initialize class with the passed collisionObject
	/// @note Must be called before calling Project() or ProjectPosition()
	/// @param collisionObject A PolygonObject that ray should be shot at. Caller owns the pointed object.
	/// @param bForce Force re-initialization of GeRayCollider, even if collisinObject did not change since the last Init() call
	/// @return True if initialization was successful, otherwise false
	Bool Init(PolygonObject *collisionObject, Bool bForce = false);

	/// Project a single point on collision geometry
	/// @note Init() must be called before.
	/// @param position Starting position of the ray (global space). It also returns the resulting position.
	/// @param rayDirection Shooting direction of the ray (global space)
	/// @param rayLength Length of the ray
	/// @param collisionObjectMg Global Matrix of the collision geometry
	/// @param collisionObjectMgI Inverted global matrix of the collision geometry
	/// @param offset Offset of the resulting collision position along the ray direction
	/// @param blend Blends between the original and the resulting position
	/// @return False if there was a problem, otherwise true (even if the ray shot into the void, because that's not an error)
	Bool ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI, Float offset = 0.0, Float blend = 0.0);

	/// Project all points of a PointObject on collision geometry
	/// @note Init() must be called before.
	/// @param op The PointObject that should be projected. Caller owns the pointed object.
	/// @param params Parameters for projection
	/// @param thread If called in a threaded context, pass the pointer to the thread here
	/// @return False if there was a problem, otherwise true
	Bool Project(PointObject *op, const wsPointProjectorParams &params, BaseThread *thread = nullptr);

	/// Default constructor
	wsPointProjector() : _collisionObject(nullptr), _initialized(false)
	{ }
};

#endif
