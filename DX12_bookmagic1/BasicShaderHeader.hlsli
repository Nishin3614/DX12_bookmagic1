//	�\����	//
//	���_�V�F�[�_�[����s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION;	//	�V�X�e���p���_���W(�r���[�}�g���b�N�X�����������ŏI�I�Ȉʒu�i���j�j
	float4 pos : POSITION;		//	���_���W�i���f���̈ʒu�i���j�j
	float4 normal : NORMAL0;	//	�@���x�N�g��
	float4 vnormal : NORMAL1;	//	�r���[�ԊҌ�̖@���x�N�g��
	float2 uv : TEXCOORD;		//	uv�l
	float3 ray : VECTOR;		//	�x�N�g��
	uint   instNo : SV_InstanceID;	//	�C���X�^���X�ԍ�
	float4 tpos : TPOS;				//	���C�g�r���[�ō��W�ϊ��������
};

Texture2D<float4> tex : register(t0);	//	0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph : register(t1);	//	1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2);	//	2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> toon : register(t3);	//	3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> lightDepthTex : register(t4);	//	�V���h�E�}�b�v�p���C�g�[�x�e�N�X�`��

SamplerState smp : register(s0);		//	0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState smpToon : register(s1);	//	1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerComparisonState shadowSmp : register(s2);	//	2�ԁF��r���ʂ��擾����

cbuffer SceneMatrix : register(b0)		//	0�ԃX���b�g�ɐݒ肳�ꂽ�萔�o�b�t�@0
{
	matrix view;	//	�r���[�s��
	matrix proj;	//	�r���[�v���W�F�N�V�����s��
	matrix lightCamera;	//	���C�g�r���[�v���W�F�N�V����
	matrix shadow;	//	�e
	float3 eye;		//	���_���W
}
cbuffer Transform : register(b1)	//	1�ԃX���b�g�ɐݒ肳�ꂽ�萔�o�b�t�@
{
	matrix world;		//	���[���h�ϊ��s��
	matrix bones[256];	//	�{�[���s��
}
cbuffer Material : register(b2)	//	�萔�o�b�t�@1�@�}�e���A���p
{
	float4 diffuse;		//	�f�B�t���[�Y�F
	float4 specular;	//	�X�y�L�����F
	float3 ambient;		//	�A���r�G���g
}

//	�s�N�Z���V�F�[�_�[�o�͗p
struct PixelOutput
{
	float4 col : SV_TARGET0;		//	�J���[�l���o��
	float4 normal : SV_TARGET1;		//	�@�����o��
};