#pragma once

class Drawable;
class Volcano;

// Small procedural meshes keep the assignment portable and avoid requiring
// external model files for simple stylized props.
Drawable* createUnitSphereDrawable(int stacks = 12, int slices = 18);
Drawable* createUnitCylinderDrawable(int sides = 14);

// Builds one combined mesh containing many small grass blades. Keeping the
// complete field in one Drawable makes dense vegetation a single draw call.
Drawable* createGrassFieldDrawable(const Volcano& terrain);
