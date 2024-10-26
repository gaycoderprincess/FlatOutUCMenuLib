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

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_DrawMenuOption(const ChloeMenuLib::tMenuOption* menu) {
	if (menu->size < 11) return false;
	ChloeMenuLib::tMenuOption opt;
	memcpy(&opt, menu, std::min(menu->size, sizeof(opt)));
	return ::DrawMenuOption(opt);
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_AddTextInputToString(char* str, int len, bool numbersOnly) {
	AddTextInputToString(str, len, numbersOnly);
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_GetMoveLeft() {
	return GetMenuMoveLeft();
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_GetMoveRight() {
	return GetMenuMoveRight();
}

extern "C" __declspec(dllexport) int __cdecl ChloeMenuLib_GetMoveLR() {
	return GetMenuMoveLR();
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_BackOut() {
	if (nCurrentMenuLevel > 0) nCurrentMenuLevel--;
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_RegisterMenuStyle(const char* name, void(*func)(ChloeMenuLib::tMenuStyleState*)) {
	for (auto& style : aMenuStyles) {
		if (style.func == func) return;
	}
	aMenuStyles.push_back({name, "", func});
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_RegisterMenuStyleWithAuthor(const char* name, const char* author, void(*func)(ChloeMenuLib::tMenuStyleState*)) {
	for (auto& style : aMenuStyles) {
		if (style.func == func) return;
	}
	aMenuStyles.push_back({name, "Author: " + (std::string)author, func});
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_RegisterD3DReset(void(*func)()) {
	for (auto& reset : aMenuD3DResets) {
		if (reset == func) return;
	}
	aMenuD3DResets.push_back(func);
}

extern "C" __declspec(dllexport) int __cdecl ChloeMenuLib_GetMenuYSize() {
	return nMenuYSize;
}

extern "C" __declspec(dllexport) void __cdecl ChloeMenuLib_SetMenuYSize(int size) {
	nMenuYSize = size;
}

extern "C" __declspec(dllexport) bool __cdecl ChloeMenuLib_IsMenuOpen() {
	return bMenuUp;
}