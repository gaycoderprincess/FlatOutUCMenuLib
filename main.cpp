#include <windows.h>
#include <d3d9.h>
#include <mutex>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "fouc.h"

float fMenuYTop = 0.18;
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
	const char* string = nullptr;
	bool hoverOnly = false;
	bool nonSelectable = false;
	bool isSubmenu = true;
};

void BeginNewMenu() {
	nTempLevelCounter++;
}

void EndNewMenu() {
	nTempLevelCounter--;
}

bool DrawMenuOption(const tMenuOption& menu) {
	if (nTempLevelCounter < 0) return false;
	if (nCurrentMenuLevel < nTempLevelCounter) return false;

	auto menuState = &aMenuStates[nTempLevelCounter];
	menuState->optionLocations[menuState->nTempOptionCounter] = menuState->nTempOptionCounterVisual;
	auto selected = menuState->nTempOptionCounter == menuState->nSelectedOption;
	if (menu.nonSelectable) selected = false;
	int scroll = menuState->nTempOptionCounterVisual - menuState->nMenuScroll;
	if (scroll >= 0 && scroll <= nMenuYSize && nTempLevelCounter == nCurrentMenuLevel) {
		tNyaStringData data;
		data.x = 0.5;
		data.y = fMenuYTop;
		data.size = 0.03;
		data.y += data.size * scroll;
		data.XCenterAlign = true;
		if (selected) {
			data.SetColor(241, 193, 45, 255);
		}
		else if (menu.nonSelectable) {
			data.SetColor(255, 255, 255, 255);
		}
		else {
			data.SetColor(127, 127, 127, 255);
		}
		DrawStringFO2(data, menu.string);
	}
	bool retValue = false;
	if (selected) {
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

void ResetMenuScroll() {
	auto menu = GetMenuState();
	menu->nMenuScroll = 0;
	menu->nSelectedOption = 0;
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

void MenuLibLoop() {
	static CNyaTimer gTimer;
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
	if (menuState->nSelectedOption >= menuState->nTempOptionCounter) menuState->nSelectedOption = 0;

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
		opt.string = menu.label.c_str();
		BeginNewMenu();
		if (DrawMenuOption(opt)) {
			menu.pFunction();
		}
		EndNewMenu();
		nTempLevelCounter = -1;
	}

	if (menuState->nTempOptionCounterVisual == 0 && nCurrentMenuLevel == 0) {
		BeginNewMenu();
		tMenuOption opt;
		opt.string = "There doesn't seem to be anything here...";
		opt.isSubmenu = false;
		DrawMenuOption(opt);
		sEnterHint = "";
		EndNewMenu();
	}

	if (menuState->nTempOptionCounterVisual == 0 && nCurrentMenuLevel > 0) {
		nCurrentMenuLevel--;
	}

	if (menuState->nMenuScroll > 0) {
		tNyaStringData data;
		data.x = 0.5;
		data.y = fMenuYTop;
		data.size = 0.03;
		data.y += data.size * -1;
		data.XCenterAlign = true;
		DrawStringFO2(data, "...");
	}
	if ((menuState->nTempOptionCounterVisual - menuState->nMenuScroll) > nMenuYSize + 1) {
		tNyaStringData data;
		data.x = 0.5;
		data.y = fMenuYTop;
		data.size = 0.03;
		data.y += data.size * (nMenuYSize + 1);
		data.XCenterAlign = true;
		DrawStringFO2(data, "...");
	}

	// menu title
	{
		tNyaStringData data;
		data.x = 0.5;
		data.y = fMenuYTop;
		data.size = 0.03;
		data.y += data.size * -2;
		data.XCenterAlign = true;
		DrawStringFO2(data, "FlatOut UC Menu Lib 1.0");
	}

	// menu prompts
	{
		tNyaStringData data;
		data.x = 0.5;
		data.y = fMenuYTop;
		data.size = 0.03;
		int max = menuState->nTempOptionCounterVisual;
		if (max > nMenuYSize + 1) max = nMenuYSize + 1;
		data.y += data.size * (max + 1);
		data.XCenterAlign = true;
		std::string str;
		if (nCurrentMenuLevel > 0) str += "[ESC] Back";
		if (!sLRScrollHint.empty()) {
			if (!str.empty()) str += " ";
			str += "[LEFT/RIGHT] " + sLRScrollHint;
		}
		if (!sEnterHint.empty()) {
			if (!str.empty()) str += " ";
			str += "[ENTER] " + sEnterHint;
		}
		DrawStringFO2(data, str);
	}
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
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_RegisterMenu(const char* label, void(*func)()) {
	mDLLExportMutex.lock();
	for (auto& menu : aMenus) {
		if (menu.label == label && menu.pFunction == func) {
			mDLLExportMutex.unlock();
			return;
		}
	}
	tImportedMenu menu;
	menu.label = label;
	menu.pFunction = func;
	aMenus.push_back(menu);
	mDLLExportMutex.unlock();
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_SetEnterHint(const char* label) {
	sEnterHint = label;
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_SetLRScrollHint(const char* label) {
	sLRScrollHint = label;
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_BeginNewMenu() {
	BeginNewMenu();
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_EndNewMenu() {
	EndNewMenu();
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_DrawMenuOption(const tMenuOption* menu) {
	if (menu->size < 11) return false;
	return DrawMenuOption(*menu);
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_AddTextInputToString(char* str, int len, bool numbersOnly) {
	AddTextInputToString(str, len, numbersOnly);
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_GetMoveLeft() {
	return IsKeyJustPressed(VK_LEFT);
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_GetMoveRight() {
	return IsKeyJustPressed(VK_RIGHT);
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_BackOut() {
	if (nCurrentMenuLevel > 0) nCurrentMenuLevel--;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x24CEF7) {
				MessageBoxA(nullptr, aFOUCVersionFail, "nya?!~", MB_ICONERROR);
				exit(0);
				return TRUE;
			}

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