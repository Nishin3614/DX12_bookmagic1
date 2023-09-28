#pragma once
//	�C���N���[�h
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <iostream>
#include "polygon2D.h"

//	���C�u����
#pragma comment(lib,"d3d12.lib")

//	�N���X
class Dx12Wrapper;
class Renderer2D
{
public:
	//	�R���X�g���N�^
	Renderer2D(Dx12Wrapper* pWrap);
	//	����������
	void Init(void);
	//	�`�揈��
	void Draw(void);
	//	�I�u�W�F�N�g�쐬
	Polygon2D* Create2D(std::string texName);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*	�֐�	*/
	//	���[�g�V�O�l�C�`���쐬
	void CreateRootSignature(void);
	//	�p�C�v���C���쐬
	void CreatePipeline(void);

	/*	�ϐ�	*/
	ComPtr<ID3D12PipelineState> _pls = nullptr;		//	pipeline�X�e�[�g
	ComPtr<ID3D12RootSignature> _rs = nullptr;		//	���[�g�V�O�l�C�`��

	//	���N���X�̃C���X�^���X
	Dx12Wrapper* _pWrap;
	std::vector<std::unique_ptr<Polygon2D>> _Polygons;				//	�|���S��
};