#ifndef _OPROJECTOR_H_
#define _OPROJECTOR_H_

enum
{
	PROJECTOR_GROUP_PARAMS        = 10000,      // GROUP
	PROJECTOR_LINK                = 10001,      // LINK
	PROJECTOR_MODE                = 10002,      // LONG CYCLE
		PROJECTOR_MODE_PARALLEL       = 1,          // CYCLE VALUE
		PROJECTOR_MODE_SPHERICAL      = 2,          // CYCLE VALUE
	PROJECTOR_OFFSET              = 10003,      // REAL
	PROJECTOR_BLEND               = 10004,      // REAL
	PROJECTOR_GEOMFALLOFF_ENABLE  = 10005,      // BOOL
	PROJECTOR_GEOMFALLOFF_DIST    = 10006       // REAL
};

#endif
