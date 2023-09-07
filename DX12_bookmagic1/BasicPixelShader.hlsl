//	インクルード
#include "BasicShaderHeader.hlsli"

//	エントリーポイント
PixelOutput BasicPS(Output input)
{
	PixelOutput output;
	
	//	シャドウマップ用
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
		shadowSmp,					//	比較サンプラー
		shadowUV,					//	uv
		posFromLightVP.z - 0.005f	//	比較対象値
	);
	shadowWeight = lerp(0.8f, 1.0f, depthFromLight);	//	0.0になった際、0.5になる

	//	影を描画する場合
	if (input.instNo == 1)
	{
		output.col = float4(0.3f, 0.3f, 0.3f, 1.0f);
		output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
		output.normal.a = 1;
		output.highLum = 0.0f;
		return output;
	}
	//	光の向かうベクトル（現在は平行光線）
	float3 light = normalize(float3(1,-1,1));	//	右下奥に向かっていくベクトル

	//	ライトのカラー
	float3 lightColor = float3(1, 1, 1);


	//	ディフューズ輝度計算
	float diffuseB = saturate(dot(-light, input.normal.xyz));
	float4 toonDiff = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//	光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	//	スペキュラーの輝度
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	//	スフィアマップ
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5f, -0.5f);
	
	//	テクスチャカラー
	float4 texColor = tex.Sample(smp, input.uv);	//	テクスチャーカラー	
	float4 sphColor = sph.Sample(smp, sphereMapUV);	//	スフィアマップ（乗算）
	float4 spaColor = spa.Sample(smp, sphereMapUV);	//	スフィアマップ（加算）

	float4 col = max(
		toonDiff									//	輝度（トゥーン）
		* diffuse									//	ディフューズカラー
		* texColor									//	テクスチャーカラー
		* sphColor									//	スフィアマップ（乗算）
		+ saturate(spaColor * texColor						//	スフィアマップ（加算）
			+ float4(specularB * specular.rgb, 1))		//	スペキュラー
		, float4(ambient.rgb * texColor.rgb * sphColor.rgb, 1)	//	アンビエント
		+ spaColor
	)
		;

	//	通常レンダリング結果（カラー）
	output.col = float4(col.rgb  * shadowWeight, col.a);
	//	法線結果
	output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
	output.normal.a = 1;
	//	高輝度結果
	float y = dot(float3(0.299f, 0.587f, 0.114f), output.col.rgb);
	output.highLum = y > 0.99f ? output.col : 0.0f;
	output.highLum.a = 1;
	return output;
	/*
	return// max(
		diffuseB								//	ディフューズ輝度
		* diffuse									//	ディフューズカラー
		* texColor									//	テクスチャーカラー
		+ float4(specularB * specular.rgb, 1)		//	スペキュラー
		//,float4(ambient * texColor, 1)			//	アンビエント
		//)
		;
	return float4(brightness, brightness, brightness, 1)	//	輝度
		* diffuse							//	ディフューズ色
		* color								//	テクスチャカラー
		* sph.Sample(smp, sphereMapUV)		//	sph(乗算）
		+ spa.Sample(smp, sphereMapUV)		//	spa（加算）
		+ float4(color * ambient, 1)		//	アンビエント
		;
		*/
	//return float4(tex.Sample(smp,input.uv));
}