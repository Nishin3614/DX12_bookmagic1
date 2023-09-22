#pragma once

//	�C���N���[�h
#include <DirectXTex.h>
#include <d3d12.h>
#include <dxgi1_6.h>
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
	//	�N���A
	void Clear(void);
	//	�t���b�v
	void Flip(void);
	//	�o���A�ݒ菈��
	void SetBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES pre, D3D12_RESOURCE_STATES dest);

	/*	���擾�֘A�̏���*/
	//	�X���b�v�`�F�C���̎擾
	ComPtr<IDXGISwapChain4> GetSwapchain(void) { return _swapchain; }
	//	�f�o�C�X�̎擾
	ComPtr<ID3D12Device> GetDevice(void) { return _dev; }
	//	�R�}���h���X�g�̎擾
	ComPtr<ID3D12GraphicsCommandList> GetCmdList(void) { return _cmdList; }
	//	�R�}���h�L���[�̎擾
	ComPtr<ID3D12CommandQueue> GetCmdQue(void) { return _cmdQueue; }
	//	�o�b�N�o�b�t�@�̃f�B�X�N���擾
	D3D12_RESOURCE_DESC GetBackDesc(void) { return _backBuffers[0]->GetDesc(); }
	//	�f�B�X�N���v�^�q�[�v�̃f�B�X�N���擾
	D3D12_DESCRIPTOR_HEAP_DESC GetDescriptorHeapD(void) { return rtvHeaps->GetDesc(); }
	//	�e�N�X�`���[�ǂݍ���
	ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//	�e�e�N�X�`�����o�b�t�@�擾
	ComPtr<ID3D12Resource> GetNoneTexture(E_NONETEX nTex) { return _noneTexTable[static_cast<int>(nTex)]; }
	//	Imgui�̃q�[�v�擾
	ComPtr<ID3D12DescriptorHeap> GetHeapForImgui() { return _heapForImgui; }

	//	�w�i�F�̃Q�b�^�[�Z�b�^�[
	float* GetBgCol(void) { return _bgColor; };
	void SetBgCol(float bgCol[4]) { std::copy_n(bgCol, 4, std::begin(_bgColor)); }

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
	//	imgui
	void CreateDescriptorHeapForImgui(void);

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
	float _bgColor[4] = { 0.5f,0.5f,0.5f,1.0f };		//	�w�i�J���[

	//	imgui
	ComPtr<ID3D12DescriptorHeap> _heapForImgui;	//	�q�[�v�ێ��p

	//	�e�N�X�`���[�o�b�t�@�̃^�[�u���Ȃ�	//
	std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;	//	�t�@�C�����p�X�ƃ��\�[�X�̃}�b�v�e�[�u��
	ComPtr < ID3D12Resource> _noneTexTable[3];				//	�t�@�C���Ȃ��e�N�X�`���o�b�t�@
};