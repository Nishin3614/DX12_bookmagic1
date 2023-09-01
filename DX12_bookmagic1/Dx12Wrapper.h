#pragma once

//	インクルード
#include <DirectXTex.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
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
	//	シェーダー側に渡すためお基本的な行列データ
	struct SceneMatrix
	{
		DirectX::XMMATRIX view;		//	ビュー行列
		DirectX::XMMATRIX proj;		//	プロジェクション行列
		DirectX::XMMATRIX lightCamera;	//	ライトから見たビュー
		DirectX::XMMATRIX shadow;	//	影
		DirectX::XMFLOAT3 eye;		//	視点座標
	};
	
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
	//	最終描画
	void Draw(void);
	//	クリア
	void Clear(void);
	//	フリップ
	void Flip(void);
	//	マルチパスレンダリング用描画1
	void PreOriginDraw(void);
	void EndOriginDraw(void);
	//	マルチパスレンダリング用描画2
	void ProceDraw(void);
	void ShadowDraw(void);
	//	シーンビューのセット命令
	void CommandSet_SceneView(void);
	//	縮小バッファぼかし描画処理
	void DrawShrinkTextureForBlur(void);

	/*	情報取得関連の処理*/
	//	スワップチェインの取得
	ComPtr<IDXGISwapChain4> GetSwapchain(void) { return _swapchain; }
	//	デバイスの取得
	ComPtr<ID3D12Device> GetDevice(void) { return _dev; }
	//	コマンドリストの取得
	ComPtr<ID3D12GraphicsCommandList> GetCmdList(void) { return _cmdList; }
	//	テクスチャー読み込み
	ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//	各テクスチャ無バッファ取得
	ComPtr<ID3D12Resource> GetNoneTexture(E_NONETEX nTex) { return _noneTexTable[static_cast<int>(nTex)]; }

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
	//	深度バッファの作成
	void CreateDepthView(void);
	//	ビュー・プロジェクション行列バッファの作成
	void CreateViewProjectionView(void);
	//	モデルなどの描画用のレンダーターゲットを作成
	void CreateOriginRenderTarget(void);
	void CreateProcessRenderTarget(void);
	//	ペラポリゴンの頂点バッファ作成
	void CreatePeraVertexBuff(void);
	//	ぼけ定数バッファ作成
	void CreateBokeConstantBuff(void);
	//	ペラポリゴン用のルートシグネイチャ作成
	void CreatePeraRootSignature(void);
	//	ペラポリゴン用のパイプライン作成
	void CreatePeraGraphicPipeLine(void);
	//	エフェクト用のバッファとビュー作成
	bool CreateEffectBufferAndView(void);
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
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	深度バッファ用のディスクリプタヒープ
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	深度バッファ

	ComPtr<ID3D12DescriptorHeap> _ScenevHeap = nullptr;	//	シーンディスクリプタ
	ComPtr<ID3D12Resource> _SceneBuffer = nullptr;		//	シーンバッファ
	SceneMatrix* _pMapSceneMtx;							//	シーン行列のマップ
	DirectX::XMFLOAT3 _parallelLightVec;				//	平行ライトの向き
	DirectX::XMFLOAT3 _eye;		//	視点
	DirectX::XMFLOAT3 _target;	//	注視点
	DirectX::XMFLOAT3 _up;		//	上ベクトル
	SIZE _windowSize;			//	ウィンドウサイズ


	//	テクスチャーバッファのターブルなど	//
	std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;	//	ファイル名パスとリソースのマップテーブル
	ComPtr < ID3D12Resource> _noneTexTable[3];				//	ファイルなしテクスチャバッファ


	//	モデルなどのレンダーターゲット先用の変数
	std::array<ComPtr<ID3D12Resource>,2> _origin1Resource;	//	貼りつけ元のリソース
											//	ペラポリゴンに張り付けるためのテクスチャリソース仮(この中に今まで作成してきたモデルやらが描画されている）
	ComPtr<ID3D12DescriptorHeap> _originRTVHeap;	//	レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> _originSRVHeap;	//	テクスチャ用

	//	ペラポリゴン作成用の変数
	ComPtr<ID3D12Resource> _prPoriVB;			//	ペラポリゴン頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _prPoriVBV;		//	ペラポリゴン頂点バッファビュー
	ComPtr<ID3D12RootSignature> _prPoriRS;	//	ペラポリゴン用ルートシグネイチャー
	ComPtr <ID3D12PipelineState> _prPoriPipeline;	//	ペラポリゴン用グラフィックパイプラインステート
	ComPtr<ID3D12Resource> _bokehParamBuffer;		//	ぼかし用の定数バッファ

	//	加工用のレンダーターゲット作成用の変数
	ComPtr<ID3D12Resource> _proceResource;	//	ペラポリゴンに張り付けられたリソースを加工するための変数
	ComPtr<ID3D12PipelineState> _procePipeline;	//	加工用グラフィックパイプライン

	//	歪みテクスチャー用
	ComPtr<ID3D12DescriptorHeap> _effectSRVHeap;
	ComPtr<ID3D12Resource> _efffectTexBuffer;
	ComPtr<ID3D12PipelineState> _effectPipeline;

	//	シャドウマップ用深度バッファ
	ComPtr<ID3D12Resource> _lightDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;		//	深度値テクスチャー用ヒープ

	//	ブルーム用バッファ
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//	ブルーム用バッファ
	ComPtr<ID3D12PipelineState> _blurPipeline;			//	画面全体ぼかし用パイプライン
};