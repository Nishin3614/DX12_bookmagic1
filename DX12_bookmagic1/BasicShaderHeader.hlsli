Texture2D<float4> _tex : register(t0);	//	0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> _sph : register(t1);	//	1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> _spa : register(t2);	//	2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> _toon : register(t3);	//	3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> _lightDepthTex : register(t4);	//	�V���h�E�}�b�v�p���C�g�[�x�e�N�X�`��

SamplerState _smp : register(s0);		//	0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerState _smpToon : register(s1);	//	1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[
SamplerComparisonState _shadowSmp : register(s2);	//	2�ԁF��r���ʂ��擾����

cbuffer SceneMatrix : register(b0)		//	0�ԃX���b�g�ɐݒ肳�ꂽ�萔�o�b�t�@0
{
	matrix _view;	//	�r���[�s��
	matrix _proj;	//	�v���W�F�N�V�����s��
	matrix _invproj;	//	�t�v���W�F�N�V�����s��
	matrix _lightCamera;	//	���C�g�r���[�v���W�F�N�V����
	matrix _shadow;	//	�e
	float4 _lightVec;	//	�����x�N�g��
	float3 _eye;		//	���_���W
	bool _bSelfShadow;	//	�V���h�E�}�b�v�t���O
}
cbuffer Transform : register(b1)	//	1�ԃX���b�g�ɐݒ肳�ꂽ�萔�o�b�t�@
{
	matrix _world;		//	���[���h�ϊ��s��
	matrix _bones[256];	//	�{�[���s��
}
cbuffer Material : register(b2)	//	�萔�o�b�t�@1�@�}�e���A���p
{
	float4 _diffuse;		//	�f�B�t���[�Y�F
	float4 _specular;	//	�X�y�L�����F
	float3 _ambient;		//	�A���r�G���g
}

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

//	�s�N�Z���V�F�[�_�[�o�͗p
struct PixelOutput
{
	float4 col : SV_TARGET0;		//	�J���[�l���o��
	float4 normal : SV_TARGET1;		//	�@�����o��
	float4 highLum : SV_TARGET2;	//	���P�x
};