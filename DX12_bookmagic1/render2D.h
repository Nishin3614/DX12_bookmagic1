#pragma once
//	インクルード
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <iostream>
#include "polygon2D.h"

//	ライブラリ
#pragma comment(lib,"d3d12.lib")

//	クラス
class Dx12Wrapper;
class Renderer2D
{
public:
	//	コンストラクタ
	Renderer2D(Dx12Wrapper* pWrap);
	//	初期化処理
	void Init(void);
	//	描画処理
	void Draw(void);
	//	オブジェクト作成
	Polygon2D* Create2D(std::string texName);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*	関数	*/
	//	ルートシグネイチャ作成
	void CreateRootSignature(void);
	//	パイプライン作成
	void CreatePipeline(void);

	/*	変数	*/
	ComPtr<ID3D12PipelineState> _pls = nullptr;		//	pipelineステート
	ComPtr<ID3D12RootSignature> _rs = nullptr;		//	ルートシグネイチャ

	//	他クラスのインスタンス
	Dx12Wrapper* _pWrap;
	std::vector<std::unique_ptr<Polygon2D>> _Polygons;				//	ポリゴン
};