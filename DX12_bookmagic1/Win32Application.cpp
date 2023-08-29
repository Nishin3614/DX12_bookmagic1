//	インクルード
#include "Win32Application.h"
#include "DXApplication.h"

//	定数宣言
const unsigned int window_width = 1280;
const unsigned int window_height = 780;

//	インスタンスを取得
Win32Application& Win32Application::Instance(void)
{
	static Win32Application instance;
	return instance;
}

//	コンストラクタ
Win32Application::Win32Application()
{

}

//	アプリケーション実行処理
void Win32Application::Run(HINSTANCE hInstance)
{
	//	ウィンドウクラスの生成&登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	//	コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");			//	アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);		//	ハンドルの取得

	RegisterClassEx(&w);	//	アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0,0,window_width,window_height };	//	ウィンドウサイズを決める

	//	関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//	ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,		//	クラス名指定
		_T("DX12テスト"),		//	タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//	タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			//	表示x座標はOSにおまかせ
		CW_USEDEFAULT,			//	表示y座標はOSにおまかせ
		wrc.right - wrc.left,	//	ウィンドウ幅
		wrc.bottom - wrc.top,	//	ウィンドウ高
		nullptr,				//	親ウィンドウハンドル
		nullptr,				//	メニューハンドル
		w.hInstance,			//	呼び出しアプリケーションハンドル
		nullptr);				//	追加パラメーター	

	//	ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	//	デバイスの初期化処理　
	//	※デバイスの作成に失敗した際、アプリケーションを終了させる処理がまだ作成されていない
	DXApplication& DxApi = DXApplication::Instance();
	DxApi.OnInit(hwnd,window_width,window_height);

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//	更新処理
			DxApi.OnUpdate();
			//	描画処理
			DxApi.OnRender();
		}
		//	アプリケーションが終わるときにmessageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	DxApi.OnRelease();
}

//	ウィンドウサイズの取得
SIZE Win32Application::GetWindowSize(void)
{
	SIZE rec;
	rec.cx = window_width;
	rec.cy = window_height;
	return rec;
}


LRESULT Win32Application::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//	ウィンドウが破棄されたら呼ばれる処理
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);	//	OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//	既定の処理を行う
}
