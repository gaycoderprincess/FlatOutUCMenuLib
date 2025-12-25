#include <windows.h>
#include <d3d9.h>
#include <mutex>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "fouc.h"
#include "fo2versioncheck.h"
#include "include/chloemenulib.h"

void DisableKeyboardInput(bool disable) {
	// reset key status
	if (disable) {
		memset((void*)0x846060, 0, 0x100);
	}
	NyaHookLib::Patch<uint64_t>(0x5AEB2F, disable ? 0x68A190000001BCE9 : 0x68A1000001BB8C0F);
}

void D3DHookMain();
void OnEndScene() {
	auto bak = *(float*)0x716034;
	*(float*)0x716034 = 480.1f; // hack to fix font scale in chloe collection
	D3DHookMain();
	*(float*)0x716034 = bak;
}

void UpdateD3DProperties() {
	g_pd3dDevice = pDeviceD3d->pD3DDevice;
	ghWnd = pDeviceD3d->hWnd;
	nResX = nGameResolutionX;
	nResY = nGameResolutionY;
}

const char* versionString = "FlatOut UC Menu Lib 1.15";

#include "menulib.h"

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			DoFlatOutVersionCheck(FO2Version::FOUC_GFWL);

			if (!std::filesystem::exists("MenuStyles")) {
				MessageBoxA(nullptr, "FlatOutUCMenuLib: Failed to find any menu styles!", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			InitAndLoadConfig("FlatOutUCMenuLib_gcp.toml");
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