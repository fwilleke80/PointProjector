#include "c4d.h"
#include "c4d_falloffdata.h"
#include "c4d_symbols.h"
#include "oProjector.h"
#include "wsPointProjector.h"
#include "main.h"


const Int32 ID_PROJECTOROBJECT = 1026403;	///< PointProjector plugin ID


/// Draw an arrow
/// @param bd Pointer to BaseDraw
/// @param pos Position of the arrow in global space
/// @param length Length of the arrow
/// @param extra Draw some more pointy lines
static void DrawArrow(BaseDraw *bd, const Vector &pos, Float length, Bool extra = false)
{
	// Good practice: Always check if required pointers are set, or if we need to draw anything at all
	if (!bd || length == 0.0)
		return;
	
	// Precalculate some values we'll need a lot
	const Float length025 = length * 0.25;
	const Float length125 = length * 0.125;
	const Vector tip = pos + Vector(0.0, 0.0, length);

	bd->DrawLine(pos, tip, 0);	// Main line
	bd->DrawLine(tip, tip + Vector(length125, 0.0, -length025), 0);		// Pointy line
	bd->DrawLine(tip, tip + Vector(-length125, 0.0, -length025), 0);	// Pointy line

	// Draw even more pointy lines
	if (extra)
	{
		bd->DrawLine(tip, tip + Vector(0.0, length125, -length025), 0);
		bd->DrawLine(tip, tip + Vector(0.0, -length125, -length025), 0);
	}
}

/// Draw a star
/// @param bd Pointer to BaseDraw
/// @param pos position of the star in global space
/// @param size Size of the star
static void DrawStar(BaseDraw *bd, const Vector &pos, Float size)
{
	// Good practice: Always check if required pointers are set, or if we need to draw anything at all
	if (!bd || size <= 0.0)
		return;
	
	// Precalculate this value, we'll need it a lot
	const Float size2 = size * 0.7;
	
	// Straight lines
	bd->DrawLine(Vector(0.0, -size, 0.0), Vector(0.0, size, 0.0), 0);
	bd->DrawLine(Vector(-size, 0.0, 0.0), Vector(size, 0.0, 0.0), 0);
	bd->DrawLine(Vector(0.0, 0.0, -size), Vector(0.0, 0.0, size), 0);
	
	// Diagonals
	bd->DrawLine(Vector(-size2, -size2, -size2), Vector(size2, size2, size2), 0);
	bd->DrawLine(Vector(-size2, size2, -size2), Vector(size2, -size2, size2), 0);
	bd->DrawLine(Vector(-size2, -size2, size2), Vector(size2, size2, -size2), 0);
	bd->DrawLine(Vector(size2, -size2, -size2), Vector(-size2, size2, size2), 0);
}


/// Object plugin class
/// This implements the Projector object inside Cinema 4D
class oProjector : public ObjectData
{
	INSTANCEOF(oProjector, ObjectData)
	
private:
	wsPointProjector	_projector;					///< Projector object that does all the work for us (and nicely separates the projection code from the Deformer/Object code)
	UInt32						_lastlopdirty = 0;	///< Used to store the last retreived dirty checksum for later comparison
	AutoAlloc<C4D_Falloff>	_falloff;			///< Provides the functions needed to support falloffs
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, Int32 type, void *data);
	virtual DRAWRESULT Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);
	virtual void CheckDirty(BaseObject *op, BaseDocument *doc);
	virtual Bool CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);

	static NodeData *Alloc();
};


// Initialize node
Bool oProjector::Init(GeListNode *node)
{
	// Get BaseObject
	BaseObject *op = static_cast<BaseObject*>(node);
	if (!op)
		return false;
	
	// Get container, we want to set some data here
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	// Init projection mode attributes
	bc->SetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);
	bc->SetFloat(PROJECTOR_OFFSET, 0.0);
	bc->SetFloat(PROJECTOR_BLEND, 1.0);
	bc->SetBool(PROJECTOR_GEOMFALLOFF_ENABLE, false);
	bc->SetFloat(PROJECTOR_GEOMFALLOFF_DIST, 150.0);

	return SUPER::Init(node);
}

// Catch messages
Bool oProjector::Message(GeListNode *node, Int32 type, void *data)
{
	// Good practice: Always check if all required pointers are set
	if (!node || !_falloff)
		return false;

	BaseContainer *bc = (static_cast<BaseObject*>(node))->GetDataInstance();
	if (!bc)
		return false;

	// Enable the deformer object
	if (type == MSG_MENUPREPARE)
	{
		(static_cast<BaseObject*>(node))->SetDeformMode(true);
	}

	// Forward messages to the falloff, it might need them
	if (!_falloff->Message(type, bc, data))
		return false;
	
	return SUPER::Message(node, type, data);
}

// Draw visualization
DRAWRESULT oProjector::Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	// Good practice: Always check if all required pointers are set
	if (!op || !bd || !bh)
		return DRAWRESULT_SKIP;
	
	if (drawpass == DRAWPASS_OBJECT)
	{
		// Get container, skip if that doesn't work (actually, if this doesn'T work, something is really wrong!)
		BaseContainer *bc = op->GetDataInstance();
		if (!bc)
			return DRAWRESULT_SKIP;

		// Get document, skip if no document set
		BaseDocument *doc = op->GetDocument();
		if (!doc)
			return DRAWRESULT_OK;

		// Set draw matrix
		bd->SetMatrix_Matrix(op, bh->GetMg());
		
		// Set draw color
		bd->SetPen(bd->GetObjectColor(bh, op));

		// Get projector mode
		PROJECTORMODE mode = (PROJECTORMODE)bc->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);
		
		// Draw arrows in parallel mode, or star in spherical mode
		if (mode == PROJECTORMODE_PARALLEL)
		{
			DrawArrow(bd, Vector(50.0, 0.0, 0.0), 100.0, true);
			DrawArrow(bd, Vector(-50.0, 0.0, 0.0), 100.0, true);
			DrawArrow(bd, Vector(0.0, 50.0, 0.0), 100.0, true);
			DrawArrow(bd, Vector(0.0, -50.0, 0.0), 100.0, true);
		}
		else
		{
			DrawStar(bd, Vector(0.0), 100.0);
		}

		// Let the falloff draw itself, too
		_falloff->Draw(bd, bh, drawpass, bc);
	}
	
	// Reset draw matrix, so we don't screw up successive draw calls
	bd->SetMatrix_Matrix(nullptr, Matrix());
	
	return SUPER::Draw(op, drawpass, bd, bh);
}

// Modify points of input object
Bool oProjector::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread)
{
	// Good practice: Always check if all required pointers are set
	if (!op || !mod || !_falloff)
		return false;
	
	// Don't continue if op is not a PointObject
	if (!op->IsInstanceOf(Opoint))
		return true;
	
	// Get modifier's container, we need to read the parameters from it
	BaseContainer *bc = mod->GetDataInstance();
	if (!bc)
		return false;
	
	// Get collision object
	PolygonObject *collisionObject = static_cast<PolygonObject*>(bc->GetObjectLink(PROJECTOR_LINK, doc));
	if (!collisionObject)
		return true;

	// Get parameters
	PROJECTORMODE mode = (PROJECTORMODE)bc->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);
	Float offset = bc->GetFloat(PROJECTOR_OFFSET, 0.0);
	Float blend = bc->GetFloat(PROJECTOR_BLEND, 1.0);
	Bool geometryFalloffEnabled = bc->GetBool(PROJECTOR_GEOMFALLOFF_ENABLE, false);
	Float geometryFalloffDist = bc->GetFloat(PROJECTOR_GEOMFALLOFF_DIST, 100.0);
	
	// Calculate weight map from vertex maps linked in restriction tag
	// TODO: Call CalcVertexMap() only if any of the linked vertex map tags has changed since the last time
	Float32* weightMap = nullptr;
	weightMap = ToPoint(op)->CalcVertexMap(mod);

	// Initialize falloff
	if (!_falloff->InitFalloff(bc, doc, mod))
		return false;
	
	// Initialize projector
	if (!_projector.Init(collisionObject))
		return false;

	// Parameters for projection
	wsPointProjectorParams projectorParams(mod->GetMg(), mode, offset, blend, geometryFalloffEnabled, geometryFalloffDist, weightMap, _falloff);
	
	// Perform projection
	if (!_projector.Project(static_cast<PointObject*>(op), projectorParams, thread))
		return false;
	
	// Free weight map (important! Otherwise the memory fills up rather quickly)
	DeleteMem(weightMap);

	// The object was probably deformed, so send update message
	op->Message(MSG_UPDATE);

	return true;
}

// Check if modifier or linked object have been changed in any way
// If so, set the modifier dirty, which will trigger a recalculation
void oProjector::CheckDirty(BaseObject *op, BaseDocument *doc)
{
	// Good practice: Always check if all required pointers are set
	if (!op || !doc)
		return;

	// Get modifier's container, we need to read the parameters from it
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return;
	
	// We'll sum up all the dirty checksums of participating objects here
	UInt32 dirtyness = 0;

	// Get linked collision object
	BaseObject *collisionObject = static_cast<BaseObject*>(bc->GetObjectLink(PROJECTOR_LINK, doc));
	if (!collisionObject)
		return;

	// Iterate collision object and its parents, and add their dirty checksums
	while (collisionObject)
	{
		dirtyness += collisionObject->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		collisionObject = collisionObject->GetUp();
	}

	// Iterate modifier's parents and add their dirty checksums
	BaseObject *parentObject = op->GetUp();
	while (parentObject)
	{
		dirtyness += parentObject->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		parentObject = parentObject->GetUp();
	}
	
	//  Add modifier's dirty checksum
	dirtyness += op->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);

	// Compare dirty checksum to previous one, set modifier dirty if necessary
	if (dirtyness != _lastlopdirty + 1)
	{
		// Store dirty checksum for next comparison
		_lastlopdirty = dirtyness;
		
		// Set modifier dirty. It will be recalculated.
		op->SetDirty(DIRTYFLAGS_DATA);
	}
}

// Copy private data
Bool oProjector::CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn)
{
	// Good practice: Always check if all required pointers are set
	if (!dest || !_falloff)
		return false;
	
	// Cast to oProjector, otherwise we can't access the members
	oProjector* destNodeData = static_cast<oProjector*>(dest);
	
	// Copy members
	destNodeData->_lastlopdirty = _lastlopdirty;

	// Copy falloff
	if (!_falloff->CopyTo(destNodeData->_falloff))
		return false;

	return SUPER::CopyTo(dest, snode, dnode, flags, trn);
}

// Load description and add Falloff elements
Bool oProjector::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
{
	// Good practice: Always check if all required pointers are set
	if (!node || !description || !_falloff)
		return false;
	
	// Get BaseObject and its container
	BaseObject *op = static_cast<BaseObject*>(node);
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	// Cancel if description wasn't loaded correctly
	if (!description->LoadDescription(op->GetType()))
		return false;

	// Signalize the description has been loaded successfully
	flags |= DESCFLAGS_DESC_LOADED;

	// Add falloff description
	if (!_falloff->AddFalloffToDescription(description, bc))
		return false;

	return SUPER::GetDDescription(node, description, flags);
}

// Enable and disable ('gray out') user controls
Bool oProjector::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
	// Good practice: Always check if all required pointers are set
	if (!node)
		return false;
	
	// Get BaseObject and its container
	BaseObject *op = static_cast<BaseObject*>(node);
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	switch (id[0].id)
	{
		// Only enable geometry falloff distance parameter of geometry falloff is active
		case PROJECTOR_GEOMFALLOFF_DIST:
			return bc->GetBool(PROJECTOR_GEOMFALLOFF_ENABLE, false);
	}
	
	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);
}

// Allocate an instance of oProjector
NodeData* oProjector::Alloc()
{
	return NewObjClear(oProjector);
}


// Register plugin
Bool RegisterProjectorObject()
{
	return RegisterObjectPlugin(ID_PROJECTOROBJECT, GeLoadString(IDS_PROJECTOROBJECT), OBJECT_MODIFIER, oProjector::Alloc, "oProjector", AutoBitmap("oProjector.tif"), 0);
}
