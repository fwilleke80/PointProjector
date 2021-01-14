#include "wsFunctions.h"
#include "c4d_fielddata.h"


void DrawArrow(BaseDraw *bd, const Vector &pos, Float length, Bool extra)
{
	// Good practice: Always check if required pointers are set, or if we need to draw anything at all
	if (!bd || length == 0.0)
		return;
	
	// Precalculate some values we'll need a lot
	const Float length025 = length * 0.25;
	const Float length125 = length * 0.125;
	const Vector tip = pos + Vector(0.0, 0.0, length);
	
	bd->DrawLine(pos, tip, 0);  // Main line
	bd->DrawLine(tip, tip + Vector(length125, 0.0, -length025), 0);   // Pointy line
	bd->DrawLine(tip, tip + Vector(-length125, 0.0, -length025), 0);  // Pointy line
	
	// Draw even more pointy lines
	if (extra)
	{
		bd->DrawLine(tip, tip + Vector(0.0, length125, -length025), 0);
		bd->DrawLine(tip, tip + Vector(0.0, -length125, -length025), 0);
	}
}


void DrawStar(BaseDraw *bd, const Vector &pos, Float size)
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


PolygonObject* GetRealGeometry(BaseObject* op)
{
	// Create AliasTranslate
	AutoAlloc<AliasTrans> aliasTrans;
	if (!aliasTrans || !aliasTrans->Init(op->GetDocument()))
		return nullptr;
	
	// Create clone of op. We only need this for the modeling command.
	BaseObject *tmpOp = static_cast<BaseObject*>(op->GetClone(COPYFLAGS::NONE, aliasTrans));
	if (!tmpOp)
		return nullptr;
	
	// Translate BaseLinks, maybe the cloned object needs that
	aliasTrans->Translate(true);
	
	// Create temporary document
	AutoAlloc<BaseDocument> tmpDoc;
	if (!tmpDoc)
	{
		// Free tmpOp and return
		BaseObject::Free(tmpOp);
		return nullptr;
	}
	
	// Insert tmpOp into tmpDoc. From now on, tmpDoc has the ownership over tmpOp,
	// so we don't need to free tmpOp manually anymore (because tmpDoc will auto-free itself at the end of the scope, as we used AutoAlloc to create it).
	tmpDoc->InsertObject(tmpOp, nullptr, nullptr);
	
	// Build modeling command data
	ModelingCommandData mcd;
	mcd.doc = tmpDoc;
	mcd.op = tmpOp;
	
	// Perform modeling command
	if (!SendModelingCommand(MCOMMAND_CURRENTSTATETOOBJECT, mcd))
		return nullptr;
	
	// Get result
	PolygonObject *res = static_cast<PolygonObject*>(mcd.result->GetIndex(0));
	
	// Set original matrix
	if (res)
		res->SetMg(op->GetMg());
	
	// Return result
	return res;
}


Bool GeneratesPolygons(BaseObject* op)
{
	// Check for nullptr, otherwise the -GetInfo() call will crash
	if (!op)
		return false;
	
	// Get node info
	Int32 opInfo = op->GetInfo();
	
	// Get some properties
	Bool isGenerator = (opInfo&OBJECT_GENERATOR) == OBJECT_GENERATOR;	// Is dropOp a generator object?
	Bool isSpline = (opInfo&OBJECT_ISSPLINE) == OBJECT_ISSPLINE;	// Is it a spline?
	Bool isDeformer = (opInfo&OBJECT_MODIFIER) == OBJECT_MODIFIER;	// Is it a deformer?
	Bool isParticleModifier = (opInfo&OBJECT_PARTICLEMODIFIER) == OBJECT_PARTICLEMODIFIER;
	Bool isPolygon = (opInfo&OBJECT_POLYGONOBJECT) == OBJECT_POLYGONOBJECT;
	
	Bool isPolyInstance = op->IsInstanceOf(Opolygon);	// Is it a polygon object instance?
	
	// We want objects that are polygon object instances or generators that are neither splines nor deformers nor anything else we know we don't want.
	// This part seems a bit messy, but there are so many possibilities that we should try and cover as many as we can.
	return isPolyInstance || isPolygon || (isGenerator && !isSpline && !isDeformer && !isParticleModifier);
}


UInt32 AddDirtySums(BaseObject *op, Bool goDown, DIRTYFLAGS flags)
{
	// Check for nullptr, for speed reasons
	if (!op)
		return 0;
	
	UInt32 dirtyness = 0;
	
	while (op)
	{
		dirtyness += op->GetDirty(flags);
		if (goDown)
			op = op->GetDown();
		else
			op = op->GetUp();
	}

	return dirtyness;
}


FieldLayer* IterateNextFieldLayer(FieldLayer* layer)
{
	if (!layer)
		return nullptr;

	if (layer->GetDown())
		return layer->GetDown();

	while (!layer->GetNext() && layer->GetUp())
		layer = layer->GetUp();

	return layer->GetNext();
}
