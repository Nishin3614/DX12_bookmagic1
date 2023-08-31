//	インクルード
#include "Dx12Wrapper.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "helper.h"

//	マルチパスレンダリング用
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

namespace//列挙型用
{
	//	オリジン用レンダーターゲットビュー種類
	enum class E_ORIGIN_RTV : int
	{
		COL,	//	通常カラー
		NORMAL,	//	法線
		MAX_NORMALDROW,

		BLOOM = MAX_NORMALDROW,
		//BLOOM2,
		MAX_BLOOM,

		PROCE = MAX_BLOOM,	//	加工用
		MAX
	};

	//	オリジン用SRV,CBV種類
	enum class E_ORIGIN_SRV : int
	{
		//	SRV
		COL,	//	通常カラー
		NORMAL,	//	法線
		MAX_NORMALDROW,

		BLOOM = MAX_NORMALDROW,
		BLOOM2,

		PROCE,	//	加工用

		//	CBV
		BOKE,

		MAX
	};

}

//	名前空間
using namespace Microsoft::WRL;
using namespace DirectX;

//	定数定義
constexpr float shadow_difinition = 40.0f;	//	ライトデプスの縦横サイズ

//	コンストラクタ
Dx12Wrapper::Dx12Wrapper() :
	_parallelLightVec(1,-1,1),
	_eye(0, 10, -15),
	_target(0, 10, 0),
	_up(0, 1, 0),
	_pMapSceneMtx(nullptr),
	_prPoriVBV({})
{
	//	ウィンドウサイズの取得
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_windowSize = WinApp.GetWindowSize();
}

/*	初期化関連の処理	*/
//	初期化処理
void Dx12Wrapper::Init(HWND hwnd)
{
#ifdef _DEBUG
	EnableDebugLayer();
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgifactory.ReleaseAndGetAddressOf()));
#else
	//	ファクトリー作成
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgifactory.ReleaseAndGetAddressOf()));
#endif // _DEBUG

	//	デバイスの作成
	CreateDevice();
	
	//	コマンドリストの作成
	CreateCommand();

	//	スワップチェインの作成
	CreateSwapchain(hwnd);

	//	フェンスの作成
	result = _dev->CreateFence(
		_fenceVal,				//	初期化値
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())
	);

	//	深度バッファの作成
	CreateDepthView();

	//	シーンバッファの作成
	CreateViewProjectionView();
	
	//	ペラポリゴンに張り付けるためのリソースを作成
	//	※今までBackBufferに書き込んでいたのをperaResourceに書き込み先を変更する
	CreateOriginRenderTarget();
	//	ポストエフェクト用のバッファ、ビュー作成
	CreateEffectBufferAndView();
	//	ペラポリゴンの作成
	//	※BackBufferにこちらのペラポリゴンを作成し、ペラポリゴンに
	//	_originResourceを張り付けるイメージ
	CreatePeraVertexBuff();
	CreatePeraRootSignature();
	CreatePeraGraphicPipeLine();

	//	テクスチャ無バッファテーブルを作成
	_noneTexTable[static_cast<int>(E_NONETEX::WHITE)] = CreateWhiteTexture();
	_noneTexTable[static_cast<int>(E_NONETEX::BLACK)] = CreateBlackTexture();
	_noneTexTable[static_cast<int>(E_NONETEX::GRADUATION)] = CreateGrayGradationTexture();
}

//	デバッグレイヤー有効化
void Dx12Wrapper::EnableDebugLayer(void)
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));

	debugLayer->EnableDebugLayer();	//	デバッグレイヤーを有効化する
}

//	デバイスの作成
void Dx12Wrapper::CreateDevice(void)
{
	//	アダプターの列挙用
	std::vector <IDXGIAdapter*> adapters;

	//	ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	//	アダプターオブジェクトをすべて取得
	for (int i = 0; _dxgifactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	//	設定したいアダプターが探し出す
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	//	アダプターの説明オブジェクト取得

		std::wstring strDesc = adesc.Description;

		//	探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//	デバイス作成
	D3D_FEATURE_LEVEL levels[] =	//	各フィーチャーレベル格納
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,

	};
	D3D_FEATURE_LEVEL featureLevel;	//	フィーチャーレベル

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;	//	生成可能なバージョンが見つかったのでループを打ち切り
		}
	}
	//	デバイスが作成できない際、関数を抜ける
	if (_dev == nullptr)
	{
		Helper::DebugOutputFormatString("デバイスが作成できませんでした。");
		return;
	}
}

//	コマンド作成
void Dx12Wrapper::CreateCommand(void)
{	//	コマンドアロケーターの作成
	auto result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	//	コマンドリストの作成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	_cmdList->Close();
	//	解決→初期化時にリストをクローズにして、実行中ではなくさせることで解決した
	result = _cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);	//	再びコマンドリストをためる準備

	//	コマンドキューの設定
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//	タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//	アダプターを1つしか使わない時は0でよい
	cmdQueueDesc.NodeMask = 0;
	//	プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//	コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//	コマンドキュー作成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));
}

//	スワップチェイン作成
void Dx12Wrapper::CreateSwapchain(HWND hwnd)
{
	//	スワップチェインの設定
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _windowSize.cx;
	swapchainDesc.Height = _windowSize.cy;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//	バックバッファは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//	フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//	特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//	ウィンドウ⇔フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//	スワップチェイン作成
	auto result = _dxgifactory->CreateSwapChainForHwnd(
		_cmdQueue.Get(), hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf());

	//	ディスクリプタヒープの設定(レンダーターゲットビュー	）
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		//	レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;						//	表裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	//	特に指定なし
	//	ディスクリプタヒープの作成
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));

	//	スワップチェーンのメモリと紐づける
	_backBuffers.resize(swapchainDesc.BufferCount);
	//	先頭のアドレスを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	//	レンダーターゲットビューの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		//	フォーマット
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		//	フォーマット

	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	//	テクスチャーの次元
	//	レンダーターゲットビューの生成
	for (unsigned int idx = 0; idx < swapchainDesc.BufferCount; ++idx)
	{
		//	スワップチェーンのメモリを取得
		_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		//	レンダーターゲットビュー作成
		_dev->CreateRenderTargetView(
			_backBuffers[idx],	//	バックバッファ
			&rtvDesc,			//	レンダーターゲットビューの設定
			handle);			//	どのハンドルから生成するか
		//	ポインター移動
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
}

//	深度バッファの作成
void Dx12Wrapper::CreateDepthView(void)
{
	//	深度バッファの設定
	CD3DX12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		//DXGI_FORMAT_D32_FLOAT,					//	深度値書き込み用フォーマット
		DXGI_FORMAT_R32_TYPELESS,		//	型レス（ビュー側で自由に型を決められる）
		_windowSize.cx,							//	レンダーターゲットと同じ
		_windowSize.cy,							//	レンダーターゲットと同じ
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL	//	デプスステンシルとして使用
	);
	//	深度値用ヒーププロパティ
	CD3DX12_HEAP_PROPERTIES depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	クリアバリューの設定
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;	//	深さ1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	//	32ビットfloat値としてクリア
	//	深度バッファ生成
	auto result = _dev->CreateCommittedResource(
		&depthHeapProp,						//	ヒープ設定
		D3D12_HEAP_FLAG_NONE,				//	ヒープフラグ
		&depthResDesc,						//	深度バッファの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//	深度値の書き込みに使用
		&depthClearValue,					//	クリアバリュー
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf())			//	バッファ
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("深度バッファ作成失敗\n");
		return;
	}

	//	シャドウマップ用深度バッファ
	depthResDesc.Width = shadow_difinition;
	depthResDesc.Height = shadow_difinition;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,						//	ヒープ設定
		D3D12_HEAP_FLAG_NONE,				//	ヒープフラグ
		&depthResDesc,						//	深度バッファの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//	深度値の書き込みに使用
		&depthClearValue,					//	クリアバリュー
		IID_PPV_ARGS(_lightDepthBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("シャドウマップ用深度バッファ作成失敗\n");
		return;
	}

	//	深度バッファビューの作成	//
	//	ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 2;							//	ディスクリプタ数
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;		//	デプスステンシルビューとして使用
	//	深度のためのディスクリプタヒープ作成
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));
	//	深度バッファビューの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					//	深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	//	2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;					//	フラグなし
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	ディスクリプタヒープ内に深度バッファビューを作成
	_dev->CreateDepthStencilView(
		_depthBuffer.Get(),
		&dsvDesc,
		handle
	);
	//	ライトデプス用の深度バッファビュー作成
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_dev->CreateDepthStencilView(
		_lightDepthBuffer.Get(),
		&dsvDesc,
		handle
	);

	//	深度値テクスチャービュー作成	//
	//	深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC SRVheapDesc = {};
	SRVheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SRVheapDesc.NodeMask = 0;
	SRVheapDesc.NumDescriptors = 2;
	SRVheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&SRVheapDesc, IID_PPV_ARGS(_depthSRVHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("深度値テクスチャー用のディスクリプタヒープ作成失敗\n");
		return;
	}
	//	通常デプス→テクスチャ用
	D3D12_SHADER_RESOURCE_VIEW_DESC srvResDesc  = {};
	srvResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvResDesc.Texture2D.MipLevels = 1;
	srvResDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvResDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//	2Dテクスチャ
	handle = _depthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(
		_depthBuffer.Get(),
		&srvResDesc,
		handle
	);
	//	ライトデプス→テクスチャ用
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_dev->CreateShaderResourceView(
		_lightDepthBuffer.Get(),
		&srvResDesc,
		handle
	);
}

//	ビュープロジェクションバッファの作成
void Dx12Wrapper::CreateViewProjectionView(void)
{
	//	ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	シェーダから見えるように
	descHeapDesc.NodeMask = 0;										//	マスク
	descHeapDesc.NumDescriptors = 1;								//	定数バッファビュー（CBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	シェーダリソースビュー用
	//	ディスクリプタヒープの作成
	auto result = _dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_ScenevHeap.ReleaseAndGetAddressOf())
	);
	//	ヒーププロパティー設定
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	シーンバッファの設定
	UINT64 BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256アライメントにそろえたサイズ
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
	//	シーンバッファの作成
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_SceneBuffer.ReleaseAndGetAddressOf())
	);
	//	シーンバッファビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _SceneBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = BufferSize;
	auto HeapHandle = _ScenevHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&cbvDesc, HeapHandle);

	//	シーンに使用する行列の設定
	result = _SceneBuffer->Map(0, nullptr, (void**)&_pMapSceneMtx);
	//	ビュー行列設定
	auto eyePos = XMLoadFloat3(&_eye);			//	視点
	auto targetPos = XMLoadFloat3(&_target);	//	注視点
	auto upPos = XMLoadFloat3(&_up);			//	上ベクトル
	DirectX::XMFLOAT3 target(0, 10, 0);	//	注視点
	DirectX::XMFLOAT3 up(0, 1, 0);		//	上ベクトル
	_pMapSceneMtx->eye = _eye;
	_pMapSceneMtx->view = DirectX::XMMatrixLookAtLH(
		eyePos,
		targetPos,
		upPos
	);
	//	プロジェクション行列設定
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	透視投影処理(パースあり）
		DirectX::XM_PIDIV2,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);

	//	ライトビュープロジェクション
	XMVECTOR lightPos = targetPos + XMVector3Normalize(-XMLoadFloat3(&_parallelLightVec))
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	平行投影処理（パースなし）
			shadow_difinition,		//	左右の範囲
			shadow_difinition,		//	上下の範囲
			1.0f,	//	near
			100.0f	//	for
		);
	//	影行列設定
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	平面の方程式
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		-XMLoadFloat3(&_parallelLightVec)
	);
}

//	オリジンのレンダーターゲット作成
void Dx12Wrapper::CreateOriginRenderTarget(void)
{
	//	バッファ作成	//
	//	使っているバックバッファの情報を利用する
	auto& bbuff = _backBuffers[0];
	auto resDesc = bbuff->GetDesc();
	//	ヒーププロパティー設定
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	レンダリング時のクリア値と同じ値
	float clsClr[4] = { 0.5f,0.5f,0.5f,1.0f };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clsClr);
	//	バッファの作成
	for (auto& res : _origin1Resource)
	{
		auto result = _dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(res.ReleaseAndGetAddressOf()));
		if (FAILED(result))
		{
			Helper::DebugOutputFormatString("オリジンのレンダーターゲットバッファ作成失敗");
			return;
		}
	}

	//	ブルームバッファの作成
	for (auto& res : _bloomBuffer)
	{
		auto result = _dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(res.ReleaseAndGetAddressOf()));
		//	サイズを半分にする
		resDesc.Width >>= 1;
		resDesc.Height >>= 1;
		if (FAILED(result))
		{
			Helper::DebugOutputFormatString("ブルームのレンダーターゲットバッファ作成失敗");
			return;
		}
	}

	//	加工用のレンダーターゲット作成
	CreateProcessRenderTarget();

	//	ディスクリプタヒープ作成	//
	//	作成済みのヒープ情報を使ってもう1毎作る
	auto heapDesc = rtvHeaps->GetDesc();
	//	RTV用ヒープを作る
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_RTV::MAX);
	auto result = _dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(_originRTVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("マルチパスレンダリング：RVT用ディスクリプタヒープ作成失敗");
		return;
	}

	//	オリジン用のレンダーターゲットビュー作成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();						//	RTVのスタートポイント
	int offset = 0;																	//	ビューのオフセット位置
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	レンダーターゲットビューのインクリメントサイズ
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(baseH);											//	ハンドル

	//	通常描画のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::COL);
	for (auto& res : _origin1Resource)
	{
		handle.InitOffsetted(baseH, offset);
		_dev->CreateRenderTargetView(
			res.Get(),
			&rtvDesc,
			handle
		);
		offset += incSize;
	}

	//	ブルーム用のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::BLOOM);
	handle.InitOffsetted(baseH, offset);
	_dev->CreateRenderTargetView(
		_bloomBuffer[0].Get(),
		&rtvDesc,
		handle
	);

	/*
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::BLOOM);
	for (auto& res : _bloomBuffer)
	{
		handle.InitOffsetted(baseH, offset);
		_dev->CreateRenderTargetView(
			res.Get(),
			&rtvDesc,
			handle
		);
		offset += incSize;
	}
	*/

	//	加工用のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::PROCE);
	handle.InitOffsetted(baseH, offset);
	_dev->CreateRenderTargetView(
		_proceResource.Get(),
		&rtvDesc,
		handle
	);

	//	SRV用ヒープを作る
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_SRV::MAX);
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = _dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(_originSRVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("マルチパスレンダリング：SRV用ディスクリプタヒープ作成失敗");
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//	オリジンのシェーダーリソースビュー作成
	baseH = _originSRVHeap->GetCPUDescriptorHandleForHeapStart();						//	SRVのスタートポイント
	offset = 0;																	//	ビューのオフセット位置
	incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//	レンダーターゲットビューのインクリメントサイズ

	offset = incSize * static_cast<int>(E_ORIGIN_SRV::COL);
	for (auto& res : _origin1Resource)
	{
		handle.InitOffsetted(baseH, offset);
		_dev->CreateShaderResourceView(
			res.Get(),
			&srvDesc,
			handle
		);
		offset += incSize;
	}

	//	ブルームのシェーダーリソースビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::BLOOM);
	handle.InitOffsetted(baseH, offset);
	_dev->CreateShaderResourceView(
		_bloomBuffer[0].Get(),
		&srvDesc,
		handle
	);

	/*
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::BLOOM);
	for (auto& res : _bloomBuffer)
	{
		handle.InitOffsetted(baseH, offset);
		_dev->CreateShaderResourceView(
			res.Get(),
			&srvDesc,
			handle
		);
		offset += incSize;
	}
	*/

	//	加工用のシェーダーリソースビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::PROCE);
	handle.InitOffsetted(baseH, offset);
	_dev->CreateShaderResourceView(
		_proceResource.Get(),
		&srvDesc,
		handle
	);

	//	ぼけ定数バッファ作成
	CreateBokeConstantBuff();
	//	ぼけ定数バッファビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _bokehParamBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _bokehParamBuffer->GetDesc().Width;
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::BOKE);
	handle.InitOffsetted(baseH, offset);
	_dev->CreateConstantBufferView(&cbvDesc, handle);

}

//	加工用のレンダーターゲット作成
void Dx12Wrapper::CreateProcessRenderTarget(void)
{
	//	バッファ作成	//
//	使っているバックバッファの情報を利用する
	auto& bbuff = _backBuffers[0];
	auto resDesc = bbuff->GetDesc();
	//	ヒーププロパティー設定
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	レンダリング時のクリア値と同じ値
	float clsClr[4] = { 0.5f,0.5f,0.5f,1.0f };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clsClr);
	//	バッファの作成
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_proceResource.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("マルチパスレンダリング：バッファ作成失敗");
		return;
	}
}

//	ペラポリゴンの頂点バッファ作成
void Dx12Wrapper::CreatePeraVertexBuff(void)
{
	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	PeraVertex pv[4] = {
		{{-1.0f,-1.0f,0.1f},{0,1}},		//	左下
		{{-1.0f,1.0f,0.1f},{0,0}},		//	左上
		{{1.0f,-1.0f,0.1f},{1,1}},		//	右下
		{{1.0f,1.0f,0.1f},{1,0}}		//	右上

	};

	//	頂点バッファ作成
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_prPoriVB.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ペラポリゴンの頂点バッファ作成失敗");
		return;
	}
	//	頂点バッファビュー作成
	_prPoriVBV.BufferLocation = _prPoriVB->GetGPUVirtualAddress();
	_prPoriVBV.SizeInBytes = sizeof(pv);
	_prPoriVBV.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera = nullptr;
	_prPoriVB->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	_prPoriVB->Unmap(0, nullptr);
}

//	ぼけ定数バッファ作成
void Dx12Wrapper::CreateBokeConstantBuff(void)
{
	//	分散値の計算
	//	ウェイト値計算
	std::vector<float> weights = Helper::GetGaussianWeights(8, 5.0f);;	//	各ウェイト値

	//	定数バッファの作成
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(
		Helper::AlignmentedSize(sizeof(weights[0]) * weights.size(), 256));
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_bokehParamBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ぼけ定数バッファ作成失敗");
		return;
	}
	//	値をコピー
	float* mappedWeight = nullptr;
	result = _bokehParamBuffer->Map(0, nullptr, (void**)&mappedWeight);
	std::copy(weights.begin(), weights.end(), mappedWeight);
	_bokehParamBuffer->Unmap(0, nullptr);
}

//	ペラポリゴン用ルートシグネイチャー作成
void Dx12Wrapper::CreatePeraRootSignature(void)
{
	//	レンジの設定
	D3D12_DESCRIPTOR_RANGE ranges[5] = {};
	//	通常カラー、法線セット、高輝度
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 3;	//	t0,t1,t2
	//	ぼけ定数バッファセット
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;	//	b0
	//	ポストエフェクト用のバッファセット
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].NumDescriptors = 1;	//	t3
	//	深度値テクスチャ用
	ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[3].NumDescriptors = 1;	//	t4
	//	ライトデプステクスチャ用
	ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[4].NumDescriptors = 1;	//	t5

	//	レギスター設定
	UINT nSRVRegister = 0;
	UINT nCBVRegister = 0;
	for (auto& range : ranges)
	{
		//	SRV
		if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
		{
			range.BaseShaderRegister = nSRVRegister;
			nSRVRegister += range.NumDescriptors;
		}
		else if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			range.BaseShaderRegister = nCBVRegister;
			nCBVRegister += range.NumDescriptors;
		}
		else {}
	}

	//	ルートパラメータの設定
	D3D12_ROOT_PARAMETER rp[5] = {};
	//	テクスチャーバッファセット
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	//	ぼけ定数バッファセット
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;
	//	ポストエフェクト用バッファセット
	rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[2].DescriptorTable.pDescriptorRanges = &ranges[2];
	rp[2].DescriptorTable.NumDescriptorRanges = 1;
	//	深度値テクスチャー用
	rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[3].DescriptorTable.pDescriptorRanges = &ranges[3];
	rp[3].DescriptorTable.NumDescriptorRanges = 1;
	//	ライトデプステクスチャ用
	rp[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[4].DescriptorTable.pDescriptorRanges = &ranges[4];
	rp[4].DescriptorTable.NumDescriptorRanges = 1;


	//	サンプラー設定
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	ルートシグネイチャ設定
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 5;
	rsDesc.pParameters = rp;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &sampler;

	ComPtr<ID3DBlob> rsBlob;
	ComPtr<ID3DBlob> errBlob;

	auto result = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	Helper::DebugShaderError(result, errBlob.Get());

	result = _dev->CreateRootSignature(
		0,
		rsBlob->GetBufferPointer(),
		rsBlob->GetBufferSize(),
		IID_PPV_ARGS(_prPoriRS.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ペラポリゴン用ルートシグネイチャー作成失敗\n");
		return;
	}

}

//	ペラポリゴン用グラフィックスパイプライン作成
void Dx12Wrapper::CreatePeraGraphicPipeLine(void)
{
	//	レイアウト
	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{	//	位置
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{	//	UV
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

	//	各シェーダーの読み込み
	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;
	//	頂点シェーダーコンパイル
	auto result = D3DCompileFromFile(
		L"peraVertex.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		vs.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	Helper::DebugShaderError(result, errBlob.Get());
	//	ピクセルシェーダコンパイル
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	Helper::DebugShaderError(result, errBlob.Get());

	//	グラフィックスパイプラインの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;

	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsDesc.DepthStencilState;
	gpsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpsDesc.pRootSignature = _prPoriRS.Get();
	//	ペラポリゴンのパイプライン作成
	result = _dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_prPoriPipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ペラポリゴン用グラフィックパイプライン作成失敗\n");
		return;
	}

	//	加工用のパイプライン作成
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VerticalBokehPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	result = _dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_procePipeline.ReleaseAndGetAddressOf())
	);

	//	ポストエフェクトのパイプライン作成
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PostEffectPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	result = _dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_effectPipeline.ReleaseAndGetAddressOf())
	);

}

//	エフェクト用のバッファとビュー作成
bool Dx12Wrapper::CreateEffectBufferAndView(void)
{
	//	ポストエフェクト用テクスチャーバッファを作成
	std::string sNormal = "normal/glass_n.png";
	_efffectTexBuffer = LoadTextureFromFile(sNormal);
	if (_efffectTexBuffer == nullptr)
	{
		Helper::DebugOutputFormatString("法線マップ画像の読み込みに失敗\n");
		return false;
	}

	//	ポストエフェクト用のディスクリプタヒープ生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_effectSRVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return false;
	}

	//	ポストエフェクト用シェーダーリソースビュー作成
	auto desc = _efffectTexBuffer->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_dev->CreateShaderResourceView(
		_efffectTexBuffer.Get(),
		&srvDesc,
		_effectSRVHeap->GetCPUDescriptorHandleForHeapStart()
	);


	return false;
}

//	白いテクスチャーを生成する処理
ComPtr < ID3D12Resource> Dx12Wrapper::CreateWhiteTexture(void)
{
	//	プロパティの設定
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	//	リソース設定
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4,
		1,
		1
	);

	//	テクスチャーバッファの生成
	ComPtr < ID3D12Resource> whiteBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	特に指定なし
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);	//	すべての値を255に統一する

	//	テクスチャバッファの転送
	result = whiteBuff->WriteToSubresource(
		0,								//	サブリソースインデックス
		nullptr,						//	書き込み領域の指定（nullptr = 全領域へのコピー）
		data.data(),					//	書き込みたいデータのアドレス
		4 * 4,					//	1行当たりのデータサイズ
		data.size()					//	スライス当たりのデータサイズ
	);

	return whiteBuff;
}

//	黒テクスチャを生成する処理
ComPtr < ID3D12Resource> Dx12Wrapper::CreateBlackTexture(void)
{
	//	プロパティの設定
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	//	リソース設定
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4,
		1,
		1
	);

	//	テクスチャーバッファの生成
	ComPtr < ID3D12Resource> blackBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	特に指定なし
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(blackBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);	//	すべての値を0に統一する

	//	テクスチャバッファの転送
	result = blackBuff->WriteToSubresource(
		0,								//	サブリソースインデックス
		nullptr,						//	書き込み領域の指定（nullptr = 全領域へのコピー）
		data.data(),					//	書き込みたいデータのアドレス
		4 * 4,					//	1行当たりのデータサイズ
		data.size()					//	スライス当たりのデータサイズ
	);

	return blackBuff;
}

//	デフォルトグラデーションテクスチャー
ComPtr < ID3D12Resource> Dx12Wrapper::CreateGrayGradationTexture(void)
{
	//	プロパティの設定
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);
	//	リソース設定
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		256,
		1,
		1
	);
	//	テクスチャーバッファの生成
	ComPtr < ID3D12Resource> gradBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	特に指定なし
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	//	上が白くてしたが黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);
		--c;
	}

	//	テクスチャバッファの転送
		//	※まれにこの処理を実行中に進行停止する
	result = gradBuff->WriteToSubresource(
		0,								//	サブリソースインデックス
		nullptr,						//	書き込み領域の指定（nullptr = 全領域へのコピー）
		data.data(),					//	書き込みたいデータのアドレス
		4 * sizeof(unsigned int),					//	1行当たりのデータサイズ
		sizeof(unsigned int) * data.size()				//	スライス当たりのデータサイズ
	);

	return gradBuff;
}

/*	描画関連の処理	*/
//	レンダーターゲットをセットする処理
void Dx12Wrapper::ShadowDraw(void)
{	
	//	深度バッファ用ディスクリプタヒープハンドル取得
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//	レンダーターゲットセット
	_cmdList->OMSetRenderTargets(0, nullptr, false, &handle);
	//	深度バッファビューをクリア
	_cmdList->ClearDepthStencilView(handle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//	座標変換用ディスクリプタヒープの指定
	_cmdList->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_ScenevHeap.GetAddressOf()		//	座標変換用ディスクリプタヒープ
	);

	//	ルートパラメータとディスクリプタヒープの関連付け
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	定数バッファ0ビュー用の指定
	_cmdList->SetGraphicsRootDescriptorTable(
		0,			//	ルートパラメータインデックス
		heapHandle	//	ヒープアドレス
	);

	D3D12_VIEWPORT vp = 
		CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形
}

//	レンダーターゲットをセットする処理
void Dx12Wrapper::PreOriginDraw(void)
{
	//	オリジンレンダーターゲットに描画する前にバリア設定を行う
	for (auto& res : _origin1Resource)
	{
		D3D12_RESOURCE_BARRIER resBarri =
			CD3DX12_RESOURCE_BARRIER::Transition(res.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		_cmdList->ResourceBarrier(1,
			&resBarri);
	}
	//	ブルーム描画前にバリア設定
	D3D12_RESOURCE_BARRIER resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(_bloomBuffer[0].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1,
		&resBarri);


	//	RTVハンドルのセット
	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();						//	RTVのスタートポイント
	uint32_t offset = 0;																	//	ビューのオフセット位置
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	レンダーターゲットビューのインクリメントサイズ
	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[static_cast<int>(E_ORIGIN_RTV::MAX_BLOOM)];											//	ハンドル
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, offset);
		offset += incSize;
	}

	//	深度バッファ用ディスクリプタヒープハンドル取得
	auto dsvHeapPointer = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	レンダーターゲットセット
	_cmdList->OMSetRenderTargets(
		static_cast<UINT>(E_ORIGIN_RTV::MAX_BLOOM), handles, false, &dsvHeapPointer);
	//クリアカラー		 R   G   B   A
	float clsClr[4] = { 0.5,0.5,0.5,1.0 };
	//	オリジン用のレンダーターゲットビューをクリア
	for (int i = 0;i<_countof(handles);i++)
	{
		//	ブルームのクリアカラーを黒にする
		if (i == 2)
		{
			clsClr[0] = clsClr[1] = clsClr[2] = 0.0f; clsClr[3] = 1.0f;
		}
		_cmdList->ClearRenderTargetView(handles[i], clsClr, 0, nullptr);
	}
	//	深度バッファビューをクリア
	_cmdList->ClearDepthStencilView(dsvHeapPointer,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

//	オリジンレンダーターゲットの描画終了
void Dx12Wrapper::EndOriginDraw(void)
{
	for (auto& res : _origin1Resource)
	{
		D3D12_RESOURCE_BARRIER resBarri =
			CD3DX12_RESOURCE_BARRIER::Transition(res.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		//	ペラポリゴン描画後バリア指定
		_cmdList->ResourceBarrier(1,
			&resBarri);
	}
	D3D12_RESOURCE_BARRIER resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(_bloomBuffer[0].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//	ブルーム描画後バリア指定
	_cmdList->ResourceBarrier(1,
		&resBarri);
}

//	加工用のレンダーターゲットの描画
void Dx12Wrapper::ProceDraw(void)
{
	//	加工用描画前バリア指定
	D3D12_RESOURCE_BARRIER resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(_proceResource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1,
		&resBarri);

	//	加工用RTVをセット
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapPointer;
	rtvHeapPointer.InitOffsetted(
		_originRTVHeap->GetCPUDescriptorHandleForHeapStart(),
		_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * static_cast<int>(E_ORIGIN_RTV::PROCE)
	);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	クリアカラー	
	float clsClr[4] = { 0.5f,0.5f,0.5f,1.0f };
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clsClr, 0, nullptr);

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形


	_cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	ペラポリゴン用のルートシグネイチャセット
	_cmdList->SetPipelineState(_prPoriPipeline.Get());						//	ペラポリゴン用のパイプラインセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	ペラポリゴン用の頂点バッファビューセット
	//	オリジン用のSRVヒープをセット
	_cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	int incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	パラメーター0番（テクスチャーリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	パラメーター0番（定数バッファリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	深度値検証用	//	
	//	通常デプス深度バッファーテクスチャ
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	//	深度バッファーテクスチャ用のリソースとヒープを関連付ける
	handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(
		3,
		handle);

	//	ライトデプス深度バッファテクスチャ
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(
		4,
		handle);

	_cmdList->DrawInstanced(4, 1, 0, 0);


	//	加工用描画後バリア指定
	resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(_proceResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	_cmdList->ResourceBarrier(1,
		&resBarri);
}

//	バックバッファのクリア
void Dx12Wrapper::Clear(void)
{
	//	バックバッファのインデックスを取得する
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	バックバッファのバリア設定
	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,		//	直前はPRESENT状態
		D3D12_RESOURCE_STATE_RENDER_TARGET	//	今からレンダーターゲット状態
	);
	_cmdList->ResourceBarrier(
		1,
		&BarrierDesc
	);

	//	バックバッファのレンダーターゲットをセット
	auto rtvHeapPointer = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	バックバッファをクリア
	float clsClr[4] = { 0.5f,0.5f,0.5f,1.0f };
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clsClr, 0, nullptr);
}

//	（ペラポリゴン仮の）描画
void Dx12Wrapper::Draw(void)
{
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetViewports(1, &vp);//ビューポート
	CD3DX12_RECT rc(0, 0, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形

	_cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	ペラポリゴン用のルートシグネイチャセット
	_cmdList->SetPipelineState(_procePipeline.Get());						//	ペラポリゴン用のパイプラインセット

	//	オリジン用のSRVヒープをセット
	_cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	//	加工用のテクスチャとヒープを関連付ける
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	int incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::PROCE));
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	パラメーター0番（定数バッファリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	ポストエフェクト用のSRVヒープをセット
	_cmdList->SetDescriptorHeaps(1, _effectSRVHeap.GetAddressOf());
	//	ポストエフェクト用のリソースとヒープを関連付ける
	_cmdList->SetGraphicsRootDescriptorTable(
		2,
		_effectSRVHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	ペラポリゴン用の頂点バッファビューセット
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

//	フリップ処理
void Dx12Wrapper::Flip(void)
{
	//バックバッファのインデックスを取得する
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	バックバッファのバリア設定
	CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,		//	直前はPRESENT状態
		D3D12_RESOURCE_STATE_PRESENT	//	今からレンダーターゲット状態
	);
	_cmdList->ResourceBarrier(
		1,
		&BarrierDesc
	);


	//	命令のクローズ
	_cmdList->Close();
	//	コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(
		1,			//	実行するコマンドリスト数
		cmdlists);	//	コマンドリスト配列の先頭アドレス

	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		//	イベントハンドルの取得
		auto event = CreateEvent(nullptr, false, false, nullptr);
		if (event != NULL)
		{
			_fence->SetEventOnCompletion(_fenceVal, event);
			//	イベントが発生するまで待ち続ける（INFINITE）
			WaitForSingleObject(event, INFINITE);
			//	イベントハンドルを閉じる
			CloseHandle(event);
		}
	}

	//	コマンドアロケーターとコマンドリストをリセットする
	auto result = _cmdAllocator->Reset();	//	キューをクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);	//	再びコマンドリストをためる準備

	_swapchain->Present(1, 0);
}

//	シーンビューのセット命令
void Dx12Wrapper::CommandSet_SceneView(void)
{
	//	座標変換用ディスクリプタヒープの指定
	_cmdList->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_ScenevHeap.GetAddressOf()		//	座標変換用ディスクリプタヒープ
	);

	//	ルートパラメータとディスクリプタヒープの関連付け
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	定数バッファ0ビュー用の指定
	_cmdList->SetGraphicsRootDescriptorTable(
		0,			//	ルートパラメータインデックス
		heapHandle	//	ヒープアドレス
	);

	//	深度SRVをセット
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	auto handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_cmdList->SetGraphicsRootDescriptorTable(3, handle);

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形
}

//	テクスチャの読み込み処理
ComPtr<ID3D12Resource> Dx12Wrapper::LoadTextureFromFile(std::string& texPath)
{
	//	テクスチャーパス名が空の場合
	if (texPath.empty() || Helper::GetExtension(texPath) == "")
	{
		return nullptr;
	}
	//	テーブル内に同リソースがあったら、マップ内のリソースを返す
	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		//	リソースを返す
		return it->second;
	}

	DirectX::TexMetadata metadate = {};		//	テクスチャのメタデータ（画像ファイルに関するデータ、幅・高さ・フォーマットなど）
	DirectX::ScratchImage scratchImg = {};	//	画像ファイルデータ
	//	インターファイス関連のエラーが出現する場合、こちらのコメントを取る
	//result = CoInitializeEx(0, COINIT_MULTITHREADED);	//	COMライブラリを初期化する処理

	//	各テクスチャファイルの拡張子に合わせた関数をセット
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	//	WICファイルの読み込み用関数
	loadLambdaTable["sph"]														//	拡張子名
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	挿入するラムダ式関数
		-> HRESULT																//	ラムダ式関数の返り値
	{
		return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
	};
	//	tgaの読み込み用関数
	loadLambdaTable["tga"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	挿入するラムダ式関数
		-> HRESULT																//	ラムダ式関数の返り値
	{
		return DirectX::LoadFromTGAFile(path.c_str(), meta, img);
	};
	//	ddsの読み込み用関数
	loadLambdaTable["dds"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	挿入するラムダ式関数
		-> HRESULT																//	ラムダ式関数の返り値
	{
		return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
	};
	auto wtexpath = Helper::GetWideStringFromString(texPath);	//	ワイド文字列に変換
	auto ext = Helper::GetExtension(texPath);					//	拡張子を取得

	//	テクスチャーファイルの読み込み
	auto result = loadLambdaTable[ext](
		wtexpath.c_str(),
		&metadate,
		scratchImg
		);

	if (FAILED(result))
	{
		return nullptr;
	}

	//	生データ抽出
	auto img = scratchImg.GetImage(
		0,	//	ミップレベル
		0,	//	テクスチャ配列を使用する際のインデックス
		0	//	3Dテクスチャーにおける深さ（スライス）
	);

	//	テクスチャバッファの作成	//
	//	WriteToSubresourceで転送するためのヒープ設定
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,//	ライトバック
		D3D12_MEMORY_POOL_L0);			   //	CPU側から転送を行う
	//	リソース設定
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadate.format,
		metadate.width,
		metadate.height,
		(UINT16)metadate.arraySize,
		(UINT16)metadate.mipLevels
	);

	//	テクスチャーバッファの生成
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	特に指定なし
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	//	テクスチャバッファの転送
	result = texbuff->WriteToSubresource(
		0,								//	サブリソースインデックス
		nullptr,						//	書き込み領域の指定（nullptr = 全領域へのコピー）
		img->pixels,					//	書き込みたいデータのアドレス
		img->rowPitch,					//	1行当たりのデータサイズ
		img->slicePitch					//	スライス当たりのデータサイズ
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = texbuff;
	return texbuff;
}