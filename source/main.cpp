#include "c4d.h"
#include "wsPointProjector.h"

// forward declarations
Bool RegisterProjectorObject(void);

C4D_CrashHandler old_handler;

void SDKCrashHandler(CHAR *crashinfo)
{
	// don't forget to call the original handler!!!
	if (old_handler) (*old_handler)(crashinfo);
}

Bool PluginStart(void)
{
	// example of installing a crash handler
	old_handler = C4DOS.CrashHandler;		// backup the original handler (must be called!)
	C4DOS.CrashHandler = SDKCrashHandler;	// insert the own handler
	
	if (!RegisterProjectorObject()) return FALSE;
	GePrint("PointProjector Plugin initialized with " + wsPointProjector::GetVersionStr());

	return TRUE;
}

void PluginEnd(void)
{
	
}

Bool PluginMessage(LONG id, void *data)
{
	//use the following lines to set a plugin priority
	//
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init()) return FALSE; // don't start plugin without resource
			return TRUE;

		case C4DMSG_PRIORITY: 
			return TRUE;
	}

	return FALSE;
}
