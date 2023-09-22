#pragma once
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

//#include <LLGI.Compiler.h>
//#include <LLGI.Graphics.h>
//#include <LLGI.Platform.h>
//#include <DX12/LLGI.CommandListDX12.h>
//#include <DX12/LLGI.GraphicsDX12.h>
//#include <Utils/LLGI.CommandListPool.h>
//#include "../Utils/Window.h"

#pragma comment(lib,"EffekseerRendererDX12.lib")
#pragma comment(lib,"Effekseer.lib")
#pragma comment(lib,"LLGI.lib")
#pragma comment(lib,"EffekseerRendererCommon.lib")
#pragma comment(lib,"EffekseerRendererLLGI.lib")

class Dx12Wrapper;
class EffectEffekseer
{
public:
	EffectEffekseer(Dx12Wrapper* pWrap);

	//	初期化処理
	void Init(void);
	//	描画処理
	void Draw(void);

	//	同期
	void Syncronize(void);
private:
	//	エフェクトレンダラー
	::EffekseerRenderer::RendererRef _efkRenderer;
	//	エフェクトマネージャー
	::Effekseer::RefPtr<::Effekseer::Manager> _efkManager = nullptr;

	/*	コマンドリストを使うライブラリに対応するためのもの	*/
	//	メモリプール
	::Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> _efkMemoryPool = nullptr;
	//	コマンドリスト
	::Effekseer::RefPtr<EffekseerRenderer::CommandList> _efkCmdlList = nullptr;

	/*	エフェクト再生に必要なもの	*/
	//	エフェクト本体
	::Effekseer::RefPtr<Effekseer::Effect> _effect = nullptr;
	//	エフェクトハンドル
	Effekseer::Handle _efkHandle;

	/*	他のクラス	*/
	Dx12Wrapper* _pWrap;
};