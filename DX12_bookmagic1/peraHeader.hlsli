Texture2D<float4> _tex : register(t0);	//	通常カラーテクスチャ
Texture2D<float4> _texNormal : register(t1);	//	法線
Texture2D<float4> _highLumTex : register(t2);	//	高輝度
Texture2D<float4> _shrinkHightLumTex : register(t3);	//	縮小高輝度
Texture2D<float4> _shrinkTex	: register(t4);			//	縮小通常
Texture2D<float4> _effectTex : register(t5);	//	ポストエフェクトテクスチャー

//	深度値検証用
Texture2D<float> _depthTex : register(t6);	//	深度値テクスチャー
Texture2D<float4> _lightDepthTex : register(t7);	//	ライドデプステクスチャー
Texture2D<float> _ssaoTex : register(t8);		//	ssaoテクスチャー

SamplerState _smp : register(s0);			//	サンプラー

//	ポストエフェクト用定数バッファ
cbuffer PostEffect : register(b0)
{
	//	※配列はパッキングに含まれず、float bkweights[8]は実際にはfloat4 bkweights[8]のサイズ分確保されてしまう
	//float bkweights[8];
	float4 _bkweights[2];
}
//	ポストセット
cbuffer PostSetting : register(b1)
{
	float4 _bloomColor;	//	ブルームカラー
	//	2個目以降、bool型で宣言すると、2個目の結果が1個目のboolに反映される不具合が発生する
	//	原因はあっているかはわからないが、おそらくパッキング・アライメント関連の不具合
	//	なので、今回はint型で宣言して、フラグの働きをするように修正する
	int _DebugDispFlag;	//	デバッグ表示フラグ
	int _SSAOFlag;			//	SSAO有効フラグ
	int _MonochroR;					//	モノクロフラグ
	int _MonochroG;					//	モノクロフラグ
	int _MonochroB;					//	モノクロフラグ
	int _nReverse;					//	反転フラグ
	int _nDof;						//	被写界深度フラグ
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