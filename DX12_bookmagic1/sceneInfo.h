#pragma once

//	�C���N���[�h
#include <DirectXTex.h>
#include <d3d12.h>
#include <wrl.h>

//#include <DirectXMath.h>

//	���C�u����
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"DirectXTex.lib")

class Dx12Wrapper;
//	�N���X�錾
class SceneInfo
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	�\����	//
	//	�V�F�[�_�[���ɓn�����߂���{�I�ȍs��f�[�^
	struct SceneMatrix
	{
		DirectX::XMMATRIX view;		//	�r���[�s��
		DirectX::XMMATRIX proj;		//	�v���W�F�N�V�����s��
		DirectX::XMMATRIX invproj;	//	�t�v���W�F�N�V�����s��
		DirectX::XMMATRIX lightCamera;	//	���C�g���猩���r���[
		DirectX::XMMATRIX shadow;	//	�e
		DirectX::XMFLOAT4 lightVec;	//	�����x�N�g��
		DirectX::XMFLOAT3 eye;		//	���_���W
		bool bSelfShadow;			//	�V���h�E�}�b�vOn/Off
	};
	//	�֐�	//
	/*	�������֘A�̏���	*/
	//	�R���X�g���N�^�[
	SceneInfo(Dx12Wrapper* pWrap);

	//	����������
	void Init(void);
	//	�X�V����
	void SetSceneInfo(void);

	/*	�`��֘A�̏���	*/
	//	�V�[���r���[�̃Z�b�g����
	void CommandSet_SceneView(UINT rootPramIdx = 0);

	//	��p�ݒ�
	void SetFov(float& fov); //{ _fov = fov; }
	//	�����x�N�g��
	void SetLightVec(float vec[3]);
	//	�V���h�E�}�b�vOnOff�ݒ�
	void SetSelfShadow(bool bShadow);// { _bSelfShadow = bShadow; }

private:
	
	//	�֐�	//
	/*	�������֘A�̏���	*/
	//	�r���[�E�v���W�F�N�V�����s��o�b�t�@�̍쐬
	void CreateViewProjectionView(void);

	//	�ϐ�	//
	Dx12Wrapper* _pWrap;
	ComPtr<ID3D12DescriptorHeap> _ScenevHeap = nullptr;	//	�V�[���f�B�X�N���v�^
	ComPtr<ID3D12Resource> _SceneBuffer = nullptr;		//	�V�[���o�b�t�@
	SceneMatrix* _pMapSceneMtx;							//	�V�[���s��̃}�b�v
	DirectX::XMFLOAT3 _eye;		//	���_
	DirectX::XMFLOAT3 _target;	//	�����_
	DirectX::XMFLOAT3 _up;		//	��x�N�g��
	float _fov;					//	��p
	DirectX::XMFLOAT3 _lightVec = { 1,-1,1 };	//	�����x�N�g��
	bool _bSelfShadow;			//	�V���h�E�}�b�vOn/Off

	SIZE _windowSize;			//	�E�B���h�E�T�C�Y
};