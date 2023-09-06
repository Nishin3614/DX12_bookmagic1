//	�C���N���[�h
#include "VisualEffect.h"
#include "Dx12Wrapper.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "helper.h"

//	�}���`�p�X�����_�����O�p
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

//	�O���t�@�C������A�N�Z�X�ł��Ȃ����邽�߁A�������O��ԂŒ�`
namespace//�񋓌^�p
{
	//	�I���W���p�����_�[�^�[�Q�b�g�r���[���
	enum class E_ORIGIN_RTV : int
	{
		COL,	//	�ʏ�J���[
		NORMAL,	//	�@��
		MAX_NORMALDROW,

		BLOOM = MAX_NORMALDROW,
		SHRINKBLOOM,
		MAX_BLOOM,

		DOF = MAX_BLOOM,
		MAX_DOF,

		PROCE = MAX_DOF,	//	���H�p
		MAX
	};

	//	�I���W���pSRV,CBV���
	enum class E_ORIGIN_SRV : int
	{
		/*	SRV	*/
		//	�ʏ�`��
		COL,	//	�ʏ�J���[
		NORMAL,	//	�@��
		MAX_NORMALDROW,

		//	�u���[��
		BLOOM = MAX_NORMALDROW,
		SHRINKBLOOM,

		//	��ʊE�[�x
		DOF,	//	��ʊE�[�x

		//	�摜���H�p
		PROCE,	//	���H�p

		/*	CBV	*/
		BOKE,

		MAX
	};
	//	�萔��`
	constexpr float CLSCLR[4] = { 0.5f,0.5f,0.5f,1.0f };		//	�����_�[�^�[�Q�b�g�N���A�J���[
	constexpr float NONE_CLSCLR[4] = { 0.0f,0.0f,0.0f,1.0f };	//	�������郌���_�[�^�[�Q�b�g�̃N���A�J���[

}

//	���O���
using namespace Microsoft::WRL;
using namespace DirectX;


//	�R���X�g���N�^
VisualEffect::VisualEffect(Dx12Wrapper * pWrap) :
	_pWrap(pWrap),
	_prPoriVBV({})
{
}

/*	�������֘A�̏���	*/
//	����������
void VisualEffect::Init(void)
{	
	//	�y���|���S���ɒ���t���邽�߂̃��\�[�X���쐬
	CreateOriginRenderTarget();
	//	�|�X�g�G�t�F�N�g�p�̃o�b�t�@�A�r���[�쐬
	CreateEffectBufferAndView();
	//	�y���|���S���̍쐬
	CreatePeraVertexBuff();
	CreatePeraRootSignature();
	CreatePeraGraphicPipeLine();
}

//	�I���W���̃����_�[�^�[�Q�b�g�쐬
void VisualEffect::CreateOriginRenderTarget(void)
{
	//	�o�b�t�@�쐬	//
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	�q�[�v�v���p�e�B�[�ݒ�
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	�����_�����O���̃N���A�l�Ɠ����l
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, CLSCLR);
	//	�o�b�t�@�̍쐬
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
			Helper::DebugOutputFormatString("�I���W���̃����_�[�^�[�Q�b�g�o�b�t�@�쐬���s");
			return;
		}
	}

	//	�����_�����O���̃N���A�l�Ɠ����l
	clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, NONE_CLSCLR);
	//	�u���[���o�b�t�@�̍쐬
	for (auto& res : _bloomBuffer)
	{
		
		auto result = dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(res.ReleaseAndGetAddressOf()));
		//	�T�C�Y�𔼕��ɂ���
		resDesc.Width >>= 1;
		if (FAILED(result))
		{
			Helper::DebugOutputFormatString("�u���[���̃����_�[�^�[�Q�b�g�o�b�t�@�쐬���s");
			return;
		}
	}

	//	��ʊE�[�x�o�b�t�@�쐬
	CreateBlurForDOFBuffer();

	//	���H�p�̃����_�[�^�[�Q�b�g�쐬
	CreateProcessRenderTarget();

	//	�f�B�X�N���v�^�q�[�v�쐬	//
	//	�쐬�ς݂̃q�[�v�����g���Ă���1�����
	auto heapDesc = _pWrap->GetDescriptorHeapD();
	//	RTV�p�q�[�v�����
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_RTV::MAX);
	auto result = dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(_originRTVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�}���`�p�X�����_�����O�FRVT�p�f�B�X�N���v�^�q�[�v�쐬���s");
		return;
	}

	//	�I���W���p�̃����_�[�^�[�Q�b�g�r���[�쐬
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();						//	RTV�̃X�^�[�g�|�C���g
	int offset = 0;																	//	�r���[�̃I�t�Z�b�g�ʒu
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	�����_�[�^�[�Q�b�g�r���[�̃C���N�������g�T�C�Y
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(baseH);											//	�n���h��

	//	�ʏ�`��̃����_�[�^�[�Q�b�g�r���[�쐬
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

	//	�u���[���p�̃����_�[�^�[�Q�b�g�r���[�쐬
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

	//	��ʊE�[�x�p�̃����_�[�^�[�Q�b�g�r���[�쐬
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::DOF);
	handle.InitOffsetted(baseH, offset);
	dev->CreateRenderTargetView(
		_dofBuffer.Get(),
		&rtvDesc,
		handle
	);

	//	���H�p�̃����_�[�^�[�Q�b�g�r���[�쐬
	offset = incSize * static_cast<int>(E_ORIGIN_RTV::PROCE);
	handle.InitOffsetted(baseH, offset);
	dev->CreateRenderTargetView(
		_proceResource.Get(),
		&rtvDesc,
		handle
	);

	//	SRV�p�q�[�v�����
	heapDesc.NumDescriptors = static_cast<int>(E_ORIGIN_SRV::MAX);
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(_originSRVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�}���`�p�X�����_�����O�FSRV�p�f�B�X�N���v�^�q�[�v�쐬���s");
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//	�I���W���̃V�F�[�_�[���\�[�X�r���[�쐬
	baseH = _originSRVHeap->GetCPUDescriptorHandleForHeapStart();						//	SRV�̃X�^�[�g�|�C���g
	offset = 0;																	//	�r���[�̃I�t�Z�b�g�ʒu
	incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	//	�����_�[�^�[�Q�b�g�r���[�̃C���N�������g�T�C�Y

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

	//	�u���[���̃V�F�[�_�[���\�[�X�r���[�쐬
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

	//	��ʊE�[�x�p�̃V�F�[�_�[���\�[�X�쐬
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::DOF);
	handle.InitOffsetted(baseH, offset);
	dev->CreateShaderResourceView(
		_dofBuffer.Get(),
		&srvDesc,
		handle
	);

	//	���H�p�̃V�F�[�_�[���\�[�X�r���[�쐬
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::PROCE);
	handle.InitOffsetted(baseH, offset);
	dev->CreateShaderResourceView(
		_proceResource.Get(),
		&srvDesc,
		handle
	);

	//	�ڂ��萔�o�b�t�@�쐬
	CreateBokeConstantBuff();
	//	�ڂ��萔�o�b�t�@�r���[�쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _bokehParamBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _bokehParamBuffer->GetDesc().Width;
	offset = incSize * static_cast<int>(E_ORIGIN_SRV::BOKE);
	handle.InitOffsetted(baseH, offset);
	dev->CreateConstantBufferView(&cbvDesc, handle);

}

//	���H�p�̃����_�[�^�[�Q�b�g�쐬
void VisualEffect::CreateProcessRenderTarget(void)
{
	//	�o�b�t�@�쐬	//
//	�g���Ă���o�b�N�o�b�t�@�̏��𗘗p����
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	//	�q�[�v�v���p�e�B�[�ݒ�
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	�����_�����O���̃N���A�l�Ɠ����l
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, CLSCLR);
	//	�o�b�t�@�̍쐬
	auto result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_proceResource.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�}���`�p�X�����_�����O�F�o�b�t�@�쐬���s");
		return;
	}
}

//	��ʊE�[�x�p�o�b�t�@�쐬
void VisualEffect::CreateBlurForDOFBuffer(void)
{
	//	�g���Ă���o�b�N�o�b�t�@�̏��𗘗p����
	auto dev = _pWrap->GetDevice();
	auto resDesc = _pWrap->GetBackDesc();
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//	�q�[�v�v���p�e�B�[�ݒ�
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	�����_�����O���̃N���A�l�Ɠ����l
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, NONE_CLSCLR);
	resDesc.Width >>= 1;	//	�k���o�b�t�@�Ȃ̂ő傫���͔���
	//	�o�b�t�@�̍쐬
	auto result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_dofBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("��ʊE�[�x�p�o�b�t�@�쐬���s");
		return;
	}
}

//	�y���|���S���̒��_�o�b�t�@�쐬
void VisualEffect::CreatePeraVertexBuff(void)
{
	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	PeraVertex pv[4] = {
		{{-1.0f,-1.0f,0.1f},{0,1}},		//	����
		{{-1.0f,1.0f,0.1f},{0,0}},		//	����
		{{1.0f,-1.0f,0.1f},{1,1}},		//	�E��
		{{1.0f,1.0f,0.1f},{1,0}}		//	�E��

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
		IID_PPV_ARGS(_prPoriVB.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�y���|���S���̒��_�o�b�t�@�쐬���s");
		return;
	}
	//	���_�o�b�t�@�r���[�쐬
	_prPoriVBV.BufferLocation = _prPoriVB->GetGPUVirtualAddress();
	_prPoriVBV.SizeInBytes = sizeof(pv);
	_prPoriVBV.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera = nullptr;
	_prPoriVB->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	_prPoriVB->Unmap(0, nullptr);
}

//	�ڂ��萔�o�b�t�@�쐬
void VisualEffect::CreateBokeConstantBuff(void)
{
	//	���U�l�̌v�Z
	//	�E�F�C�g�l�v�Z
	std::vector<float> weights = Helper::GetGaussianWeights(8, 5.0f);;	//	�e�E�F�C�g�l

	//	�萔�o�b�t�@�̍쐬
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
		Helper::DebugOutputFormatString("�ڂ��萔�o�b�t�@�쐬���s");
		return;
	}
	//	�l���R�s�[
	float* mappedWeight = nullptr;
	result = _bokehParamBuffer->Map(0, nullptr, (void**)&mappedWeight);
	std::copy(weights.begin(), weights.end(), mappedWeight);
	_bokehParamBuffer->Unmap(0, nullptr);
}

//	�y���|���S���p���[�g�V�O�l�C�`���[�쐬
void VisualEffect::CreatePeraRootSignature(void)
{
	//	�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE ranges[5] = {};
	//	�ʏ�J���[�A�@���Z�b�g�A���P�x�A�k�����P�x�A�k���ʏ�
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors = 5;	//	t0,t1,t2,t3,t4
	//	�ڂ��萔�o�b�t�@�Z�b�g
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[1].NumDescriptors = 1;	//	b0
	//	�|�X�g�G�t�F�N�g�p�̃o�b�t�@�Z�b�g
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].NumDescriptors = 1;	//	t5
	//	�[�x�l�e�N�X�`���p
	ranges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[3].NumDescriptors = 1;	//	t6
	//	���C�g�f�v�X�e�N�X�`���p
	ranges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[4].NumDescriptors = 1;	//	t7

	//	���M�X�^�[�ݒ�
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

	//	���[�g�p�����[�^�̐ݒ�
	D3D12_ROOT_PARAMETER rp[5] = {};
	//	�e�N�X�`���[�o�b�t�@�Z�b�g
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;
	//	�ڂ��萔�o�b�t�@�Z�b�g
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &ranges[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;
	//	�|�X�g�G�t�F�N�g�p�o�b�t�@�Z�b�g
	rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[2].DescriptorTable.pDescriptorRanges = &ranges[2];
	rp[2].DescriptorTable.NumDescriptorRanges = 1;
	//	�[�x�l�e�N�X�`���[�p
	rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[3].DescriptorTable.pDescriptorRanges = &ranges[3];
	rp[3].DescriptorTable.NumDescriptorRanges = 1;
	//	���C�g�f�v�X�e�N�X�`���p
	rp[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[4].DescriptorTable.pDescriptorRanges = &ranges[4];
	rp[4].DescriptorTable.NumDescriptorRanges = 1;


	//	�T���v���[�ݒ�
	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//	���[�g�V�O�l�C�`���ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 5;
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
		Helper::DebugOutputFormatString("�y���|���S���p���[�g�V�O�l�C�`���[�쐬���s\n");
		return;
	}

}

//	�y���|���S���p�O���t�B�b�N�X�p�C�v���C���쐬
void VisualEffect::CreatePeraGraphicPipeLine(void)
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
		L"peraVertex.hlsl", nullptr,
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
		L"peraPixel.hlsl", nullptr,
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
	//	�y���|���S���̃p�C�v���C���쐬
	auto dev = _pWrap->GetDevice();
	result = dev->CreateGraphicsPipelineState(
		&gpsDesc,
		IID_PPV_ARGS(_prPoriPipeline.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�y���|���S���p�O���t�B�b�N�p�C�v���C���쐬���s\n");
		return;
	}

	//	���H�p�̃p�C�v���C���쐬
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
		Helper::DebugOutputFormatString("���H�p�O���t�B�b�N�p�C�v���C���쐬���s\n");
		return;
	}

	//	�|�X�g�G�t�F�N�g�̃p�C�v���C���쐬
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
		Helper::DebugOutputFormatString("�|�X�g�G�t�F�N�g�p�O���t�B�b�N�p�C�v���C���쐬���s\n");
		return;
	}

	//	�ڂ����p�C�v���C���쐬
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
		Helper::DebugOutputFormatString("�ڂ����p�O���t�B�b�N�p�C�v���C���쐬���s\n");
		return;
	}

}

//	�G�t�F�N�g�p�̃o�b�t�@�ƃr���[�쐬
bool VisualEffect::CreateEffectBufferAndView(void)
{
	//	�|�X�g�G�t�F�N�g�p�e�N�X�`���[�o�b�t�@���쐬
	std::string sNormal = "normal/glass_n.png";
	_efffectTexBuffer = _pWrap->LoadTextureFromFile(sNormal);
	if (_efffectTexBuffer == nullptr)
	{
		Helper::DebugOutputFormatString("�@���}�b�v�摜�̓ǂݍ��݂Ɏ��s\n");
		return false;
	}

	//	�|�X�g�G�t�F�N�g�p�̃f�B�X�N���v�^�q�[�v����
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

	//	�|�X�g�G�t�F�N�g�p�V�F�[�_�[���\�[�X�r���[�쐬
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

//	�����_�[�^�[�Q�b�g���Z�b�g���鏈��
void VisualEffect::PreOriginDraw(void)
{
	//	�I���W�������_�[�^�[�Q�b�g�ɕ`�悷��O�Ƀo���A�ݒ���s��
	for (auto& res : _origin1Resource)
	{
		_pWrap->SetBarrier(
			res.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	//	�u���[���`��O�Ƀo���A�ݒ�
	_pWrap->SetBarrier(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	RTV�n���h���̃Z�b�g
	auto dev = _pWrap->GetDevice();
	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();						//	RTV�̃X�^�[�g�|�C���g
	uint32_t offset = 0;																	//	�r���[�̃I�t�Z�b�g�ʒu
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//	�����_�[�^�[�Q�b�g�r���[�̃C���N�������g�T�C�Y
	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[static_cast<int>(E_ORIGIN_RTV::BLOOM) + 1];											//	�n���h��
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, offset);
		offset += incSize;
	}

	//	�[�x�o�b�t�@�p�f�B�X�N���v�^�q�[�v�n���h���擾
	auto dsvHeap = _pWrap->GetDsvDescHeap();
	auto dsvHeapPointer = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	�����_�[�^�[�Q�b�g�Z�b�g
	auto cmdList = _pWrap->GetCmdList();
	cmdList->OMSetRenderTargets(
		(static_cast<int>(E_ORIGIN_RTV::BLOOM) + 1), handles, false, &dsvHeapPointer);
	//�N���A�J���[		 R   G   B   A
	//	�I���W���p�̃����_�[�^�[�Q�b�g�r���[���N���A
	for (int i = 0;i<_countof(handles);i++)
	{
		//	�u���[���̃N���A�J���[�����ɂ���
		if (i == static_cast<int>(E_ORIGIN_RTV::BLOOM))
		{
			cmdList->ClearRenderTargetView(handles[i], NONE_CLSCLR, 0, nullptr);
		}
		else
		{
			cmdList->ClearRenderTargetView(handles[i], CLSCLR, 0, nullptr);
		}
	}
	//	�[�x�o�b�t�@�r���[���N���A
	cmdList->ClearDepthStencilView(dsvHeapPointer,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

//	�I���W�������_�[�^�[�Q�b�g�̕`��I��
void VisualEffect::EndOriginDraw(void)
{
	//	���f���`���̃o���A�ݒ�
	for (auto& res : _origin1Resource)
	{
		_pWrap->SetBarrier(
			res.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	//	�u���[���`���o���A�w��
	_pWrap->SetBarrier(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//	���H�p�̃����_�[�^�[�Q�b�g�̕`��
void VisualEffect::ProceDraw(void)
{
	//	���H�p�`��O�o���A�w��
	_pWrap->SetBarrier(
		_proceResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	���H�pRTV���Z�b�g
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapPointer;
	auto dev = _pWrap->GetDevice();
	rtvHeapPointer.InitOffsetted(
		_originRTVHeap->GetCPUDescriptorHandleForHeapStart(),
		dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * static_cast<int>(E_ORIGIN_RTV::PROCE)
	);
	auto cmdList = _pWrap->GetCmdList();

	cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	�N���A�J���[	
	cmdList->ClearRenderTargetView(rtvHeapPointer, CLSCLR, 0, nullptr);
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//�r���[�|�[�g

	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`


	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	�y���|���S���p�̃��[�g�V�O�l�C�`���Z�b�g
	cmdList->SetPipelineState(_prPoriPipeline.Get());						//	�y���|���S���p�̃p�C�v���C���Z�b�g
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	�y���|���S���p�̒��_�o�b�t�@�r���[�Z�b�g
	//	�I���W���p��SRV�q�[�v���Z�b�g
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	int incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	�p�����[�^�[0�ԁi�e�N�X�`���[���\�[�X�j�ƃq�[�v���֘A�t����
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	�p�����[�^�[0�ԁi�萔�o�b�t�@���\�[�X�j�ƃq�[�v���֘A�t����
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	�ʏ�f�v�X�[�x�o�b�t�@�[�e�N�X�`��
	auto depthSrvHeap = _pWrap->GetDepthSRVDescHeap();
	cmdList->SetDescriptorHeaps(1, depthSrvHeap.GetAddressOf());
	//	�[�x�o�b�t�@�[�e�N�X�`���p�̃��\�[�X�ƃq�[�v���֘A�t����
	handle = depthSrvHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(
		3,
		handle);
	
	//	���C�g�f�v�X�[�x�o�b�t�@�e�N�X�`��
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetGraphicsRootDescriptorTable(
		4,
		handle);

	cmdList->DrawInstanced(4, 1, 0, 0);


	//	���H�p�`���o���A�w��
	_pWrap->SetBarrier(
		_proceResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

//	�i�y���|���S�����́j�`��
void VisualEffect::EndDraw(void)
{
	auto cmdList = _pWrap->GetCmdList();
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	auto size = WinApp.GetWindowSize();

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, size.cx, size.cy);
	cmdList->RSSetViewports(1, &vp);//�r���[�|�[�g
	CD3DX12_RECT rc(0, 0, size.cx, size.cy);
	cmdList->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`

	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());					//	�y���|���S���p�̃��[�g�V�O�l�C�`���Z�b�g
	cmdList->SetPipelineState(_procePipeline.Get());						//	�y���|���S���p�̃p�C�v���C���Z�b�g

	//	�I���W���p��SRV�q�[�v���Z�b�g
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());
	//	���H�p�̃e�N�X�`���ƃq�[�v���֘A�t����
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	auto baseH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	auto dev = _pWrap->GetDevice();
	int incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::PROCE));
	cmdList->SetGraphicsRootDescriptorTable(0, handle);
	//	�p�����[�^�[0�ԁi�萔�o�b�t�@���\�[�X�j�ƃq�[�v���֘A�t����
	handle.InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_SRV::BOKE));
	cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//	�|�X�g�G�t�F�N�g�p��SRV�q�[�v���Z�b�g
	cmdList->SetDescriptorHeaps(1, _effectSRVHeap.GetAddressOf());
	//	�|�X�g�G�t�F�N�g�p�̃��\�[�X�ƃq�[�v���֘A�t����
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		_effectSRVHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);						//	�y���|���S���p�̒��_�o�b�t�@�r���[�Z�b�g
	cmdList->DrawInstanced(4, 1, 0, 0);
}

#define SHRINKCOUNT 8	//	�k����
//	�k���o�b�t�@�ڂ����`�揈��
void VisualEffect::DrawShrinkTextureForBlur(void)
{
	auto cmdList = _pWrap->GetCmdList();

	cmdList->SetPipelineState(_blurPipeline.Get());
	cmdList->SetGraphicsRootSignature(_prPoriRS.Get());

	//	���_�o�b�t�@�Z�b�g
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &_prPoriVBV);

	//	�k���o�b�t�@�̓����_�[�^�[�Q�b�g��
	_pWrap->SetBarrier(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	�ʏ�ڂ����̃����_�[�^�[�Q�b�g
	_pWrap->SetBarrier(
		_dofBuffer.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	//	�����_�[�^�[�Q�b�g�Z�b�g
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	auto baseH = _originRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto dev = _pWrap->GetDevice();
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvHandles[0].InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_RTV::SHRINKBLOOM));
	rtvHandles[1].InitOffsetted(baseH, incSize * static_cast<int>(E_ORIGIN_RTV::DOF));
	cmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);
	//	�����_�[�^�[�Q�b�g�N���A
	for (auto& rtv : rtvHandles)
	{
		cmdList->ClearRenderTargetView(rtv, NONE_CLSCLR, 0, nullptr);
	}

	//	�V�F�[�_�[���\�[�X�Z�b�g
	cmdList->SetDescriptorHeaps(1, _originSRVHeap.GetAddressOf());

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
	auto srvH = _originSRVHeap->GetGPUDescriptorHandleForHeapStart();
	incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//	�ʏ탌���_�����O�Z�b�g
	srvHandle.InitOffsetted(
		srvH,
		incSize * static_cast<int>(E_ORIGIN_SRV::COL));
	cmdList->SetGraphicsRootDescriptorTable(static_cast<int>(E_ORIGIN_SRV::COL), srvHandle);
	//	���P�x�e�N�X�`���[�Z�b�g
	srvHandle.InitOffsetted(
		srvH,
		incSize * static_cast<int>(E_ORIGIN_SRV::BLOOM));
	cmdList->SetGraphicsRootDescriptorTable(static_cast<int>(E_ORIGIN_SRV::BLOOM), srvHandle);

	//	�k���o�b�t�@�̏����T�C�Y�ݒ�
	//		�����T�C�Y�̔����̃T�C�Y�ɏ��������Ă���
	auto desc = _bloomBuffer[0]->GetDesc();
	D3D12_VIEWPORT vp = {};
	D3D12_RECT sr = {};
	
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.Height = desc.Height / 2;
	vp.Width = desc.Width / 2;
	sr.top = 0;
	sr.left = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	for (int i = 0; i < SHRINKCOUNT; i++)
	{
		//	�`��͈̓Z�b�g
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &sr);
		//	�`��
		cmdList->DrawInstanced(4, 1, 0, 0);

		//	���ɂ��炷
		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;

		//	��������������
		//	��sr.right���ύX����̂��H
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
		sr.right = vp.Width;
	}

	//	�k�����P�x���V�F�[�_�[���\�[�X��
	_pWrap->SetBarrier(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//	�k���ʏ���V�F�[�_�[���\�[�X��
	_pWrap->SetBarrier(
		_dofBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}