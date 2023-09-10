#pragma once

//	インクルード
#include <DirectXTex.h>
#include <d3d12.h>
#include <wrl.h>

//#include <DirectXMath.h>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"DirectXTex.lib")

class Dx12Wrapper;
//	クラス宣言
class SceneInfo
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	構造体	//
	//	シェーダー側に渡すためお基本的な行列データ
	struct SceneMatrix
	{
		DirectX::XMMATRIX view;		//	ビュー行列
		DirectX::XMMATRIX proj;		//	プロジェクション行列
		DirectX::XMMATRIX invproj;	//	逆プロジェクション行列
		DirectX::XMMATRIX lightCamera;	//	ライトから見たビュー
		DirectX::XMMATRIX shadow;	//	影
		DirectX::XMFLOAT3 eye;		//	視点座標
	};
	//	関数	//
	/*	初期化関連の処理	*/
	//	コンストラクター
	SceneInfo(Dx12Wrapper* pWrap);

	//	初期化処理
	void Init(void);

	/*	描画関連の処理	*/
	//	シーンビューのセット命令
	void CommandSet_SceneView(UINT rootPramIdx = 0);

private:

	//	関数	//
	/*	初期化関連の処理	*/
	//	ビュー・プロジェクション行列バッファの作成
	void CreateViewProjectionView(void);

	//	変数	//
	Dx12Wrapper* _pWrap;
	ComPtr<ID3D12DescriptorHeap> _ScenevHeap = nullptr;	//	シーンディスクリプタ
	ComPtr<ID3D12Resource> _SceneBuffer = nullptr;		//	シーンバッファ
	SceneMatrix* _pMapSceneMtx;							//	シーン行列のマップ
	DirectX::XMFLOAT3 _parallelLightVec;				//	平行ライトの向き
	DirectX::XMFLOAT3 _eye;		//	視点
	DirectX::XMFLOAT3 _target;	//	注視点
	DirectX::XMFLOAT3 _up;		//	上ベクトル
	SIZE _windowSize;			//	ウィンドウサイズ
};