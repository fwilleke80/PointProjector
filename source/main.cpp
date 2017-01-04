#include "c4d.h"
#include "c4d_symbols.h"
#include "wsPointProjector.h"
#include "main.h"


#define PLUGIN_NAME "PointProjector 1.2"

Bool PluginStart()
{
	GePrint(PLUGIN_NAME);
	
	if (!RegisterProjectorObject())
		return false;

	return true;
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init())
				return false; // don't start plugin without resource
			return true;

		case C4DMSG_PRIORITY: 
			return true;
	}

	return false;
}
