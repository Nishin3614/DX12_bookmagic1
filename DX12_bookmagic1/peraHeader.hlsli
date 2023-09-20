Texture2D<float4> _tex : register(t0);	//	�ʏ�J���[�e�N�X�`��
Texture2D<float4> _texNormal : register(t1);	//	�@��
Texture2D<float4> _highLumTex : register(t2);	//	���P�x
Texture2D<float4> _shrinkHightLumTex : register(t3);	//	�k�����P�x
Texture2D<float4> _shrinkTex	: register(t4);			//	�k���ʏ�
Texture2D<float4> _effectTex : register(t5);	//	�|�X�g�G�t�F�N�g�e�N�X�`���[

//	�[�x�l���ؗp
Texture2D<float> _depthTex : register(t6);	//	�[�x�l�e�N�X�`���[
Texture2D<float4> _lightDepthTex : register(t7);	//	���C�h�f�v�X�e�N�X�`���[
Texture2D<float> _ssaoTex : register(t8);		//	ssao�e�N�X�`���[

SamplerState _smp : register(s0);			//	�T���v���[

//	�|�X�g�G�t�F�N�g�p�萔�o�b�t�@
cbuffer PostEffect : register(b0)
{
	//	���z��̓p�b�L���O�Ɋ܂܂ꂸ�Afloat bkweights[8]�͎��ۂɂ�float4 bkweights[8]�̃T�C�Y���m�ۂ���Ă��܂�
	//float bkweights[8];
	float4 _bkweights[2];
}
//	�|�X�g�Z�b�g
cbuffer PostSetting : register(b1)
{
	float4 _bloomColor;	//	�u���[���J���[
	//	2�ڈȍ~�Abool�^�Ő錾����ƁA2�ڂ̌��ʂ�1�ڂ�bool�ɔ��f�����s�����������
	//	�����͂����Ă��邩�͂킩��Ȃ����A�����炭�p�b�L���O�E�A���C�����g�֘A�̕s�
	//	�Ȃ̂ŁA�����int�^�Ő錾���āA�t���O�̓���������悤�ɏC������
	int _DebugDispFlag;	//	�f�o�b�O�\���t���O
	int _SSAOFlag;			//	SSAO�L���t���O
	int _MonochroR;					//	���m�N���t���O
	int _MonochroG;					//	���m�N���t���O
	int _MonochroB;					//	���m�N���t���O
	int _nReverse;					//	���]�t���O
	int _nDof;						//	��ʊE�[�x�t���O
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