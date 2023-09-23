#pragma once

#include <SpriteFont.h>				//	�������\�����邽��
#include <ResourceUploadBatch.h>	//	DirectXTK�֘A�̃��\�[�X���g�p���邽��
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib,"DirectXTK12.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")

class Dx12Wrapper;
class StringDisp
{
public:
	//	�R���X�g���N�^
	StringDisp(Dx12Wrapper* pWrap);
	//	����������
	void Init(void);
	//	�`�揈��
	void Draw(void);
	//	������\���I��
	void EndStrDisp(void);
	//	�I������
	void Release(void);

private:
	/*	�֐�	*/
	//	�f�B�X�N���v�^�q�[�v�쐬
	void CreateDH(void);

	/*	�ϐ�	*/
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12DescriptorHeap> _strDispDH;			//	DH
	DirectX::GraphicsMemory* _gmemory = nullptr;	//	�O���t�B�b�N�X�������I�u�W�F�N�g
	DirectX::SpriteFont* _spriteFont = nullptr;		//	�t�H���g�\���p�I�u�W�F�N�g
	DirectX::SpriteBatch* _spriteBatch = nullptr;	//	�X�v���C�g�\���p�I�u�W�F�N�g

	//	�ق��̃N���X�̃C���X�^���X
	Dx12Wrapper* _pWrap;
};