//	�C���N���[�h�K�[�h
#pragma once
#include <d3d12.h>
#include <vector>
#include <string>

namespace Helper
{
	//	�R�}���h�v�����v�g�Ƀf�o�b�O�����o�͂���
	void DebugOutputFormatString(const char* format, ...);
	//	�V�F�[�_�[�G���[���o����
	void DebugShaderError(HRESULT result, ID3DBlob* errorBlob);

	//	�K�E�V�A���E�F�C�g�v�Z����
	std::vector<float> GetGaussianWeights(
		size_t count,	//	�v�f��
		float s			//	���U�l
	);

	//	�t�@�C��������g���q���擾����
	//	@param path �Ώۂ̃p�X������
	//	@return �g���q
	std::string GetExtension(const std::string& path);

	//	std::string�i�}���`�o�C�g������j����std::wstring�i���C�h������j�𓾂鏈��
	//	@param str �}���`�o�C�g������
	//	@return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromString(const std::string& str);

	//	�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
	//	@param size			���̑傫��
	//	@param alignment	�A���C�����g�T�C�Y
	//	@return				�A���C�����g�����낦���T�C�Y
	size_t AlignmentedSize(size_t size, size_t alignment);

	//	���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂鏈��
	//	@param modelPath �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	//	@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	//	@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(
		const std::string& modelPath,	//	���f���p�X
		const char* texPath				//	�e�N�X�`���[�p�X
	);
	//	�e�N�X�`���̃p�X���Z�p���[�^�[�����ŕ�������
	//	@param path �Ώۂ̃p�X������
	//	@param splitter ��؂蕶��
	//	@return �����O��̕�����y�A
	std::pair<std::string, std::string> SplitFileName(
		const std::string& path, const char splitter = '*');

}