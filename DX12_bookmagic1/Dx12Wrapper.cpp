//	インクルード
#include "Dx12Wrapper.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "helper.h"

//	名前空間
using namespace Microsoft::WRL;
using namespace DirectX;

namespace//	定数定義
{
	constexpr float shadow_difinition = 40.0f;	//	ライトデプスの縦横サイズ
	constexpr float CLSCLR[4] = { 0.5f,0.5f,0.5f,1.0f };		//	レンダーターゲットクリアカラー
}

//	コンストラクタ
Dx12Wrapper::Dx12Wrapper() :
	_parallelLightVec(1,-1,1),
	_eye(0, 10, -15),
	_target(0, 10, 0),
	_up(0, 1, 0),
	_pMapSceneMtx(nullptr)
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

	//	シーンバッファの作成
	CreateViewProjectionView();
	
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
	UINT64 u_64BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256アライメントにそろえたサイズ
	UINT BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256アライメントにそろえたサイズ

	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(u_64BufferSize);
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
	//	逆プロジェクション行列設定
	DirectX::XMVECTOR det;
	_pMapSceneMtx->invproj = DirectX::XMMatrixInverse(
		&det,				//	逆行列が取得できない際「0」が入る
		_pMapSceneMtx->proj);

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
//	バックバッファのクリア
void Dx12Wrapper::Clear(void)
{
	//	バックバッファのインデックスを取得する
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	バックバッファのバリア設定
	SetBarrier(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,		//	直前はPRESENT状態
		D3D12_RESOURCE_STATE_RENDER_TARGET);	//	今からレンダーターゲット状態

	//	バックバッファのレンダーターゲットをセット
	auto rtvHeapPointer = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	バックバッファをクリア
	_cmdList->ClearRenderTargetView(rtvHeapPointer, CLSCLR, 0, nullptr);
}

//	フリップ処理
void Dx12Wrapper::Flip(void)
{
	//バックバッファのインデックスを取得する
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	バックバッファのバリア設定
	SetBarrier(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,		//	直前はPRESENT状態
		D3D12_RESOURCE_STATE_PRESENT);			//	今からレンダーターゲット状態

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

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形
}

//	バリア設定
void Dx12Wrapper::SetBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES pre, D3D12_RESOURCE_STATES dest)
{
	auto resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(res,
			pre,
			dest);
	_cmdList->ResourceBarrier(1,
		&resBarri);

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