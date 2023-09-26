Texture2D<float4> _tex : register(t0);
SamplerState _smp : register(s0);			//	サンプラー

cbuffer cbuff0 : register(b0)
{
	matrix _mat;	//	変換行列
}

//	出力用
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};