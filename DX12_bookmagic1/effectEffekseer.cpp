#include "effectEffekseer.h"

#include "Dx12Wrapper.h"
#include "DXApplication.h"
#include "sceneInfo.h"

EffectEffekseer::EffectEffekseer(Dx12Wrapper *pWrap) : _pWrap(pWrap)
{
}

void EffectEffekseer::Init(void)
{
	//	�����_���[�̏�����
	//DXGI_FORMAT dxgiFormat = _pWrap->GetBackDesc().Format;
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	_efkRenderer = EffekseerRendererDX12::Create(
		_pWrap->GetDevice().Get(),	//	DirectX12�̃f�o�C�X
		_pWrap->GetCmdQue().Get(),	//	DirectX12�̃R�}���h�L���[
		2,							//	�o�b�N�o�b�t�@�[�̐�
		&dxgiFormat,				//	�����_�[�^�[�Q�b�g�t�H�[�}�b�g
		1, 							//	�����_�[�^�[�Q�b�g��
		DXGI_FORMAT_UNKNOWN,	//	�[�x�o�b�t�@�̃t�H�[�}�b�g
		false,						//	���΃f�v�X����Ȃ�
		10000						//	�p�[�e�B�N����
	);
	//	�}�l�[�W���[�̏�����
	_efkManager = Effekseer::Manager::Create(10000);	//	�C���X�^���X��

	//	���W�n������n�ɂ���
	_efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	//	�`��p�C���X�^���X����`��@�\��ݒ�
	_efkManager->SetSpriteRenderer(_efkRenderer->CreateSpriteRenderer());
	_efkManager->SetRibbonRenderer(_efkRenderer->CreateRibbonRenderer());
	_efkManager->SetRingRenderer(_efkRenderer->CreateRingRenderer());
	_efkManager->SetTrackRenderer(_efkRenderer->CreateTrackRenderer());
	_efkManager->SetModelRenderer(_efkRenderer->CreateModelRenderer());
	//	�`��p�C���X�^���X����e�N�X�`���[�̓ǂݍ��݋@�\��ݒ�
	//	�Ǝ��g�����\
	_efkManager->SetTextureLoader(_efkRenderer->CreateTextureLoader());
	_efkManager->SetModelLoader(_efkRenderer->CreateModelLoader());
	//	Directx12���L�̏���
	_efkMemoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(_efkRenderer->GetGraphicsDevice());
	_efkCmdlList = EffekseerRenderer::CreateCommandList(_efkRenderer->GetGraphicsDevice(), _efkMemoryPool);
	_efkRenderer->SetCommandList(_efkCmdlList);

	//	�G�t�F�N�g�̓ǂݍ���
	_effect = Effekseer::Effect::Create(
		_efkManager,
		(const EFK_CHAR*)L"effect/10/SimpleLaser.efk",
		1.0f,
		(const EFK_CHAR*)L"effect/10"
	);

	//	�G�t�F�N�g�̓ǂݍ���
	_effect2 = Effekseer::Effect::Create(
		_efkManager,
		(const EFK_CHAR*)L"effect/10/SimpleLaser.efk",
		1.0f,
		(const EFK_CHAR*)L"effect/10"
	);
}

//	�`�揈��
void EffectEffekseer::Draw(void)
{
	Syncronize();
	_efkManager->Update();		//	���ԍX�V
	_efkMemoryPool->NewFrame();	//	�`�悷�ׂ������_�[�^�[�Q�b�g��I��

	EffekseerRendererDX12::BeginCommandList(_efkCmdlList, _pWrap->GetCmdList().Get());

	_efkRenderer->BeginRendering();	//	�`��O����
	_efkManager->Draw();			//	�G�t�F�N�g�`��
	_efkRenderer->EndRendering();	//	�`��㏈��

	EffekseerRendererDX12::EndCommandList(_efkCmdlList);
}

//	��������
void EffectEffekseer::Syncronize(void)
{
	Effekseer::Matrix44 fkViewMat;
	Effekseer::Matrix44 fkProjMat;

	DXApplication& pDxApp = DXApplication::Instance();
	auto pSceneInfo = pDxApp.GetSceneInfo();
	auto view = pSceneInfo->GetViewMatrix();	//	�r���[�s��擾
	auto proj = pSceneInfo->GetProjMatrix();	//	�v���W�F�N�V�����s��擾

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			fkViewMat.Values[i][j] = view.r[i].m128_f32[j];
			fkProjMat.Values[i][j] = proj.r[i].m128_f32[j];
		}
	}
	_efkRenderer->SetCameraMatrix(fkViewMat);
	_efkRenderer->SetProjectionMatrix(fkProjMat);
}

//	�Đ�
void EffectEffekseer::Play(void)
{
	//	�G�t�F�N�g�̍Đ�
	_efkHandle = _efkManager->Play(_effect, 0, 5, 0);
	//	�G�t�F�N�g�̍Đ�
	_efkHandle2 = _efkManager->Play(_effect2, 5, 5, 0);
}

//	��~
void EffectEffekseer::Stop(void)
{
	//	�G�t�F�N�g�̍Đ�
	_efkManager->StopAllEffects();
}

