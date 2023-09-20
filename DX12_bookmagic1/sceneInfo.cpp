#include "sceneInfo.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "Dx12Wrapper.h"

namespace//	�萔��`
{
	constexpr float shadow_difinition = 40.0f;	//	���C�g�f�v�X�̏c���T�C�Y
}

using namespace DirectX;

//	�R���X�g���N�^�[
SceneInfo::SceneInfo(Dx12Wrapper* pWrap) : 
	_pWrap(pWrap),
	_eye(0,10,-15),
	_target(0,10,0),
	_up(0,1,0),
	_pMapSceneMtx(nullptr)
{
}

//	����������
void SceneInfo::Init(void)
{
	//	�V�[���o�b�t�@�̍쐬
	CreateViewProjectionView();
}

//	�V�[�����X�V����
void SceneInfo::SetSceneInfo(void)
{
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();

	//	��p�̕ύX
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	�������e����(�p�[�X����j
		_fov,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);

	//	�����x�N�g���̕ύX
	_pMapSceneMtx->lightVec.x = _lightVec.x;
	_pMapSceneMtx->lightVec.y = _lightVec.y;
	_pMapSceneMtx->lightVec.z = _lightVec.z;
	//	���C�g�r���[�v���W�F�N�V����
	auto eyePos = XMLoadFloat3(&_eye);			//	���_
	auto targetPos = XMLoadFloat3(&_target);	//	�����_
	auto upPos = XMLoadFloat3(&_up);			//	��x�N�g��
	XMVECTOR lightVec = -XMLoadFloat3(&_lightVec);
	XMVECTOR lightPos = targetPos
		+ XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	���s���e�����i�p�[�X�Ȃ��j
			shadow_difinition,		//	���E�͈̔�
			shadow_difinition,		//	�㉺�͈̔�
			1.0f,	//	near
			100.0f	//	for
		);
	//	�e�s��ݒ�
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	���ʂ̕�����
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		lightVec
	);


	//	�V���h�E�}�b�vOnOff�̕ύX
	_pMapSceneMtx->bSelfShadow = _bSelfShadow;
}

//	�r���[�E�v���W�F�N�V�����s��o�b�t�@�̍쐬
void SceneInfo::CreateViewProjectionView(void)
{
	//	�f�B�X�N���v�^�q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;										//	�}�X�N
	descHeapDesc.NumDescriptors = 1;								//	�萔�o�b�t�@�r���[�iCBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	�V�F�[�_���\�[�X�r���[�p
	//	�f�B�X�N���v�^�q�[�v�̍쐬
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_ScenevHeap.ReleaseAndGetAddressOf())
	);
	//	�q�[�v�v���p�e�B�[�ݒ�
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	�V�[���o�b�t�@�̐ݒ�
	UINT64 u_64BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256�A���C�����g�ɂ��낦���T�C�Y
	UINT BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256�A���C�����g�ɂ��낦���T�C�Y

	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(u_64BufferSize);
	//	�V�[���o�b�t�@�̍쐬
	result = dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_SceneBuffer.ReleaseAndGetAddressOf())
	);
	//	�V�[���o�b�t�@�r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _SceneBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = BufferSize;
	auto HeapHandle = _ScenevHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&cbvDesc, HeapHandle);

	//	�V�[���Ɏg�p����s��̐ݒ�
	result = _SceneBuffer->Map(0, nullptr, (void**)&_pMapSceneMtx);
	//	�r���[�s��ݒ�
	auto eyePos = XMLoadFloat3(&_eye);			//	���_
	auto targetPos = XMLoadFloat3(&_target);	//	�����_
	auto upPos = XMLoadFloat3(&_up);			//	��x�N�g��
	_pMapSceneMtx->eye = _eye;
	_pMapSceneMtx->view = DirectX::XMMatrixLookAtLH(
		eyePos,
		targetPos,
		upPos
	);
	//	�v���W�F�N�V�����s��ݒ�
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_fov = DirectX::XM_PIDIV2;
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	�������e����(�p�[�X����j
		_fov,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);
	//	�t�v���W�F�N�V�����s��ݒ�
	DirectX::XMVECTOR det;
	_pMapSceneMtx->invproj = DirectX::XMMatrixInverse(
		&det,				//	�t�s�񂪎擾�ł��Ȃ��ہu0�v������
		_pMapSceneMtx->proj);

	//	���C�g�r���[�v���W�F�N�V����
	XMVECTOR lightVec = -XMLoadFloat3(&_lightVec);
	XMVECTOR lightPos = targetPos + XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	���s���e�����i�p�[�X�Ȃ��j
			shadow_difinition,		//	���E�͈̔�
			shadow_difinition,		//	�㉺�͈̔�
			1.0f,	//	near
			100.0f	//	for
		);
	//	�e�s��ݒ�
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	���ʂ̕�����
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		lightVec
	);
}

//	�V�[���r���[�̃Z�b�g����
void SceneInfo::CommandSet_SceneView(UINT rootPramIdx)
{
	auto cmdList = _pWrap->GetCmdList();
	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�̎w��
	cmdList->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_ScenevHeap.GetAddressOf()		//	���W�ϊ��p�f�B�X�N���v�^�q�[�v
	);

	//	���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	�萔�o�b�t�@0�r���[�p�̎w��
	cmdList->SetGraphicsRootDescriptorTable(
		rootPramIdx,			//	���[�g�p�����[�^�C���f�b�N�X
		heapHandle	//	�q�[�v�A�h���X
	);
}

//	��p�ݒ�
void SceneInfo::SetFov(float& fov)
{
	_fov = fov;
}

//	�����x�N�g���ݒ�
void SceneInfo::SetLightVec(float vec[3])
{
	_lightVec.x = vec[0];
	_lightVec.y = vec[1];
	_lightVec.z = vec[2];
}

//	�V���h�E�}�b�vOnOff�ݒ�
void SceneInfo::SetSelfShadow(bool bShadow)
{
	_bSelfShadow = bShadow;
}
