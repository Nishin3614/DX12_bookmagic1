//	インクルード
#include "VisualEffect.h"
#include "DXApplication.h"
#include "Dx12Wrapper.h"
#include "sceneInfo.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "helper.h"

//	マルチパスレンダリング用
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

//	外部ファイルからアクセスできなくするため、無名名前空間で定義
namespace//列挙型用
{
	//	オリジン用レンダーターゲットビュー種類
	enum class E_ORIGIN_RTV : int
	{
		COL,	//	通常カラー
		NORMAL,	//	法線
		MAX_NORMALDROW,

		BLOOM = MAX_NORMALDROW,
		SHRINKBLOOM,
		MAX_BLOOM,

		DOF = MAX_BLOOM,
		MAX_DOF,

		PROCE = MAX_DOF,	//	加工用
		MAX
	};

	//	オリジン用SRV,CBV種類
	enum class E_ORIGIN_SRV : int
	{
		/*	SRV	*/
		//	通常描画
		COL,	//	通常カラー
		NORMAL,	//	法線
		MAX_NORMALDROW,

		//	ブルーム
		BLOOM = MAX_NORMALDROW,
		SHRINKBLOOM,

		//	被写界深度
		DOF,	//	被写界深度

		//	画像加工用
		PROCE,	//	加工用

		/*	CBV	*/
		BOKE,

		MAX
	};
	//	定数定義
	//constexpr float CLSCLR[4] = { 0.5f,0.5f,0.5f,1.0f };		//	レンダーターゲットクリアカラー
	constexpr float NONE_CLSCLR[4] = { 0.0f,0.0f,0.0f,1.0f };	//	合成するレンダーターゲットのクリアカラー
	constexpr float WHITE_CLSCLR[4] = { 1.0f,1.0f,1.0f,1.0f };	//	白のクリアカラー
	constexpr float SHADOW_DIFINITION = 40.0f;					//	ライトデプスの縦横サイズ
	constexpr UINT UINT_SHADOW_DIFINITION = 40;
	constexpr UINT64 UINT64_SHADOW_DIFINITION = 40;
}

//	名前空間
using namespace Microsoft::WRL;
using namespace DirectX;


//	コンストラクタ
VisualEffect::VisualEffect(Dx12Wrapper * pWrap) :
	_pWrap(pWrap),
	_prPoriVBV({})
{
}

/*	初期化関連の処理	*/
//	初期化処理
void VisualEffect::Init(void)
{	
	//	深度バッファ作成
	CreateDepthView();
	//	ペラポリゴンに張り付けるためのリソースを作成
	CreateOriginRenderTarget();
	//	ポストエフェクト用のバッファ、ビュー作成
	CreateEffectBufferAndView();
	//	ペラポリゴンの作成
	CreatePeraVertexBuff();
	CreatePeraRootSignature();
	CreateAoRootSignature();
	CreatePeraGraphicPipeLine();
	//	ssao作成
	CreateAmbientOcculusionBuffer();
	CreateAmbientOcculusionDescriptorHeap();
	//	ポスト設定用作成
	CreatePostSetting();
}

//	オリジンのレンダーターゲット作成
void VisualEffect::CreateOriginRenderTarget(void)
{
	//	バッファ作成	//
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	ヒーププロパティー設定
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	レンダリング時のクリア値と同じ値
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, _pWrap->GetBgCol());
	//	バッファの作成
	for (auto& res : _origin1Resource)
	{
		auto result = dev->CreateCommittedResource(
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

	//	レンダリング時のクリア値と同じ値
	clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, NONE_CLSCLR);
	//	ブルームバッファの作成
	for (auto& res : _bloomBuffer)
	{
		
		auto result = dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(res.ReleaseAndGetAddressOf()));
		//	サイズを半分にする
		resDesc.Width >>= 1;
		if (FAILED(result))
		{
			Helper::DebugOutputFormatString("ブルームのレンダーターゲットバッファ作成失敗");
			return;
		}
	}

	//	被写界深度バッファ作成
	CreateBlurForDOFBuffer();

	//	加工用のレンダーターゲット作成
	CreateProcessRenderTarget();

	//	ディスクリプタヒープ作成	//
	//	作成済みのヒープ情報を使ってもう1毎作る
	auto heapDesc = _pWrap->GetDescriptorHeapD();
	//	RTV用ヒープを作る
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_RTV::MAX);
	auto result = dev->CreateDescriptorHeap(
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
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	レンダーターゲットビューのインクリメントサイズ
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(baseH);											//	ハンドル

	//	通常描画のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::COL);
	for (auto& res : _origin1Resource)
	{
		handle.InitOffsetted(baseH, offset);
		dev->CreateRenderTargetView(
			res.Get(),
			&rtvDesc,
			handle
		);
		offset += incSize;
	}

	//	ブルーム用のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::BLOOM);
	for (auto& res : _bloomBuffer)
	{
		handle.InitOffsetted(baseH, offset);
		dev->CreateRenderTargetView(
			res.Get(),
			&rtvDesc,
			handle
		);
		offset += incSize;
	}

	//	被写界深度用のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::DOF);
	handle.InitOffsetted(baseH, offset);
	dev->CreateRenderTargetView(
		_dofBuffer.Get(),
		&rtvDesc,
		handle
	);

	//	加工用のレンダーターゲットビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::PROCE);
	handle.InitOffsetted(baseH, offset);
	dev->CreateRenderTargetView(
		_proceResource.Get(),
		&rtvDesc,
		handle
	);

	//	SRV用ヒープを作る
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_SRV::MAX);
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(
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
	incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//	レンダーターゲットビューのインクリメントサイズ

	offset = incSize * static_cast<int>(E_ORIGIN_SRV::COL);
	for (auto& res : _origin1Resource)
	{
		handle.InitOffsetted(baseH, offset);
		dev->CreateShaderResourceView(
			res.Get(),
			&srvDesc,
			handle
		);
		offset += incSize;
	}

	//	ブルームのシェーダーリソースビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::BLOOM);
	for (auto& res : _bloomBuffer)
	{
		handle.InitOffsetted(baseH, offset);
		dev->CreateShaderResourceView(
			res.Get(),
			&srvDesc,
			handle
		);
		offset += incSize;
	}

	//	被写界深度用のシェーダーリソース作成
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::DOF);
	handle.InitOffsetted(baseH, offset);
	dev->CreateShaderResourceView(
		_dofBuffer.Get(),
		&srvDesc,
		handle
	);

	//	加工用のシェーダーリソースビュー作成
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::PROCE);
	handle.InitOffsetted(baseH, offset);
	dev->CreateShaderResourceView(
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
	dev->CreateConstantBufferView(&cbvDesc, handle);

}

//	加工用のレンダーターゲット作成
void VisualEffect::CreateProcessRenderTarget(void)
{
	//	バッファ作成	//
//	使っているバックバッファの情報を利用する
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	//	ヒーププロパティー設定
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	レンダリング時のクリア値と同じ値
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, _pWrap->GetBgCol());
	//	バッファの作成
	auto result = dev->CreateCommittedResource(
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

//	被写界深度用バッファ作成
void VisualEffect::CreateBlurForDOFBuffer(void)
{
	//	使っているバックバッファの情報を利用する
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	ヒーププロパティー設定
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	レンダリング時のクリア値と同じ値
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, NONE_CLSCLR);
	resDesc.Width >>= 1;	//	縮小バッファなので大きさは半分
	//	バッファの作成
	auto result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_dofBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("被写界深度用バッファ作成失敗");
		return;
	}
}

//	ペラポリゴンの頂点バッファ作成
void VisualEffect::CreatePeraVertexBuff(void)
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
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateCommittedResource(
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
void VisualEffect::CreateBokeConstantBuff(void)
{
	//	分散値の計算
	//	ウェイト値計算
	std::vector<float> weights = Helper::GetGaussianWeights(8, 5.0f);;	//	各ウェイト値

	//	定数バッファの作成
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(
		Helper::AlignmentedSize(sizeof(weights[0]) * weights.size(), 256));
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateCommittedResource(
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
void VisualEffect::CreatePeraRootSignature(void)
{
	//	レンジの設定
	D3D12_DESCRIPTOR_RANGE ranges[7] = {};
	//	通常カラー、法線セット、高輝度、縮小高輝度、縮小通常
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 5;	//	t0,t1,t2,t3,t4
	//	ぼけ定数バッファセット
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;	//	b0
	//	ポストエフェクト用のバッファセット
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].NumDescriptors = 1;	//	t5
	//	深度値テクスチャ用
	ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[3].NumDescriptors = 1;	//	t6
	//	ライトデプステクスチャ用
	ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[4].NumDescriptors = 1;	//	t7
	//	SSAOテクスチャ用
	ranges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[5].NumDescriptors = 1;
	//	ポスト設定定数バッファ
	ranges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[6].NumDescriptors = 1;	//	b1
	ranges[6].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

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
		//	CBV
		else if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			range.BaseShaderRegister = nCBVRegister;
			nCBVRegister += range.NumDescriptors;
		}
		else {}
	}

	//	ルートパラメータの設定
	D3D12_ROOT_PARAMETER rp[7] = {};
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
	//	SSAOテクスチャ用
	rp[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[5].DescriptorTable.pDescriptorRanges = &ranges[5];
	rp[5].DescriptorTable.NumDescriptorRanges = 1;
	//	ポスト設定バッファ
	rp[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[6].DescriptorTable.pDescriptorRanges = &ranges[6];
	rp[6].DescriptorTable.NumDescriptorRanges = 1;

	//	サンプラー設定
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	ルートシグネイチャ設定
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 7;
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
		IID_PPV_ARGS(_prPoriRS.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ペラポリゴン用ルートシグネイチャー作成失敗\n");
		return;
	}

}

//	AOルートシグネイチャ作成
void VisualEffect::CreateAoRootSignature(void)
{
	//	レンジの設定
	D3D12_DESCRIPTOR_RANGE ranges[3] = {};
	//	通常カラー、法線セット、高輝度、縮小高輝度、縮小通常
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 5;	//	t0,t1,t2,t3,t4
	//	深度値テクスチャ用
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[1].NumDescriptors = 1;	//	t5
	//	シーン情報定数バッファセット
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[2].NumDescriptors = 1;	//	b0

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
		//	CBV
		else if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			range.BaseShaderRegister = nCBVRegister;
			nCBVRegister += range.NumDescriptors;
		}
		else {}
	}

	//	ルートパラメータの設定
	D3D12_ROOT_PARAMETER rp[3] = {};
	//	テクスチャーバッファセット
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	//	深度値テクスチャー用
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;
	//	シーン情報定数バッファセット
	rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[2].DescriptorTable.pDescriptorRanges = &ranges[2];
	rp[2].DescriptorTable.NumDescriptorRanges = 1;
	   
	//	サンプラー設定
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	ルートシグネイチャ設定
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 3;
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
		IID_PPV_ARGS(_aoRS.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ペラポリゴン用ルートシグネイチャー作成失敗\n");
		return;
	}
}

//	ペラポリゴン用グラフィックスパイプライン作成
void VisualEffect::CreatePeraGraphicPipeLine(void)
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
	auto dev = _pWrap->GetDevice();
	result = dev->CreateGraphicsPipelineState(
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
	Helper::DebugShaderError(result, errBlob.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_procePipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("加工用グラフィックパイプライン作成失敗\n");
		return;
	}

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
	Helper::DebugShaderError(result, errBlob.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_effectPipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポストエフェクト用グラフィックパイプライン作成失敗\n");
		return;
	}

	//	ぼかしパイプライン作成
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BlurPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	Helper::DebugShaderError(result, errBlob.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	gpsDesc.NumRenderTargets = 2;
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_blurPipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ぼかし用グラフィックパイプライン作成失敗\n");
		return;
	}

	//	SSAO用のパイプライン作成
	result = D3DCompileFromFile(
		L"ssaoPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		ps.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf()
	);
	Helper::DebugShaderError(result, errBlob.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.pRootSignature = _aoRS.Get();
	gpsDesc.BlendState.RenderTarget[0].BlendEnable = false;	//	formatがR32_FLOATのため、a値が0.0fでブレンドされてしまうためfalseにする
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_aoPipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ao用グラフィックパイプライン作成失敗\n");
		return;
	}
}

//	エフェクト用のバッファとビュー作成
bool VisualEffect::CreateEffectBufferAndView(void)
{
	//	ポストエフェクト用テクスチャーバッファを作成
	std::string sNormal = "normal/glass_n.png";
	_efffectTexBuffer = _pWrap->LoadTextureFromFile(sNormal);
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
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateDescriptorHeap(
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
	dev->CreateShaderResourceView(
		_efffectTexBuffer.Get(),
		&srvDesc,
		_effectSRVHeap->GetCPUDescriptorHandleForHeapStart()
	);


	return false;
}

//	深度バッファの作成
void VisualEffect::CreateDepthView(void)
{
	//	ウィンドウサイズを取得
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();
	//	デバイス情報取得
	auto dev = _pWrap->GetDevice();

	//	深度バッファの設定
	CD3DX12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		//DXGI_FORMAT_D32_FLOAT,					//	深度値書き込み用フォーマット
		DXGI_FORMAT_R32_TYPELESS,		//	型レス（ビュー側で自由に型を決められる）
		size.cx,							//	レンダーターゲットと同じ
		size.cy,							//	レンダーターゲットと同じ
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
	auto result = dev->CreateCommittedResource(
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
	depthResDesc.Width = SHADOW_DIFINITION;
	depthResDesc.Height = SHADOW_DIFINITION;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	result = dev->CreateCommittedResource(
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
	result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));
	//	深度バッファビューの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					//	深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	//	2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;					//	フラグなし
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	ディスクリプタヒープ内に深度バッファビューを作成
	dev->CreateDepthStencilView(
		_depthBuffer.Get(),
		&dsvDesc,
		handle
	);
	//	ライトデプス用の深度バッファビュー作成
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dev->CreateDepthStencilView(
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
	result = dev->CreateDescriptorHeap(&SRVheapDesc, IID_PPV_ARGS(_depthSRVHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("深度値テクスチャー用のディスクリプタヒープ作成失敗\n");
		return;
	}
	//	通常デプス→テクスチャ用
	D3D12_SHADER_RESOURCE_VIEW_DESC srvResDesc = {};
	srvResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvResDesc.Texture2D.MipLevels = 1;
	srvResDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvResDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//	2Dテクスチャ
	handle = _depthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateShaderResourceView(
		_depthBuffer.Get(),
		&srvResDesc,
		handle
	);
	//	ライトデプス→テクスチャ用
	handle.ptr += dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dev->CreateShaderResourceView(
		_lightDepthBuffer.Get(),
		&srvResDesc,
		handle
	);
}

//	ポスト設定作成
void VisualEffect::CreatePostSetting(void)
{
	//	バッファサイズ
	auto bufferSize = Helper::AlignmentedSize(
		sizeof(PostSetting),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
	);
	UINT64 u_64BufferSize = (UINT64)Helper::AlignmentedSize(
		sizeof(PostSetting),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);	//	256アライメントにそろえたサイズ
	UINT BufferSize = (UINT)Helper::AlignmentedSize(
		sizeof(PostSetting),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);	//	256アライメントにそろえたサイズ


	//	バッファ作成
	D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(u_64BufferSize);
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_postSettingResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("postSettingのバッファ作成失敗\n");
	}
	//	ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_postSettingDH.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("postSettingのディスクリプタヒープ作成失敗\n");
	}
	//	ビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _postSettingResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = BufferSize;
	dev->CreateConstantBufferView(
		&cbvDesc,
		_postSettingDH->GetCPUDescriptorHandleForHeapStart()
	);
	//	ポスト設定
	result = _postSettingResource->Map(0, nullptr, (void**)&_pMapPostSetting);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("postSettingのマップ失敗\n");
	}
	_pMapPostSetting->nDebugDisp = 0;
	_pMapPostSetting->nSSAO = 0;
	_pMapPostSetting->bloomColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}

//	アンビエントオクルージョンバッファの作成
bool VisualEffect::CreateAmbientOcculusionBuffer(void)
{
	//	バッファ作成	//
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Color[0] = clearValue.Color[1]
		= clearValue.Color[2]
		= clearValue.Color[3]
		= 1.0f;
	clearValue.Format = resDesc.Format;
	auto result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_aoBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("アンビエントオクルージョンバッファの作成失敗\n");
		assert(0);
		return false;
	}
	return true;
}

//	アンビエントオクルージョンディスクリプタヒープ作成
bool VisualEffect::CreateAmbientOcculusionDescriptorHeap(void)
{
	auto dev = _pWrap->GetDevice();

	//	RTV用ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	auto result = dev->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(_aoRTVDH.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("AOのRTV用ディスクリプターヒープの作成失敗\n");
		assert(0);
		return false;
	}
	//	RTV作成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	dev->CreateRenderTargetView(
		_aoBuffer.Get(),
		&rtvDesc,
		_aoRTVDH->GetCPUDescriptorHandleForHeapStart()
	);

	//	SRV用ヒープ作成
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = dev->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(_aoSRVDH.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("AOのSRV用ディスクリプターヒープの作成失敗\n");
		assert(0);
		return false;
	}

	//	SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	dev->CreateShaderResourceView(
		_aoBuffer.Get(),
		&srvDesc,
		_aoSRVDH->GetCPUDescriptorHandleForHeapStart()
	);
	return true;
}

//	レンダーターゲットをセットする処理
void VisualEffect::PreOriginDraw(void)
{
	//	オリジンレンダーターゲットに描画する前にバリア設定を行う
	for (auto& res : _origin1Resource)
	{
		_pWrap->SetBarrier(
			res.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	//	ブルーム描画前にバリア設定
	_pWrap->SetBarrier(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	RTVハンドルのセット
	auto dev = _pWrap->GetDevice();
	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();						//	RTVのスタートポイント
	uint32_t offset = 0;																	//	ビューのオフセット位置
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	レンダーターゲットビューのインクリメントサイズ
	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[static_cast<int>(E_ORIGIN_RTV::BLOOM) + 1];											//	ハンドル
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, offset);
		offset += incSize;
	}

	//	深度バッファ用ディスクリプタヒープハンドル取得
	auto dsvHeapPointer = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	レンダーターゲットセット
	auto cmdList = _pWrap->GetCmdList();
	cmdList->OMSetRenderTargets(
		(static_cast<int>(E_ORIGIN_RTV::BLOOM) + 1), handles, false, &dsvHeapPointer);
	//クリアカラー		 R   G   B   A
	//	オリジン用のレンダーターゲットビューをクリア
	for (int i = 0;i<_countof(handles);i++)
	{
		//	ブルームのクリアカラーを黒にする
		if (i == static_cast<int>(E_ORIGIN_RTV::BLOOM))
		{
			cmdList->ClearRenderTargetView(handles[i], NONE_CLSCLR, 0, nullptr);
		}
		else
		{
			cmdList->ClearRenderTargetView(handles[i], _pWrap->GetBgCol(), 0, nullptr);
		}
	}
	//	深度バッファビューをクリア
	cmdList->ClearDepthStencilView(dsvHeapPointer,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	//	描画箇所セット
	auto& WinApp = Win32Application::Instance();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//ビューポート
	
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形

}

//	オリジンレンダーターゲットの描画終了
void VisualEffect::EndOriginDraw(void)
{
	//	モデル描画後のバリア設定
	for (auto& res : _origin1Resource)
	{
		_pWrap->SetBarrier(
			res.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	//	ブルーム描画後バリア指定
	_pWrap->SetBarrier(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//	加工用のレンダーターゲットの描画
void VisualEffect::ProceDraw(void)
{
	//	加工用描画前バリア指定
	_pWrap->SetBarrier(
		_proceResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	加工用RTVをセット
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapPointer;
	auto dev = _pWrap->GetDevice();
	rtvHeapPointer.InitOffsetted(
		_originRTVHeap->GetCPUDescriptorHandleForHeapStart(),
		dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * static_cast<int>(E_ORIGIN_RTV::PROCE)
	);
	auto cmdList = _pWrap->GetCmdList();

	cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	クリアカラー	
	cmdList->ClearRenderTargetView(rtvHeapPointer, _pWrap->GetBgCol(), 0, nullptr);
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形


	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	ペラポリゴン用のルートシグネイチャセット
	cmdList->SetPipelineState(_prPoriPipeline.Get());						//	ペラポリゴン用のパイプラインセット
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	ペラポリゴン用の頂点バッファビューセット
	//	オリジン用のSRVヒープをセット
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	int incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	パラメーター0番（テクスチャーリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	パラメーター0番（定数バッファリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	通常デプス深度バッファーテクスチャ
	cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	//	深度バッファーテクスチャ用のリソースとヒープを関連付ける
	handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(
		3,
		handle);
	
	//	ライトデプス深度バッファテクスチャ
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetGraphicsRootDescriptorTable(
		4,
		handle);
	//	ポスト設定をヒープと関連付ける
	cmdList->SetDescriptorHeaps(1, _postSettingDH.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(
		6,
		_postSettingDH->GetGPUDescriptorHandleForHeapStart()
	);

	cmdList->DrawInstanced(4, 1, 0, 0);


	//	加工用描画後バリア指定
	_pWrap->SetBarrier(
		_proceResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//	（ペラポリゴン仮の）描画
void VisualEffect::EndDraw(void)
{
	auto cmdList = _pWrap->GetCmdList();
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//ビューポート
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形

	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	ペラポリゴン用のルートシグネイチャセット
	cmdList->SetPipelineState(_procePipeline.Get());						//	ペラポリゴン用のパイプラインセット

	//	オリジン用のSRVヒープをセット
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	//	加工用のテクスチャとヒープを関連付ける
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	auto dev = _pWrap->GetDevice();
	int incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::PROCE));
	cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	パラメーター0番（定数バッファリソース）とヒープを関連付ける
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	ポストエフェクト用のSRVヒープをセット
	cmdList->SetDescriptorHeaps(1, _effectSRVHeap.GetAddressOf());
	//	ポストエフェクト用のリソースとヒープを関連付ける
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		_effectSRVHeap->GetGPUDescriptorHandleForHeapStart());

	//	SSAOテクスチャとヒープを関連付ける
	cmdList->SetDescriptorHeaps(1, _aoSRVDH.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(
		5,
		_aoSRVDH->GetGPUDescriptorHandleForHeapStart()
	);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	ペラポリゴン用の頂点バッファビューセット
	cmdList->DrawInstanced(4, 1, 0, 0);
}

#define SHRINKCOUNT 8	//	縮小回数
//	縮小バッファぼかし描画処理
void VisualEffect::DrawShrinkTextureForBlur(void)
{
	auto cmdList = _pWrap->GetCmdList();

	cmdList->SetPipelineState(_blurPipeline.Get());
	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());

	//	頂点バッファセット
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);

	//	縮小バッファはレンダーターゲットに
	_pWrap->SetBarrier(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	通常ぼかしのレンダーターゲット
	_pWrap->SetBarrier(
		_dofBuffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	レンダーターゲットセット
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto dev = _pWrap->GetDevice();
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvHandles[0].InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_RTV::SHRINKBLOOM));
	rtvHandles[1].InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_RTV::DOF));
	cmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);
	//	レンダーターゲットクリア
	for (auto& rtv : rtvHandles)
	{
		cmdList->ClearRenderTargetView(rtv, NONE_CLSCLR, 0, nullptr);
	}

	//	シェーダーリソースセット
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
	auto srvH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	通常レンダリングセット
	srvHandle.InitOffsetted(
		srvH,
		incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	cmdList->SetGraphicsRootDescriptorTable(static_cast<int>(E_ORIGIN_SRV::COL), srvHandle);

	//	縮小バッファの初期サイズ設定
	//		原寸サイズの半分のサイズに初期化している
	auto desc = _bloomBuffer[0]->GetDesc();
	D3D12_VIEWPORT vp = {};
	D3D12_RECT sr = {};
	
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.Height = desc.Height / 2.0f;
	vp.Width = desc.Width / 2.0f;
	sr.top = 0;
	sr.left = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	for (int i = 0; i < SHRINKCOUNT; i++)
	{
		//	描画範囲セット
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &sr);
		//	描画
		cmdList->DrawInstanced(4, 1, 0, 0);

		//	下にずらす
		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;

		//	幅も高さも半分
		//	※sr.rightも変更するのか？
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
		sr.right = vp.Width;
	}

	//	縮小高輝度をシェーダーリソースに
	_pWrap->SetBarrier(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//	縮小通常をシェーダーリソースに
	_pWrap->SetBarrier(
		_dofBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//	シャドウマップ描画
void VisualEffect::ShadowDraw(void)
{
	auto cmdList = _pWrap->GetCmdList();
	auto dev = _pWrap->GetDevice();

	//	深度バッファ用ディスクリプタヒープハンドル取得
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//	レンダーターゲットセット
	cmdList->OMSetRenderTargets(0, nullptr, false, &handle);
	//	深度バッファビューをクリア
	cmdList->ClearDepthStencilView(handle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, SHADOW_DIFINITION, SHADOW_DIFINITION);
	cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, SHADOW_DIFINITION, SHADOW_DIFINITION);
	cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形
}

//	深度用SRVセット
void VisualEffect::DepthSRVSet(void)
{
	auto cmdList = _pWrap->GetCmdList();
	auto dev = _pWrap->GetDevice();
	//	深度SRVをセット
	cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	auto handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetGraphicsRootDescriptorTable(3, handle);
}

//	アンビエントオクルージョンによる描画
void VisualEffect::DrawAmbientOcculusion(void)
{
	//	アンビエントオクルージョン用描画前バリア指定
	_pWrap->SetBarrier(
		_aoBuffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	アンビエントオクルージョン用RTVをセット
	auto cmdList = _pWrap->GetCmdList();
	auto rtvBaseH = _aoRTVDH->GetCPUDescriptorHandleForHeapStart();
	cmdList->OMSetRenderTargets(1, &rtvBaseH, false, nullptr);

	//	クリアカラー	
	cmdList->ClearRenderTargetView(rtvBaseH, WHITE_CLSCLR, 0, nullptr);

	//	ビューポート・シザー矩形セット
	auto& WinApp = Win32Application::Instance();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//ビューポート
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形

	//	ルートシグネイチャ、pipelineのセット
	cmdList->SetGraphicsRootSignature(_aoRS.Get());					//	ペラポリゴン用のルートシグネイチャセット
	cmdList->SetPipelineState(_aoPipeline.Get());						//	ssaopipeline用のパイプラインセット

	//	頂点バッファビューのセット
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	ペラポリゴン用の頂点バッファビューセット

	//	法線テクスチャのため
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	auto dev = _pWrap->GetDevice();
	int incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	深度用テクスチャのため
	cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(
		1,
		handle);
	//	シーン情報の定数バッファ
	auto& pDX = DXApplication::Instance();
	auto pSceneInfo = pDX.GetSceneInfo();
	pSceneInfo->CommandSet_SceneView(2);
	//	描画
	cmdList->DrawInstanced(4, 1, 0, 0);


	//	アンビエントオクルージョン用描画後バリア指定
	_pWrap->SetBarrier(
		_aoBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

}

//	ポスト設定
void VisualEffect::SetPostSetting(bool bDebugDisp, bool bSSAO,bool bMonochro[3], bool bReverse, bool bDof, float bloomColor[3])
{
	_pMapPostSetting->nDebugDisp = static_cast<int>(bDebugDisp);
	_pMapPostSetting->nSSAO = static_cast<int>(bSSAO);
	for (int nCntMono = 0;nCntMono < 3;nCntMono++)
	{
		_pMapPostSetting->nMonochro[nCntMono] = static_cast<int>(bMonochro[nCntMono]);
	}
	_pMapPostSetting->nReverse = static_cast<int>(bReverse);
	_pMapPostSetting->nDof = static_cast<int>(bDof);
	_pMapPostSetting->bloomColor.x = bloomColor[0];
	_pMapPostSetting->bloomColor.y = bloomColor[1];
	_pMapPostSetting->bloomColor.z = bloomColor[2];
}
