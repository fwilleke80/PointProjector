# R20 version of PointProjector

A simple Cinema 4D plugin that projects the points of a spline or polygon object on geometry. It is basically like Cinema's own "Project" command, but implemented as a deformer, working with both, Splines and Polygon objects, and working non-destructively.

This plugin demonstrates the following C4D API aspects:
* Deformer object plugins, derived from `class ObjectData`
* Ray intersections with `class GeRayCollider`
* Supporting MoGraph falloffs with `class C4D_Falloff`
* Achieving good performance by using a custom caching mechanism with `ObjectData::CheckDirty()`
* Getting polygons and caches from generator objects
* Drawing in the viewport
* Vector and matrix math in general
