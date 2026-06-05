#include <iostream>

#include "WinInclude.h"
#include "MinimalDXApp.h"

int main()
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	int nCmdShow = SW_SHOW;

	MinimalDXApp app;
	app.createWindow(hInstance, nCmdShow);
	app.initializeDX();
	app.loadAssets();
	app.run();
}
