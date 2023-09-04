//	----------	シングルトン設計	----------	//
#ifndef _H_DXAPPLICATION_
#define _H_DXAPPLICATION_

//	インクルード
#include <Windows.h>

//	クラスの前方宣言
class Dx12Wrapper;
class PMDActor;
class PMDRenderer;
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

private:
	//	関数	//
	// シングルトンのため、コンストラクタをprivateに
	// さらにコピーと代入を禁止する
	DXApplication();
	DXApplication(const DXApplication&) = delete;
	void operator=(const DXApplication&) = delete;

	//	コピーテクスチャレギオンでのテクスチャー貼り付け←公式から推奨される処理
	//void ByCopyTextureRegion(void);

	//	シャドウマップ描画
	void ShadowMapDraw(void);
	//	モデル描画
	void ModelDraw(void);

	//	変数	//
	Dx12Wrapper* _pDxWrap;									//	DirectX周りのクラス
	PMDActor* _pPmdAct;										//	PMDアクタークラス
	PMDActor* _pPmdAct2;
	PMDRenderer* _pPmdRender;								//	PMDレンダラークラス
};

#endif // !_H_DXAPPLICATION_
