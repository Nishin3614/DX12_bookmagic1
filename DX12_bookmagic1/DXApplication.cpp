//	�C���N���[�h
#include "DXApplication.h"
#include "sceneInfo.h"
#include "Dx12Wrapper.h"
#include "VisualEffect.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//	IMGUI�t�@�C��
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

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
	_pSceneInfo(nullptr),
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

	//	�V�[�����̏���������
	_pSceneInfo = new SceneInfo(_pDxWrap);
	_pSceneInfo->Init();
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

	//	Imgui����������
	InitImgui(hwnd);
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
	//	Imgui�`��
	DrawImgui();


	//	�t���b�v
	_pDxWrap->Flip();
}

//	�V���h�E�}�b�v�`��
void DXApplication::ShadowMapDraw(void)
{
	_pPmdRender->PreShadowDraw();
	_pSceneInfo->CommandSet_SceneView();
	_pVFX->ShadowDraw();
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
	_pSceneInfo->CommandSet_SceneView();
	_pVFX->DepthSRVSet();
	//	PMD���f���̕`�揈��
	_pPmdAct->Draw();
	_pPmdAct2->Draw();

	//	�I���W�������_�[�^�[�Q�b�g�̕`��I��
	_pVFX->EndOriginDraw();
}

//	Imgui�̏���������
void DXApplication::InitImgui(HWND hwnd)
{
	//	�R���e�L�X�g�N���A
	if (ImGui::CreateContext() == nullptr)
	{
		assert(0);
		return;
	}
	
	//	Windows�p�̏���������
	bool bResult = ImGui_ImplWin32_Init(hwnd);
	if (!bResult)
	{
		assert(0);
		return;
	}

	//	DirectX12�p�̏���������
	bResult = ImGui_ImplDX12_Init(
		_pDxWrap->GetDevice().Get(),
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		_pDxWrap->GetHeapForImgui().Get(),
		_pDxWrap->GetHeapForImgui()->GetCPUDescriptorHandleForHeapStart(),
		_pDxWrap->GetHeapForImgui()->GetGPUDescriptorHandleForHeapStart()
	);
}

//	Imgui�̕`��
void DXApplication::DrawImgui(void)
{

	//	�`�揈���O
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//	�E�B���h�E��`
	ImGui::Begin("Rendering Test Menu");
	ImGui::SetWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);
	//	Imgui�̃R���g���[���\��
	DrawControlImgui();
	ImGui::End();

	//	�`�揈��
	ImGui::Render();
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1, _pDxWrap->GetHeapForImgui().GetAddressOf()
	);
	ImGui_ImplDX12_RenderDrawData(
		ImGui::GetDrawData(), _pDxWrap->GetCmdList().Get()
	);
}

//	Imgui�̃R���g���[���\��
void DXApplication::DrawControlImgui(void)
{

	//	�f�o�b�O�\��
	static bool bDebugDisp = false;
	ImGui::Checkbox("Debug Display", &bDebugDisp);

	//	��p
	constexpr float pi = 3.141592653589f;	//	�~����
	static float  fFov = pi / 2.0f;
	ImGui::SliderFloat(
		"Field of view", &fFov, pi / 6.0f, pi / 6.0f * 5.0f
	);

	//	�����x�N�g��
	static float afLightVec[3] = { 1.0f,-1.0f,1.0f };
	ImGui::SliderFloat3("Light Vector", afLightVec, -1.0f, 1.0f);

	//	�w�i�F
	static float bgCol[4] = { 0.5f,0.5f,0.5f,1.0f };
	ImGui::ColorPicker4("BG color",bgCol,
		ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerHueWheel |
		ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaBar);

	//	�u���[���J���[
	static float bloomCol[3] = {0.0f,0.0f,0.0f };
	ImGui::ColorPicker3("Bloom color", bloomCol);

	//	��ʊE�[�x on/off
	static bool bDof = false;
	ImGui::Checkbox("Dof on/off", &bDof);

	//	SSAO ON/OFF
	static bool bSSAO = false;
	ImGui::Checkbox("SSAO on/off", &bSSAO);

	//	�V���h�E�}�b�v ON/OFF
	static bool bShadow = false;
	ImGui::Checkbox("SelfShadow on/off", &bShadow);

	//	���]
	static bool bReverse = false;
	ImGui::Checkbox("Reverse on/off", &bReverse);

	//	���m�N��
	static bool bMonoChro[3] = {};
	if (ImGui::TreeNode("Monochro"))
	{
		ImGui::Checkbox("r", &bMonoChro[0]);
		ImGui::SameLine();
		ImGui::Checkbox("g", &bMonoChro[1]);
		ImGui::SameLine();
		ImGui::Checkbox("b", &bMonoChro[2]);
		ImGui::TreePop();
	}

	//	�Z�b�g
	_pSceneInfo->SetFov(fFov);
	_pDxWrap->SetBgCol(bgCol);
	_pSceneInfo->SetLightVec(afLightVec);
	_pSceneInfo->SetSelfShadow(bShadow);
	_pSceneInfo->SetSceneInfo();
	_pVFX->SetPostSetting(bDebugDisp,bSSAO,bMonoChro,bReverse,bDof,bloomCol);
}


//	�I�u�W�F�N�g�̉������
void DXApplication::OnRelease(void)
{
	//	DirectX����̉��
	delete _pDxWrap;
	_pDxWrap = nullptr;

	//	�V�[�����̉��
	delete _pSceneInfo;
	_pSceneInfo = nullptr;

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