//	インクルード
#include "DXApplication.h"
#include "Dx12Wrapper.h"
#include "VisualEffect.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

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
	//	フリップ
	_pDxWrap->Flip();
}

//	シャドウマップ描画
void DXApplication::ShadowMapDraw(void)
{
	_pPmdRender->PreShadowDraw();
	_pDxWrap->ShadowDraw();
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
	_pDxWrap->CommandSet_SceneView();
	//	PMDモデルの描画処理
	_pPmdAct->Draw();
	_pPmdAct2->Draw();
	//	オリジンレンダーターゲットの描画終了
	_pVFX->EndOriginDraw();
}


//	オブジェクトの解放処理
void DXApplication::OnRelease(void)
{
	//	DirectX周りの解放
	delete _pDxWrap;
	_pDxWrap = nullptr;

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