#ifndef _H_WIN32APPLICATION_
#define _H_WIN32APPLICATION_

//	インクルード
#include <Windows.h>
#include <tchar.h>

//	クラス
class Win32Application
{
public:
	//	Applicationのシングルトンインスタンスを得る
	static Win32Application& Instance(void);

	//	アプリケーション実行処理
	void Run(HINSTANCE hInstance);

	//	ウィンドウのサイズを取得
	SIZE GetWindowSize(void);

private:	
	// シングルトンのため、コンストラクタをprivateに
	// さらにコピーと代入を禁止する
	Win32Application();
	Win32Application(const Win32Application&) = delete;
	void operator=(const Win32Application&) = delete;


	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

#endif