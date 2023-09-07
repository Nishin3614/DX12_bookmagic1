#include "ssaoHeader.hlsli"
//	現在のuv値を元に乱数を返す処理
float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

//	ペラポリゴン用のピクセルシェーダー	//
float ps(Output input) : SV_Target
{
	float dp = depthTex.Sample(smp,input.uv);	//	現在のuvの深度
	float w, h, miplevel;

	//	深度用テクスチャの情報取得
	depthTex.GetDimensions(0, w, h, miplevel);
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//	SSAO
	//	元の座標を復元する
	float4 respos = mul(
		invproj,
		float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1)
	);
	respos.xyz = respos.xyz / respos.w;

	float div = 0.0f;	//	遮蔽を考えない結果の合計
	float ao = 0.0f;	//	アンビエントオクルージョン
	float3 norm = normalize((normaltex.Sample(smp, input.uv).xyz * 2) - 1);	//	法線ベクトル
	const int trycnt = 256;		//	試行回数
	const float radius = 0.5f;	//	半径

	if (dp < 1.0f)
	{
		for (int i = 0; i < trycnt; i++)
		{
			float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
			float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
			float rnd3 = random(float2(rnd1,rnd2)) * 2 - 1;
			float3 omega = normalize(float3(rnd1, rnd2, rnd3));
			omega = normalize(omega);

			//	乱数の結果法線の反対側に向いていたら反転する
			float dt = dot(norm, omega);
			float sgn = sign(dt);
			omega *= sgn;

			//	結果の座標を再び射影変換する
			float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
			rpos.xyz /= rpos.w;
			dt *= sgn;	//	正の値にしてcosθを得る
			div += dt;	//	遮蔽を考えない結果を加算する

			//	計算結果が現在の場所の深度より奥に入っているなら
			//	遮蔽されているので加算
			ao += step(
				depthTex.Sample(smp, (rpos.xy + float2(1, -1))
					* float2(0.5f, -0.5f)), rpos.z) * dt;

		}
		ao /= div;
	}
	return 1.0f - ao;
}