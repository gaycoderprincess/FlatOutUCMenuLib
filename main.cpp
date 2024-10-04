#include <windows.h>
#include <d3d9.h>
#include <mutex>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "fouc.h"
#include "include/chloemenulib.h"

struct tMenuConfig {
	std::string defaultStyle = "Default";
	float xPos = 0.5;
	float yPos = 0.18;
} gConfig;

int nMenuYSize = 16;

const int nNumMenuLevels = 128;
struct tMenuState {
	int nSelectedOption = 0;
	int nTempOptionCounter = 0;
	int nTempOptionCounterVisual = 0;
	int nMenuScroll = 0;
	int optionLocations[512];
} aMenuStates[nNumMenuLevels];
int nCurrentMenuLevel = 0;
int nTempLevelCounter = 0;

bool bMenuUp = false;
std::string sEnterHint;
std::string sLRScrollHint;

std::mutex mDLLExportMutex;
struct tImportedMenu {
	std::string label;
	void(*pFunction)();
};
std::vector<tImportedMenu> aMenus;

bool IsTypeableCharacterInFO2(wchar_t c) {
	// number row
	if (c == '`') return true;
	if (c == '1') return true;
	if (c == '2') return true;
	if (c == '3') return true;
	if (c == '4') return true;
	if (c == '5') return true;
	if (c == '6') return true;
	if (c == '7') return true;
	if (c == '8') return true;
	if (c == '9') return true;
	if (c == '0') return true;
	if (c == '-') return true;
	if (c == '=') return true;

	// number row + shift
	if (c == '~') return true;
	if (c == '!') return true;
	if (c == '@') return true;
	if (c == '#') return true;
	if (c == '$') return true;
	//if (c == '%') return true; // used for printf
	if (c == '^') return true;
	if (c == '&') return true;
	if (c == '*') return true;
	if (c == '(') return true;
	if (c == ')') return true;
	if (c == '_') return true;
	if (c == '+') return true;

	// letters
	if (c >= 'a' && c <= 'z') return true;
	if (c >= 'A' && c <= 'Z') return true;

	// symbols next to enter
	if (c == '[') return true;
	if (c == ']') return true;
	if (c == ';') return true;
	if (c == '\'') return true;
	if (c == '\\') return true;
	if (c == ',') return true;
	if (c == '.') return true;
	if (c == '/') return true;
	if (c == '{') return true;
	if (c == '}') return true;
	if (c == ':') return true;
	if (c == '"') return true;
	if (c == '|') return true;
	if (c == '<') return true;
	if (c == '>') return true;
	if (c == '?') return true;

	// spacebar
	if (c == ' ') return true;
	return false;
}

bool IsTypeableNumberInFO2(wchar_t c) {
	if (c == '1') return true;
	if (c == '2') return true;
	if (c == '3') return true;
	if (c == '4') return true;
	if (c == '5') return true;
	if (c == '6') return true;
	if (c == '7') return true;
	if (c == '8') return true;
	if (c == '9') return true;
	if (c == '0') return true;
	if (c == '-') return true;
	if (c == '.') return true;
	return false;
}

bool IsStringValidForFO2Drawing(const std::string& str, bool numbersOnly) {
	if (numbersOnly) {
		for (auto& c: str) {
			if (!IsTypeableNumberInFO2(c)) return false;
		}
	}
	else {
		for (auto& c: str) {
			if (!IsTypeableCharacterInFO2(c)) return false;
		}
	}
	return true;
}

std::string GetClipboardText() {
	if (!OpenClipboard(nullptr)) return "";

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (!hData) return "";

	auto pszText = (char*)GlobalLock(hData);
	if (!pszText) return "";

	std::string text(pszText);
	GlobalUnlock(hData);
	CloseClipboard();
	return text;
}

void AddTextInputToString(char* str, int len, bool numbersOnly) {
	std::string text = str;
	if (text.length() < len - 1 && IsStringValidForFO2Drawing(sKeyboardInput, numbersOnly)) text += sKeyboardInput;
	if (IsKeyJustPressed(VK_BACK) && !text.empty()) text.pop_back();
	if (IsKeyPressed(VK_CONTROL) && IsKeyJustPressed('V')) {
		text += GetClipboardText();
	}
	if (text.length() < len && IsStringValidForFO2Drawing(text, numbersOnly)) strcpy_s(str, len, text.c_str());
}

tMenuState* GetMenuState() {
	return &aMenuStates[nCurrentMenuLevel];
}

struct tMenuOption {
	size_t size = sizeof(tMenuOption);
	const char* label = nullptr;
	bool hoverOnly = false;
	bool nonSelectable = false;
	bool isSubmenu = true;
};

std::vector<ChloeMenuLib::tMenuOptionDraw> aMenuOptionsForDrawing;

struct tMenuStyleDef {
	std::string name;
	void(*func)(ChloeMenuLib::tMenuStyleState*);
};
std::vector<tMenuStyleDef> aMenuStyles;
std::vector<void(*)()> aMenuD3DResets;

void(*pCurrentMenuStyle)(ChloeMenuLib::tMenuStyleState*) = nullptr;

void BeginNewMenu() {
	nTempLevelCounter++;
}

void EndNewMenu() {
	nTempLevelCounter--;
}

bool bLastDrawnOptionHighlighted = false;
bool DrawMenuOption(const tMenuOption& menu) {
	bLastDrawnOptionHighlighted = false;
	if (nTempLevelCounter < 0) return false;

	auto menuState = &aMenuStates[nTempLevelCounter];
	if (menuState->nTempOptionCounter >= 512) return false;
	menuState->optionLocations[menuState->nTempOptionCounter] = menuState->nTempOptionCounterVisual;
	auto selected = menuState->nTempOptionCounter == menuState->nSelectedOption;
	if (menu.nonSelectable) selected = false;

	ChloeMenuLib::tMenuOptionDraw optDraw;
	optDraw.label = menu.label;
	optDraw.yAbsolute = menuState->nTempOptionCounterVisual;
	optDraw.y = optDraw.yAbsolute - menuState->nMenuScroll;
	optDraw.level = nTempLevelCounter;
	optDraw.nonSelectable = menu.nonSelectable;
	optDraw.isHighlighted = selected;
	aMenuOptionsForDrawing.push_back(optDraw);

	bool retValue = false;
	if (selected) {
		if (nCurrentMenuLevel == nTempLevelCounter) bLastDrawnOptionHighlighted = true;

		retValue = menu.hoverOnly || nCurrentMenuLevel > nTempLevelCounter;
		if (!menu.isSubmenu && nCurrentMenuLevel > nTempLevelCounter) {
			nCurrentMenuLevel--;
		}
	}
	if (!menu.nonSelectable) menuState->nTempOptionCounter++;
	menuState->nTempOptionCounterVisual++;
	return retValue;
}

void DisableKeyboardInput(bool disable) {
	NyaHookLib::Patch<uint64_t>(0x5AEB2F, disable ? 0x68A190000001BCE9 : 0x68A1000001BB8C0F);
}

void SetMenuScroll() {
	auto menu = GetMenuState();
	int tmp = menu->nMenuScroll + nMenuYSize;
	while (tmp < menu->optionLocations[menu->nSelectedOption]) {
		menu->nMenuScroll++;
		tmp = menu->nMenuScroll + nMenuYSize;
	}
	while (menu->nMenuScroll > menu->optionLocations[menu->nSelectedOption]) {
		menu->nMenuScroll--;
	}
}

void LoadMenuStyles() {
	for (const auto& entry : std::filesystem::directory_iterator("MenuStyles")) {
		if (entry.is_directory()) continue;

		const auto& path = entry.path();
		if (path.extension() != ".dll") continue;
		LoadLibraryW(path.c_str());
	}

	for (auto& style : aMenuStyles) {
		if (style.name == gConfig.defaultStyle) pCurrentMenuStyle = style.func;
	}
	if (!pCurrentMenuStyle && !aMenuStyles.empty()) pCurrentMenuStyle = aMenuStyles[0].func;
}

CNyaTimer gTimer;

bool GetMenuMoveLeft() {
	static double fHoldMoveTimer = 0;
	if (IsKeyPressed(VK_LEFT)) {
		if (IsKeyJustPressed(VK_LEFT)) {
			fHoldMoveTimer = 0;
			return true;
		}
		fHoldMoveTimer += gTimer.fDeltaTime;
		if (fHoldMoveTimer > 0.2) {
			fHoldMoveTimer -= 0.2;
			return true;
		}
	}
	return false;
}

bool GetMenuMoveRight() {
	static double fHoldMoveTimer = 0;
	if (IsKeyPressed(VK_RIGHT)) {
		if (IsKeyJustPressed(VK_RIGHT)) {
			fHoldMoveTimer = 0;
			return true;
		}
		fHoldMoveTimer += gTimer.fDeltaTime;
		if (fHoldMoveTimer > 0.2) {
			fHoldMoveTimer -= 0.2;
			return true;
		}
	}
	return false;
}

int GetMenuMoveLR() {
	int n = 0;
	if (GetMenuMoveLeft()) n--;
	if (GetMenuMoveRight()) n++;
	return n;
}

void MenuLibLoop() {
	static bool bOnce = true;
	if (bOnce) {
		LoadMenuStyles();
		bOnce = false;
	}
	if (!pCurrentMenuStyle) return;

	gTimer.Process();
	static double fHoldMoveTimer = 0;

	if (IsKeyJustPressed(VK_F5)) {
		bMenuUp = !bMenuUp;
		if (!bMenuUp) DisableKeyboardInput(false);
	}
	if (!bMenuUp) return;
	DisableKeyboardInput(true);

	if (IsKeyJustPressed(VK_ESCAPE)) {
		nCurrentMenuLevel--;
		if (nCurrentMenuLevel < 0) nCurrentMenuLevel = 0;
	}
	if (IsKeyJustPressed(VK_RETURN)) {
		nCurrentMenuLevel++;
	}

	auto menuState = GetMenuState();
	if (IsKeyPressed(VK_UP)) {
		if (IsKeyJustPressed(VK_UP)) {
			menuState->nSelectedOption--;
			fHoldMoveTimer = 0;
		}
		fHoldMoveTimer += gTimer.fDeltaTime;
		if (fHoldMoveTimer > 0.2) {
			menuState->nSelectedOption--;
			fHoldMoveTimer -= 0.2;
		}
	}
	if (IsKeyPressed(VK_DOWN)) {
		if (IsKeyJustPressed(VK_DOWN)) {
			menuState->nSelectedOption++;
			fHoldMoveTimer = 0;
		}
		fHoldMoveTimer += gTimer.fDeltaTime;
		if (fHoldMoveTimer > 0.2) {
			menuState->nSelectedOption++;
			fHoldMoveTimer -= 0.2;
		}
	}
	if (menuState->nSelectedOption < 0) menuState->nSelectedOption = menuState->nTempOptionCounter - 1;
	if (menuState->nSelectedOption >= menuState->nTempOptionCounter) {
		menuState->nSelectedOption = 0;
		menuState->nMenuScroll = 0;
	}

	SetMenuScroll();

	nTempLevelCounter = -1;
	for (auto& state : aMenuStates) {
		state.nTempOptionCounter = 0;
		state.nTempOptionCounterVisual = 0;
		memset(state.optionLocations,0,sizeof(state.optionLocations));
	}
	sEnterHint = "Select";
	sLRScrollHint = "";

	for (auto& menu : aMenus) {
		tMenuOption opt;
		opt.label = menu.label.c_str();
		BeginNewMenu();
		if (DrawMenuOption(opt)) {
			menu.pFunction();
		}
		EndNewMenu();
		nTempLevelCounter = -1;
	}

	if (aMenuStyles.size() > 1) {
		BeginNewMenu();
		tMenuOption opt;
		opt.label = "Menu Config";
		if (DrawMenuOption(opt)) {
			BeginNewMenu();
			opt.label = "Menu Styles";
			if (DrawMenuOption(opt)) {
				BeginNewMenu();
				for (auto& style : aMenuStyles) {
					opt.label = style.name.c_str();
					opt.isSubmenu = false;
					if (DrawMenuOption(opt)) {
						pCurrentMenuStyle = style.func;
					}
				}
				EndNewMenu();
			}
			auto str = std::format("Menu X Position < {:.2f} >", gConfig.xPos);
			opt.label = str.c_str();
			opt.isSubmenu = false;
			DrawMenuOption(opt);
			if (bLastDrawnOptionHighlighted) {
				gConfig.xPos += GetMenuMoveLR() * 0.02;
			}
			str = std::format("Menu Y Position < {:.2f} >", gConfig.yPos);
			opt.label = str.c_str();
			opt.isSubmenu = false;
			DrawMenuOption(opt);
			if (bLastDrawnOptionHighlighted) {
				gConfig.yPos += GetMenuMoveLR() * 0.02;
			}
			EndNewMenu();
		}
		EndNewMenu();
	}

	menuState = GetMenuState();
	if (menuState->nTempOptionCounterVisual == 0 && nCurrentMenuLevel == 0) {
		BeginNewMenu();
		tMenuOption opt;
		opt.label = "There doesn't seem to be anything here...";
		opt.isSubmenu = false;
		DrawMenuOption(opt);
		sEnterHint = "";
		EndNewMenu();
	}

	if (menuState->nTempOptionCounterVisual == 0 && nCurrentMenuLevel > 0) {
		nCurrentMenuLevel--;
	}

	ChloeMenuLib::tMenuStyleState state;
	state.menuOptions = &aMenuOptionsForDrawing[0];
	state.numMenuOptions = aMenuOptionsForDrawing.size();
	state.menuLevel = nCurrentMenuLevel;
	state.menuYSize = nMenuYSize;
	state.menuScroll = menuState->nMenuScroll;
	state.menuSelectedOption = menuState->nSelectedOption;
	state.menuSelectedOptionVisual = menuState->optionLocations[menuState->nSelectedOption];
	state.libVersion = "FlatOut UC Menu Lib 1.0";
	state.enterHint = sEnterHint.c_str();
	state.lrHint = sLRScrollHint.c_str();
	state.backHint = nCurrentMenuLevel > 0 ? "Back" : "";
	state.g_pd3dDevice = g_pd3dDevice;
	state.ghWnd = ghWnd;
	state.nResX = nResX;
	state.nResY = nResY;
	state.posX = gConfig.xPos;
	state.posY = gConfig.yPos;
	pCurrentMenuStyle(&state);

	aMenuOptionsForDrawing.clear();
}

void HookLoop() {
	mDLLExportMutex.lock();
	MenuLibLoop();
	CommonMain();
	mDLLExportMutex.unlock();
}

void UpdateD3DProperties() {
	g_pd3dDevice = *(IDirect3DDevice9**)(0x7242B0 + 0x60);
	ghWnd = *(HWND*)(0x7242B0 + 0x7C);
	nResX = *(int*)0x764A84;
	nResY = *(int*)0x764A88;
}

bool bDeviceJustReset = false;
void D3DHookMain() {
	if (!g_pd3dDevice) {
		UpdateD3DProperties();
		InitHookBase();
	}

	if (bDeviceJustReset) {
		ImGui_ImplDX9_CreateDeviceObjects();
		bDeviceJustReset = false;
	}
	HookBaseLoop();
}

void OnEndScene() {
	auto bak = *(float*)0x716034;
	*(float*)0x716034 = 480.1f; // hack to fix font scale in chloe collection
	D3DHookMain();
	*(float*)0x716034 = bak;
}

void OnD3DReset() {
	if (g_pd3dDevice) {
		UpdateD3DProperties();
		ImGui_ImplDX9_InvalidateDeviceObjects();
		bDeviceJustReset = true;
	}
	for (auto& reset : aMenuD3DResets) {
		reset();
	}
}

#include "exports.h"

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x24CEF7) {
				MessageBoxA(nullptr, aFOUCVersionFail, "nya?!~", MB_ICONERROR);
				exit(0);
				return TRUE;
			}

			static auto config = toml::parse_file("FlatOutUCMenuLib_gcp.toml");
			std::string style = config["main"]["default_style"].value_or("Default");
			if (!style.empty()) gConfig.defaultStyle = style;
			gConfig.xPos = config["main"]["x_pos"].value_or(0.5);
			gConfig.yPos = config["main"]["y_pos"].value_or(0.18);

			NyaFO2Hooks::PlaceD3DHooks();
			NyaFO2Hooks::aEndSceneFuncs.push_back(OnEndScene);
			NyaFO2Hooks::aD3DResetFuncs.push_back(OnD3DReset);
			NyaFO2Hooks::PlaceWndProcHook();
			NyaFO2Hooks::aWndProcFuncs.push_back(WndProcHook);
		} break;
		default:
			break;
	}
	return TRUE;
}