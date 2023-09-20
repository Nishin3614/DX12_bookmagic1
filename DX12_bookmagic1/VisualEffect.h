#pragma once

//	�C���N���[�h
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <DirectXMath.h>

//	���C�u����
#pragma comment(lib,"d3d12.lib")

//	�O���錾
class Dx12Wrapper;
//	�N���X�錾
class VisualEffect
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	�\����	//
	struct PostSetting
	{
		DirectX::XMFLOAT4 bloomColor;	//	�u���[���J���[
		int nDebugDisp;				//	�f�o�b�O�\���t���O
		int nSSAO;						//	SSAO�\���t���O
		int nMonochro[3];				//	���m�N���t���O
		int nReverse;					//	���]�t���O
		int nDof;						//	��ʊE�[�x�t���O
	};
	//	�֐�	//
	//	�R���X�g���N�^
	VisualEffect(Dx12Wrapper* pWrap);
	/*	�������֘A�̏���	*/
	//	����������
	void Init(void);

	/*	�`��֘A�̏���	*/
	//	�ŏI�`��
	void EndDraw(void);
	//	�}���`�p�X�����_�����O�p�`��1
	void PreOriginDraw(void);
	void EndOriginDraw(void);
	//	�}���`�p�X�����_�����O�p�`��2
	void ProceDraw(void);
	//	�k���o�b�t�@�ڂ����`�揈��
	void DrawShrinkTextureForBlur(void);
	//	�V���h�E�}�b�v�`��
	void ShadowDraw(void);
	//	�[�x�pSRV�Z�b�g
	void DepthSRVSet(void);

	//	�A���r�G���g�I�N���[�W�����ɂ��`��
	void DrawAmbientOcculusion(void);

	//	�|�X�g�ݒ菈��
	void SetPostSetting(bool bDebugDisp,bool bSSAO,bool bMonochro[3],bool bReverse,bool bDof,float bloomColor[3]);

private:

	//	�֐�	//
	/*	�������֘A�̏���	*/
	//	���f���Ȃǂ̕`��p�̃����_�[�^�[�Q�b�g���쐬
	void CreateOriginRenderTarget(void);
	//	���H�p�o�b�t�@�쐬
	void CreateProcessRenderTarget(void);
	//	��ʊE�[�x�p�o�b�t�@�쐬
	void CreateBlurForDOFBuffer(void);
	//	�y���|���S���̒��_�o�b�t�@�쐬
	void CreatePeraVertexBuff(void);
	//	�ڂ��萔�o�b�t�@�쐬
	void CreateBokeConstantBuff(void);
	//	�y���|���S���p�̃��[�g�V�O�l�C�`���쐬
	void CreatePeraRootSignature(void);
	//	�y���|���S���p�̃p�C�v���C���쐬
	void CreatePeraGraphicPipeLine(void);
	//	�G�t�F�N�g�p�̃o�b�t�@�ƃr���[�쐬
	bool CreateEffectBufferAndView(void);
	//	�[�x�o�b�t�@�̍쐬
	void CreateDepthView(void);
	//	�|�X�g�ݒ�p�̍쐬
	void CreatePostSetting(void);

	//	�A���r�G���g�I�N���[�W�����o�b�t�@�̍쐬
	bool CreateAmbientOcculusionBuffer(void);
	//	�A���r�G���g�I�N���[�W�����f�B�X�N���v�^�q�[�v�쐬
	bool CreateAmbientOcculusionDescriptorHeap(void);

	//	�ϐ�	//
	// DX12�̊�b���
	Dx12Wrapper* _pWrap;
	//	���f���Ȃǂ̃����_�[�^�[�Q�b�g��p�̕ϐ�
	std::array<ComPtr<ID3D12Resource>,2> _origin1Resource;	//	�\������̃��\�[�X
											//	�y���|���S���ɒ���t���邽�߂̃e�N�X�`�����\�[�X��(���̒��ɍ��܂ō쐬���Ă������f����炪�`�悳��Ă���j
	ComPtr<ID3D12DescriptorHeap> _originRTVHeap;	//	�����_�[�^�[�Q�b�g�p
	ComPtr<ID3D12DescriptorHeap> _originSRVHeap;	//	�e�N�X�`���p

	//	�y���|���S���쐬�p�̕ϐ�
	ComPtr<ID3D12Resource> _prPoriVB;			//	�y���|���S�����_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW _prPoriVBV;		//	�y���|���S�����_�o�b�t�@�r���[
	ComPtr<ID3D12RootSignature> _prPoriRS;	//	�y���|���S���p���[�g�V�O�l�C�`���[
	ComPtr <ID3D12PipelineState> _prPoriPipeline;	//	�y���|���S���p�O���t�B�b�N�p�C�v���C���X�e�[�g
	ComPtr<ID3D12Resource> _bokehParamBuffer;		//	�ڂ����p�̒萔�o�b�t�@

	//	���H�p�̃����_�[�^�[�Q�b�g�쐬�p�̕ϐ�
	ComPtr<ID3D12Resource> _proceResource;	//	�y���|���S���ɒ���t����ꂽ���\�[�X�����H���邽�߂̕ϐ�
	ComPtr<ID3D12PipelineState> _procePipeline;	//	���H�p�O���t�B�b�N�p�C�v���C��

	//	�c�݃e�N�X�`���[�p
	ComPtr<ID3D12DescriptorHeap> _effectSRVHeap;
	ComPtr<ID3D12Resource> _efffectTexBuffer;
	ComPtr<ID3D12PipelineState> _effectPipeline;

	//	�u���[���p�o�b�t�@
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//	�u���[���p�o�b�t�@
	ComPtr<ID3D12PipelineState> _blurPipeline;			//	��ʑS�̂ڂ����p�p�C�v���C��

	//	��ʊE�[�x�p�o�b�t�@
	ComPtr<ID3D12Resource> _dofBuffer;	//	��ʊE�[�x�p�ڂ����o�b�t�@

	//	�A���r�G���g�I�N���[�W�����p
	ComPtr<ID3D12Resource> _aoBuffer;
	ComPtr<ID3D12RootSignature> _aoRS;
	void CreateAoRootSignature(void);
	ComPtr<ID3D12PipelineState> _aoPipeline;
	ComPtr<ID3D12DescriptorHeap> _aoRTVDH;
	ComPtr<ID3D12DescriptorHeap> _aoSRVDH;

	//	�[�x�p�o�b�t�@
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	�[�x�o�b�t�@�p�̃f�B�X�N���v�^�q�[�v
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	�[�x�o�b�t�@
	ComPtr<ID3D12Resource> _lightDepthBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;		//	�[�x�l�e�N�X�`���[�p�q�[�v

	//	Imgui�f�o�b�O�@�\
	ComPtr<ID3D12Resource> _postSettingResource;
	PostSetting* _pMapPostSetting;
	ComPtr<ID3D12DescriptorHeap> _postSettingDH;
};