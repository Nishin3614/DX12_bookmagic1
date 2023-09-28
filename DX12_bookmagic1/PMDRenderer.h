#ifndef _H_PMDRENDERER_
#define _H_PMDRENDERER_

//	インクルード
#include <d3d12.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <memory>
#include <iostream>
#include "PMDActor.h"

#include <DirectXMath.h>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")

//	クラスの前方宣言
class Dx12Wrapper;
class SceneInfo;
class VisualEffect;
class PMDActor;
//	クラス宣言
class PMDRenderer
{
public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	//	構造体	//
	//	シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;	//	ディフューズ色
		float alpha;		//	ディフューズα
		DirectX::XMFLOAT3 specular;	//	スペキュラ色
		float specularity;	//	スペキュラの強さ（乗算値）
		DirectX::XMFLOAT3 ambient;	//	アンビエント色
	};
	//	それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath;	//	テクスチャファイルパス
		int toonIdx;			//	トゥーン番号
		bool edgeFlg;			//	マテリアルごとの輪郭線フラグ
	};
	//	全体をまとめるデータ
	struct Material
	{
		unsigned int indicesNum;	//	インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	//	関数	//
	// コンストラクタ
	PMDRenderer(Dx12Wrapper* pDxWap);
	//	初期化処理
	void Init(void);
	//	描画処理
	void Draw(SceneInfo* pScene,VisualEffect* pVef);
	//	シャドウマップ用描画処理
	void PreShadowDraw(void);
	//	モデル描画
	void PreModelDraw(void);

	//	解放処理
	void Release(void);
	//	更新処理
	void Update(void);

	//	生成処理
	PMDActor* Create(const char* modelpath, const char* vmdpath, DirectX::XMFLOAT3 position = { 0.0f,0.0f,0.0f });
private:
	//	関数	//
	//	ルートパラメータとルートシグネイチャの作成
	void CreateRootParameterOrRootSignature(void);
	//	グラフィックパイプラインの作成
	void CreateGraphicPipeline(void);
	
	//	シャドウマップ描画
	void DrawShadowMap(SceneInfo* pScene, VisualEffect* pVef);
	//	モデル描画
	void DrawModel(SceneInfo* pScene, VisualEffect* pVef);
	//	画面効果
	void DrawVisualEffect(VisualEffect* pVef);

	//	変数	//
	ComPtr < ID3D12PipelineState> _pipelinestate = nullptr;		//	グラフィックスパイプラインステート
	ComPtr < ID3D12RootSignature> _rootsignature = nullptr;		//	ルートシグネイチャ

	//	シャドウマップ用
	ComPtr<ID3D12PipelineState> _plsShadow = nullptr;			//	シャドウマップ用パイプライン

	//	他のクラスのインスタンス
	Dx12Wrapper* _pDxWap;
	std::vector<std::unique_ptr<PMDActor>> _pmdActor;
};

#endif // !_H_PMDRENDERER_
