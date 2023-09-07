Texture2D<float4> tex : register(t0);	//	�ʏ�J���[�e�N�X�`��
Texture2D<float4> texNormal : register(t1);	//	�@��
Texture2D<float4> highLumTex : register(t2);	//	���P�x
Texture2D<float4> shrinkHightLumTex : register(t3);	//	�k�����P�x
Texture2D<float4> shrinkTex	: register(t4);			//	�k���ʏ�
Texture2D<float4> effectTex : register(t5);	//	�|�X�g�G�t�F�N�g�e�N�X�`���[

//	�[�x�l���ؗp
Texture2D<float> depthTex : register(t6);	//	�[�x�l�e�N�X�`���[
Texture2D<float4> lightDepthTex : register(t7);	//	���C�h�f�v�X�e�N�X�`���[
Texture2D<float> ssaoTex : register(t8);		//	ssao�e�N�X�`���[

SamplerState smp : register(s0);			//	�T���v���[

//	�|�X�g�G�t�F�N�g�p�萔�o�b�t�@
cbuffer PostEffect : register(b0)
{
	//	���z��̓p�b�L���O�Ɋ܂܂ꂸ�Afloat bkweights[8]�͎��ۂɂ�float4 bkweights[8]�̃T�C�Y���m�ۂ���Ă��܂�
	//float bkweights[8];
	float4 bkweights[2];
}

//	�o�͗p�ϐ�
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

//	�ڂ����o�͗p�ϐ�
struct BlurOutput
{
	float4 highLum : SV_Target0;	//	���P�x
	float4 col : SV_Target1;		//	�ʏ�̃����_�����O����
};