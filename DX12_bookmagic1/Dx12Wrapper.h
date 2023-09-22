#pragma once

//	インクルード
#include <DirectXTex.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <map>
#include <string>
#include <array>

//#include <DirectXMath.h>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")


//	クラス宣言
class Dx12Wrapper
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	構造体	//

	//	テクスチャー無の場合のテクスチャーバッファ種類
	enum class E_NONETEX : int
	{
		WHITE,
		BLACK,
		GRADUATION
	};

	//	関数	//
	//	コンストラクタ
	Dx12Wrapper();
	/*	初期化関連の処理	*/
	//	初期化処理
	void Init(HWND hwnd);

	/*	描画関連の処理	*/
	//	クリア
	void Clear(void);
	//	フリップ
	void Flip(void);
	//	バリア設定処理
	void SetBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES pre, D3D12_RESOURCE_STATES dest);

	/*	情報取得関連の処理*/
	//	スワップチェインの取得
	ComPtr<IDXGISwapChain4> GetSwapchain(void) { return _swapchain; }
	//	デバイスの取得
	ComPtr<ID3D12Device> GetDevice(void) { return _dev; }
	//	コマンドリストの取得
	ComPtr<ID3D12GraphicsCommandList> GetCmdList(void) { return _cmdList; }
	//	コマンドキューの取得
	ComPtr<ID3D12CommandQueue> GetCmdQue(void) { return _cmdQueue; }
	//	バックバッファのディスク情報取得
	D3D12_RESOURCE_DESC GetBackDesc(void) { return _backBuffers[0]->GetDesc(); }
	//	ディスクリプタヒープのディスク情報取得
	D3D12_DESCRIPTOR_HEAP_DESC GetDescriptorHeapD(void) { return rtvHeaps->GetDesc(); }
	//	テクスチャー読み込み
	ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//	各テクスチャ無バッファ取得
	ComPtr<ID3D12Resource> GetNoneTexture(E_NONETEX nTex) { return _noneTexTable[static_cast<int>(nTex)]; }
	//	Imguiのヒープ取得
	ComPtr<ID3D12DescriptorHeap> GetHeapForImgui() { return _heapForImgui; }

	//	背景色のゲッターセッター
	float* GetBgCol(void) { return _bgColor; };
	void SetBgCol(float bgCol[4]) { std::copy_n(bgCol, 4, std::begin(_bgColor)); }

private:

	//	関数	//
	/*	初期化関連の処理	*/
	//	デバッグレイヤー有効化処理
	void EnableDebugLayer(void);
	//	デバイスの作成
	void CreateDevice(void);
	//	コマンドリストなどの作成
	void CreateCommand(void);
	//	スワップチェインの作成
	void CreateSwapchain(HWND hwnd);
	//	imgui
	void CreateDescriptorHeapForImgui(void);

	ComPtr < ID3D12Resource> CreateWhiteTexture(void);			//	白テクスチャーバッファ作成
	ComPtr < ID3D12Resource> CreateBlackTexture(void);			//	黒テクスチャーバッファ作成
	ComPtr < ID3D12Resource> CreateGrayGradationTexture(void);	//	グラデーションバッファ作成

	//	変数	//
	ComPtr<ID3D12Device> _dev = nullptr;						//	デバイス
	ComPtr<IDXGIFactory6> _dxgifactory = nullptr;				//	ファクトリー
	ComPtr<IDXGISwapChain4> _swapchain = nullptr;				//	スワップチェイン
	ComPtr < ID3D12CommandAllocator> _cmdAllocator = nullptr;	//	コマンドアロケーター
	ComPtr < ID3D12GraphicsCommandList> _cmdList = nullptr;		//	コマンドリスト
	ComPtr < ID3D12CommandQueue> _cmdQueue = nullptr;			//	コマンドキュー
	ComPtr < ID3D12DescriptorHeap> rtvHeaps = nullptr;			//	ディスクリプタヒープ（レンダーターゲット）
	std::vector< ID3D12Resource*> _backBuffers;			//	バックバッファ
	ComPtr < ID3D12Fence> _fence = nullptr;						//	フェンス
	UINT64 _fenceVal = 0;								//	フェイス値
	float _bgColor[4] = { 0.5f,0.5f,0.5f,1.0f };		//	背景カラー

	//	imgui
	ComPtr<ID3D12DescriptorHeap> _heapForImgui;	//	ヒープ保持用

	//	テクスチャーバッファのターブルなど	//
	std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;	//	ファイル名パスとリソースのマップテーブル
	ComPtr < ID3D12Resource> _noneTexTable[3];				//	ファイルなしテクスチャバッファ
};