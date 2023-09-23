#include "stringDisp.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"

//	コンストラクタ
StringDisp::StringDisp(Dx12Wrapper* pWrap) : _pWrap(pWrap)
{
}

//	初期化処理
void StringDisp::Init(void)
{
	auto dev = _pWrap->GetDevice();
	//	GraphicsMemoryオブジェクトの初期化
	_gmemory = new DirectX::GraphicsMemory(dev.Get());
	//	SpriteBatchオブジェクトの初期化
	DirectX::ResourceUploadBatch resUploadBatch(dev.Get());
	resUploadBatch.Begin();
	DirectX::RenderTargetState rtState(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_D32_FLOAT
	);
	DirectX::SpriteBatchPipelineStateDescription pd(rtState);
	_spriteBatch = new DirectX::SpriteBatch(dev.Get(),
		resUploadBatch,
		pd
	);
	//	SpriteFontオブジェクトの初期化
	CreateDH();
	_spriteFont = new DirectX::SpriteFont(
		dev.Get(),
		resUploadBatch,
		L"font/meiryo.spritefont",
		_strDispDH->GetCPUDescriptorHandleForHeapStart(),
		_strDispDH->GetGPUDescriptorHandleForHeapStart()
	);
	//	ビューポートセット
	auto& WinApp = Win32Application::Instance();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp = { 0.0f,0.0f,size.cx,size.cy,0.0f,1.0f };
	_spriteBatch->SetViewport(vp);
	auto future = resUploadBatch.End(_pWrap->GetCmdQue().Get());
	//	待ち
	_pWrap->WaitForCommandQueue();
	future.wait();
}

//	描画処理
void StringDisp::Draw(void)
{
	_pWrap->GetCmdList()->SetDescriptorHeaps(1, _strDispDH.GetAddressOf());
	_spriteBatch->Begin(_pWrap->GetCmdList().Get());
	_spriteFont->DrawString(
		_spriteBatch,
		L"こんにちはハロー",		//	ひらがな、カナカナの際はLを付けることを忘れずに
		DirectX::XMFLOAT2(102, 102),
		DirectX::Colors::Black
	);
	_spriteFont->DrawString(
		_spriteBatch,
		L"こんにちは",
		DirectX::XMFLOAT2(100, 100),
		DirectX::Colors::Yellow
	);
	_spriteBatch->End();
}

//	文字列表示終了
void StringDisp::EndStrDisp(void)
{
	_gmemory->Commit(_pWrap->GetCmdQue().Get());
}

//	終了処理
void StringDisp::Release(void)
{
	delete _gmemory;	//	グラフィックスメモリオブジェクト
	_gmemory = nullptr;
	delete _spriteFont;
	 _spriteFont = nullptr;		//	フォント表示用オブジェクト
	 delete _spriteBatch;
	_spriteBatch = nullptr;	//	スプライト表示用オブジェクト
}

//	ディスクリプタヒープ作成処理
void StringDisp::CreateDH(void)
{
	//	ディスクリプタヒープ作成
	auto dev = _pWrap->GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = dev->CreateDescriptorHeap(
		&desc, IID_PPV_ARGS(_strDispDH.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("strDisp用のディスクリプタヒープ作成失敗\n");
	}
}
