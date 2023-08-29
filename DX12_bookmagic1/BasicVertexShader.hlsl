//	�C���N���[�h
#include "BasicShaderHeader.hlsli"

//	�ʏ�`��p
Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	min16uint edge : EDGE_FLAG,
	uint instNo : SV_InstanceID
)
{
	Output output;									//	�s�N�Z���V�F�[�_�[�ɓn���l
	float w = (float)weight / 100.0f;
	matrix bm = bones[boneno[0]] * w	//	�{�[��0�̃{�[���e���x
		+ bones[boneno[1]] * (1 - w);	//	�{�[��1�̃{�[���e���x
	pos = mul(bm, pos);
	pos = mul(world, pos);
	//	�C���X�^���X�ԍ���1�Ȃ�A�e�s����v�Z����
	if (instNo == 1)
	{
		pos = mul(shadow, pos);
	}
	output.pos = pos;
	pos = mul(mul(proj, view), pos);	//	�ʏ�J�������瓊�e�����s��
	//pos = mul(lightCamera, output.pos);	//	�ʏ�J�������瓊�e�����s��

	output.svpos = pos;
	output.instNo = instNo;							//	�C���X�^���X�ԍ�
	normal.w = 0;									//	���s�ړ������𖳌��ɂ���
	output.normal = mul(world, normal);				//	�@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);		//	�r���[�ϊ����ꂽ�@���x�N�g��
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);			//	�����x�N�g��
	output.tpos = mul(lightCamera, output.pos);
	return output;
}

//	�e�p���_���W�ϊ��i���W�ϊ��̂݁j
float4 ShadowVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	min16uint edge : EDGE_FLAG
) : SV_POSITION
{
	float fWeight = float(weight) / 100.0f;
	
	matrix conBone =
		bones[boneno[0]] * fWeight
		+ bones[boneno[1]] * (1 - fWeight);

	pos = mul(world, mul(conBone, pos));
	return mul(lightCamera, pos);

}