#include "polygon2D.hlsli"

float4 ps(Output input) : SV_TARGET
{
	return _tex.Sample(_smp, input.uv);
}