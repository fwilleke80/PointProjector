#include "c4d.h"
#include "c4d_symbols.h"
#include "oProjector.h"
#include "wsPointProjector.h"
#include "main.h"


const Int32 ID_PROJECTOROBJECT = 1026403;	// Unique plugin ID from www.plugincafe.com


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
	wsPointProjector						_projector;
	UInt32											_lastlopdirty;
	
public:
	virtual Bool Init						(GeListNode *node);
	virtual Bool Message				(GeListNode *node, Int32 type, void *data);
	virtual DRAWRESULT Draw			(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool ModifyObject   (BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);
	virtual void CheckDirty(BaseObject *op, BaseDocument *doc);
	
	static NodeData *Alloc(void) { return NewObjClear(oProjector); }
};


// Initialize node
Bool oProjector::Init(GeListNode *node)
{
	BaseObject		*op   = (BaseObject*)node;
	if (!op)
		return false;
	
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;
	
	// Init projection mode attribute
	bc->SetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);

	return true;
}

// Catch messages
Bool oProjector::Message(GeListNode *node, Int32 type, void *data)
{
	if (type == MSG_MENUPREPARE)
	{
		((BaseObject*)node)->SetDeformMode(true);
	}
	
	return true;
}

// Draw visualization
DRAWRESULT oProjector::Draw(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh)
{
	if (type == DRAWPASS_OBJECT)
	{
		BaseContainer *data = op->GetDataInstance();
		if (!data) return DRAWRESULT_OK;

		BaseDocument *doc = op->GetDocument();
		if (!doc) return DRAWRESULT_OK;

		BaseObject *lop = (BaseObject*)data->GetObjectLink(PROJECTOR_LINK, doc);
		if (!lop) return DRAWRESULT_OK;
		
		PROJECTORMODE mode = (PROJECTORMODE)data->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);

		bd->SetMatrix_Matrix(op, op->GetMg());
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
	
	PolygonObject *collisionObject = static_cast<PolygonObject*>(bc->GetObjectLink(PROJECTOR_LINK, doc));
	if (!collisionObject)
		return true;

	PROJECTORMODE mode = (PROJECTORMODE)bc->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);

	// Initialize projector
	if (!_projector.Init(collisionObject))
		return false;

	// Parameters for projection
	wsPointProjectorParams projectorParams(mod->GetMg(), mode);

	// Perform projection
	if(!_projector.Project((PolygonObject*)op, projectorParams)) return true;

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
	BaseObject *collisionObject = (BaseObject*)bc->GetLink(PROJECTOR_LINK, doc, Obase);
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


Bool RegisterProjectorObject()
{
	return RegisterObjectPlugin(ID_PROJECTOROBJECT, GeLoadString(IDS_PROJECTOROBJECT), OBJECT_MODIFIER, oProjector::Alloc, "oProjector", AutoBitmap("oProjector.tif"), 0);
}
