#include <windows.h>
#include <iostream>
#include <crtdbg.h>
#include <memory>
#include "core/core_loop.hpp"

int CommonAppMain() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UINT saveOutCP = GetConsoleOutputCP();
	UINT saveCP = GetConsoleCP();

	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);

	int returnCode = 0;
	bool isReset = false;
	std::unique_ptr<Pame::Core::CoreLoop> coreLoop;
	try {
		do {
			coreLoop.reset();
			coreLoop = std::make_unique<Pame::Core::CoreLoop>();

			coreLoop->Execute();
			isReset = coreLoop->IsResetRequired();
		} while (isReset);
	}
	catch (...) {
		returnCode = -1;
	}

	SetConsoleOutputCP(saveOutCP);
	SetConsoleCP(saveCP);

	return returnCode;
}

// Release build entry point
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return CommonAppMain();
}

// Debug build entry point
int main(int argc, char** argv) {
	return CommonAppMain();
}
