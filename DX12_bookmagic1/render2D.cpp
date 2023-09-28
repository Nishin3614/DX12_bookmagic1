#include "render2D.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"
#include "polygon2D.h"
#include <d3dx12.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

//	�R���X�g���N�^
Renderer2D::Renderer2D(Dx12Wrapper* pWrap) : _pWrap(pWrap)
{
}

//	����������
void Renderer2D::Init(void)
{
	//	���[�g�V�O�l�C�`���쐬
	CreateRootSignature();
	//	�ς��Ղ炢��X�e�[�g�쐬
	CreatePipeline();
	//	�|���S���쐬
	//Create2D("img/TitleScreenUI/Titlelogo00.png");
}

//	�`�揈��
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
	cmdlist->RSSetViewports(1, &vp);//�r���[�|�[�g
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdlist->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`

	for (int nCnt = 0; nCnt < _Polygons.size(); nCnt++)
	{
		_Polygons[nCnt]->Draw();
	}
}

//
Polygon2D* Renderer2D::Create2D(std::string texName)
{
	//	�|���S���쐬
	std::unique_ptr p = std::make_unique<Polygon2D>(_pWrap, texName);
	p->Init();
	return _Polygons.emplace_back(std::move(p)).get();
}

//	���[�g�V�O�l�C�`���쐬
void Renderer2D::CreateRootSignature(void)
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
void Renderer2D::CreatePipeline(void)
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