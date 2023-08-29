//	構造体	//
//	頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;	//	システム用頂点座標(ビューマトリックスを加味した最終的な位置（仮））
	float4 pos : POSITION;		//	頂点座標（モデルの位置（仮））
	float4 normal : NORMAL0;	//	法線ベクトル
	float4 vnormal : NORMAL1;	//	ビュー返還後の法線ベクトル
	float2 uv : TEXCOORD;		//	uv値
	float3 ray : VECTOR;		//	ベクトル
	uint   instNo : SV_InstanceID;	//	インスタンス番号
	float4 tpos : TPOS;				//	ライトビューで座標変換した情報
};

Texture2D<float4> tex : register(t0);	//	0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);	//	1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2);	//	2番スロットに設定されたテクスチャ
Texture2D<float4> toon : register(t3);	//	3番スロットに設定されたテクスチャ
Texture2D<float4> lightDepthTex : register(t4);	//	シャドウマップ用ライト深度テクスチャ

SamplerState smp : register(s0);		//	0番スロットに設定されたサンプラー
SamplerState smpToon : register(s1);	//	1番スロットに設定されたサンプラー
SamplerComparisonState shadowSmp : register(s2);	//	2番：比較結果を取得する

cbuffer SceneMatrix : register(b0)		//	0番スロットに設定された定数バッファ0
{
	matrix view;	//	ビュー行列
	matrix proj;	//	ビュープロジェクション行列
	matrix lightCamera;	//	ライトビュープロジェクション
	matrix shadow;	//	影
	float3 eye;		//	視点座標
}
cbuffer Transform : register(b1)	//	1番スロットに設定された定数バッファ
{
	matrix world;		//	ワールド変換行列
	matrix bones[256];	//	ボーン行列
}
cbuffer Material : register(b2)	//	定数バッファ1　マテリアル用
{
	float4 diffuse;		//	ディフューズ色
	float4 specular;	//	スペキュラ色
	float3 ambient;		//	アンビエント
}

//	ピクセルシェーダー出力用
struct PixelOutput
{
	float4 col : SV_TARGET0;		//	カラー値を出力
	float4 normal : SV_TARGET1;		//	法線を出力
};