#pragma once
//	インクルード
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")

//	クラス
class Dx12Wrapper;
class Polygon2D
{
public:
	//	コンストラクタ
	Polygon2D(Dx12Wrapper* pWrap,std::string texName);
	//	初期化処理
	void Init(void);
	//	描画処理
	void Draw(void);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	/*	関数	*/
	//	頂点バッファ作成
	void CreateVertexBuffer(void);
	//	テクスチャバッファ作成
	void CreateTexBuffer(void);
	//	行列バッファ作成
	void CreateMatBaffer(void);

	/*	変数	*/
	ComPtr<ID3D12Resource> _vb = nullptr;			//	頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _vbv = {};				//	頂点バッファビュー
	ComPtr<ID3D12Resource> _texBuffer = nullptr;	//	テクスチャーバッファ
	ComPtr<ID3D12Resource> _constBuffer = nullptr;	//	定数バッファ
	ComPtr<ID3D12DescriptorHeap> _dh = nullptr;		//	ディスクリプタヒープ

	DirectX::XMFLOAT2 _pos = {};					//	位置
	std::string _texName;

	//	他クラスのインスタンス
	Dx12Wrapper* _pWrap;
};