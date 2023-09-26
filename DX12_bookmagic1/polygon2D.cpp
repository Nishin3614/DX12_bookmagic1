#include "polygon2D.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"
#include <d3dx12.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")


//	�R���X�g���N�^
Polygon2D::Polygon2D(Dx12Wrapper* pWrap) : _pWrap(pWrap)
{
}

//	����������
void Polygon2D::Init(void)
{
	//	�e�N�X�`���[�쐬
	CreateTexBuffer();
	//	���_�o�b�t�@�쐬
	CreateVertexBuffer();
	//	���[�g�V�O�l�C�`���쐬
	CreateRootSignature();
	//	�ς��Ղ炢��X�e�[�g�쐬
	CreatePipeline();
}

//	�`�揈��
void Polygon2D::Draw(void)
{
	auto cmdlist = _pWrap->GetCmdList();
	cmdlist->SetPipelineState(_pls.Get());
	cmdlist->SetGraphicsRootSignature(_rs.Get());

	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdlist->RSSetViewports(1, &vp);//�r���[�|�[�g
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdlist->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`

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
	std::string sTexName = "img/TitleScreenUI/Titlelogo00.png";
	_texBuffer = _pWrap->LoadTextureFromFile(sTexName);
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

//	���[�g�V�O�l�C�`���쐬
void Polygon2D::CreateRootSignature(void)
{
	//	�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE ranges[2] = {};
	//	�e�N�X�`��
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 1;	//	t0
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//	�萔�o�b�t�@
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;	//	b0
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//	���[�g�p�����[�^�̐ݒ�
	D3D12_ROOT_PARAMETER rp[2] = {};
	//	�e�N�X�`���[�o�b�t�@�Z�b�g
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	//	�萔�o�b�t�@�Z�b�g
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;


	//	�T���v���[�ݒ�
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	���[�g�V�O�l�C�`���ݒ�
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
		Helper::DebugOutputFormatString("�|���S��2D�p�̃��[�g�V�O�l�C�`���[�쐬���s\n");
		return;
	}
}

//	�p�C�v���C���쐬
void Polygon2D::CreatePipeline(void)
{
	//	���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{	//	�ʒu
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

	//	�e�V�F�[�_�[�̓ǂݍ���
	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;
	//	���_�V�F�[�_�[�R���p�C��
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
	//	�s�N�Z���V�F�[�_�R���p�C��
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

	//	�O���t�B�b�N�X�p�C�v���C���̐ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;

	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	// �u�����h
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;	//	false:RT[0]�̂ݔ��f������Atrue:RT[0~7]�e���Őݒ肷��

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		blendDesc.RenderTarget[i].BlendEnable = true;						//	�u�����h���邩�ۂ�
		blendDesc.RenderTarget[i].LogicOpEnable = false;					//	�_�����Z���s�����ۂ�
		blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;			//	RGB�l�ɑ΂��ă�����Z����iSRCrgb * SRC���j
		blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;	//	RGB�l�ɑ΂���1-�����v�Z����iDESTrgb * DEST���j
		blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;				//	���Z����
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;			//	�f�ނ̌��̐F�ɂȂ�
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;		//	�Ȃ�ł��[���ɂ���
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;		//	���Z����
		blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;			//	���Z���Ȃ��A�`���̂܂܁iDest)
		blendDesc.RenderTarget[i].RenderTargetWriteMask =					//	���ׂĂ̗v�f���u�����h����
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
	//	�y���|���S���̃p�C�v���C���쐬
	auto dev = _pWrap->GetDevice();
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_pls.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�|���S��2D�p�O���t�B�b�N�p�C�v���C���쐬���s\n");
		return;
	}

}