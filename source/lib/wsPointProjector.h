#ifndef __WS_POINTPROJECTOR_H__
#define __WS_POINTPROJECTOR_H__

#include "c4d.h"
#include "lib_collider.h"

#define STR_POINTPROJECTOR_VERSION "PointProjector 1.00 beta"


////////////////////////////////////////////////////////////////////
// Settings struct
////////////////////////////////////////////////////////////////////

struct wsPointProjectorParams
{
	Vector	vDir;
	
	Bool		bSubdivEnabled;
	LONG		lSubdivValue;

	wsPointProjectorParams(void) { }
	wsPointProjectorParams(const Vector &direction) { vDir = direction; }
};


////////////////////////////////////////////////////////////////////
// Class declaration
////////////////////////////////////////////////////////////////////

class wsPointProjector
{
private:
	AutoAlloc<GeRayCollider>	m_rc;
	PolygonObject							*m_collop;
	Bool											m_initialized;
	
public:
	Bool	Init(PolygonObject *op, Bool bForce = FALSE);
	Bool	ProjectPosition(Vector &pos, const Vector &dir, Real dist);
	Bool	Project(PointObject *op, const wsPointProjectorParams &params);
	
	static String GetVersionStr(void)
	{
		return String(STR_POINTPROJECTOR_VERSION);
	}
	
	static wsPointProjector *Alloc(void)
	{
		return gNew wsPointProjector;
	}
	
	static void Free(wsPointProjector *&ptr)
	{
		gDelete(ptr);
	}

	wsPointProjector(void)
	{
		m_initialized = FALSE;
	}
	
	~wsPointProjector(void)	{	}
};

#endif
