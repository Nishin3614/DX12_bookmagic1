#pragma once

//	インクルード
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <DirectXMath.h>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")

//	前方宣言
class Dx12Wrapper;
//	クラス宣言
class VisualEffect
{

public:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	構造体	//
	struct PostSetting
	{
		DirectX::XMFLOAT4 bloomColor;	//	ブルームカラー
		int nDebugDisp;				//	デバッグ表示フラグ
		int nSSAO;						//	SSAO表示フラグ
		int nMonochro[3];				//	モノクロフラグ
		int nReverse;					//	反転フラグ
		int nDof;						//	被写界深度フラグ
	};
	//	関数	//
	//	コンストラクタ
	VisualEffect(Dx12Wrapper* pWrap);
	/*	初期化関連の処理	*/
	//	初期化処理
	void Init(void);

	/*	描画関連の処理	*/
	//	最終描画
	void EndDraw(void);
	//	マルチパスレンダリング用描画1
	void PreOriginDraw(void);
	void EndOriginDraw(void);
	//	マルチパスレンダリング用描画2
	void ProceDraw(void);
	//	縮小バッファぼかし描画処理
	void DrawShrinkTextureForBlur(void);
	//	シャドウマップ描画
	void ShadowDraw(void);
	//	深度用SRVセット
	void DepthSRVSet(void);

	//	アンビエントオクルージョンによる描画
	void DrawAmbientOcculusion(void);

	//	ポスト設定処理
	void SetPostSetting(bool bDebugDisp,bool bSSAO,bool bMonochro[3],bool bReverse,bool bDof,float bloomColor[3]);

private:

	//	関数	//
	/*	初期化関連の処理	*/
	//	モデルなどの描画用のレンダーターゲットを作成
	void CreateOriginRenderTarget(void);
	//	加工用バッファ作成
	void CreateProcessRenderTarget(void);
	//	被写界深度用バッファ作成
	void CreateBlurForDOFBuffer(void);
	//	ペラポリゴンの頂点バッファ作成
	void CreatePeraVertexBuff(void);
	//	ぼけ定数バッファ作成
	void CreateBokeConstantBuff(void);
	//	ペラポリゴン用のルートシグネイチャ作成
	void CreatePeraRootSignature(void);
	//	ペラポリゴン用のパイプライン作成
	void CreatePeraGraphicPipeLine(void);
	//	エフェクト用のバッファとビュー作成
	bool CreateEffectBufferAndView(void);
	//	深度バッファの作成
	void CreateDepthView(void);
	//	ポスト設定用の作成
	void CreatePostSetting(void);

	//	アンビエントオクルージョンバッファの作成
	bool CreateAmbientOcculusionBuffer(void);
	//	アンビエントオクルージョンディスクリプタヒープ作成
	bool CreateAmbientOcculusionDescriptorHeap(void);

	//	変数	//
	// DX12の基礎情報
	Dx12Wrapper* _pWrap;
	//	モデルなどのレンダーターゲット先用の変数
	std::array<ComPtr<ID3D12Resource>,2> _origin1Resource;	//	貼りつけ元のリソース
											//	ペラポリゴンに張り付けるためのテクスチャリソース仮(この中に今まで作成してきたモデルやらが描画されている）
	ComPtr<ID3D12DescriptorHeap> _originRTVHeap;	//	レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> _originSRVHeap;	//	テクスチャ用

	//	ペラポリゴン作成用の変数
	ComPtr<ID3D12Resource> _prPoriVB;			//	ペラポリゴン頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _prPoriVBV;		//	ペラポリゴン頂点バッファビュー
	ComPtr<ID3D12RootSignature> _prPoriRS;	//	ペラポリゴン用ルートシグネイチャー
	ComPtr <ID3D12PipelineState> _prPoriPipeline;	//	ペラポリゴン用グラフィックパイプラインステート
	ComPtr<ID3D12Resource> _bokehParamBuffer;		//	ぼかし用の定数バッファ

	//	加工用のレンダーターゲット作成用の変数
	ComPtr<ID3D12Resource> _proceResource;	//	ペラポリゴンに張り付けられたリソースを加工するための変数
	ComPtr<ID3D12PipelineState> _procePipeline;	//	加工用グラフィックパイプライン

	//	歪みテクスチャー用
	ComPtr<ID3D12DescriptorHeap> _effectSRVHeap;
	ComPtr<ID3D12Resource> _efffectTexBuffer;
	ComPtr<ID3D12PipelineState> _effectPipeline;

	//	ブルーム用バッファ
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//	ブルーム用バッファ
	ComPtr<ID3D12PipelineState> _blurPipeline;			//	画面全体ぼかし用パイプライン

	//	被写界深度用バッファ
	ComPtr<ID3D12Resource> _dofBuffer;	//	被写界深度用ぼかしバッファ

	//	アンビエントオクルージョン用
	ComPtr<ID3D12Resource> _aoBuffer;
	ComPtr<ID3D12RootSignature> _aoRS;
	void CreateAoRootSignature(void);
	ComPtr<ID3D12PipelineState> _aoPipeline;
	ComPtr<ID3D12DescriptorHeap> _aoRTVDH;
	ComPtr<ID3D12DescriptorHeap> _aoSRVDH;

	//	深度用バッファ
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	深度バッファ用のディスクリプタヒープ
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	深度バッファ
	ComPtr<ID3D12Resource> _lightDepthBuffer = nullptr;
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;		//	深度値テクスチャー用ヒープ

	//	Imguiデバッグ機能
	ComPtr<ID3D12Resource> _postSettingResource;
	PostSetting* _pMapPostSetting;
	ComPtr<ID3D12DescriptorHeap> _postSettingDH;
};