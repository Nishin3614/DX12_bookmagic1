#include "helper.h"
#include <cstdarg>
#include <assert.h>
#include <string>


#ifdef _DEBUG
#include <iostream>
#endif

//	�R�}���h�v�����v�g�Ƀf�o�b�O�����o�͂���
void Helper::DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//	�V�F�[�_�[�G���[���o����
void Helper::DebugShaderError(HRESULT result, ID3DBlob* errorBlob)
{
	//	�V�F�[�_�[�G���[�m�F����
	if (FAILED(result))
	{
		//	�t�@�C������������Ȃ��Ƃ�
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			//	�o�̓E�B���h�E�̃f�o�b�O�ɕ\�������
			::OutputDebugStringA("�t�@�C������������܂���");
			return;
		}
		//	����ȊO
		else
		{
			std::string errstr;	//	�G���[�����p
			errstr.resize(errorBlob->GetBufferSize());	//	�K�v�T�C�Y�m��
			//	�f�[�^�R�s�[
			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin()
			);
			errstr += "\n";
			//	�G���[���̃f�[�^�\��
			::OutputDebugStringA(errstr.c_str());
		}
	}
}

//	�K�E�V�A���E�F�C�g�v�Z����
std::vector<float> Helper::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);	//	�E�F�C�g�z��ԋp�p
	float x = 0.0f;
	float total = 0.0f;

	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));	//	�l�C�s�A����x��
		total += wgt;
		x += 1.0f;
	}

	//	���ۂ͍��E�Ώ̂Ȃ̂ŁA���v�l��2�{
	//	�܂��A���S0�̃s�N�Z�����d�����邽�߁A1(e^0)�������Ă��܂����킹��
	total = total * 2.0f - 1;
	//float one = 0.0f;
	//	-7�`7�̃E�F�C�g�l�����ׂđ�����1�ɂȂ�悤�ɂ���
	for (auto& wgt : weights)
	{
		wgt /= total;
		//one += wgt;
	}
	/*	�E�F�C�g�l��������1�ɂȂ��Ă��邩�m���߂悤
	one *= 2;
	one -= weights[0];
	Debugoutput::DebugOutputFormatString("%f\n",one);
	*/

	return weights;
}

std::string Helper::GetExtension(const std::string& path)
{
	size_t idx = path.rfind('.');
	if (idx == std::string::npos)
	{
		return "";
	}
	else
	{
		return path.substr(
			idx + 1, path.length() - idx - 1);
	}
}

std::wstring Helper::GetWideStringFromString(const std::string& str)
{
	//	�Ăяo��1��ځi�����񐔂𓾂�j
	auto num1 = MultiByteToWideChar(
		CP_ACP,									//	Windows ANSI�i���{�ł���Shift JIS�j
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,	//	�\���ςݕ����i��j�u�ρv����̕�����Ƃ��Ă݂�j | �����ȕ��������m
		str.c_str(),							//	�}���`�o�C�g������̃A�h���X
		-1,										//	�}���`�o�C�g������̃T�C�Y�i-1�̏ꍇ�����Œ������v�Z���Ă����j
		nullptr,								//	���C�h������̃A�h���X
		0										//	�o�b�t�@�̃T�C�Y�i0���w�肷��ƁA���C�h������ɕϊ����邽�߂̕K�v�ȃo�b�t�@�̃T�C�Y��Ԃ��j
	);

	std::wstring wstr;	//	string��wchar_t��
	wstr.resize(num1);	//	1��ڂœ��������񐔂��m��

	//	�Ăяo��2��ځi�m�ۍς݂�wstr�ɕϊ���������R�s�[�j
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1
	);

	//	�擾�����ϊ�������̃o�b�t�@�T�C�Y�͐��������m�F
	assert(num1 == num2);
	return wstr;
}

//	�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//	@param size			���̑傫��
//	@param alignment	�A���C�����g�T�C�Y
//	@return				�A���C�����g�����낦���T�C�Y
size_t Helper::AlignmentedSize(size_t size, size_t alignment)
{
	//	�A���C�����g�ʂ�̃T�C�Y�Ȃ�A���̂܂܃T�C�Y��Ԃ�
	if (size % alignment == 0)
	{
		return size;
	}
	return size + alignment - size % alignment;
}

//	���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂鏈��
//	@param modelPath �A�v���P�[�V�������猩���e�N�X�`���̃p�X
//	@param texPath PMD���f�����猩���e�N�X�`���̃p�X
//	@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
std::string Helper::GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath)
{
	//	/,\\�̂ǂ���Őݒ肳��Ă��邩�킩��Ȃ����߁A���ʂ��鏈��
	//	rfind�Ō�����Ȃ��ꍇ�Aepos(-1��0xffffffff�j��Ԃ�
	size_t pathIndex1 = modelPath.rfind('/');
	size_t pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = min(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);	//	�t�H���_�������擾
	return folderPath + texPath;
}

//	�e�N�X�`���̃p�X���Z�p���[�^�[�����ŕ�������
//	@param path �Ώۂ̃p�X������
//	@param splitter ��؂蕶��
//	@return �����O��̕�����y�A
std::pair<std::string, std::string> Helper::SplitFileName(const std::string& path, const char splitter)
{
	size_t idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(
		idx + 1, path.length() - idx - 1);
	return ret;
}
