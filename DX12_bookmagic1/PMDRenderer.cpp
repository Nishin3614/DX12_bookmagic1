//	インクルード
#include "PMDRenderer.h"
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "Dx12Wrapper.h"
#include "helper.h"

//	ライブラリリンク
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

//	コンストラクタ
PMDRenderer::PMDRenderer(Dx12Wrapper* pDxWap) :
	_pDxWap(pDxWap)
{
}

//	初期化処理
void PMDRenderer::Init(void)
{
	//	ルートパラメータ、ルートシグネイチャの作成
	CreateRootParameterOrRootSignature();
	//	グラフィックパイプラインの作成
	CreateGraphicPipeline();
}

//	描画処理
void PMDRenderer::Draw(void)
{
	//	パイプラインステートをセット
	_pDxWap->GetCmdList()->SetPipelineState(_pipelinestate.Get());

	//	ルートシグネイチャをセット
	_pDxWap->GetCmdList()->SetGraphicsRootSignature(_rootsignature.Get());
}

void PMDRenderer::PreShadowDraw(void)
{
	//	パイプラインステートをセット
	_pDxWap->GetCmdList()->SetPipelineState(_plsShadow.Get());

	//	ルートシグネイチャをセット
	_pDxWap->GetCmdList()->SetGraphicsRootSignature(_rootsignature.Get());
}

//	オブジェクトの解放処理
void PMDRenderer::Release(void)
{

}

//	更新処理
void PMDRenderer::Update(void)
{

}

//	ルートパラメータとルートシグネイチャの作成
void PMDRenderer::CreateRootParameterOrRootSignature(void)
{
	//	ルートパラメーターの作成	//	
	//	ディスクリプタレンジの設定
	CD3DX12_DESCRIPTOR_RANGE descTblRange[5] = {};
	//	シーン用
	descTblRange[0].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,		//	種別：定数バッファ（定数バッファビュー（CBV)）
		1, 										//	ディスクリプタ数
		0,										//	0番のスロットから
		0,										//	レジスター領域
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND	//	連続したディスクリプタレンジが前のディスクリプタレンジの直後に来る
	);
	//	ワールド行列用
	descTblRange[1].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1,
		1
	);
	//	マテリアル用
	descTblRange[2].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,		
		1, 										
		2										
	);

	//	歪みテクスチャ
	descTblRange[3].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,		
		4, 										
		0										
	);

	//	シャドウマップ用テクスチャ
	descTblRange[4].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 4
	);

	//	ルートパラメータの設定
	CD3DX12_ROOT_PARAMETER rootparam[4] = {};
	//	ルートパラメータ　シーン用
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	//	ルートパラメータ　位置座標用
	rootparam[1].InitAsDescriptorTable(1, &descTblRange[1]);
	//	ルートパラメータ　マテリアル、テクスチャ用
	rootparam[2].InitAsDescriptorTable(2, &descTblRange[2]);
	//	シャドウマップ用
	rootparam[3].InitAsDescriptorTable(1, &descTblRange[4]);

	//	サンプラー設定
	CD3DX12_STATIC_SAMPLER_DESC samplerdesc[3] = {};
	samplerdesc[0].Init(0);
	
	//	トゥーンテクスチャ用
	samplerdesc[1].Init(1,
		D3D12_FILTER_ANISOTROPIC,			//	異方性補完（テクセルを取り出す際、複数の代表地点からテクセルを取り出す方式）
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//	横方向の繰り返さない
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//	縦方向の繰り返さない
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//	奥行の繰り返さない;

	//	シャドウマップ用
	samplerdesc[2] = samplerdesc[0];
	samplerdesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;	//	<=であればtrue(1.0),そうでなければfalse(0.0f)
	samplerdesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;	//	比較結果をバイジニア補間
	samplerdesc[2].MaxAnisotropy = 1;									//	深度傾斜を有効にする
	samplerdesc[2].ShaderRegister = 2;

	//	ルートシグネイチャの設定	//	
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	//	頂点情報（入力アセンブラ）が存在するという列挙型
	rootSignatureDesc.pParameters = rootparam;							//	ルートパラメータ配列の先頭アドレス
	rootSignatureDesc.NumParameters = 4;								//	ルートパラメータ数
	rootSignatureDesc.pStaticSamplers = samplerdesc;					//	サンプラー設定
	rootSignatureDesc.NumStaticSamplers = 3;							//	サンプラー数
	//	バイナリコードの作成
	ComPtr < ID3DBlob> rootSigBlob = nullptr;					//	ルートシグネイチャのデータ
	ComPtr < ID3DBlob> errorBlob = nullptr;						//	エラー用オブジェクト
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,				//	ルートシグネイチャの設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,	//	ルートシグネイチャバージョン
		&rootSigBlob,					//	ルートシグネイチャのデータ
		&errorBlob						//	エラーオブジェ
	);
	//	ルートシグネイチャのバイナリコードが正しく作成されているか確認
	Helper::DebugShaderError(result, errorBlob.Get());

	//	ルートシグネイチャオブジェの作成
	result = _pDxWap->GetDevice()->CreateRootSignature(
		0,									//	nodemask（今回GPUは一つなため0）
		rootSigBlob->GetBufferPointer(),	//	バイナリデータのポインター
		rootSigBlob->GetBufferSize(),		//	バイナリデータのサイズ
		IID_PPV_ARGS(_rootsignature.ReleaseAndGetAddressOf())		//	IDとルートシグネイチャオブジェ
	);
}

//	グラフィックパイプラインの作成
void PMDRenderer::CreateGraphicPipeline(void)
{
	//	頂点レイアウトを設定
	D3D12_INPUT_ELEMENT_DESC inputlayout[] =
	{
		{//	座標情報
			"POSITION",										//	セマンティクス名
			0,												//	同じセマンティクス名を使用するときのインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,					//	フォーマット
			0,												//	入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,					//	データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1頂点ごとに設定したレイアウトを指定する
			0												//	一度に描画するインスタンス数
		},
		{//	法線ベクトル
			"NORMAL",										//	セマンティクス名
			0,												//	同じセマンティクス名を使用するときのインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,					//	フォーマット
			0,												//	入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,					//	データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1頂点ごとに設定したレイアウトを指定する
			0												//	一度に描画するインスタンス数
		},
		{//	uv
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{//	ボーン番号
			"BONE_NO",										//	セマンティクス名
			0,												//	同じセマンティクス名を使用するときのインデックス
			DXGI_FORMAT_R16G16_UINT,						//	フォーマット
			0,												//	入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,					//	データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1頂点ごとに設定したレイアウトを指定する
			0												//	一度に描画するインスタンス数
		},
		{//	ボーン影響力
			"WEIGHT",										//	セマンティクス名
			0,												//	同じセマンティクス名を使用するときのインデックス
			DXGI_FORMAT_R8_UINT,						//	フォーマット
			0,												//	入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,					//	データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1頂点ごとに設定したレイアウトを指定する
			0												//	一度に描画するインスタンス数
		},
		{//	輪郭線フラグ
			"EDGE_FLAG",										//	セマンティクス名
			0,												//	同じセマンティクス名を使用するときのインデックス
			DXGI_FORMAT_R8_UINT,						//	フォーマット
			0,												//	入力スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,					//	データのオフセット位置
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1頂点ごとに設定したレイアウトを指定する
			0												//	一度に描画するインスタンス数
		}
	};

	//	グラフィックスパイプラインの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	//	ルートシグネイチャオブジェのセット
	gpipeline.pRootSignature = _rootsignature.Get();							//	ルートシグネイチャ

	//	シェーダーのセット
		//	エラーオブジェクト
	ComPtr<ID3DBlob> vsBlob = nullptr;
	ComPtr<ID3DBlob> psBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	//	頂点シェーダー読み込み
	auto result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",							//	シェーダー名
		nullptr,											//	defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	インクルードファイルはデフォルト
		"BasicVS",											//	エントリーポイントの関数名
		"vs_5_0",											//	対象シェーダー
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	デバッグ用 | 最適化なし
		0,													//	効果ファイル（今回はシェーダーファイルなので、0）
		vsBlob.ReleaseAndGetAddressOf(),					//	シェーダーオブジェクト
		errorBlob.ReleaseAndGetAddressOf()					//	エラーオブジェクト
	);
	//	頂点シェーダーの読み込みが正常に行えているか確認
	Helper::DebugShaderError(result, errorBlob.Get());

	//	ピクセルシェーダー読み込み
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",							//	シェーダー名
		nullptr,											//	defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	インクルードファイルはデフォルト
		"BasicPS",											//	エントリーポイントの関数名
		"ps_5_0",											//	対象シェーダー
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	デバッグ用 | 最適化なし
		0,													//	効果ファイル（今回はシェーダーファイルなので、0）
		psBlob.ReleaseAndGetAddressOf(),					//	シェーダーオブジェクト
		errorBlob.ReleaseAndGetAddressOf()											//	エラーオブジェクト
	);
	//	ピクセルシェーダーの読み込みが正常に行えているか確認
	Helper::DebugShaderError(result, errorBlob.Get());
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();	//	頂点シェーダーポインター
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();		//	頂点シェーダーのバッファサイズ
	gpipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();	//	ピクセルシェーダーポインター
	gpipeline.PS.BytecodeLength = psBlob->GetBufferSize();		//	ピクセルシェーダーのバッファサイズ

	//	デフォルトのサンプルマスクを表す定数（0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//	ラスタライザーステートの設定
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	//	カリングしない

	//	ブレンドステートの設定
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//	レンダーターゲットのブレンドステート設定
	//D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//renderTargetBlendDesc.BlendEnable = false;									//	ブレンドするか否か
	//renderTargetBlendDesc.LogicOpEnable = false;								//	論理演算するか否か
	//renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	//	書き込むときのマスク値

	//	入力レイアウトの設定
	gpipeline.InputLayout.pInputElementDescs = inputlayout;		//	レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputlayout);	//	レイアウト配列の要素数

	//	表現方法の設定
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//	カットなし

	//	構成要素の設定
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//	三角形で構成

	//	レンダーターゲットの設定
	{
		//	ターゲットの種類
		enum E_TARGET
		{
			COL,
			NORMAL,
			BLOOM,
			MAX
		};
		gpipeline.NumRenderTargets = MAX;							//	レンダーターゲット数
		for (int nTarget = 0; nTarget < MAX; nTarget++)
		{
			gpipeline.RTVFormats[nTarget] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//	0～1に正規化したsRGBA
		}
	}

	//	アンチエイリアシングのためのサンプル数設定
	gpipeline.SampleDesc.Count = 1;		//	サンプリングは1ピクセルにつき1
	gpipeline.SampleDesc.Quality = 0;	//	クオリティーは最低（0）

	//	デプスステンシルの設定
	gpipeline.DepthStencilState.DepthEnable = true;								//	深度バッファ使用
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//	ピクセル描画時に深度バッファに深度値を書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//	深度値が小さい方を採用する
	gpipeline.DepthStencilState.StencilEnable = false;							//	ステンシルバッファ未対応
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//	グラフィックスパイプラインステートオブジェクトの作成
	result = _pDxWap->GetDevice()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf())
	);

	//	シャドウマップ用のパイプライン作成	//
	//	シャドウマップ用頂点シェーダー読み込み
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",							//	シェーダー名
		nullptr,											//	defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	インクルードファイルはデフォルト
		"ShadowVS",											//	エントリーポイントの関数名
		"vs_5_0",											//	対象シェーダー
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	デバッグ用 | 最適化なし
		0,													//	効果ファイル（今回はシェーダーファイルなので、0）
		vsBlob.ReleaseAndGetAddressOf(),					//	シェーダーオブジェクト
		errorBlob.ReleaseAndGetAddressOf()					//	エラーオブジェクト
	);
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();	//	頂点シェーダーポインター
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();		//	頂点シェーダーのバッファサイズ
	gpipeline.PS.pShaderBytecode = nullptr;						//	ピクセルシェーダーなし
	gpipeline.PS.BytecodeLength = 0;
	//	ターゲットフォーマットの初期化
	for (unsigned int nTarget = 0; nTarget < gpipeline.NumRenderTargets; nTarget++)
	{
		gpipeline.RTVFormats[nTarget] = DXGI_FORMAT_UNKNOWN;
	}
	gpipeline.NumRenderTargets = 0;								//	レンダーターゲットなし

	result = _pDxWap->GetDevice()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_plsShadow.ReleaseAndGetAddressOf())
	);

}