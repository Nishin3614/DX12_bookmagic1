Texture2D<float4> normaltex : register(t1);	//	法線テクスチャ
Texture2D<float> depthTex : register(t6);	//	深度値テクスチャー

SamplerState smp : register(s0);			//	サンプラー

cbuffer SceneMatrix : register(b1)		//	0番スロットに設定された定数バッファ0
{
	matrix view;	//	ビュー行列
	matrix proj;	//	プロジェクション行列
	matrix invproj;	//	逆プロジェクション行列
	matrix lightCamera;	//	ライトビュープロジェクション
	matrix shadow;	//	影
	float3 eye;		//	視点座標
}

//	出力用変数
struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};
