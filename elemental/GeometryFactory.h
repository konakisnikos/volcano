#pragma once

class Drawable;
class Volcano;

// Procedural meshes for flowers and grass.
Drawable* createUnitSphereDrawable(int stacks = 12, int slices = 18);
Drawable* createUnitCylinderDrawable(int sides = 14);

// Builds one combined mesh containing many small grass blades. Keeping the
// complete field in one Drawable makes dense vegetation a single draw call.
Drawable* createGrassFieldDrawable(const Volcano& terrain);
