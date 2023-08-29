//	�C���N���[�h
#include "Win32Application.h"
#include "DXApplication.h"

//	�萔�錾
const unsigned int window_width = 1280;
const unsigned int window_height = 780;

//	�C���X�^���X���擾
Win32Application& Win32Application::Instance(void)
{
	static Win32Application instance;
	return instance;
}

//	�R���X�g���N�^
Win32Application::Win32Application()
{

}

//	�A�v���P�[�V�������s����
void Win32Application::Run(HINSTANCE hInstance)
{
	//	�E�B���h�E�N���X�̐���&�o�^
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	//	�R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");			//	�A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);		//	�n���h���̎擾

	RegisterClassEx(&w);	//	�A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc = { 0,0,window_width,window_height };	//	�E�B���h�E�T�C�Y�����߂�

	//	�֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//	�E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(
		w.lpszClassName,		//	�N���X���w��
		_T("DX12�e�X�g"),		//	�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	//	�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			//	�\��x���W��OS�ɂ��܂���
		CW_USEDEFAULT,			//	�\��y���W��OS�ɂ��܂���
		wrc.right - wrc.left,	//	�E�B���h�E��
		wrc.bottom - wrc.top,	//	�E�B���h�E��
		nullptr,				//	�e�E�B���h�E�n���h��
		nullptr,				//	���j���[�n���h��
		w.hInstance,			//	�Ăяo���A�v���P�[�V�����n���h��
		nullptr);				//	�ǉ��p�����[�^�[	

	//	�E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	//	�f�o�C�X�̏����������@
	//	���f�o�C�X�̍쐬�Ɏ��s�����ہA�A�v���P�[�V�������I�������鏈�����܂��쐬����Ă��Ȃ�
	DXApplication& DxApi = DXApplication::Instance();
	DxApi.OnInit(hwnd,window_width,window_height);

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//	�X�V����
			DxApi.OnUpdate();
			//	�`�揈��
			DxApi.OnRender();
		}
		//	�A�v���P�[�V�������I���Ƃ���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	DxApi.OnRelease();
}

//	�E�B���h�E�T�C�Y�̎擾
SIZE Win32Application::GetWindowSize(void)
{
	SIZE rec;
	rec.cx = window_width;
	rec.cy = window_height;
	return rec;
}


LRESULT Win32Application::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//	�E�B���h�E���j�����ꂽ��Ă΂�鏈��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);	//	OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//	����̏������s��
}
