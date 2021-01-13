#include "maxon/apibase.h"
#include "c4d_symbols.h"
#include "wsPointProjector.h"
#include "main.h"


#define PLUGIN_NAME "PointProjector 1.4.4"


Bool PluginStart()
{
	String pluginName = "PointProjector 1.4.4"_s;
	GePrint(pluginName);
	
	if (!RegisterProjectorObject())
		return false;

	return true;
}


void PluginEnd()
{ }


Bool PluginMessage(Int32 id, void *data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			return g_resource.Init();  // don't start plugin without resource
	}

	return false;
}
