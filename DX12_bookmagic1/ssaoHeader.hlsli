Texture2D<float4> normaltex : register(t1);	//	�@���e�N�X�`��
Texture2D<float> depthTex : register(t5);	//	�[�x�l�e�N�X�`���[

SamplerState smp : register(s0);			//	�T���v���[

cbuffer SceneMatrix : register(b0)		//	0�ԃX���b�g�ɐݒ肳�ꂽ�萔�o�b�t�@0
{
	matrix view;	//	�r���[�s��
	matrix proj;	//	�v���W�F�N�V�����s��
	matrix invproj;	//	�t�v���W�F�N�V�����s��
	matrix lightCamera;	//	���C�g�r���[�v���W�F�N�V����
	matrix shadow;	//	�e
	float4 lightVec;	//	�����x�N�g��
	float3 eye;		//	���_���W
	bool bSelfShadow;	//	�V���h�E�}�b�v�t���O
}

//	�o�͗p�ϐ�
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};
