Texture2D<float4> _tex : register(t0);
SamplerState _smp : register(s0);			//	�T���v���[

cbuffer cbuff0 : register(b0)
{
	matrix _mat;	//	�ϊ��s��
}

//	�o�͗p
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};