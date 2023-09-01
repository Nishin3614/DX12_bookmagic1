#include "peraHeader.hlsli"
//	通常描画
float4 Normal(Output input)
{
	return tex.Sample(smp,input.uv);
}

//	モノクロ化処理
float4 monokuro(float4 col)
{
	//	モノクロ化
	float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	return float4(Y, Y, Y, col.a);

}

//	色の反転処理
float4 Reverse(float4 col)
{
	return float4(1.0 - col.rgb, col.a);
}

//	色の階調を落とす処理
//	※この処理だけだと汚くなるので、ディザなどの技術も併用する必要がある
float4 ToneDown(float4 col)
{
	//	8ビットの階調を4ビットの階調へ落とす
	return float4(col.rgb - fmod(col.rgb,0.25f),col.a);
}

//	簡易なぼかし処理（画素の平均化）
float4 Blur(Output input)
{
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy));//	左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));		//	上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy));	//	右上
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));		//	左
	ret += tex.Sample(smp, input.uv);							//	自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0));		//	右
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy));	//	左下
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy));		//	下
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));	//	右下

	//	画素の平均値を返す
	return ret / 9.0f;
}

//	エンボス加工処理
float4 Emboss(Output input)
{
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;//	左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));		//	上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	右上

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));		//	左
	ret += tex.Sample(smp, input.uv);							//	自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	右

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	左下
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	下
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -2;	//	右下

	//	画素の平均値を返す
	return ret;
}

//	シャープネス（エッジの強調）
float4 Sharpness(Output input)
{
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;		//	上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	右上

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;		//	左
	ret += tex.Sample(smp, input.uv) * 5;							//	自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	右

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	左下
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	下
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	右下

	//	画素の平均値を返す
	return ret;
}

//	輪郭線抽出（簡単）
float4 OutLine(Output input)
{
	float4 col = tex.Sample(smp, input.uv);
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	左上
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;		//	上
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	右上

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;		//	左
	ret += tex.Sample(smp, input.uv) * 4;							//	自分
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	右

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	左下
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	下
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	右下

	//	モノクロ化
	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
	Y = pow(1.0f - Y, 10.0f);	//	エッジを強調する
	Y = step(0.2, Y);			//	余分なエッジを消す
	//	画素の平均値を返す
	return float4(Y,Y,Y, col.a);
}

//	細かいぼかし処理
float4 DetailBlur(Texture2D<float4> destTex, SamplerState destSmp,float2 uv,float dx,float dy)
{
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);

	//	最上段
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 2 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 2 * dy)) * 1 / 256;

	//	1つ上段
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 1 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 1 * dy)) * 4 / 256;

	//	中段
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 0 * dy)) * 36 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 0 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 0 * dy)) * 6 / 256;

	//	1つ下段
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, -1 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, -1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, -1 * dy)) * 4 / 256;

	//	最下段
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, -2 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, -2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, -2 * dy)) * 1 / 256;


	//	画素の平均値を返す
	return ret;
}

//	ガウシアンぼかし処理
float4 GaussianBlur(Output input)
{
	float4 col = tex.Sample(smp, input.uv);
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dx = 1.0f / w;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);
	//ret += bkweights[0] * col;

	//	0~7の合計値
	for (int i = 0; i < 8; ++i)
	{
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(i * dx, 0));
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(-i * dx, 0));
	}

	//	x値だけのガウシアンぼかし時の値を返す
	return float4(ret.rgb,col.a);
}

//	ペラポリゴン用のピクセルシェーダー	//
float4 ps(Output input) : SV_Target
{
	//	深度出力
	if (input.uv.x < 0.2f && input.uv.y < 0.2f)
	{
		float dep = depthTex.Sample(smp, input.uv * 5);
		dep = 1.0f - pow(dep, 30);
		return float4(dep, dep, dep, 1.0f);
	}
	//	ライトからの深度出力
	else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
	{
		float lightdep = lightDepthTex.Sample(smp, (input.uv - float2(0.0f,0.2f)) * 5);
		lightdep = 1.0f - lightdep;
		return float4(lightdep, lightdep, lightdep, 1.0f);
	}
	//	法線出力
	else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
	{
		return texNormal.Sample(smp,(input.uv - float2(0,0.4f)) * 5);
	}
	//	高輝度出力
	else if (input.uv.x < 0.2f && input.uv.y < 0.8f)
	{
		return highLumTex.Sample(smp, (input.uv - float2(0, 0.6f)) * 5);
	}
	//	高輝度縮小バッファ出力
	else if (input.uv.x < 0.2f && input.uv.y < 1.0f)
	{
		return ShrinkHightLumTex.Sample(smp, (input.uv - float2(0, 0.8f)) * 5);
	}
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);
	//	画素間
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//	縮小高輝度の計算
	float4 bloomAccum = float4(0, 0, 0, 0);
	float2 uvSize = float2(1.0f, 0.5f);
	float2 uvOfst = float2(0, 0);
	for (int i = 0; i < 8; i++)
	{
		bloomAccum += DetailBlur(
			ShrinkHightLumTex, smp, input.uv * uvSize + uvOfst, dx, dy
		);
		uvOfst.y += uvSize.y;
		uvSize *= 0.5f;
	}

	//	通常描画
	return tex.Sample(smp, input.uv)
		+ DetailBlur(highLumTex, smp, input.uv, dx, dy)	//	1枚目の高輝度テクスチャをぼかす
		+ saturate(bloomAccum)							//	縮小ぼかし済み
		;
	//	モノクロ化
	return GaussianBlur(input);


	//	ライトデプス検証用
}

//	ぼかしのピクセルシェーダー	//
float4 VerticalBokehPS(Output input) : SV_Target
{
	//	通常描画
	return tex.Sample(smp, input.uv);

	/*
	float2 normalTex = effectTex.Sample(smp,input.uv).xy;
	//	RGB→法線ベクトルの変換
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTexの範囲は-1~1だが、幅1がテクスチャ1枚の大きさであり、
	//	歪みすぎるため0.1を乗算する
	return tex.Sample(smp, input.uv + normalTex * 0.1f);
	*/

	//return tex.Sample(smp, input.uv);
	float4 col = tex.Sample(smp, input.uv);
	//	テクスチャのサイズ情報取得
	float w, h, level;
	tex.GetDimensions(
		0,			//	ミップレベル
		w,			//	幅
		h, 			//	高さ
		level		//	ミップマップのレベル数
	);

	//	画素間
	float dy = 1.0f / h;
	//	近傍テーブルの合計値
	float4 ret = float4(0, 0, 0, 0);

	//	x方向の合成？
	//	※なぜbkweights[0]（0~3)までをcolでかけているのか？
	//ret += bkweights[0] * col;
	//ret += bkweights[1] * col;

	//	0~7の合計値
	for (int i = 0; i < 8; ++i)
	{
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(0, dy * i));
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(0, -dy * i));
	}

	//	y値だけのガウシアンぼかし時の値を返す
	return float4(ret.rgb,col.a);
}

//	メインテクスチャを詳細ぼかしピクセルシェーダー
float4 BlurPS(Output input) : SV_Target
{
	float w,h,miplevels;
	tex.GetDimensions(0, w, h, miplevels);
	return DetailBlur(tex, smp, input.uv, 1.0f / w, 1.0f / h);
}

//	ポストエフェクトPS
float4 PostEffectPS(Output input) : SV_Target
{
	return tex.Sample(smp, input.uv);

	float2 normalTex = effectTex.Sample(smp,input.uv).xy;
	//	RGB→法線ベクトルの変換
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTexの範囲は-1~1だが、幅1がテクスチャ1枚の大きさであり、
	//	歪みすぎるため0.1を乗算する
	return tex.Sample(smp, input.uv + normalTex * 0.1f);
}
