namespace ChloeMenuLib {
	template<typename T>
	T GetFuncPtr(const char* funcName) {
		if (auto dll = LoadLibraryA("FlatOutUCMenuLib_gcp.asi")) {
			return (T)GetProcAddress(dll, funcName);
		}
		return nullptr;
	}

	struct tMenuOption {
		size_t size = sizeof(tMenuOption);
		const char* string = nullptr;
		bool hoverOnly = false;
		bool nonSelectable = false;
		bool isSubmenu = true;
	};

	void RegisterMenu(const char* label, void(*func)()) {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)(const char*, void(*)())>("ChloeMenuLib_RegisterMenu");
		if (!funcPtr) return;
		funcPtr(label, func);
	}

	void SetEnterHint(const char* label) {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)(const char*)>("ChloeMenuLib_SetEnterHint");
		if (!funcPtr) return;
		funcPtr(label);
	}

	void SetLRScrollHint(const char* label) {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)(const char*)>("ChloeMenuLib_SetLRScrollHint");
		if (!funcPtr) return;
		funcPtr(label);
	}

	void BeginMenu() {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)()>("ChloeMenuLib_BeginNewMenu");
		if (!funcPtr) return;
		funcPtr();
	}

	void EndMenu() {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)()>("ChloeMenuLib_EndNewMenu");
		if (!funcPtr) return;
		funcPtr();
	}

	bool DrawMenuOption(const tMenuOption& menu) {
		static auto funcPtr = GetFuncPtr<bool(__cdecl*)(const tMenuOption*)>("ChloeMenuLib_DrawMenuOption");
		if (!funcPtr) return false;
		return funcPtr(&menu);
	}

	void AddTextInputToString(char* str, int len, bool numbersOnly) {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)(char*, int, bool)>("ChloeMenuLib_AddTextInputToString");
		if (!funcPtr) return;
		funcPtr(str, len, numbersOnly);
	}

	bool GetMoveLeft() {
		static auto funcPtr = GetFuncPtr<bool(__cdecl*)()>("ChloeMenuLib_GetMoveLeft");
		if (!funcPtr) return false;
		return funcPtr();
	}

	bool GetMoveRight() {
		static auto funcPtr = GetFuncPtr<bool(__cdecl*)()>("ChloeMenuLib_GetMoveRight");
		if (!funcPtr) return false;
		return funcPtr();
	}

	void BackOut() {
		static auto funcPtr = GetFuncPtr<void(__cdecl*)()>("ChloeMenuLib_BackOut");
		if (!funcPtr) return;
		funcPtr();
	}
}

bool DrawMenuOption(const std::string& label, bool grayedOut = false, bool isSubmenu = true) {
	ChloeMenuLib::tMenuOption menu;
	menu.nonSelectable = grayedOut;
	menu.string = label.c_str();
	menu.isSubmenu = isSubmenu;
	return ChloeMenuLib::DrawMenuOption(menu);
}