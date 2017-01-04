#include "c4d.h"
#include "c4d_symbols.h"
#include "oProjector.h"
#include "wsPointProjector.h"
#include "main.h"
#include "c4d_falloffdata.h"


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

// Check if a falloff can be skipped
static Bool SkipFalloff(BaseContainer *bc)
{
	if (!bc)
		return true;
	
	return (bc->GetInt32(FALLOFF_MODE) == FALLOFF_MODE_INFINITE && bc->GetFloat(FALLOFF_STRENGTH) == 1.0);
}


// Object plugin class
class oProjector : public ObjectData
{
	INSTANCEOF(oProjector, ObjectData)
	
private:
	wsPointProjector	_projector;
	UInt32						_lastlopdirty;
	C4D_Falloff*			_falloff;
	
public:
	virtual Bool Init(GeListNode *node);
	virtual Bool Message(GeListNode *node, Int32 type, void *data);
	virtual DRAWRESULT Draw(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh);
	virtual Bool ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);
	virtual void CheckDirty(BaseObject *op, BaseDocument *doc);
	virtual Bool CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual Bool GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);
	
	Bool AllocFalloff();
	void FreeFalloff();
	
	oProjector() : _lastlopdirty(0), _falloff(nullptr)
	{
	}
	
	~oProjector()
	{
		C4D_Falloff::Free(_falloff);
	}
	
	static NodeData *Alloc(void)
	{
		return NewObjClear(oProjector);
	}
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
	bc->SetFloat(PROJECTOR_OFFSET, 0.0);
	bc->SetFloat(PROJECTOR_BLEND, 1.0);
	
	if (!AllocFalloff())
		return false;

	return SUPER::Init(node);
}

// Catch messages
Bool oProjector::Message(GeListNode *node, Int32 type, void *data)
{
	if (type == MSG_MENUPREPARE)
	{
		((BaseObject*)node)->SetDeformMode(true);
	}
	
	if (!_falloff)
	{
		if (!_falloff->Message(type))
			return false;
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

		BaseObject *lop = (BaseObject*)data->GetObjectLink(PROJECTOR_LINK, doc);
		if (!lop) return DRAWRESULT_OK;
		
		PROJECTORMODE mode = (PROJECTORMODE)data->GetInt32(PROJECTOR_MODE, PROJECTOR_MODE_PARALLEL);

		if (_falloff && type == DRAWPASS_OBJECT)
		{
			_falloff->SetMg(bh->GetMg());
			
			if (!_falloff->Draw(bd, bh, type, data))
				return DRAWRESULT_OK;
		}

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
	
	// Check if falloff must be evaluated
	Bool skipFalloff = SkipFalloff(bc);
	
	// Initialize falloff
	if (_falloff && !skipFalloff)
	{
		if (!_falloff->InitFalloff(bc, doc, op))
			return false;
		
		_falloff->SetMg(mod->GetMg());
	}
	
	// Initialize projector
	if (!_projector.Init(collisionObject))
		return false;

	// Parameters for projection
	wsPointProjectorParams projectorParams(mod->GetMg(), mode, offset, blend, skipFalloff ? nullptr : _falloff);
	
	// Perform projection
	if(!_projector.Project((PointObject*)op, projectorParams)) return true;

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

// Copy private data
Bool oProjector::CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn)
{
	if (!dest || !snode || !dnode)
		return false;
	
	oProjector* destNode = static_cast<oProjector*>(dest);
	
	destNode->_lastlopdirty = _lastlopdirty;
	if (_falloff)
	{
		_falloff->CopyTo(destNode->_falloff);
	}
	
	return SUPER::CopyTo(dest, snode, dnode, flags, trn);
}

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
	
	if (_falloff)
	{
		if (!_falloff->SetMode(bc->GetInt32(FALLOFF_MODE, FALLOFF_MODE_INFINITE), bc))
			return false;
		
		if (!_falloff->AddFalloffToDescription(description, bc))
			return false;
	}
	
	flags |= DESCFLAGS_DESC_LOADED;
	
	return SUPER::GetDDescription(node, description, flags);
}

Bool oProjector::AllocFalloff()
{
	if (!_falloff)
	{
		_falloff = C4D_Falloff::Alloc();
		if (!_falloff)
			return false;
	}
	
	return true;
}

void oProjector::FreeFalloff()
{
	C4D_Falloff::Free(_falloff);
}


Bool RegisterProjectorObject()
{
	return RegisterObjectPlugin(ID_PROJECTOROBJECT, GeLoadString(IDS_PROJECTOROBJECT), OBJECT_MODIFIER, oProjector::Alloc, "oProjector", AutoBitmap("oProjector.tif"), 0);
}
