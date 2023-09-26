#include "polygon2D.hlsli"

Output vs( float4 pos : POSITION,float2 uv : TEXCOORD )
{
	Output output;
	output.svpos = mul(_mat,pos);
	output.uv = uv;
	return output;
}