#ifndef _H_PMDRENDERER_
#define _H_PMDRENDERER_

//	�C���N���[�h
#include <d3d12.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <memory>

#include <DirectXMath.h>

//	���C�u����
#pragma comment(lib,"d3d12.lib")

//	�N���X�̑O���錾
class Dx12Wrapper;
//	�N���X�錾
class PMDRenderer
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	//	�\����	//
	//	�V�F�[�_�[���ɓ�������}�e���A���f�[�^
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;	//	�f�B�t���[�Y�F
		float alpha;		//	�f�B�t���[�Y��
		DirectX::XMFLOAT3 specular;	//	�X�y�L�����F
		float specularity;	//	�X�y�L�����̋����i��Z�l�j
		DirectX::XMFLOAT3 ambient;	//	�A���r�G���g�F
	};
	//	����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial
	{
		std::string texPath;	//	�e�N�X�`���t�@�C���p�X
		int toonIdx;			//	�g�D�[���ԍ�
		bool edgeFlg;			//	�}�e���A�����Ƃ̗֊s���t���O
	};
	//	�S�̂��܂Ƃ߂�f�[�^
	struct Material
	{
		unsigned int indicesNum;	//	�C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	//	�֐�	//
	// �R���X�g���N�^
	PMDRenderer(Dx12Wrapper* pDxWap);
	//	����������
	void Init(void);
	//	�`�揈��
	void Draw(void);
	//	�V���h�E�}�b�v�p�`�揈��
	void PreShadowDraw(void);
	//	�������
	void Release(void);
	//	�X�V����
	void Update(void);

private:
	//	�֐�	//
	//	���[�g�p�����[�^�ƃ��[�g�V�O�l�C�`���̍쐬
	void CreateRootParameterOrRootSignature(void);
	//	�O���t�B�b�N�p�C�v���C���̍쐬
	void CreateGraphicPipeline(void);

	//	�ϐ�	//
	ComPtr < ID3D12PipelineState> _pipelinestate = nullptr;		//	�O���t�B�b�N�X�p�C�v���C���X�e�[�g
	ComPtr < ID3D12RootSignature> _rootsignature = nullptr;		//	���[�g�V�O�l�C�`��

	//	�V���h�E�}�b�v�p
	ComPtr<ID3D12PipelineState> _plsShadow = nullptr;			//	�V���h�E�}�b�v�p�p�C�v���C��

	//	���̃N���X�̃C���X�^���X
	Dx12Wrapper* _pDxWap;
};

#endif // !_H_PMDRENDERER_
