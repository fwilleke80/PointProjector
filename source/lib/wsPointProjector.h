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
	PROJECTORMODE	_mode = PROJECTORMODE_PARALLEL;
	Float		_offset = 0.0;
	Float		_blend = 0.0;
	Bool		_enableGeometryFalloff = false;
	Float		_geometryFalloffDist = 0.0;
	C4D_Falloff*	_falloff = nullptr;
	
	wsPointProjectorParams()
	{ }
	
	wsPointProjectorParams(const Matrix &modifierMg, PROJECTORMODE mode, Float offset, Float blend, Bool enableGeometryFalloff, Float geometryFalloffDist, C4D_Falloff* falloff) : _modifierMg(modifierMg), _mode(mode), _offset(offset), _blend(blend), _enableGeometryFalloff(enableGeometryFalloff), _geometryFalloffDist(geometryFalloffDist), _falloff(falloff)
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
	Bool	Project(PointObject *op, const wsPointProjectorParams &params);
	
	wsPointProjector() : _collisionObject(nullptr), _initialized(false)
	{ }
};

#endif
