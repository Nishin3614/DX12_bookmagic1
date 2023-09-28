#include "polygon2D.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"
#include <d3dx12.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")


//	コンストラクタ
Polygon2D::Polygon2D(Dx12Wrapper* pWrap,std::string texName) : _pWrap(pWrap),_texName(texName)
{
}

//	初期化処理
void Polygon2D::Init(void)
{
	//	テクスチャー作成
	CreateTexBuffer();
	//	頂点バッファ作成
	CreateVertexBuffer();
}

//	描画処理
void Polygon2D::Draw(void)
{
	auto cmdlist = _pWrap->GetCmdList();
	//	テクスチャとヒープを関連付ける
	cmdlist->SetDescriptorHeaps(1, _dh.GetAddressOf());
	auto dhH = _dh->GetGPUDescriptorHandleForHeapStart();
	cmdlist->SetGraphicsRootDescriptorTable(0, dhH);
	dhH.ptr += _pWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdlist->SetGraphicsRootDescriptorTable(1, dhH);

	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdlist->IASetVertexBuffers(0, 1, &_vbv);						//	ペラポリゴン用の頂点バッファビューセット
	cmdlist->DrawInstanced(4, 1, 0, 0);
}

//	頂点バッファ作成
void Polygon2D::CreateVertexBuffer(void)
{
	struct PeraVertex
	{
		DirectX::XMFLOAT3 point;
		DirectX::XMFLOAT2 uv;
	};
	PeraVertex pv[4] = {
		{{0.0f,100.0f,0.0f},{0,1}},		//	左下
		{{0.0f,0.0f,0.0f},{0,0}},		//	左上
		{{100.0f,100.0f,0.0f},{1,1}},		//	右下
		{{100.0f,0.0f,0.0f},{1,0}}		//	右上

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
		IID_PPV_ARGS(_vb.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポリゴン2Dの頂点バッファ作成失敗");
		return;
	}
	//	頂点バッファビュー作成
	_vbv.BufferLocation = _vb->GetGPUVirtualAddress();
	_vbv.SizeInBytes = sizeof(pv);
	_vbv.StrideInBytes = sizeof(PeraVertex);

	//	頂点情報コピー
	PeraVertex* mappedPera = nullptr;
	UINT64 width = _texBuffer->GetDesc().Width;
	UINT height = _texBuffer->GetDesc().Height;
	pv[0].point.y = pv[2].point.y = height;
	pv[2].point.x = pv[3].point.x = width;
	_vb->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	_vb->Unmap(0, nullptr);
}

//	バッファ作成
void Polygon2D::CreateTexBuffer(void)
{
	//	バッファ作成	//
	auto dev = _pWrap->GetDevice();
	//	テクスチャー読みこみ
	_texBuffer = _pWrap->LoadTextureFromFile(_texName);
	//	定数バッファ作成
	CreateMatBaffer();
	//	ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
	dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dhDesc.NodeMask = 0;
	dhDesc.NumDescriptors = 2;
	dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = dev->CreateDescriptorHeap(
		&dhDesc,
		IID_PPV_ARGS(_dh.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポリゴン2D描画のバッファ作成失敗");
	}
	//	シェーダーリソースビュー作成（テクスチャー）
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = _texBuffer->GetDesc().Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	auto dhH =_dh->GetCPUDescriptorHandleForHeapStart();
	dev->CreateShaderResourceView(
		_texBuffer.Get(),
		&srvDesc,
		dhH
	);
	//	定数バッファビュー作成
	dhH.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _constBuffer->GetDesc().Width;
	dev->CreateConstantBufferView(&cbvDesc, dhH);
}

//	行列バッファ作成
void Polygon2D::CreateMatBaffer(void)
{
	//	定数バッファ作成
	auto dev = _pWrap->GetDevice();
	D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(DirectX::XMMATRIX) + 0xff) & ~0xff);
	auto result = dev->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_constBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("ポリゴン2Dの定数バッファ作成失敗");
	}

	//	定数のコピー
	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
	auto size = Win32Application::Instance().GetWindowSize();
	DirectX::XMMATRIX* mapMat;
	result = _constBuffer->Map(0, nullptr, (void**)&mapMat);
	{	//	ピクセル座標→ディスプレイ座標
		mat.r[0].m128_f32[0] = 2.0f / size.cx;
		mat.r[1].m128_f32[1] = -2.0f / size.cy;
		mat.r[3].m128_f32[0] = -1.0f;
		mat.r[3].m128_f32[1] = 1.0f;
	}
	//	位置情報更新
	_pos.x = 0.0f;
	_pos.y = 0.0f;
	mat = DirectX::XMMatrixTranslation(_pos.x, _pos.y, 0.0f) * mat;
	
	*mapMat = mat;
	_constBuffer->Unmap(0,nullptr);
}