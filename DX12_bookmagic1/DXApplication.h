//	----------	シングルトン設計	----------	//
#ifndef _H_DXAPPLICATION_
#define _H_DXAPPLICATION_

//	インクルード
#define NOMINMAX	//	std::min,std::maxを使用できるようにするため
#include <Windows.h>

//	クラスの前方宣言
class Dx12Wrapper;
class SceneInfo;
class VisualEffect;
class PMDRenderer;
class EffectEffekseer;
class StringDisp;
class Renderer2D;
//	クラス宣言
class DXApplication
{

public:
	//	構造体	//

	//	関数	//
	//	Applicationのシングルトンインスタンスを得る
	static DXApplication& Instance(void);
	//	初期化処理
	void OnInit(HWND hwnd,unsigned int width,unsigned int height);
	//	描画処理
	void OnRender(void);
	//	解放処理
	void OnRelease(void);
	//	更新処理
	void OnUpdate(void);

	/*	取得関数	*/
	Dx12Wrapper* GetWrapper(void) { return _pDxWrap; }
	SceneInfo* GetSceneInfo(void) { return _pSceneInfo; }
	Renderer2D* GetRenderer2D(void) { return _pRender2D; }
	EffectEffekseer* GetEffectEffekseer(void) { return _pEffectEffekseer; }

private:
	//	関数	//
	// シングルトンのため、コンストラクタをprivateに
	// さらにコピーと代入を禁止する
	DXApplication();
	DXApplication(const DXApplication&) = delete;
	void operator=(const DXApplication&) = delete;

	//	コピーテクスチャレギオンでのテクスチャー貼り付け←公式から推奨される処理
	//void ByCopyTextureRegion(void);

	//	Imguiの初期化処理
	void InitImgui(HWND hwnd);
	//	Imguiの描画処理
	void DrawImgui(void);
	//	Imguiのコントロール表示
	void DrawControlImgui(void);

	//	変数	//
	Dx12Wrapper* _pDxWrap;									//	DirectX周りのクラス
	SceneInfo* _pSceneInfo;									//	シーン情報クラス
	VisualEffect* _pVFX;										//	ビジュアルエフェクト
	PMDRenderer* _pPmdRender;								//	PMDレンダラークラス
	EffectEffekseer* _pEffectEffekseer;
	StringDisp* _pStringDisp;
	Renderer2D* _pRender2D;
};

#endif // !_H_DXAPPLICATION_
