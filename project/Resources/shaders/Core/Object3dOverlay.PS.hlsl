// Overlay rendering binds a single UI render target and therefore must not
// expose the regular Object3D bloom MRT output.
#define OBJECT3D_OVERLAY_PASS 1
#include "Object3d.PS.hlsl"
