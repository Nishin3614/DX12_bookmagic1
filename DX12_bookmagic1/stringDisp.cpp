#include "stringDisp.h"
#include "Dx12Wrapper.h"
#include "Win32Application.h"
#include "helper.h"

//	�R���X�g���N�^
StringDisp::StringDisp(Dx12Wrapper* pWrap) : _pWrap(pWrap)
{
}

//	����������
void StringDisp::Init(void)
{
	auto dev = _pWrap->GetDevice();
	//	GraphicsMemory�I�u�W�F�N�g�̏�����
	_gmemory = new DirectX::GraphicsMemory(dev.Get());
	//	SpriteBatch�I�u�W�F�N�g�̏�����
	DirectX::ResourceUploadBatch resUploadBatch(dev.Get());
	resUploadBatch.Begin();
	DirectX::RenderTargetState rtState(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_D32_FLOAT
	);
	DirectX::SpriteBatchPipelineStateDescription pd(rtState);
	_spriteBatch = new DirectX::SpriteBatch(dev.Get(),
		resUploadBatch,
		pd
	);
	//	SpriteFont�I�u�W�F�N�g�̏�����
	CreateDH();
	_spriteFont = new DirectX::SpriteFont(
		dev.Get(),
		resUploadBatch,
		L"font/meiryo.spritefont",
		_strDispDH->GetCPUDescriptorHandleForHeapStart(),
		_strDispDH->GetGPUDescriptorHandleForHeapStart()
	);
	//	�r���[�|�[�g�Z�b�g
	auto& WinApp = Win32Application::Instance();
	auto size = WinApp.GetWindowSize();
	D3D12_VIEWPORT vp = { 0.0f,0.0f,size.cx,size.cy,0.0f,1.0f };
	_spriteBatch->SetViewport(vp);
	auto future = resUploadBatch.End(_pWrap->GetCmdQue().Get());
	//	�҂�
	_pWrap->WaitForCommandQueue();
	future.wait();
}

//	�`�揈��
void StringDisp::Draw(void)
{
	_pWrap->GetCmdList()->SetDescriptorHeaps(1, _strDispDH.GetAddressOf());
	_spriteBatch->Begin(_pWrap->GetCmdList().Get());
	_spriteFont->DrawString(
		_spriteBatch,
		L"����ɂ��̓n���[",		//	�Ђ炪�ȁA�J�i�J�i�̍ۂ�L��t���邱�Ƃ�Y�ꂸ��
		DirectX::XMFLOAT2(102, 102),
		DirectX::Colors::Black
	);
	_spriteFont->DrawString(
		_spriteBatch,
		L"����ɂ���",
		DirectX::XMFLOAT2(100, 100),
		DirectX::Colors::Yellow
	);
	_spriteBatch->End();
}

//	������\���I��
void StringDisp::EndStrDisp(void)
{
	_gmemory->Commit(_pWrap->GetCmdQue().Get());
}

//	�I������
void StringDisp::Release(void)
{
	delete _gmemory;	//	�O���t�B�b�N�X�������I�u�W�F�N�g
	_gmemory = nullptr;
	delete _spriteFont;
	 _spriteFont = nullptr;		//	�t�H���g�\���p�I�u�W�F�N�g
	 delete _spriteBatch;
	_spriteBatch = nullptr;	//	�X�v���C�g�\���p�I�u�W�F�N�g
}

//	�f�B�X�N���v�^�q�[�v�쐬����
void StringDisp::CreateDH(void)
{
	//	�f�B�X�N���v�^�q�[�v�쐬
	auto dev = _pWrap->GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = dev->CreateDescriptorHeap(
		&desc, IID_PPV_ARGS(_strDispDH.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("strDisp�p�̃f�B�X�N���v�^�q�[�v�쐬���s\n");
	}
}
