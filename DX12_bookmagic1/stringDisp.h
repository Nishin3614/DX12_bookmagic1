#pragma once

#include <SpriteFont.h>				//	文字列を表示するため
#include <ResourceUploadBatch.h>	//	DirectXTK関連のリソースを使用するため
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib,"DirectXTK12.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")

class Dx12Wrapper;
class StringDisp
{
public:
	//	コンストラクタ
	StringDisp(Dx12Wrapper* pWrap);
	//	初期化処理
	void Init(void);
	//	描画処理
	void Draw(void);
	//	文字列表示終了
	void EndStrDisp(void);
	//	終了処理
	void Release(void);

private:
	/*	関数	*/
	//	ディスクリプタヒープ作成
	void CreateDH(void);

	/*	変数	*/
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<ID3D12DescriptorHeap> _strDispDH;			//	DH
	DirectX::GraphicsMemory* _gmemory = nullptr;	//	グラフィックスメモリオブジェクト
	DirectX::SpriteFont* _spriteFont = nullptr;		//	フォント表示用オブジェクト
	DirectX::SpriteBatch* _spriteBatch = nullptr;	//	スプライト表示用オブジェクト

	//	ほかのクラスのインスタンス
	Dx12Wrapper* _pWrap;
};