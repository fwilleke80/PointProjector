#ifndef __WS_POINTPROJECTOR_H__
#define __WS_POINTPROJECTOR_H__

#include "c4d.h"
#include "lib_collider.h"


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
	
	wsPointProjectorParams() : _mode(PROJECTORMODE_PARALLEL)
	{ }
	
	wsPointProjectorParams(const Matrix &modifierMg, PROJECTORMODE mode) : _modifierMg(modifierMg), _mode(mode)
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
	
	Bool	ProjectPosition(Vector &position, const Vector &rayDirection, Float rayLength, const Matrix &collisionObjectMg, const Matrix &collisionObjectMgI);
	Bool	Project(PointObject *op, const wsPointProjectorParams &params);
	
	wsPointProjector() : _collisionObject(nullptr), _initialized(false)
	{ }
};

#endif
