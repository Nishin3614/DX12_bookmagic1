#ifndef _H_WIN32APPLICATION_
#define _H_WIN32APPLICATION_

//	�C���N���[�h
#include <Windows.h>
#include <tchar.h>

//	�N���X
class Win32Application
{
public:
	//	Application�̃V���O���g���C���X�^���X�𓾂�
	static Win32Application& Instance(void);

	//	�A�v���P�[�V�������s����
	void Run(HINSTANCE hInstance);

	//	�E�B���h�E�̃T�C�Y���擾
	SIZE GetWindowSize(void);

private:	
	// �V���O���g���̂��߁A�R���X�g���N�^��private��
	// ����ɃR�s�[�Ƒ�����֎~����
	Win32Application();
	Win32Application(const Win32Application&) = delete;
	void operator=(const Win32Application&) = delete;


	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

#endif