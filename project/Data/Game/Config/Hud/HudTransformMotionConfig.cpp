#include "HudTransformMotionConfig.h"

namespace Calyx2D {

	HudTransformMotionConfig::HudTransformMotionConfig() {
		// Position
		AddField("posEnabled",  posEnabled);
		AddField("posStart",    posStart);
		AddField("posEnd",      posEnd);
		AddField("posDuration", posDuration);
		AddField("posEaseInt",  posEaseInt);

		// Scale
		AddField("scaleEnabled",  scaleEnabled);
		AddField("scaleStart",    scaleStart);
		AddField("scaleEnd",      scaleEnd);
		AddField("scaleDuration", scaleDuration);
		AddField("scaleEaseInt",  scaleEaseInt);

		// Rotation
		AddField("rotEnabled",  rotEnabled);
		AddField("rotStart",    rotStart);
		AddField("rotEnd",      rotEnd);
		AddField("rotDuration", rotDuration);
		AddField("rotEaseInt",  rotEaseInt);

		// Alpha
		AddField("alphaEnabled",  alphaEnabled);
		AddField("alphaStart",    alphaStart);
		AddField("alphaEnd",      alphaEnd);
		AddField("alphaDuration", alphaDuration);
		AddField("alphaEaseInt",  alphaEaseInt);
	}

} // namespace Calyx2D