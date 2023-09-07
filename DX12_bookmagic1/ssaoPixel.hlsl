#include "ssaoHeader.hlsli"
//	���݂�uv�l�����ɗ�����Ԃ�����
float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

//	�y���|���S���p�̃s�N�Z���V�F�[�_�[	//
float ps(Output input) : SV_Target
{
	float dp = depthTex.Sample(smp,input.uv);	//	���݂�uv�̐[�x
	float w, h, miplevel;

	//	�[�x�p�e�N�X�`���̏��擾
	depthTex.GetDimensions(0, w, h, miplevel);
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//	SSAO
	//	���̍��W�𕜌�����
	float4 respos = mul(
		invproj,
		float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1)
	);
	respos.xyz = respos.xyz / respos.w;

	float div = 0.0f;	//	�Օ����l���Ȃ����ʂ̍��v
	float ao = 0.0f;	//	�A���r�G���g�I�N���[�W����
	float3 norm = normalize((normaltex.Sample(smp, input.uv).xyz * 2) - 1);	//	�@���x�N�g��
	const int trycnt = 256;		//	���s��
	const float radius = 0.5f;	//	���a

	if (dp < 1.0f)
	{
		for (int i = 0; i < trycnt; i++)
		{
			float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
			float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
			float rnd3 = random(float2(rnd1,rnd2)) * 2 - 1;
			float3 omega = normalize(float3(rnd1, rnd2, rnd3));
			omega = normalize(omega);

			//	�����̌��ʖ@���̔��Α��Ɍ����Ă����甽�]����
			float dt = dot(norm, omega);
			float sgn = sign(dt);
			omega *= sgn;

			//	���ʂ̍��W���Ăюˉe�ϊ�����
			float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
			rpos.xyz /= rpos.w;
			dt *= sgn;	//	���̒l�ɂ���cos�Ƃ𓾂�
			div += dt;	//	�Օ����l���Ȃ����ʂ����Z����

			//	�v�Z���ʂ����݂̏ꏊ�̐[�x��艜�ɓ����Ă���Ȃ�
			//	�Օ�����Ă���̂ŉ��Z
			ao += step(
				depthTex.Sample(smp, (rpos.xy + float2(1, -1))
					* float2(0.5f, -0.5f)), rpos.z) * dt;

		}
		ao /= div;
	}
	return 1.0f - ao;
}