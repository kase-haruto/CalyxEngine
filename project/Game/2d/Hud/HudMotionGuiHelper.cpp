#include "HudMotionGuiHelper.h"

#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

namespace Calyx2D {

	bool DrawChannelPosition(HudTransformMotionConfig& c) {
		bool changed = false;

		changed |= GuiCmd::CheckBox("Position Enabled", c.posEnabled);
		if(!c.posEnabled) return changed;

		changed |= GuiCmd::DragFloat2("Pos Start", c.posStart);
		changed |= GuiCmd::DragFloat2("Pos End",   c.posEnd);
		changed |= GuiCmd::DragFloat ("Pos Duration", c.posDuration);
		changed |= CalyxEase::SelectEaseInt("Pos Ease", c.posEaseInt);

		return changed;
	}

	bool DrawChannelScale(HudTransformMotionConfig& c) {
		bool changed = false;

		changed |= GuiCmd::CheckBox("Scale Enabled", c.scaleEnabled);
		if(!c.scaleEnabled) return changed;

		changed |= GuiCmd::DragFloat2("Scale Start", c.scaleStart);
		changed |= GuiCmd::DragFloat2("Scale End",   c.scaleEnd);
		changed |= GuiCmd::DragFloat ("Scale Duration", c.scaleDuration);
		changed |= CalyxEase::SelectEaseInt("Scale Ease", c.scaleEaseInt);

		return changed;
	}

	bool DrawChannelRotation(HudTransformMotionConfig& c) {
		bool changed = false;

		changed |= GuiCmd::CheckBox("Rotation Enabled", c.rotEnabled);
		if(!c.rotEnabled) return changed;

		changed |= GuiCmd::DragFloat("Rot Start", c.rotStart);
		changed |= GuiCmd::DragFloat("Rot End",   c.rotEnd);
		changed |= GuiCmd::DragFloat("Rot Duration", c.rotDuration);
		changed |= CalyxEase::SelectEaseInt("Rot Ease", c.rotEaseInt);

		return changed;
	}

	bool DrawChannelAlpha(HudTransformMotionConfig& c) {
		bool changed = false;

		changed |= GuiCmd::CheckBox("Alpha Enabled", c.alphaEnabled);
		if(!c.alphaEnabled) return changed;

		changed |= GuiCmd::DragFloat("Alpha Start", c.alphaStart);
		changed |= GuiCmd::DragFloat("Alpha End",   c.alphaEnd);
		changed |= GuiCmd::DragFloat("Alpha Duration", c.alphaDuration);
		changed |= CalyxEase::SelectEaseInt("Alpha Ease", c.alphaEaseInt);

		return changed;
	}

	bool DrawTransformMotionGui(HudTransformMotionConfig& cfg) {
		bool changed = false;

		if(ImGui::CollapsingHeader("Position")) {
			changed |= DrawChannelPosition(cfg);
		}
		if(ImGui::CollapsingHeader("Scale")) {
			changed |= DrawChannelScale(cfg);
		}
		if(ImGui::CollapsingHeader("Rotation")) {
			changed |= DrawChannelRotation(cfg);
		}
		if(ImGui::CollapsingHeader("Alpha")) {
			changed |= DrawChannelAlpha(cfg);
		}

		return changed;
	}

}