#pragma once
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

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
	//	�G�t�F�N�g�Đ�
	void Play(void);
	//	�G�t�F�N�g��~
	void Stop(void);
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

	::Effekseer::RefPtr<Effekseer::Effect> _effect2 = nullptr;
	//	�G�t�F�N�g�n���h��
	Effekseer::Handle _efkHandle2;


	/*	���̃N���X	*/
	Dx12Wrapper* _pWrap;
};