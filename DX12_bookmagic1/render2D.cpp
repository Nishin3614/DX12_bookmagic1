#include "render2D.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"
#include "polygon2D.h"
#include <d3dx12.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

//	コンストラクタ
Renderer2D::Renderer2D(Dx12Wrapper* pWrap) : _pWrap(pWrap)
{
}

//	初期化処理
void Renderer2D::Init(void)
{
	//	ルートシグネイチャ作成
	CreateRootSignature();
	//	ぱいぷらいんステート作成
	CreatePipeline();
	//	ポリゴン作成
	//Create2D("img/TitleScreenUI/Titlelogo00.png");
}

//	描画処理
void Renderer2D::Draw(void)
{
	auto cmdlist = _pWrap->GetCmdList();
	cmdlist->SetPipelineState(_pls.Get());
	cmdlist->SetGraphicsRootSignature(_rs.Get());

	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdlist->RSSetViewports(1, &vp);//ビューポート
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdlist->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形

	for (int nCnt = 0; nCnt < _Polygons.size(); nCnt++)
	{
		_Polygons[nCnt]->Draw();
	}
}

//
Polygon2D* Renderer2D::Create2D(std::string texName)
{
	//	ポリゴン作成
	std::unique_ptr p = std::make_unique<Polygon2D>(_pWrap, texName);
	p->Init();
	return _Polygons.emplace_back(std::move(p)).get();
}

//	ルートシグネイチャ作成
void Renderer2D::CreateRootSignature(void)
{
	//	レンジの設定
	D3D12_DESCRIPTOR_RANGE ranges[2] = {};
	//	テクスチャ
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 1;	//	t0
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//	定数バッファ
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;	//	b0
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//	ルートパラメータの設定
	D3D12_ROOT_PARAMETER rp[2] = {};
	//	テクスチャーバッファセット
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	//	定数バッファセット
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;


	//	サンプラー設定
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	ルートシグネイチャ設定
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 2;
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
	auto dev = _pWrap->GetDevice();

	result = dev->CreateRootSignature(
		0,
		rsBlob->GetBufferPointer(),
		rsBlob->GetBufferSize(),
		IID_PPV_ARGS(_rs.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポリゴン2D用のルートシグネイチャー作成失敗\n");
		return;
	}
}

//	パイプライン作成
void Renderer2D::CreatePipeline(void)
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
		L"polygon2DVS.hlsl", nullptr,
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
		L"polygon2DPS.hlsl", nullptr,
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

	// ブレンド
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;	//	false:RT[0]のみ反映させる、true:RT[0~7]各自で設定する

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		blendDesc.RenderTarget[i].BlendEnable = true;						//	ブレンドするか否か
		blendDesc.RenderTarget[i].LogicOpEnable = false;					//	論理演算を行うか否か
		blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;			//	RGB値に対してαを乗算する（SRCrgb * SRCα）
		blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;	//	RGB値に対して1-αを計算する（DESTrgb * DESTα）
		blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;				//	加算合成
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;			//	素材の元の色になる
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;		//	なんでもゼロにする
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;		//	加算合成
		blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;			//	演算しない、描画先のまま（Dest)
		blendDesc.RenderTarget[i].RenderTargetWriteMask =					//	すべての要素をブレンドする
			D3D12_COLOR_WRITE_ENABLE_ALL;									
	}
	gpsDesc.BlendState = blendDesc;

	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsDesc.DepthStencilState;
	gpsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpsDesc.pRootSignature = _rs.Get();
	//	ペラポリゴンのパイプライン作成
	auto dev = _pWrap->GetDevice();
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_pls.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポリゴン2D用グラフィックパイプライン作成失敗\n");
		return;
	}

}