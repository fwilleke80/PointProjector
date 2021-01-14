#ifndef WS_FUNCTIONS_H__
#define WS_FUNCTIONS_H__


#include "c4d.h"


/// Draw an arrow
/// @param bd Pointer to BaseDraw
/// @param pos Position of the arrow in global space
/// @param length Length of the arrow
/// @param extra Draw some more pointy lines
void DrawArrow(BaseDraw *bd, const Vector &pos, Float length, Bool extra = false);

/// Draw a star
/// @param bd Pointer to BaseDraw
/// @param pos position of the star in global space
/// @param size Size of the star
void DrawStar(BaseDraw *bd, const Vector &pos, Float size);

/// Get the actual geometry from an object
/// @note This is done by calling SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT). For that, a clone of op is created and inserted into a temporary document. In cases like this, SendModelingCommand() should always work on object clones in temporary documents.
/// @warning This is a time-consuming call and relatively memory-intensive function. Don't call it unless you really have to.
/// @param op The object to get the actual geometry from
/// @return The actual geometry. The caller owns the pointed object.
PolygonObject* GetRealGeometry(BaseObject* op);

/// Finds out if an object will generate or contain polygons
/// @param op The object that should be tested
/// @return True if op generates or contains polygons, otherwise false
Bool GeneratesPolygons(BaseObject* op);

/// Iterates an object hierarchy and adds up the dirty checksums of all found objects
/// @param op Hierarchy iteration will start with this object
/// @param goDown If true, hierarchy iteration will check for children. Otherwise, it will check for parents
/// @param flags Set the dirty flags that should be checked here
/// @return The sum of the dirty checksums of all found objects
UInt32 AddDirtySums(BaseObject *op, Bool goDown, DIRTYFLAGS flags);

/// Returns the next FieldLayer in a FieldList
/// @param layer The current layer
/// @return The next layer
FieldLayer* IterateNextFieldLayer(FieldLayer* layer);

Bool IsValidFieldLayer(FieldLayer* fieldLayer, BaseDocument* doc);

Bool IsActiveFieldLayer(FieldLayer* fieldLayer);

Int CountActiveAndValidFieldLayers(FieldList* fieldList, BaseDocument* doc);

#endif // WS_FUNCTIONS_H__
