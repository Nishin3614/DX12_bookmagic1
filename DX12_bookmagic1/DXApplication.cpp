//	�C���N���[�h
#include "DXApplication.h"
#include "Dx12Wrapper.h"
#include "VisualEffect.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

using namespace DirectX;

//	�C���X�^���X���擾
DXApplication& DXApplication::Instance(void)
{
	static DXApplication instance;
	return instance;
}

//	�R���X�g���N�^
DXApplication::DXApplication() :
	_pDxWrap(nullptr),
	_pVFX(nullptr),
	_pPmdAct(nullptr),
	_pPmdAct2(nullptr),
	_pPmdRender(nullptr)
{
}

//	����������
void DXApplication::OnInit(HWND hwnd, unsigned int window_width, unsigned int window_height)
{
	//	DirectX����̏���������
	_pDxWrap = new Dx12Wrapper;
	_pDxWrap->Init(hwnd);

	//	�r�W���A���G�t�F�N�g�̏���������
	_pVFX = new VisualEffect(_pDxWrap);
	_pVFX->Init();

	//	PMD���f���̏���������
	_pPmdAct = new PMDActor(_pDxWrap, "Model/�����~�N.pmd", "motion/pose.vmd");
	_pPmdAct->Init();

	_pPmdAct2 = new PMDActor(_pDxWrap, "Model/�㉹�n�N.pmd", "motion/motion.vmd", {15.0f,0.0f,5.0f});
	_pPmdAct2->Init();

	//	PMD�����_���[�̏���������
	_pPmdRender = new PMDRenderer(_pDxWrap);
	_pPmdRender->Init();
}

//	�`�揈��
void DXApplication::OnRender(void)
{
	//	�V���h�E�}�b�v�`��
	ShadowMapDraw();

	//	���f���`��
	ModelDraw();

	//	�A���r�G���g�I�N���[�W�����`��
	_pVFX->DrawAmbientOcculusion();

	//	�k���o�b�t�@�̃����_�[�^�[�Q�b�g�̕`��
	_pVFX->DrawShrinkTextureForBlur();

	//	���H�p�̃����_�[�^�[�Q�b�g�̕`��
	_pVFX->ProceDraw();

	//	�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�̃Z�b�g�y�сA�̃N���A
	_pDxWrap->Clear();
	//	�`��
	_pVFX->EndDraw();
	//	�t���b�v
	_pDxWrap->Flip();
}

//	�V���h�E�}�b�v�`��
void DXApplication::ShadowMapDraw(void)
{
	_pPmdRender->PreShadowDraw();
	_pDxWrap->ShadowDraw();
	_pPmdAct->ShadowMapDraw();
	_pPmdAct2->ShadowMapDraw();
}

//	���f���`��
void DXApplication::ModelDraw(void)
{
	//	�I���W�������_�[�^�[�Q�b�g���Z�b�g
	_pVFX->PreOriginDraw();
	//	PMD�����_���[�ɂāA���[�g�V�O�l�C�`���Ȃǂ��Z�b�g
	_pPmdRender->Draw();
	//	�V�[���r���[�̕`��Z�b�g
	_pDxWrap->CommandSet_SceneView();
	//	PMD���f���̕`�揈��
	_pPmdAct->Draw();
	_pPmdAct2->Draw();
	//	�I���W�������_�[�^�[�Q�b�g�̕`��I��
	_pVFX->EndOriginDraw();
}


//	�I�u�W�F�N�g�̉������
void DXApplication::OnRelease(void)
{
	//	DirectX����̉��
	delete _pDxWrap;
	_pDxWrap = nullptr;

	//	�r�W���A���G�t�F�N�g�̉��
	delete _pVFX;
	_pVFX = nullptr;

	//	PMD���f���̉��
	delete _pPmdAct;
	_pPmdAct = nullptr;

	delete 	_pPmdAct2;
	_pPmdAct2 = nullptr;

	//	PMD�����_���[�̉��
	delete _pPmdRender;
	_pPmdRender = nullptr;
}

//	�X�V����
void DXApplication::OnUpdate(void)
{
	_pPmdAct->Update();
	_pPmdAct2->Update();
}