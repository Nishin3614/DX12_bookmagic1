//	インクルード
#include "Win32Application.h"
#include <tchar.h>
#include "helper.h"
#ifdef _DEBUG
#include <iostream>
#endif

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

#endif // _DEBUG
	Win32Application& WinApp = Win32Application::Instance();
	WinApp.Run(nullptr);

	Helper::DebugOutputFormatString("Show window test.");
	getchar();	//	キーボード入力待ち
	return 0;
}