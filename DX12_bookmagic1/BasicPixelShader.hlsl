//	�C���N���[�h
#include "BasicShaderHeader.hlsli"

//	�G���g���[�|�C���g
PixelOutput BasicPS(Output input)
{
	PixelOutput output;
	
	//	�V���h�E�}�b�v�p
	float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5f, -0.5f);
	float shadowWeight = 1.0f;
	/*
	
	float depthFromLight = lightDepthTex.Sample(
		smp,
		shadowUV					//	uv
	);
	if (depthFromLight < posFromLightVP.z - 0.001f)
	{
		shadowWeight = 0.75f;
	}
	*/
	float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp,					//	��r�T���v���[
		shadowUV,					//	uv
		posFromLightVP.z - 0.005f	//	��r�Ώےl
	);
	shadowWeight = lerp(0.8f, 1.0f, depthFromLight);	//	0.0�ɂȂ����ہA0.5�ɂȂ�

	//	�e��`�悷��ꍇ
	if (input.instNo == 1)
	{
		output.col = float4(0.3f, 0.3f, 0.3f, 1.0f);
		output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
		output.normal.a = 1;
		output.highLum = 0.0f;
		return output;
	}
	//	���̌������x�N�g���i���݂͕��s�����j
	float3 light = normalize(float3(1,-1,1));	//	�E�����Ɍ������Ă����x�N�g��

	//	���C�g�̃J���[
	float3 lightColor = float3(1, 1, 1);


	//	�f�B�t���[�Y�P�x�v�Z
	float diffuseB = saturate(dot(-light, input.normal.xyz));
	float4 toonDiff = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//	���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	//	�X�y�L�����[�̋P�x
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	//	�X�t�B�A�}�b�v
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5f, -0.5f);
	
	//	�e�N�X�`���J���[
	float4 texColor = tex.Sample(smp, input.uv);	//	�e�N�X�`���[�J���[	
	float4 sphColor = sph.Sample(smp, sphereMapUV);	//	�X�t�B�A�}�b�v�i��Z�j
	float4 spaColor = spa.Sample(smp, sphereMapUV);	//	�X�t�B�A�}�b�v�i���Z�j

	float4 col = max(
		toonDiff									//	�P�x�i�g�D�[���j
		* diffuse									//	�f�B�t���[�Y�J���[
		* texColor									//	�e�N�X�`���[�J���[
		* sphColor									//	�X�t�B�A�}�b�v�i��Z�j
		+ saturate(spaColor * texColor						//	�X�t�B�A�}�b�v�i���Z�j
			+ float4(specularB * specular.rgb, 1))		//	�X�y�L�����[
		, float4(ambient.rgb * texColor.rgb * sphColor.rgb, 1)	//	�A���r�G���g
		+ spaColor
	)
		;

	//	�ʏ탌���_�����O���ʁi�J���[�j
	output.col = float4(col.rgb  * shadowWeight, col.a);
	//	�@������
	output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
	output.normal.a = 1;
	//	���P�x����
	float y = dot(float3(0.299f, 0.587f, 0.114f), output.col.rgb);
	output.highLum = y > 0.99f ? output.col : 0.0f;
	output.highLum.a = 1;
	return output;
	/*
	return// max(
		diffuseB								//	�f�B�t���[�Y�P�x
		* diffuse									//	�f�B�t���[�Y�J���[
		* texColor									//	�e�N�X�`���[�J���[
		+ float4(specularB * specular.rgb, 1)		//	�X�y�L�����[
		//,float4(ambient * texColor, 1)			//	�A���r�G���g
		//)
		;
	return float4(brightness, brightness, brightness, 1)	//	�P�x
		* diffuse							//	�f�B�t���[�Y�F
		* color								//	�e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV)		//	sph(��Z�j
		+ spa.Sample(smp, sphereMapUV)		//	spa�i���Z�j
		+ float4(color * ambient, 1)		//	�A���r�G���g
		;
		*/
	//return float4(tex.Sample(smp,input.uv));
}