//	インクルード
#include "DXApplication.h"
#include "sceneInfo.h"
#include "Dx12Wrapper.h"
#include "VisualEffect.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

//	IMGUIファイル
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

using namespace DirectX;

//	インスタンスを取得
DXApplication& DXApplication::Instance(void)
{
	static DXApplication instance;
	return instance;
}

//	コンストラクタ
DXApplication::DXApplication() :
	_pDxWrap(nullptr),
	_pSceneInfo(nullptr),
	_pVFX(nullptr),
	_pPmdAct(nullptr),
	_pPmdAct2(nullptr),
	_pPmdRender(nullptr)
{
}

//	初期化処理
void DXApplication::OnInit(HWND hwnd, unsigned int window_width, unsigned int window_height)
{
	//	DirectX周りの初期化処理
	_pDxWrap = new Dx12Wrapper;
	_pDxWrap->Init(hwnd);

	//	シーン情報の初期化処理
	_pSceneInfo = new SceneInfo(_pDxWrap);
	_pSceneInfo->Init();
	//	ビジュアルエフェクトの初期化処理
	_pVFX = new VisualEffect(_pDxWrap);
	_pVFX->Init();

	//	PMDモデルの初期化処理
	_pPmdAct = new PMDActor(_pDxWrap, "Model/初音ミク.pmd", "motion/pose.vmd");
	_pPmdAct->Init();

	_pPmdAct2 = new PMDActor(_pDxWrap, "Model/弱音ハク.pmd", "motion/motion.vmd", {15.0f,0.0f,5.0f});
	_pPmdAct2->Init();

	//	PMDレンダラーの初期化処理
	_pPmdRender = new PMDRenderer(_pDxWrap);
	_pPmdRender->Init();

	//	Imgui初期化処理
	InitImgui(hwnd);
}

//	描画処理
void DXApplication::OnRender(void)
{
	//	シャドウマップ描画
	ShadowMapDraw();

	//	モデル描画
	ModelDraw();

	//	アンビエントオクルージョン描画
	_pVFX->DrawAmbientOcculusion();

	//	縮小バッファのレンダーターゲットの描画
	_pVFX->DrawShrinkTextureForBlur();

	//	加工用のレンダーターゲットの描画
	_pVFX->ProceDraw();

	//	バックバッファをレンダーターゲットのセット及び、のクリア
	_pDxWrap->Clear();
	//	描画
	_pVFX->EndDraw();
	//	Imgui描画
	DrawImgui();


	//	フリップ
	_pDxWrap->Flip();
}

//	シャドウマップ描画
void DXApplication::ShadowMapDraw(void)
{
	_pPmdRender->PreShadowDraw();
	_pSceneInfo->CommandSet_SceneView();
	_pVFX->ShadowDraw();
	_pPmdAct->ShadowMapDraw();
	_pPmdAct2->ShadowMapDraw();
}

//	モデル描画
void DXApplication::ModelDraw(void)
{
	//	オリジンレンダーターゲットをセット
	_pVFX->PreOriginDraw();
	//	PMDレンダラーにて、ルートシグネイチャなどをセット
	_pPmdRender->Draw();
	//	シーンビューの描画セット
	_pSceneInfo->CommandSet_SceneView();
	_pVFX->DepthSRVSet();
	//	PMDモデルの描画処理
	_pPmdAct->Draw();
	_pPmdAct2->Draw();

	//	オリジンレンダーターゲットの描画終了
	_pVFX->EndOriginDraw();
}

//	Imguiの初期化処理
void DXApplication::InitImgui(HWND hwnd)
{
	//	コンテキストクリア
	if (ImGui::CreateContext() == nullptr)
	{
		assert(0);
		return;
	}
	
	//	Windows用の初期化処理
	bool bResult = ImGui_ImplWin32_Init(hwnd);
	if (!bResult)
	{
		assert(0);
		return;
	}

	//	DirectX12用の初期化処理
	bResult = ImGui_ImplDX12_Init(
		_pDxWrap->GetDevice().Get(),
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		_pDxWrap->GetHeapForImgui().Get(),
		_pDxWrap->GetHeapForImgui()->GetCPUDescriptorHandleForHeapStart(),
		_pDxWrap->GetHeapForImgui()->GetGPUDescriptorHandleForHeapStart()
	);
}

//	Imguiの描画
void DXApplication::DrawImgui(void)
{

	//	描画処理前
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//	ウィンドウ定義
	ImGui::Begin("Rendering Test Menu");
	ImGui::SetWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);
	//	Imguiのコントロール表示
	DrawControlImgui();
	ImGui::End();

	//	描画処理
	ImGui::Render();
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1, _pDxWrap->GetHeapForImgui().GetAddressOf()
	);
	ImGui_ImplDX12_RenderDrawData(
		ImGui::GetDrawData(), _pDxWrap->GetCmdList().Get()
	);
}

//	Imguiのコントロール表示
void DXApplication::DrawControlImgui(void)
{

	//	デバッグ表示
	static bool bDebugDisp = false;
	ImGui::Checkbox("Debug Display", &bDebugDisp);

	//	画角
	constexpr float pi = 3.141592653589f;	//	円周率
	static float  fFov = pi / 2.0f;
	ImGui::SliderFloat(
		"Field of view", &fFov, pi / 6.0f, pi / 6.0f * 5.0f
	);

	//	光源ベクトル
	static float afLightVec[3] = { 1.0f,-1.0f,1.0f };
	ImGui::SliderFloat3("Light Vector", afLightVec, -1.0f, 1.0f);

	//	背景色
	static float bgCol[4] = { 0.5f,0.5f,0.5f,1.0f };
	ImGui::ColorPicker4("BG color",bgCol,
		ImGuiColorEditFlags_::ImGuiColorEditFlags_PickerHueWheel |
		ImGuiColorEditFlags_::ImGuiColorEditFlags_AlphaBar);

	//	ブルームカラー
	static float bloomCol[3] = {1.0f,1.0f,1.0f};
	ImGui::ColorPicker3("Bloom color", bloomCol);

	//	SSAO ON/OFF
	static bool bSSAO = false;
	ImGui::Checkbox("SSAO on/off", &bSSAO);

	//	シャドウマップ ON/OFF
	static bool bShadow = false;
	ImGui::Checkbox("SelfShadow on/off", &bShadow);

	//	反転
	static bool bReverse = false;
	ImGui::Checkbox("Reverse on/off", &bReverse);

	//	モノクロ
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

	//	セット
	_pSceneInfo->SetFov(fFov);
	_pDxWrap->SetBgCol(bgCol);
	_pSceneInfo->SetLightVec(afLightVec);
	_pSceneInfo->SetSelfShadow(bShadow);
	_pSceneInfo->SetSceneInfo();
	_pVFX->SetPostSetting(bDebugDisp,bSSAO,bMonoChro,bReverse,bloomCol);
}


//	オブジェクトの解放処理
void DXApplication::OnRelease(void)
{
	//	DirectX周りの解放
	delete _pDxWrap;
	_pDxWrap = nullptr;

	//	シーン情報の解放
	delete _pSceneInfo;
	_pSceneInfo = nullptr;

	//	ビジュアルエフェクトの解放
	delete _pVFX;
	_pVFX = nullptr;

	//	PMDモデルの解放
	delete _pPmdAct;
	_pPmdAct = nullptr;

	delete 	_pPmdAct2;
	_pPmdAct2 = nullptr;

	//	PMDレンダラーの解放
	delete _pPmdRender;
	_pPmdRender = nullptr;
}

//	更新処理
void DXApplication::OnUpdate(void)
{
	_pPmdAct->Update();
	_pPmdAct2->Update();
}