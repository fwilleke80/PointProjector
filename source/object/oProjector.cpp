#include "c4d.h"
#include "c4d_symbols.h"
#include "oProjector.h"
#include "wsPointProjector.h"
#include "main.h"


// Unique plugin ID from www.plugincafe.com
const Int32 ID_PROJECTOROBJECT = 1026403;


// Draw an arrow
static void DrawArrow(BaseDraw *bd, const Vector &pos, Float length, Bool extra = false)
{
	if (!bd || length == 0.0) return;
	
	// Precalculate some values
	const Float length025 = length * 0.25;
	const Float length125 = length * 0.125;
	const Vector tip = pos + Vector(0.0, 0.0, length);

	bd->DrawLine(pos, tip, 0);	// Main line
	bd->DrawLine(tip, tip + Vector(length125, 0.0, -length025), 0);
	bd->DrawLine(tip, tip + Vector(-length125, 0.0, -length025), 0);

	if (extra)
	{
		bd->DrawLine(tip, tip + Vector(0.0, length125, -length025), 0);
		bd->DrawLine(tip, tip + Vector(0.0, -length125, -length025), 0);
	}
}

// Draw a star
static void DrawStar(BaseDraw *bd,const Vector &pos, Float size)
{
	if (!bd || size <= 0.0) return;
	
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


// Object plugin class
class oProjector : public ObjectData
{
	INSTANCEOF(oProjector, ObjectData)
	
private:
	wsPointProjector	_projector;
	UInt32						_lastlopdirty = 0;
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, Int32 type, void *data);
	virtual DRAWRESULT Draw(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);
	virtual void CheckDirty(BaseObject *op, BaseDocument *doc);
	virtual Bool CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id,const GeData &t_data,DESCFLAGS_ENABLE flags,const BaseContainer *itemdesc);
	
	static NodeData *Alloc();
};


// Initialize node
Bool oProjector::Init(GeListNode *node)
{
	BaseObject *op = static_cast<BaseObject*>(node);
	if (!op)
		return false;
	
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	// Init projection mode attribute
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
	if (type == MSG_MENUPREPARE)
	{
		(static_cast<BaseObject*>(node))->SetDeformMode(true);
	}
	
	return SUPER::Message(node, type, data);
}

// Draw visualization
DRAWRESULT oProjector::Draw(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh)
{
	if (!op || !bd || !bh)
		return DRAWRESULT_SKIP;
	
	if (type == DRAWPASS_OBJECT)
	{
		BaseContainer *data = op->GetDataInstance();
		if (!data) return DRAWRESULT_OK;

		BaseDocument *doc = op->GetDocument();
		if (!doc) return DRAWRESULT_OK;

		BaseObject *lop = static_cast<BaseObject*>(data->GetObjectLink(PROJECTOR_LINK, doc));
		if (!lop) return DRAWRESULT_OK;
		
		PROJECTORMODE mode = (PROJECTORMODE)data->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);

		bd->SetMatrix_Matrix(op, bh->GetMg());
		bd->SetPen(bd->GetObjectColor(bh, op));

		// Draw arrows
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
	}
	
	bd->SetMatrix_Matrix(nullptr, Matrix());
	
	return DRAWRESULT_OK;
}

// Modify points of input object
Bool oProjector::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread)
{
	// Cancel if something's wrong
	if (!op || !mod)
		return true;
	
	if (!op->IsInstanceOf(Opoint))
		return true;
	
	BaseContainer *bc = mod->GetDataInstance();
	if (!bc)
		return true;
	
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
	
	// Initialize projector
	if (!_projector.Init(collisionObject))
		return false;

	// Parameters for projection
	wsPointProjectorParams projectorParams(mod->GetMg(), mode, offset, blend, geometryFalloffEnabled, geometryFalloffDist);
	
	// Perform projection
	if(!_projector.Project(static_cast<PointObject*>(op), projectorParams)) return true;

	// Send update message
	mod->Message(MSG_UPDATE);

	return true;
}

// Check if modifier or linked object have been changed in any way
void oProjector::CheckDirty(BaseObject *op, BaseDocument *doc)
{
	if (!op || !doc)
		return;

	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return;
	
	UInt32 dirtyness = 0;

	// Get linked object
	BaseObject *collisionObject = static_cast<BaseObject*>(bc->GetObjectLink(PROJECTOR_LINK, doc));
	if (!collisionObject)
		return;

	// Check for linked object's (or its parents') dirtyness
	while (collisionObject)
	{
		dirtyness += collisionObject->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		collisionObject = collisionObject->GetUp();
	}

	// Check for modifier's (or its parents') dirtyness
	BaseObject *parentObject = op;
	while (parentObject)
	{
		dirtyness += parentObject->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		parentObject = parentObject->GetUp();
	}

	// Compare dirtyness to previous dirtyness, set modifier dirty if necessary
	if (dirtyness != _lastlopdirty + 1)
	{
		_lastlopdirty = dirtyness;
		op->SetDirty(DIRTYFLAGS_DATA);
	}
}

// Copy private data
Bool oProjector::CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn)
{
	if (!dest || !snode || !dnode)
		return false;
	
	oProjector* destNode = static_cast<oProjector*>(dest);
	
	destNode->_lastlopdirty = _lastlopdirty;
	
	return SUPER::CopyTo(dest, snode, dnode, flags, trn);
}

// Load description and add Falloff elements
Bool oProjector::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
{
	if (!node || !description)
		return false;
	
	BaseObject *op = static_cast<BaseObject*>(node);
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	if (!description->LoadDescription(op->GetType()))
		return false;

	flags |= DESCFLAGS_DESC_LOADED;

	return SUPER::GetDDescription(node, description, flags);
}

// Enable and disable ('gray out') user controls
Bool oProjector::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
	if (!node)
		return false;
	
	BaseObject *op = static_cast<BaseObject*>(node);
	BaseContainer *data = op->GetDataInstance();
	if (!data)
		return false;
	
	switch (id[0].id)
	{
			// General Settings
		case PROJECTOR_GEOMFALLOFF_DIST:
			return data->GetBool(PROJECTOR_GEOMFALLOFF_ENABLE, false);
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
