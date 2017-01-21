#ifndef __WS_POINTPROJECTOR_H__
#define __WS_POINTPROJECTOR_H__


#include "c4d.h"
#include "lib_collider.h"
#include "c4d_falloffdata.h"


// Modes of projection
enum PROJECTORMODE
{
	PROJECTORMODE_PARALLEL	= 1,
	PROJECTORMODE_SPHERICAL	= 2
} ENUM_END_LIST(PROJECTORMODE);


// Parameters for projection
struct wsPointProjectorParams
{
	Matrix	_modifierMg;
	PROJECTORMODE	_mode;
	Float		_offset;
	Float		_blend;
	Bool		_geometryFalloffEnabled;
	Float		_geometryFalloffDist;
	Float32			*_weightMap;
	C4D_Falloff	*_falloff;
	
	wsPointProjectorParams() : _mode(PROJECTORMODE_PARALLEL), _offset(0.0), _blend(0.0), _geometryFalloffEnabled(false), _geometryFalloffDist(0.0), _weightMap(nullptr), _falloff(nullptr)
	{ }
	
	wsPointProjectorParams(const Matrix &modifierMg, PROJECTORMODE mode, Float offset, Float blend, Bool geometryFalloffEnabled, Float geometryFalloffDist, Float32 *weightMap = nullptr, C4D_Falloff *falloff = nullptr) : _modifierMg(modifierMg), _mode(mode), _offset(offset), _blend(blend), _geometryFalloffEnabled(geometryFalloffEnabled), _geometryFalloffDist(geometryFalloffDist), _weightMap(weightMap), _falloff(falloff)
	{ }
};


class wsPointProjector
{
private:
	AutoAlloc<GeRayCollider>	_collider;
	PolygonObject							*_collisionObject;
	Bool											_initialized;
	
public:
	Bool	Init(PolygonObject *collisionObject, Bool bForce = false);
	void	Clear();
	
	Bool	ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI, Float offset = 0.0, Float blend = 0.0);
	Bool	Project(PointObject *op, const wsPointProjectorParams &params, BaseThread *thread = nullptr);
	
	wsPointProjector() : _collisionObject(nullptr), _initialized(false)
	{ }
};

#endif
