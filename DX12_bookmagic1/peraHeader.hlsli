Texture2D<float4> tex : register(t0);	//	通常カラーテクスチャ
Texture2D<float4> texNormal : register(t1);	//	法線
Texture2D<float4> effectTex : register(t2);	//	ポストエフェクトテクスチャー

//	深度値検証用
Texture2D<float4> depthTex : register(t3);	//	深度値テクスチャー
Texture2D<float4> lightDepthTex : register(t4);	//	ライドデプステクスチャー

SamplerState smp : register(s0);			//	サンプラー

//	ポストエフェクト用定数バッファ
cbuffer PostEffect : register(b0)
{
	//	※配列はパッキングに含まれず、float bkweights[8]は実際にはfloat4 bkweights[8]のサイズ分確保されてしまう
	//float bkweights[8];
	float4 bkweights[2];
}

//	出力用変数
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};