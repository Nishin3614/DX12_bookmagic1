Texture2D<float4> tex : register(t0);	//	通常カラーテクスチャ
Texture2D<float4> texNormal : register(t1);	//	法線
Texture2D<float4> highLumTex : register(t2);	//	高輝度
Texture2D<float4> shrinkHightLumTex : register(t3);	//	縮小高輝度
Texture2D<float4> shrinkTex	: register(t4);			//	縮小通常
Texture2D<float4> effectTex : register(t5);	//	ポストエフェクトテクスチャー

//	深度値検証用
Texture2D<float> depthTex : register(t6);	//	深度値テクスチャー
Texture2D<float4> lightDepthTex : register(t7);	//	ライドデプステクスチャー
Texture2D<float> ssaoTex : register(t8);		//	ssaoテクスチャー

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

//	ぼかし出力用変数
struct BlurOutput
{
	float4 highLum : SV_Target0;	//	高輝度
	float4 col : SV_Target1;		//	通常のレンダリング結果
};