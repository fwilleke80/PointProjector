# PointProjector

A simple Cinema 4D plugin that projects the points of a spline or polygon object on geometry. It is basically like Cinema's own "Project" command, but implemented as a deformer, working with both, Splines and Polygon objects, and working non-destructively.

This plugin demonstrates the following C4D API aspects:
* Deformer object plugins, derived from `class ObjectData`
* Ray intersections with class `GeRayCollider`
* Drawing in the viewport
* Achieving good performance using a custom caching mechanism with `ObjectData::CheckDirty()`
* Sampling Falloffs
