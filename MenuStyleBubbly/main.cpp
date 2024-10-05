#include <windows.h>
#include <d3d9.h>

#include "nya_dx9_hookbase.h"
#include "chloemenulib.h"

void HookLoop() {}

bool bDeviceJustReset = false;

float fMenuLeft;
float fMenuTextSize = 0.02;
float fMenuBoxSpacing;

namespace MenuStyleDefault {
	bool bDarkMode = false;

	tNyaStringData GetDefaultStringData(const ChloeMenuLib::tMenuStyleState* state) {
		tNyaStringData ret;
		ret.YCenterAlign = true;
		ret.clipMaxX = 1 - (fMenuLeft - fMenuBoxSpacing);
		ret.clipMinX = fMenuLeft - fMenuBoxSpacing;
		ret.clipMaxX += state->posX - 0.5;
		ret.clipMinX += state->posX - 0.5;
		return ret;
	}

	bool DrawMenuOption(const ChloeMenuLib::tMenuStyleState* state, const ChloeMenuLib::tMenuOptionDraw& opt) {
		if (opt.y >= 0 && opt.y <= state->menuYSize && opt.level == state->menuLevel) {
			auto data = GetDefaultStringData(state);
			data.x = fMenuLeft = 0.5 - (0.15 * GetAspectRatioInv());
			data.x += state->posX - 0.5;
			data.y = state->posY;
			data.size = fMenuTextSize;
			data.y += data.size * opt.y;
			data.topLevel = true;
			//if (opt.isHighlighted) {
			//	data.SetColor(255, 255, 255, 255);
			//}
			if (opt.nonSelectable) {
				if (bDarkMode) {
					data.SetColor(200, 200, 200, 255);
				}
				else {
					data.SetColor(64, 64, 64, 255);
				}
			}
			else {
				data.SetColor(255, 255, 255, 255);
				if (!opt.isHighlighted && !bDarkMode) {
					data.outlinea = 255;
					data.outlinedist = 0.05;
				}
			}
			DrawString(data, opt.label);
			return true;
		}
		return false;
	}

	template<int texType>
	void Draw(ChloeMenuLib::tMenuStyleState* state) {
		bDarkMode = texType != 0;

		if (!g_pd3dDevice) {
			ChloeMenuLib::UpdateD3DProperties(state);
			InitHookBase();
		}
		if (bDeviceJustReset) {
			ImGui_ImplDX9_CreateDeviceObjects();
			bDeviceJustReset = false;
		}

		const int menuSize = 24;

		bool hasDescription = state->descriptionLabel && state->descriptionLabel[0];
		ChloeMenuLib::SetMenuYSize(hasDescription ? menuSize - 1 : menuSize);

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

		int menuBoxSize = menuSize + 1;

		const char* aTextures[] = {
			"MenuStyles/MenuStyleBubbly/CherryBlossomsLight.png",
			"MenuStyles/MenuStyleBubbly/CherryBlossomsMagenta.png",
			"MenuStyles/MenuStyleBubbly/CherryBlossomsDark.png",
			"MenuStyles/MenuStyleBubbly/Tetrus.png",
		};
		static auto tex = LoadTexture(aTextures[texType]);
		fMenuBoxSpacing = 0.02 * GetAspectRatioInv();
		auto fMenuBorderLeft = fMenuLeft - fMenuBoxSpacing;
		auto fMenuBorderRight = 1 - fMenuBorderLeft;
		fMenuBorderLeft += state->posX - 0.5;
		fMenuBorderRight += state->posX - 0.5;
		DrawRectangle(fMenuBorderLeft, fMenuBorderRight, state->posY - (fMenuTextSize * 3.5), state->posY + (fMenuTextSize * (menuBoxSize + 2.5)), {255,255,255,255}, texType == 3 ? 0 : 0.02, tex);

		// highlighted option
		{
			auto height = fMenuTextSize;
			auto y = state->posY + (fMenuTextSize * (state->menuSelectedOptionVisual - state->menuScroll));

			DrawRectangle(fMenuBorderLeft, fMenuBorderRight, y - (height * 0.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		// top border
		{
			auto height = fMenuTextSize;
			auto y = state->posY + (fMenuTextSize * -2);

			DrawRectangle(fMenuBorderLeft, fMenuBorderRight, y - (height * 0.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		// hints border
		{
			auto height = fMenuTextSize;
			auto y = state->posY + (fMenuTextSize * (menuBoxSize + 1));

			DrawRectangle(fMenuBorderLeft, fMenuBorderRight, y - (height * 1.5), y + (height * 0.5),
						  {0, 0, 0, 127});
		}

		if (hasDescription) {
			auto height = fMenuTextSize;
			auto y = state->posY + (fMenuTextSize * (menuBoxSize - 1));

			DrawRectangle(fMenuBorderLeft, fMenuBorderRight, y - (height * 0.5), y + (height * 0.5),
						  {0, 0, 0, 127});

			auto data = GetDefaultStringData(state);
			data.x = state->posX;
			data.y = state->posY;
			data.size = fMenuTextSize;
			data.y += data.size * (menuBoxSize - 1);
			data.XCenterAlign = true;
			DrawString(data, state->descriptionLabel);
		}

		{
			auto data = GetDefaultStringData(state);
			data.x = state->posX;
			data.y = state->posY;
			data.size = fMenuTextSize;
			data.y += data.size * menuBoxSize;
			data.XCenterAlign = true;
			DrawString(data, std::format("{}/{}", state->menuSelectedOption + 1, numSelectableMenuOptionsDrawn));
		}

		// menu title
		{
			auto data = GetDefaultStringData(state);
			data.x = state->posX;
			data.y = state->posY;
			data.size = fMenuTextSize;
			data.y += data.size * -2;
			data.XCenterAlign = true;
			DrawString(data, state->firstSubmenuName && state->firstSubmenuName[0] ? state->firstSubmenuName : state->libVersion);
		}

		// menu prompts
		{
			auto data = GetDefaultStringData(state);
			data.x = state->posX;
			data.y = state->posY;
			data.size = fMenuTextSize;
			data.y += data.size * (menuBoxSize + 1);
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
			ChloeMenuLib::RegisterMenuStyle("Bubbly Light Pink", MenuStyleDefault::Draw<0>);
			ChloeMenuLib::RegisterMenuStyle("Bubbly Dark Magenta", MenuStyleDefault::Draw<1>);
			ChloeMenuLib::RegisterMenuStyle("Bubbly Dark Blue", MenuStyleDefault::Draw<2>);
			ChloeMenuLib::RegisterMenuStyle("Bubbly Tetrus", MenuStyleDefault::Draw<3>);
			ChloeMenuLib::RegisterD3DReset(OnD3DReset);
		} break;
		default:
			break;
	}
	return TRUE;
}