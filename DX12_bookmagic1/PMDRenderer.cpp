//	�C���N���[�h
#include "PMDRenderer.h"
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "Dx12Wrapper.h"
#include "helper.h"

//	���C�u���������N
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

//	�R���X�g���N�^
PMDRenderer::PMDRenderer(Dx12Wrapper* pDxWap) :
	_pDxWap(pDxWap)
{
}

//	����������
void PMDRenderer::Init(void)
{
	//	���[�g�p�����[�^�A���[�g�V�O�l�C�`���̍쐬
	CreateRootParameterOrRootSignature();
	//	�O���t�B�b�N�p�C�v���C���̍쐬
	CreateGraphicPipeline();
}

//	�`�揈��
void PMDRenderer::Draw(void)
{
	//	�p�C�v���C���X�e�[�g���Z�b�g
	_pDxWap->GetCmdList()->SetPipelineState(_pipelinestate.Get());

	//	���[�g�V�O�l�C�`�����Z�b�g
	_pDxWap->GetCmdList()->SetGraphicsRootSignature(_rootsignature.Get());
}

void PMDRenderer::PreShadowDraw(void)
{
	//	�p�C�v���C���X�e�[�g���Z�b�g
	_pDxWap->GetCmdList()->SetPipelineState(_plsShadow.Get());

	//	���[�g�V�O�l�C�`�����Z�b�g
	_pDxWap->GetCmdList()->SetGraphicsRootSignature(_rootsignature.Get());
}

//	�I�u�W�F�N�g�̉������
void PMDRenderer::Release(void)
{

}

//	�X�V����
void PMDRenderer::Update(void)
{

}

//	���[�g�p�����[�^�ƃ��[�g�V�O�l�C�`���̍쐬
void PMDRenderer::CreateRootParameterOrRootSignature(void)
{
	//	���[�g�p�����[�^�[�̍쐬	//	
	//	�f�B�X�N���v�^�����W�̐ݒ�
	CD3DX12_DESCRIPTOR_RANGE descTblRange[5] = {};
	//	�V�[���p
	descTblRange[0].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,		//	��ʁF�萔�o�b�t�@�i�萔�o�b�t�@�r���[�iCBV)�j
		1, 										//	�f�B�X�N���v�^��
		0,										//	0�Ԃ̃X���b�g����
		0,										//	���W�X�^�[�̈�
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND	//	�A�������f�B�X�N���v�^�����W���O�̃f�B�X�N���v�^�����W�̒���ɗ���
	);
	//	���[���h�s��p
	descTblRange[1].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1,
		1
	);
	//	�}�e���A���p
	descTblRange[2].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,		
		1, 										
		2										
	);

	//	�c�݃e�N�X�`��
	descTblRange[3].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,		
		4, 										
		0										
	);

	//	�V���h�E�}�b�v�p�e�N�X�`��
	descTblRange[4].Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 4
	);

	//	���[�g�p�����[�^�̐ݒ�
	CD3DX12_ROOT_PARAMETER rootparam[4] = {};
	//	���[�g�p�����[�^�@�V�[���p
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);
	//	���[�g�p�����[�^�@�ʒu���W�p
	rootparam[1].InitAsDescriptorTable(1, &descTblRange[1]);
	//	���[�g�p�����[�^�@�}�e���A���A�e�N�X�`���p
	rootparam[2].InitAsDescriptorTable(2, &descTblRange[2]);
	//	�V���h�E�}�b�v�p
	rootparam[3].InitAsDescriptorTable(1, &descTblRange[4]);

	//	�T���v���[�ݒ�
	CD3DX12_STATIC_SAMPLER_DESC samplerdesc[3] = {};
	samplerdesc[0].Init(0);
	
	//	�g�D�[���e�N�X�`���p
	samplerdesc[1].Init(1,
		D3D12_FILTER_ANISOTROPIC,			//	�ٕ����⊮�i�e�N�Z�������o���ہA�����̑�\�n�_����e�N�Z�������o�������j
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//	�������̌J��Ԃ��Ȃ�
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//	�c�����̌J��Ԃ��Ȃ�
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//	���s�̌J��Ԃ��Ȃ�;

	//	�V���h�E�}�b�v�p
	samplerdesc[2] = samplerdesc[0];
	samplerdesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerdesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;	//	<=�ł����true(1.0),�����łȂ����false(0.0f)
	samplerdesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;	//	��r���ʂ��o�C�W�j�A���
	samplerdesc[2].MaxAnisotropy = 1;									//	�[�x�X�΂�L���ɂ���
	samplerdesc[2].ShaderRegister = 2;

	//	���[�g�V�O�l�C�`���̐ݒ�	//	
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	//	���_���i���̓A�Z���u���j�����݂���Ƃ����񋓌^
	rootSignatureDesc.pParameters = rootparam;							//	���[�g�p�����[�^�z��̐擪�A�h���X
	rootSignatureDesc.NumParameters = 4;								//	���[�g�p�����[�^��
	rootSignatureDesc.pStaticSamplers = samplerdesc;					//	�T���v���[�ݒ�
	rootSignatureDesc.NumStaticSamplers = 3;							//	�T���v���[��
	//	�o�C�i���R�[�h�̍쐬
	ComPtr < ID3DBlob> rootSigBlob = nullptr;					//	���[�g�V�O�l�C�`���̃f�[�^
	ComPtr < ID3DBlob> errorBlob = nullptr;						//	�G���[�p�I�u�W�F�N�g
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,				//	���[�g�V�O�l�C�`���̐ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0,	//	���[�g�V�O�l�C�`���o�[�W����
		&rootSigBlob,					//	���[�g�V�O�l�C�`���̃f�[�^
		&errorBlob						//	�G���[�I�u�W�F
	);
	//	���[�g�V�O�l�C�`���̃o�C�i���R�[�h���������쐬����Ă��邩�m�F
	Helper::DebugShaderError(result, errorBlob.Get());

	//	���[�g�V�O�l�C�`���I�u�W�F�̍쐬
	result = _pDxWap->GetDevice()->CreateRootSignature(
		0,									//	nodemask�i����GPU�͈�Ȃ���0�j
		rootSigBlob->GetBufferPointer(),	//	�o�C�i���f�[�^�̃|�C���^�[
		rootSigBlob->GetBufferSize(),		//	�o�C�i���f�[�^�̃T�C�Y
		IID_PPV_ARGS(_rootsignature.ReleaseAndGetAddressOf())		//	ID�ƃ��[�g�V�O�l�C�`���I�u�W�F
	);
}

//	�O���t�B�b�N�p�C�v���C���̍쐬
void PMDRenderer::CreateGraphicPipeline(void)
{
	//	���_���C�A�E�g��ݒ�
	D3D12_INPUT_ELEMENT_DESC inputlayout[] =
	{
		{//	���W���
			"POSITION",										//	�Z�}���e�B�N�X��
			0,												//	�����Z�}���e�B�N�X�����g�p����Ƃ��̃C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,					//	�t�H�[�}�b�g
			0,												//	���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,					//	�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1���_���Ƃɐݒ肵�����C�A�E�g���w�肷��
			0												//	��x�ɕ`�悷��C���X�^���X��
		},
		{//	�@���x�N�g��
			"NORMAL",										//	�Z�}���e�B�N�X��
			0,												//	�����Z�}���e�B�N�X�����g�p����Ƃ��̃C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,					//	�t�H�[�}�b�g
			0,												//	���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,					//	�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1���_���Ƃɐݒ肵�����C�A�E�g���w�肷��
			0												//	��x�ɕ`�悷��C���X�^���X��
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
		{//	�{�[���ԍ�
			"BONE_NO",										//	�Z�}���e�B�N�X��
			0,												//	�����Z�}���e�B�N�X�����g�p����Ƃ��̃C���f�b�N�X
			DXGI_FORMAT_R16G16_UINT,						//	�t�H�[�}�b�g
			0,												//	���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,					//	�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1���_���Ƃɐݒ肵�����C�A�E�g���w�肷��
			0												//	��x�ɕ`�悷��C���X�^���X��
		},
		{//	�{�[���e����
			"WEIGHT",										//	�Z�}���e�B�N�X��
			0,												//	�����Z�}���e�B�N�X�����g�p����Ƃ��̃C���f�b�N�X
			DXGI_FORMAT_R8_UINT,						//	�t�H�[�}�b�g
			0,												//	���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,					//	�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1���_���Ƃɐݒ肵�����C�A�E�g���w�肷��
			0												//	��x�ɕ`�悷��C���X�^���X��
		},
		{//	�֊s���t���O
			"EDGE_FLAG",										//	�Z�}���e�B�N�X��
			0,												//	�����Z�}���e�B�N�X�����g�p����Ƃ��̃C���f�b�N�X
			DXGI_FORMAT_R8_UINT,						//	�t�H�[�}�b�g
			0,												//	���̓X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,					//	�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//	1���_���Ƃɐݒ肵�����C�A�E�g���w�肷��
			0												//	��x�ɕ`�悷��C���X�^���X��
		}
	};

	//	�O���t�B�b�N�X�p�C�v���C���̐ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	//	���[�g�V�O�l�C�`���I�u�W�F�̃Z�b�g
	gpipeline.pRootSignature = _rootsignature.Get();							//	���[�g�V�O�l�C�`��

	//	�V�F�[�_�[�̃Z�b�g
		//	�G���[�I�u�W�F�N�g
	ComPtr<ID3DBlob> vsBlob = nullptr;
	ComPtr<ID3DBlob> psBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	//	���_�V�F�[�_�[�ǂݍ���
	auto result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",							//	�V�F�[�_�[��
		nullptr,											//	define�Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	�C���N���[�h�t�@�C���̓f�t�H���g
		"BasicVS",											//	�G���g���[�|�C���g�̊֐���
		"vs_5_0",											//	�ΏۃV�F�[�_�[
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	�f�o�b�O�p | �œK���Ȃ�
		0,													//	���ʃt�@�C���i����̓V�F�[�_�[�t�@�C���Ȃ̂ŁA0�j
		vsBlob.ReleaseAndGetAddressOf(),					//	�V�F�[�_�[�I�u�W�F�N�g
		errorBlob.ReleaseAndGetAddressOf()					//	�G���[�I�u�W�F�N�g
	);
	//	���_�V�F�[�_�[�̓ǂݍ��݂�����ɍs���Ă��邩�m�F
	Helper::DebugShaderError(result, errorBlob.Get());

	//	�s�N�Z���V�F�[�_�[�ǂݍ���
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",							//	�V�F�[�_�[��
		nullptr,											//	define�Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	�C���N���[�h�t�@�C���̓f�t�H���g
		"BasicPS",											//	�G���g���[�|�C���g�̊֐���
		"ps_5_0",											//	�ΏۃV�F�[�_�[
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	�f�o�b�O�p | �œK���Ȃ�
		0,													//	���ʃt�@�C���i����̓V�F�[�_�[�t�@�C���Ȃ̂ŁA0�j
		psBlob.ReleaseAndGetAddressOf(),					//	�V�F�[�_�[�I�u�W�F�N�g
		errorBlob.ReleaseAndGetAddressOf()											//	�G���[�I�u�W�F�N�g
	);
	//	�s�N�Z���V�F�[�_�[�̓ǂݍ��݂�����ɍs���Ă��邩�m�F
	Helper::DebugShaderError(result, errorBlob.Get());
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();	//	���_�V�F�[�_�[�|�C���^�[
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();		//	���_�V�F�[�_�[�̃o�b�t�@�T�C�Y
	gpipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();	//	�s�N�Z���V�F�[�_�[�|�C���^�[
	gpipeline.PS.BytecodeLength = psBlob->GetBufferSize();		//	�s�N�Z���V�F�[�_�[�̃o�b�t�@�T�C�Y

	//	�f�t�H���g�̃T���v���}�X�N��\���萔�i0xffffffff)
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//	���X�^���C�U�[�X�e�[�g�̐ݒ�
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	//	�J�����O���Ȃ�

	//	�u�����h�X�e�[�g�̐ݒ�
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//	�����_�[�^�[�Q�b�g�̃u�����h�X�e�[�g�ݒ�
	//D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	//renderTargetBlendDesc.BlendEnable = false;									//	�u�����h���邩�ۂ�
	//renderTargetBlendDesc.LogicOpEnable = false;								//	�_�����Z���邩�ۂ�
	//renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	//	�������ނƂ��̃}�X�N�l

	//	���̓��C�A�E�g�̐ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputlayout;		//	���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputlayout);	//	���C�A�E�g�z��̗v�f��

	//	�\�����@�̐ݒ�
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//	�J�b�g�Ȃ�

	//	�\���v�f�̐ݒ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//	�O�p�`�ō\��

	//	�����_�[�^�[�Q�b�g�̐ݒ�
	{
		//	�^�[�Q�b�g�̎��
		enum E_TARGET
		{
			COL,
			NORMAL,
			BLOOM,
			MAX
		};
		gpipeline.NumRenderTargets = MAX;							//	�����_�[�^�[�Q�b�g��
		for (int nTarget = 0; nTarget < MAX; nTarget++)
		{
			gpipeline.RTVFormats[nTarget] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//	0�`1�ɐ��K������sRGBA
		}
	}

	//	�A���`�G�C���A�V���O�̂��߂̃T���v�����ݒ�
	gpipeline.SampleDesc.Count = 1;		//	�T���v�����O��1�s�N�Z���ɂ�1
	gpipeline.SampleDesc.Quality = 0;	//	�N�I���e�B�[�͍Œ�i0�j

	//	�f�v�X�X�e���V���̐ݒ�
	gpipeline.DepthStencilState.DepthEnable = true;								//	�[�x�o�b�t�@�g�p
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//	�s�N�Z���`�掞�ɐ[�x�o�b�t�@�ɐ[�x�l����������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//	�[�x�l�������������̗p����
	gpipeline.DepthStencilState.StencilEnable = false;							//	�X�e���V���o�b�t�@���Ή�
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//	�O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̍쐬
	result = _pDxWap->GetDevice()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf())
	);

	//	�V���h�E�}�b�v�p�̃p�C�v���C���쐬	//
	//	�V���h�E�}�b�v�p���_�V�F�[�_�[�ǂݍ���
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",							//	�V�F�[�_�[��
		nullptr,											//	define�Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//	�C���N���[�h�t�@�C���̓f�t�H���g
		"ShadowVS",											//	�G���g���[�|�C���g�̊֐���
		"vs_5_0",											//	�ΏۃV�F�[�_�[
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//	�f�o�b�O�p | �œK���Ȃ�
		0,													//	���ʃt�@�C���i����̓V�F�[�_�[�t�@�C���Ȃ̂ŁA0�j
		vsBlob.ReleaseAndGetAddressOf(),					//	�V�F�[�_�[�I�u�W�F�N�g
		errorBlob.ReleaseAndGetAddressOf()					//	�G���[�I�u�W�F�N�g
	);
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();	//	���_�V�F�[�_�[�|�C���^�[
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();		//	���_�V�F�[�_�[�̃o�b�t�@�T�C�Y
	gpipeline.PS.pShaderBytecode = nullptr;						//	�s�N�Z���V�F�[�_�[�Ȃ�
	gpipeline.PS.BytecodeLength = 0;
	//	�^�[�Q�b�g�t�H�[�}�b�g�̏�����
	for (unsigned int nTarget = 0; nTarget < gpipeline.NumRenderTargets; nTarget++)
	{
		gpipeline.RTVFormats[nTarget] = DXGI_FORMAT_UNKNOWN;
	}
	gpipeline.NumRenderTargets = 0;								//	�����_�[�^�[�Q�b�g�Ȃ�

	result = _pDxWap->GetDevice()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_plsShadow.ReleaseAndGetAddressOf())
	);

}