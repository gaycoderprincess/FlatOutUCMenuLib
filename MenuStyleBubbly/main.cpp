#include <windows.h>
#include <d3d9.h>

#include "nya_dx9_hookbase.h"
#include "chloemenulib.h"

void HookLoop() {}

bool bDeviceJustReset = false;

float fMenuYTop = 0.18;
float fMenuYBottom = 0.9;
float fMenuLeft;
float fMenuBorderLeft;
float fMenuTextSize = 0.02;
float fMenuBoxSpacing;

namespace MenuStyleDefault {
	tNyaStringData GetDefaultStringData() {
		tNyaStringData ret;
		ret.YCenterAlign = true;
		ret.clipMaxX = 1 - (fMenuLeft - fMenuBoxSpacing);
		return ret;
	}

	bool DrawMenuOption(const ChloeMenuLib::tMenuStyleState* state, const ChloeMenuLib::tMenuOptionDraw& opt) {
		if (opt.y >= 0 && opt.y <= state->menuYSize && opt.level == state->menuLevel) {
			auto data = GetDefaultStringData();
			data.x = fMenuLeft = 0.5 - (0.15 * GetAspectRatioInv());
			data.y = fMenuYTop;
			data.size = fMenuTextSize;
			data.y += data.size * opt.y;
			data.topLevel = true;
			//if (opt.isHighlighted) {
			//	data.SetColor(255, 255, 255, 255);
			//}
			if (opt.nonSelectable) {
				data.SetColor(64, 64, 64, 255);
			}
			else {
				data.SetColor(255, 255, 255, 255);
				data.outlinea = 255;
				data.outlinedist = 0.05;
			}
			DrawString(data, opt.label);
			return true;
		}
		return false;
	}

	void Draw(ChloeMenuLib::tMenuStyleState* state) {
		if (!g_pd3dDevice) {
			ChloeMenuLib::UpdateD3DProperties(state);
			InitHookBase();
		}
		if (bDeviceJustReset) {
			ImGui_ImplDX9_CreateDeviceObjects();
			bDeviceJustReset = false;
		}
		ChloeMenuLib::SetMenuYSize(24);

		int numMenuOptionsDrawn = 0;
		int numSelectableMenuOptionsDrawn = 0;
		for (int i = 0; i < state->numMenuOptions; i++) {
			auto& opt = state->menuOptions[i];
			if (opt.level == state->menuLevel) {
				numMenuOptionsDrawn++;
				if (!opt.nonSelectable) numSelectableMenuOptionsDrawn++;
			}
			DrawMenuOption(state, opt);
		}

		int menuBoxSize = state->menuYSize + 1;

		static auto tex = LoadTexture("MenuStyles/MenuStyleBubbly/NyaTex.png");
		fMenuBoxSpacing = 0.02 * GetAspectRatioInv();
		fMenuBorderLeft = fMenuLeft - fMenuBoxSpacing;
		DrawRectangle(fMenuBorderLeft, 1 - fMenuBorderLeft, fMenuYTop - (fMenuTextSize * 3), fMenuYTop + (fMenuTextSize * (menuBoxSize + 2)), {255,255,255,255}, 0.02, tex);

		// highlighted option
		{
			auto height = fMenuTextSize;
			auto y = fMenuYTop + (fMenuTextSize * (state->menuSelectedOptionVisual - state->menuScroll));

			DrawRectangle(fMenuBorderLeft, 1 - fMenuBorderLeft, y - (height * 0.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		// top border
		{
			auto height = fMenuTextSize;
			auto y = fMenuYTop + (fMenuTextSize * -2);

			DrawRectangle(fMenuBorderLeft, 1 - fMenuBorderLeft, y - (height * 0.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		// hints border
		{
			auto height = fMenuTextSize;
			auto y = fMenuYTop + (fMenuTextSize * (menuBoxSize + 1));

			DrawRectangle(fMenuBorderLeft, 1 - fMenuBorderLeft, y - (height * 1.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		//if (state->menuScroll > 0) {
		//	auto data = GetDefaultStringData();
		//	data.x = 0.5;
		//	data.y = fMenuYTop;
		//	data.size = fMenuTextSize;
		//	data.y += data.size * -1;
		//	DrawString(data, "...");
		//}
		//if ((numMenuOptionsDrawn - state->menuScroll) > state->menuYSize + 1)
		{
			auto data = GetDefaultStringData();
			data.x = 0.5;
			data.y = fMenuYTop;
			data.size = fMenuTextSize;
			data.y += data.size * (state->menuYSize + 1);
			data.XCenterAlign = true;
			DrawString(data, std::format("{}/{}", state->menuSelectedOption + 1, numSelectableMenuOptionsDrawn));
		}

		// menu title
		{
			auto data = GetDefaultStringData();
			data.x = 0.5;
			data.y = fMenuYTop;
			data.size = fMenuTextSize;
			data.y += data.size * -2;
			data.XCenterAlign = true;
			DrawString(data, state->libVersion);
		}

		// menu prompts
		{
			auto data = GetDefaultStringData();
			data.x = 0.5;
			data.y = fMenuYTop;
			data.size = fMenuTextSize;
			int max = menuBoxSize;
			data.y += data.size * (max + 1);
			data.XCenterAlign = true;
			std::string str;
			if (state->backHint && state->backHint[0]) {
				str += "[ESC] " + (std::string)state->backHint;
			}
			if (state->lrHint && state->lrHint[0]) {
				if (!str.empty()) str += " ";
				str += "[LEFT/RIGHT] " + (std::string)state->lrHint;
			}
			if (state->enterHint && state->enterHint[0]) {
				if (!str.empty()) str += " ";
				str += "[ENTER] " + (std::string)state->enterHint;
			}
			DrawString(data, str);
		}

		CommonMain();
	}
}

void OnD3DReset() {
	if (g_pd3dDevice) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		bDeviceJustReset = true;
	}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			ChloeMenuLib::RegisterMenuStyle("Bubbly", MenuStyleDefault::Draw);
			ChloeMenuLib::RegisterD3DReset(OnD3DReset);
		} break;
		default:
			break;
	}
	return TRUE;
}