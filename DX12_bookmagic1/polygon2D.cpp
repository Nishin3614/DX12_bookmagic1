#include "polygon2D.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"
#include <d3dx12.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")


//	�R���X�g���N�^
Polygon2D::Polygon2D(Dx12Wrapper* pWrap,std::string texName) : _pWrap(pWrap),_texName(texName)
{
}

//	����������
void Polygon2D::Init(void)
{
	//	�e�N�X�`���[�쐬
	CreateTexBuffer();
	//	���_�o�b�t�@�쐬
	CreateVertexBuffer();
}

//	�`�揈��
void Polygon2D::Draw(void)
{
	auto cmdlist = _pWrap->GetCmdList();
	//	�e�N�X�`���ƃq�[�v���֘A�t����
	cmdlist->SetDescriptorHeaps(1, _dh.GetAddressOf());
	auto dhH = _dh->GetGPUDescriptorHandleForHeapStart();
	cmdlist->SetGraphicsRootDescriptorTable(0, dhH);
	dhH.ptr += _pWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdlist->SetGraphicsRootDescriptorTable(1, dhH);

	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdlist->IASetVertexBuffers(0, 1, &_vbv);						//	�y���|���S���p�̒��_�o�b�t�@�r���[�Z�b�g
	cmdlist->DrawInstanced(4, 1, 0, 0);
}

//	���_�o�b�t�@�쐬
void Polygon2D::CreateVertexBuffer(void)
{
	struct PeraVertex
	{
		DirectX::XMFLOAT3 point;
		DirectX::XMFLOAT2 uv;
	};
	PeraVertex pv[4] = {
		{{0.0f,100.0f,0.0f},{0,1}},		//	����
		{{0.0f,0.0f,0.0f},{0,0}},		//	����
		{{100.0f,100.0f,0.0f},{1,1}},		//	�E��
		{{100.0f,0.0f,0.0f},{1,0}}		//	�E��

	};

	//	���_�o�b�t�@�쐬
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
		Helper::DebugOutputFormatString("�|���S��2D�̒��_�o�b�t�@�쐬���s");
		return;
	}
	//	���_�o�b�t�@�r���[�쐬
	_vbv.BufferLocation = _vb->GetGPUVirtualAddress();
	_vbv.SizeInBytes = sizeof(pv);
	_vbv.StrideInBytes = sizeof(PeraVertex);

	//	���_���R�s�[
	PeraVertex* mappedPera = nullptr;
	UINT64 width = _texBuffer->GetDesc().Width;
	UINT height = _texBuffer->GetDesc().Height;
	pv[0].point.y = pv[2].point.y = height;
	pv[2].point.x = pv[3].point.x = width;
	_vb->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	_vb->Unmap(0, nullptr);
}

//	�o�b�t�@�쐬
void Polygon2D::CreateTexBuffer(void)
{
	//	�o�b�t�@�쐬	//
	auto dev = _pWrap->GetDevice();
	//	�e�N�X�`���[�ǂ݂���
	_texBuffer = _pWrap->LoadTextureFromFile(_texName);
	//	�萔�o�b�t�@�쐬
	CreateMatBaffer();
	//	�f�B�X�N���v�^�q�[�v�쐬
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
		Helper::DebugOutputFormatString("�|���S��2D�`��̃o�b�t�@�쐬���s");
	}
	//	�V�F�[�_�[���\�[�X�r���[�쐬�i�e�N�X�`���[�j
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
	//	�萔�o�b�t�@�r���[�쐬
	dhH.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _constBuffer->GetDesc().Width;
	dev->CreateConstantBufferView(&cbvDesc, dhH);
}

//	�s��o�b�t�@�쐬
void Polygon2D::CreateMatBaffer(void)
{
	//	�萔�o�b�t�@�쐬
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
		Helper::DebugOutputFormatString("�|���S��2D�̒萔�o�b�t�@�쐬���s");
	}

	//	�萔�̃R�s�[
	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
	auto size = Win32Application::Instance().GetWindowSize();
	DirectX::XMMATRIX* mapMat;
	result = _constBuffer->Map(0, nullptr, (void**)&mapMat);
	{	//	�s�N�Z�����W���f�B�X�v���C���W
		mat.r[0].m128_f32[0] = 2.0f / size.cx;
		mat.r[1].m128_f32[1] = -2.0f / size.cy;
		mat.r[3].m128_f32[0] = -1.0f;
		mat.r[3].m128_f32[1] = 1.0f;
	}
	//	�ʒu���X�V
	_pos.x = 0.0f;
	_pos.y = 0.0f;
	mat = DirectX::XMMatrixTranslation(_pos.x, _pos.y, 0.0f) * mat;
	
	*mapMat = mat;
	_constBuffer->Unmap(0,nullptr);
}