#include "c4d.h"
#include "c4d_symbols.h"
#include "oProjector.h"
#include "wsPointProjector.h"

#define ID_PROJECTOROBJECT 1026403

///////////////////////////////////////////////////////////////////////////
// Class declaration
///////////////////////////////////////////////////////////////////////////
class oProjector : public ObjectData
{
	INSTANCEOF(oProjector, ObjectData)

	private:
		AutoAlloc<wsPointProjector>	m_projector;
		wsPointProjectorParams			m_projectorparams;
		ULONG												m_lastlopdirty;

	public:
		virtual Bool Init						(GeListNode *node);
		virtual Bool Message				(GeListNode *node, LONG type, void *data);
		virtual DRAWRESULT Draw			(BaseObject *op, DRAWPASS type, BaseDraw *bd, BaseDrawHelp *bh);
		virtual Bool ModifyObject   (BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Real lod, LONG flags, BaseThread *thread);
		virtual void CheckDirty(BaseObject *op, BaseDocument *doc);

		static NodeData *Alloc(void) { return gNew oProjector; }
};

// Helper functions

void DrawArrow(BaseDraw *bd, const Vector &pos, Real length, Bool extra = FALSE)
{
	if (!bd || length == RCO 0.0) return;

	bd->DrawLine(pos,																						pos + Vector(RCO 0.0, RCO -length, RCO 0.0),														0);
	bd->DrawLine(pos + Vector(RCO 0.0, RCO -length, RCO 0.0),		pos + Vector(RCO length * RCO 0.25, RCO -length * RCO 0.75, RCO 0.0),		0);
	bd->DrawLine(pos + Vector(RCO 0.0, RCO -length, RCO 0.0),		pos + Vector(RCO -length * RCO 0.25, RCO -length * RCO 0.75, RCO 0.0),	0);

	if (extra)
	{
		bd->DrawLine(pos + Vector(RCO 0.0, RCO -length, RCO 0.0),	pos + Vector(RCO 0.0, RCO -length * RCO 0.75, RCO length * RCO 0.25),		0);
		bd->DrawLine(pos + Vector(RCO 0.0, RCO -length, RCO 0.0),	pos + Vector(RCO 0.0, RCO -length * RCO 0.75, RCO -length * RCO 0.25),	0);
	}
}



////////////////////////////////////////////////////////////////////
// Public Class Members
////////////////////////////////////////////////////////////////////

Bool oProjector::Init(GeListNode *node)
{
	BaseObject		*op   = (BaseObject*)node;			if (!op) return FALSE;
	BaseContainer *data = op->GetDataInstance();	if (!data) return FALSE;

	data->SetReal(PROJECTOR_LINK, NULL);

	return TRUE;
}


Bool oProjector::Message(GeListNode *node, LONG type, void *data)
{
	if (type == MSG_MENUPREPARE)
	{
		((BaseObject*)node)->SetDeformMode(TRUE);
	}
	return TRUE;
}


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

		bd->SetMatrix_Matrix(op, op->GetMg());
		bd->SetPen(bd->GetObjectColor(bh, op));

		// Draw arrows
		DrawArrow(bd, Vector(RCO 50.0, RCO 0.0, RCO 0.0), RCO 100.0, TRUE);
		DrawArrow(bd, Vector(RCO -50.0, RCO 0.0, RCO 0.0), RCO 100.0, TRUE);
		DrawArrow(bd, Vector(RCO 0.0, RCO 0.0, RCO 50.0), RCO 100.0, TRUE);
		DrawArrow(bd, Vector(RCO 0.0, RCO 0.0, RCO -50.0), RCO 100.0, TRUE);
	}

	return DRAWRESULT_OK;
}


Bool oProjector::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Real lod, LONG flags, BaseThread *thread)
{
	// Cancel if something's wrong
	if (!op || !mod) return TRUE;
	if (!op->IsInstanceOf(Opoint)) return TRUE;
	BaseContainer *data = mod->GetDataInstance(); if (!data) return TRUE;
	PolygonObject *collop = (PolygonObject*)data->GetObjectLink(PROJECTOR_LINK, doc); if (!collop) return TRUE;

	// Initialize projector
	if (!m_projector->Init(collop)) return TRUE;

	// Calculate projection direction in global space
	Vector direction = Vector(RCO 0.0, RCO -1.0, RCO 0.0) ^ mod->GetMg();

	// Parameters for projection
	m_projectorparams.bSubdivEnabled = FALSE;
	m_projectorparams.lSubdivValue = 0;
	m_projectorparams.vDir = direction;

	// Do project
	if(!m_projector->Project((PolygonObject*)op, m_projectorparams)) return TRUE;

	mod->Message(MSG_UPDATE);

	return TRUE;
}

// Check if linked object has been changed in any way
void oProjector::CheckDirty(BaseObject *op, BaseDocument *doc)
{
	if (!op || !doc) return;

	BaseContainer *bc = op->GetDataInstance();
	if (!bc) return;
	ULONG dirtyness = 0;

	// Get linked object
	BaseObject	*lop = (BaseObject*)bc->GetLink(PROJECTOR_LINK, doc, Obase);
	if (!lop) return;

	while (lop)
	{
		dirtyness += lop->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		lop = lop->GetUp();
	}

	BaseObject *uop = op;
	while (uop)
	{
		dirtyness += uop->GetDirty(DIRTYFLAGS_MATRIX|DIRTYFLAGS_DATA);
		uop = uop->GetUp();
	}

	if (dirtyness != m_lastlopdirty)
	{
		m_lastlopdirty = dirtyness;
		op->SetDirty(DIRTYFLAGS_DATA);
	}
}


////////////////////////////////////////////////////////////////////
// Register Function
////////////////////////////////////////////////////////////////////

Bool RegisterProjectorObject(void)
{
	return RegisterObjectPlugin(ID_PROJECTOROBJECT, GeLoadString(IDS_PROJECTOROBJECT), OBJECT_MODIFIER, oProjector::Alloc, "oProjector", AutoBitmap("oProjector.tif"),0);
}
