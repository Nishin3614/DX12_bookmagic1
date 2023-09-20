//	インクルード
#include "BasicShaderHeader.hlsli"

//	通常描画用
Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	min16uint edge : EDGE_FLAG,
	uint instNo : SV_InstanceID
)
{
	Output output;									//	ピクセルシェーダーに渡す値
	float w = (float)weight / 100.0f;
	matrix bm = _bones[boneno[0]] * w	//	ボーン0のボーン影響度
		+ _bones[boneno[1]] * (1 - w);	//	ボーン1のボーン影響度
	pos = mul(bm, pos);
	pos = mul(_world, pos);
	//	インスタンス番号が1なら、影行列を計算する
	if (instNo == 1)
	{
		pos = mul(_shadow, pos);
	}
	output.pos = pos;
	pos = mul(mul(_proj, _view), pos);	//	通常カメラから投影した行列
	//pos = mul(_lightCamera, output.pos);	//	通常カメラから投影した行列

	output.svpos = pos;
	output.instNo = instNo;							//	インスタンス番号
	normal.w = 0;									//	平行移動成分を無効にする
	output.normal = mul(_world, normal);				//	法線にもワールド変換を行う
	output.vnormal = mul(_view, output.normal);		//	ビュー変換された法線ベクトル
	output.uv = uv;
	output.ray = normalize(pos.xyz - _eye);			//	視線ベクトル
	output.tpos = mul(_lightCamera, output.pos);
	return output;
}

//	影用頂点座標変換（座標変換のみ）
float4 ShadowVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	min16uint edge : EDGE_FLAG
) : SV_POSITION
{
	float fWeight = float(weight) / 100.0f;
	
	matrix conBone =
		_bones[boneno[0]] * fWeight
		+ _bones[boneno[1]] * (1 - fWeight);

	pos = mul(_world, mul(conBone, pos));
	return mul(_lightCamera, pos);

}