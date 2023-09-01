//	インクルード
#include "DXApplication.h"
#include "Dx12Wrapper.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

using namespace DirectX;

//	インスタンスを取得
DXApplication& DXApplication::Instance(void)
{
	static DXApplication instance;
	return instance;
}

//	コンストラクタ
DXApplication::DXApplication() :
	_pDxWrap(nullptr),
	_pPmdAct(nullptr),
	_pPmdAct2(nullptr),
	_pPmdRender(nullptr)
{
}

//	初期化処理
void DXApplication::OnInit(HWND hwnd, unsigned int window_width, unsigned int window_height)
{
	//	DirectX周りの初期化処理
	_pDxWrap = new Dx12Wrapper;
	_pDxWrap->Init(hwnd);

	//	PMDモデルの初期化処理
	_pPmdAct = new PMDActor(_pDxWrap, "Model/初音ミク.pmd", "motion/pose.vmd");
	_pPmdAct->Init();

	_pPmdAct2 = new PMDActor(_pDxWrap, "Model/弱音ハク.pmd", "motion/motion.vmd", {15.0f,0.0f,5.0f});
	_pPmdAct2->Init();

	//	PMDレンダラーの初期化処理
	_pPmdRender = new PMDRenderer(_pDxWrap);
	_pPmdRender->Init();
}

//	描画処理
void DXApplication::OnRender(void)
{
	//	シャドウマップ描画	//
	_pPmdRender->PreShadowDraw();
	_pDxWrap->ShadowDraw();
	_pPmdAct->ShadowMapDraw();
	_pPmdAct2->ShadowMapDraw();


	//	※いつもの描画をオリジンレンダーターゲットに行う	//
	//	オリジンレンダーターゲットをセット
	_pDxWrap->PreOriginDraw();
	//	PMDレンダラーにて、ルートシグネイチャなどをセット
	_pPmdRender->Draw();
	//	シーンビューの描画セット
	_pDxWrap->CommandSet_SceneView();
	//	PMDモデルの描画処理
	_pPmdAct->Draw();
	_pPmdAct2->Draw();

	//	オリジンレンダーターゲットの描画終了
	_pDxWrap->EndOriginDraw();

	_pDxWrap->DrawShrinkTextureForBlur();
	//	加工用のレンダーターゲットの描画
	_pDxWrap->ProceDraw();


	//	バックバッファをレンダーターゲットのセット及び、のクリア
	_pDxWrap->Clear();
	//	描画
	_pDxWrap->Draw();
	
	//	フリップ
	_pDxWrap->Flip();
}

//	オブジェクトの解放処理
void DXApplication::OnRelease(void)
{
	//	DirectX周りの解放
	delete _pDxWrap;
	_pDxWrap = nullptr;

	//	PMDモデルの解放
	delete _pPmdAct;
	_pPmdAct = nullptr;

	delete 	_pPmdAct2;
	_pPmdAct2 = nullptr;

	//	PMDレンダラーの解放
	delete _pPmdRender;
	_pPmdRender = nullptr;
}

//	更新処理
void DXApplication::OnUpdate(void)
{
	_pPmdAct->Update();
	_pPmdAct2->Update();
}

// //	コピーテクスチャーレギオンでのテクスチャー貼り付け
// void DXApplication::ByCopyTextureRegion(void)
// {
// 	//	CopyTextureRegionによる、テクスチャー貼り付け処理	//
// //	WIC（Windows Imaging Component)テクスチャのロード
// //	※PNG,JPEGなどを描画できる
// 	DirectX::TexMetadata metadate = {};		//	テクスチャのメタデータ（画像ファイルに関するデータ、幅・高さ・フォーマットなど）
// 	DirectX::ScratchImage scratchImg = {};	//	画像ファイルデータ
// 	//	インターファイス関連のエラーが出現する場合、こちらのコメントを取る
// 	//result = CoInitializeEx(0, COINIT_MULTITHREADED);	//	COMライブラリを初期化する処理
// 	auto result = DirectX::LoadFromWICFile(
// 		//L"img/textest.png",	//	ファイルパス
// 		L"img/textest.png",	//	ファイルパス
// 
// 		WIC_FLAGS_NONE,		//	どのようにロードするか
// 		&metadate,			//	メタデータのポインタ
// 		scratchImg			//	画像ファイルデータ
// 	);
// 	//	生データ抽出
// 	auto img = scratchImg.GetImage(
// 		0,	//	ミップレベル
// 		0,	//	テクスチャ配列を使用する際のインデックス
// 		0	//	3Dテクスチャーにおける深さ（スライス）
// 	);
// 
// 	//	アップロードバッファの作成	//
// 	//	中間バッファーとしてのアップロードヒープ設定
// 	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
// 	//	リソース設定
// 	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height);
// 	//	アップロード用リソースの作成
// 	//ID3D12Resource* uploadbuff = nullptr;
// 	result = _dev->CreateCommittedResource(
// 		&uploadHeapProp,
// 		D3D12_HEAP_FLAG_NONE,
// 		&texresDesc,
// 		D3D12_RESOURCE_STATE_GENERIC_READ,
// 		nullptr,
// 		IID_PPV_ARGS(_uploadbuff.ReleaseAndGetAddressOf())
// 	);
// 
// 	//	コピー先リソースの作成	//
// 	//	テクスチャのためのヒープ設定
// 	CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
// 	//	テクスチャリソース設定
// 	texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
// 		metadate.format,
// 		metadate.width,
// 		metadate.height,
// 		(UINT16)metadate.arraySize,
// 		(UINT16)metadate.mipLevels
// 	);
// 
// 	//	テクスチャバッファの作成
// 	result = _dev->CreateCommittedResource(
// 		&texHeapProp,
// 		D3D12_HEAP_FLAG_NONE,
// 		&texresDesc,
// 		D3D12_RESOURCE_STATE_COPY_DEST,	//	コピー先
// 		nullptr,
// 		IID_PPV_ARGS(_texbuff.ReleaseAndGetAddressOf())
// 	);
// 
// 	//	アップロードリソースへのマップ
// 	uint8_t* mapforImg = nullptr;								//	image->pixelsと同じ型にする
// 	result = _uploadbuff->Map(0, nullptr, (void**)&mapforImg);	//	マップ
// 
// 	auto srcAddress = img->pixels;														//	元データのポインタを取得
// 	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);	//	1行当たりのサイズ取得
// 	for (int y = 0; y < img->height; ++y)
// 	{
// 		std::copy_n(srcAddress, rowPitch, mapforImg);		//	1行コピー
// 		//	1行ごとのつじつまを合わせる
// 		srcAddress += img->rowPitch;
// 		mapforImg += rowPitch;
// 	}
// 	_uploadbuff->Unmap(0, nullptr);								//	アンマップ
// 
// 	//	コピー元アップロードバッファからコピー先テクスチャーバッファにコピーする処理	//
// 	//	コピー元の設定
// 	D3D12_TEXTURE_COPY_LOCATION src = {};
// 	src.pResource = _uploadbuff.Get();									//	アップロードバッファ
// 	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		//	フットプリント（メモリ占有領域に関する情報）
// 	src.PlacedFootprint.Offset = 0;								//	メモリオフセット
// 	src.PlacedFootprint.Footprint.Width = metadate.width;		//	幅
// 	src.PlacedFootprint.Footprint.Height = metadate.height;		//	高さ
// 	src.PlacedFootprint.Footprint.Depth = metadate.depth;		//	深さ
// 	src.PlacedFootprint.Footprint.RowPitch =					//	1行当たりのバイト数
// 		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
// 	src.PlacedFootprint.Footprint.Format = img->format;			//	フォーマット
// 
// 	//	コピー先の設定
// 	D3D12_TEXTURE_COPY_LOCATION dst = {};
// 	dst.pResource = _texbuff.Get();									//	テクスチャーバッファ
// 	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		//	インデックス
// 	dst.SubresourceIndex = 0;									//	コピー先のインデックス
// 
// 	//	GPUに対する命令処理
// 	{
// 		//	コピー元→コピー先にコピーする命令
// 		_cmdList->CopyTextureRegion(
// 			&dst,		//	コピー先のポインタ
// 			0,			//	コピー先領域開始X
// 			0,			//	コピー先領域開始Y
// 			0,			//	コピー先領域開始Z
// 			&src,		//	コピー元のポインタ
// 			nullptr		//	コピー元領域ボックス
// 		);
// 
// 		//	バリアの設定
// 		CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
// 			_texbuff.Get(),
// 			D3D12_RESOURCE_STATE_COPY_DEST,
// 			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
// 		);
// 
// 		//	バリアのセット
// 		_cmdList->ResourceBarrier(1, &BarrierDesc);
// 		_cmdList->Close();
// 
// 		//	コマンドリストの実行
// 		ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
// 		_cmdQueue->ExecuteCommandLists(1, cmdlists);
// 
// 		//	フェンスの値を更新
// 		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
// 		//	GPU側で命令が完了していたら、フェンス値が一致する
// 		if (_fence->GetCompletedValue() != _fenceVal)
// 		{
// 			//	イベントハンドルの取得
// 			auto event = CreateEvent(nullptr, false, false, nullptr);
// 			_fence->SetEventOnCompletion(_fenceVal, event);
// 			//	イベントが発生するまで待ち続ける（INFINITE）
// 			WaitForSingleObject(event, INFINITE);
// 			//	イベントハンドルを閉じる
// 			CloseHandle(event);
// 		}
// 		result = _cmdAllocator->Reset();
// 		_cmdList->Reset(_cmdAllocator.Get(), nullptr);
// 	}
// 
// 
// }
