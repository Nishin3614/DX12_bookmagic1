//	----------	�V���O���g���݌v	----------	//
#ifndef _H_DXAPPLICATION_
#define _H_DXAPPLICATION_

//	�C���N���[�h
#include <Windows.h>

//	�N���X�̑O���錾
class Dx12Wrapper;
class SceneInfo;
class VisualEffect;
class PMDActor;
class PMDRenderer;
//	�N���X�錾
class DXApplication
{

public:
	//	�\����	//

	//	�֐�	//
	//	Application�̃V���O���g���C���X�^���X�𓾂�
	static DXApplication& Instance(void);
	//	����������
	void OnInit(HWND hwnd,unsigned int width,unsigned int height);
	//	�`�揈��
	void OnRender(void);
	//	�������
	void OnRelease(void);
	//	�X�V����
	void OnUpdate(void);

	/*	�擾�֐�	*/
	Dx12Wrapper* GetWrapper(void) { return _pDxWrap; }
	SceneInfo* GetSceneInfo(void) { return _pSceneInfo; }

private:
	//	�֐�	//
	// �V���O���g���̂��߁A�R���X�g���N�^��private��
	// ����ɃR�s�[�Ƒ�����֎~����
	DXApplication();
	DXApplication(const DXApplication&) = delete;
	void operator=(const DXApplication&) = delete;

	//	�R�s�[�e�N�X�`�����M�I���ł̃e�N�X�`���[�\��t�����������琄������鏈��
	//void ByCopyTextureRegion(void);

	//	�V���h�E�}�b�v�`��
	void ShadowMapDraw(void);
	//	���f���`��
	void ModelDraw(void);

	//	�ϐ�	//
	Dx12Wrapper* _pDxWrap;									//	DirectX����̃N���X
	SceneInfo* _pSceneInfo;									//	�V�[�����N���X
	VisualEffect* _pVFX;										//	�r�W���A���G�t�F�N�g
	PMDActor* _pPmdAct;										//	PMD�A�N�^�[�N���X
	PMDActor* _pPmdAct2;
	PMDRenderer* _pPmdRender;								//	PMD�����_���[�N���X
};

#endif // !_H_DXAPPLICATION_
