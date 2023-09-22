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

	//	����������
	void Init(void);
	//	�`�揈��
	void Draw(void);

	//	����
	void Syncronize(void);
private:
	//	�G�t�F�N�g�����_���[
	::EffekseerRenderer::RendererRef _efkRenderer;
	//	�G�t�F�N�g�}�l�[�W���[
	::Effekseer::RefPtr<::Effekseer::Manager> _efkManager = nullptr;

	/*	�R�}���h���X�g���g�����C�u�����ɑΉ����邽�߂̂���	*/
	//	�������v�[��
	::Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> _efkMemoryPool = nullptr;
	//	�R�}���h���X�g
	::Effekseer::RefPtr<EffekseerRenderer::CommandList> _efkCmdlList = nullptr;

	/*	�G�t�F�N�g�Đ��ɕK�v�Ȃ���	*/
	//	�G�t�F�N�g�{��
	::Effekseer::RefPtr<Effekseer::Effect> _effect = nullptr;
	//	�G�t�F�N�g�n���h��
	Effekseer::Handle _efkHandle;

	/*	���̃N���X	*/
	Dx12Wrapper* _pWrap;
};