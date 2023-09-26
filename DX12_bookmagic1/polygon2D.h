#pragma once
//	�C���N���[�h
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>

//	���C�u����
#pragma comment(lib,"d3d12.lib")

//	�N���X
class Dx12Wrapper;
class Polygon2D
{
public:
	//	�R���X�g���N�^
	Polygon2D(Dx12Wrapper* pWrap);
	//	����������
	void Init(void);
	//	�`�揈��
	void Draw(void);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*	�֐�	*/
	//	���_�o�b�t�@�쐬
	void CreateVertexBuffer(void);
	//	�e�N�X�`���o�b�t�@�쐬
	void CreateTexBuffer(void);
	//	�s��o�b�t�@�쐬
	void CreateMatBaffer(void);
	//	���[�g�V�O�l�C�`���쐬
	void CreateRootSignature(void);
	//	�p�C�v���C���쐬
	void CreatePipeline(void);

	/*	�ϐ�	*/
	ComPtr<ID3D12Resource> _vb = nullptr;			//	���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW _vbv = {};				//	���_�o�b�t�@�r���[
	ComPtr<ID3D12Resource> _texBuffer = nullptr;	//	�e�N�X�`���[�o�b�t�@
	ComPtr<ID3D12Resource> _constBuffer = nullptr;	//	�萔�o�b�t�@
	ComPtr<ID3D12DescriptorHeap> _dh = nullptr;		//	�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12PipelineState> _pls = nullptr;		//	pipeline�X�e�[�g
	ComPtr<ID3D12RootSignature> _rs = nullptr;		//	���[�g�V�O�l�C�`��

	DirectX::XMFLOAT2 _pos = {};					//	�ʒu

	//	���N���X�̃C���X�^���X
	Dx12Wrapper* _pWrap;
};