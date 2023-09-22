#include "effectEffekseer.h"

#include "Dx12Wrapper.h"
#include "DXApplication.h"
#include "sceneInfo.h"

EffectEffekseer::EffectEffekseer(Dx12Wrapper *pWrap) : _pWrap(pWrap)
{
}

void EffectEffekseer::Init(void)
{
	//	レンダラーの初期化
	//DXGI_FORMAT dxgiFormat = _pWrap->GetBackDesc().Format;
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	_efkRenderer = EffekseerRendererDX12::Create(
		_pWrap->GetDevice().Get(),	//	DirectX12のデバイス
		_pWrap->GetCmdQue().Get(),	//	DirectX12のコマンドキュー
		2,							//	バックバッファーの数
		&dxgiFormat,				//	レンダーターゲットフォーマット
		1, 							//	レンダーターゲット数
		DXGI_FORMAT_UNKNOWN,	//	深度バッファのフォーマット
		false,						//	反対デプスありなし
		10000						//	パーティクル数
	);
	//	マネージャーの初期化
	_efkManager = Effekseer::Manager::Create(10000);	//	インスタンス数

	//	座標系を左手系にする
	_efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	//	描画用インスタンスから描画機能を設定
	_efkManager->SetSpriteRenderer(_efkRenderer->CreateSpriteRenderer());
	_efkManager->SetRibbonRenderer(_efkRenderer->CreateRibbonRenderer());
	_efkManager->SetRingRenderer(_efkRenderer->CreateRingRenderer());
	_efkManager->SetTrackRenderer(_efkRenderer->CreateTrackRenderer());
	_efkManager->SetModelRenderer(_efkRenderer->CreateModelRenderer());
	//	描画用インスタンスからテクスチャーの読み込み機能を設定
	//	独自拡張も可能
	_efkManager->SetTextureLoader(_efkRenderer->CreateTextureLoader());
	_efkManager->SetModelLoader(_efkRenderer->CreateModelLoader());
	//	Directx12特有の処理
	_efkMemoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(_efkRenderer->GetGraphicsDevice());
	_efkCmdlList = EffekseerRenderer::CreateCommandList(_efkRenderer->GetGraphicsDevice(), _efkMemoryPool);
	_efkRenderer->SetCommandList(_efkCmdlList);

	//	エフェクトの読み込み
	_effect = Effekseer::Effect::Create(
		_efkManager,
		(const EFK_CHAR*)L"effect/10/SimpleLaser.efk",
		1.0f,
		(const EFK_CHAR*)L"effect/10"
	);

	//	エフェクトの読み込み
	_effect2 = Effekseer::Effect::Create(
		_efkManager,
		(const EFK_CHAR*)L"effect/10/SimpleLaser.efk",
		1.0f,
		(const EFK_CHAR*)L"effect/10"
	);
}

//	描画処理
void EffectEffekseer::Draw(void)
{
	Syncronize();
	_efkManager->Update();		//	時間更新
	_efkMemoryPool->NewFrame();	//	描画すべきレンダーターゲットを選択

	EffekseerRendererDX12::BeginCommandList(_efkCmdlList, _pWrap->GetCmdList().Get());

	_efkRenderer->BeginRendering();	//	描画前処理
	_efkManager->Draw();			//	エフェクト描画
	_efkRenderer->EndRendering();	//	描画後処理

	EffekseerRendererDX12::EndCommandList(_efkCmdlList);
}

//	同期処理
void EffectEffekseer::Syncronize(void)
{
	Effekseer::Matrix44 fkViewMat;
	Effekseer::Matrix44 fkProjMat;

	DXApplication& pDxApp = DXApplication::Instance();
	auto pSceneInfo = pDxApp.GetSceneInfo();
	auto view = pSceneInfo->GetViewMatrix();	//	ビュー行列取得
	auto proj = pSceneInfo->GetProjMatrix();	//	プロジェクション行列取得

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

//	再生
void EffectEffekseer::Play(void)
{
	//	エフェクトの再生
	_efkHandle = _efkManager->Play(_effect, 0, 5, 0);
	//	エフェクトの再生
	_efkHandle2 = _efkManager->Play(_effect2, 5, 5, 0);
}

//	停止
void EffectEffekseer::Stop(void)
{
	//	エフェクトの再生
	_efkManager->StopAllEffects();
}

