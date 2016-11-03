#ifndef __WS_POINTPROJECTOR_H__
#define __WS_POINTPROJECTOR_H__

#include "c4d.h"
#include "lib_collider.h"


struct wsPointProjectorParams
{
	Vector	_direction;
	
	Bool		_subdivEnabled;
	Int32		_subdivValue;

	wsPointProjectorParams() : _subdivEnabled(false), _subdivValue(1)
	{ }
	
	wsPointProjectorParams(const Vector &direction) : _direction(direction), _subdivEnabled(false), _subdivValue(1)
	{ }
	
	wsPointProjectorParams(const Vector &direction, Bool subdivEnabled, Int32 subdivValue) : _direction(direction), _subdivEnabled(subdivEnabled), _subdivValue(subdivValue)
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
