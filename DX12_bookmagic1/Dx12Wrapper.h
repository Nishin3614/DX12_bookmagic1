#pragma once

//	�C���N���[�h
#include <DirectXTex.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>
#include <map>
#include <string>
#include <array>

//#include <DirectXMath.h>

//	���C�u����
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")


//	�N���X�錾
class Dx12Wrapper
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
		DirectX::XMMATRIX lightCamera;	//	���C�g���猩���r���[
		DirectX::XMMATRIX shadow;	//	�e
		DirectX::XMFLOAT3 eye;		//	���_���W
	};
	
	//	�e�N�X�`���[���̏ꍇ�̃e�N�X�`���[�o�b�t�@���
	enum class E_NONETEX : int
	{
		WHITE,
		BLACK,
		GRADUATION
	};

	//	�֐�	//
	//	�R���X�g���N�^
	Dx12Wrapper();
	/*	�������֘A�̏���	*/
	//	����������
	void Init(HWND hwnd);

	/*	�`��֘A�̏���	*/
	//	�ŏI�`��
	void Draw(void);
	//	�N���A
	void Clear(void);
	//	�t���b�v
	void Flip(void);
	//	�}���`�p�X�����_�����O�p�`��1
	void PreOriginDraw(void);
	void EndOriginDraw(void);
	//	�}���`�p�X�����_�����O�p�`��2
	void ProceDraw(void);
	void ShadowDraw(void);
	//	�V�[���r���[�̃Z�b�g����
	void CommandSet_SceneView(void);
	//	�k���o�b�t�@�ڂ����`�揈��
	void DrawShrinkTextureForBlur(void);

	/*	���擾�֘A�̏���*/
	//	�X���b�v�`�F�C���̎擾
	ComPtr<IDXGISwapChain4> GetSwapchain(void) { return _swapchain; }
	//	�f�o�C�X�̎擾
	ComPtr<ID3D12Device> GetDevice(void) { return _dev; }
	//	�R�}���h���X�g�̎擾
	ComPtr<ID3D12GraphicsCommandList> GetCmdList(void) { return _cmdList; }
	//	�e�N�X�`���[�ǂݍ���
	ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//	�e�e�N�X�`�����o�b�t�@�擾
	ComPtr<ID3D12Resource> GetNoneTexture(E_NONETEX nTex) { return _noneTexTable[static_cast<int>(nTex)]; }

private:

	//	�֐�	//
	/*	�������֘A�̏���	*/
	//	�f�o�b�O���C���[�L��������
	void EnableDebugLayer(void);
	//	�f�o�C�X�̍쐬
	void CreateDevice(void);
	//	�R�}���h���X�g�Ȃǂ̍쐬
	void CreateCommand(void);
	//	�X���b�v�`�F�C���̍쐬
	void CreateSwapchain(HWND hwnd);
	//	�[�x�o�b�t�@�̍쐬
	void CreateDepthView(void);
	//	�r���[�E�v���W�F�N�V�����s��o�b�t�@�̍쐬
	void CreateViewProjectionView(void);
	//	���f���Ȃǂ̕`��p�̃����_�[�^�[�Q�b�g���쐬
	void CreateOriginRenderTarget(void);
	void CreateProcessRenderTarget(void);
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
	ComPtr < ID3D12Resource> CreateWhiteTexture(void);			//	���e�N�X�`���[�o�b�t�@�쐬
	ComPtr < ID3D12Resource> CreateBlackTexture(void);			//	���e�N�X�`���[�o�b�t�@�쐬
	ComPtr < ID3D12Resource> CreateGrayGradationTexture(void);	//	�O���f�[�V�����o�b�t�@�쐬

	//	�ϐ�	//
	ComPtr<ID3D12Device> _dev = nullptr;						//	�f�o�C�X
	ComPtr<IDXGIFactory6> _dxgifactory = nullptr;				//	�t�@�N�g���[
	ComPtr<IDXGISwapChain4> _swapchain = nullptr;				//	�X���b�v�`�F�C��
	ComPtr < ID3D12CommandAllocator> _cmdAllocator = nullptr;	//	�R�}���h�A���P�[�^�[
	ComPtr < ID3D12GraphicsCommandList> _cmdList = nullptr;		//	�R�}���h���X�g
	ComPtr < ID3D12CommandQueue> _cmdQueue = nullptr;			//	�R�}���h�L���[
	ComPtr < ID3D12DescriptorHeap> rtvHeaps = nullptr;			//	�f�B�X�N���v�^�q�[�v�i�����_�[�^�[�Q�b�g�j
	std::vector< ID3D12Resource*> _backBuffers;			//	�o�b�N�o�b�t�@
	ComPtr < ID3D12Fence> _fence = nullptr;						//	�t�F���X
	UINT64 _fenceVal = 0;								//	�t�F�C�X�l
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	�[�x�o�b�t�@�p�̃f�B�X�N���v�^�q�[�v
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	�[�x�o�b�t�@

	ComPtr<ID3D12DescriptorHeap> _ScenevHeap = nullptr;	//	�V�[���f�B�X�N���v�^
	ComPtr<ID3D12Resource> _SceneBuffer = nullptr;		//	�V�[���o�b�t�@
	SceneMatrix* _pMapSceneMtx;							//	�V�[���s��̃}�b�v
	DirectX::XMFLOAT3 _parallelLightVec;				//	���s���C�g�̌���
	DirectX::XMFLOAT3 _eye;		//	���_
	DirectX::XMFLOAT3 _target;	//	�����_
	DirectX::XMFLOAT3 _up;		//	��x�N�g��
	SIZE _windowSize;			//	�E�B���h�E�T�C�Y


	//	�e�N�X�`���[�o�b�t�@�̃^�[�u���Ȃ�	//
	std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;	//	�t�@�C�����p�X�ƃ��\�[�X�̃}�b�v�e�[�u��
	ComPtr < ID3D12Resource> _noneTexTable[3];				//	�t�@�C���Ȃ��e�N�X�`���o�b�t�@


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

	//	�V���h�E�}�b�v�p�[�x�o�b�t�@
	ComPtr<ID3D12Resource> _lightDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;		//	�[�x�l�e�N�X�`���[�p�q�[�v

	//	�u���[���p�o�b�t�@
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//	�u���[���p�o�b�t�@
	ComPtr<ID3D12PipelineState> _blurPipeline;			//	��ʑS�̂ڂ����p�p�C�v���C��
};