#include "peraHeader.hlsli"
//	���m�N��������
float4 monokuro(float4 col)
{
	//	���m�N����
	float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	if (_MonochroR)
	{
		col.r = Y;
	}
	if (_MonochroG)
	{
		col.g = Y;
	}
	if (_MonochroB)
	{
		col.b = Y;
	}

	return col;
}

//	�F�̊K���𗎂Ƃ�����
//	�����̏����������Ɖ����Ȃ�̂ŁA�f�B�U�Ȃǂ̋Z�p�����p����K�v������
float4 ToneDown(float4 col)
{
	//	8�r�b�g�̊K����4�r�b�g�̊K���֗��Ƃ�
	return float4(col.rgb - fmod(col.rgb,0.25f),col.a);
}

//	�ȈՂȂڂ��������i��f�̕��ω��j
float4 Blur(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, -2 * dy));//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, -2 * dy));		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, -2 * dy));	//	�E��
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 0));		//	��
	ret += _tex.Sample(_smp, input.uv);							//	����
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 0));		//	�E
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 2 * dy));	//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, 2 * dy));		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 2 * dy));	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret / 9.0f;
}

//	�G���{�X���H����
float4 Emboss(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, -2 * dy));		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 0));		//	��
	ret += _tex.Sample(_smp, input.uv);							//	����
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 2 * dy)) * -2;	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret;
}

//	�V���[�v�l�X�i�G�b�W�̋����j
float4 Sharpness(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, -2 * dy)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 0)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv) * 5;							//	����
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret;
}

//	�֊s�����o�i�ȒP�j
float4 OutLine(Output input)
{
	float4 col = _tex.Sample(_smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, -2 * dy)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 0)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv) * 4;							//	����
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += _tex.Sample(_smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += _tex.Sample(_smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += _tex.Sample(_smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	�E��

	//	���m�N����
	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
	Y = pow(1.0f - Y, 10.0f);	//	�G�b�W����������
	Y = step(0.2, Y);			//	�]���ȃG�b�W������
	//	��f�̕��ϒl��Ԃ�
	return float4(Y,Y,Y, col.a);
}

//	�ׂ����ڂ�������
float4 DetailBlur(Texture2D<float4> destTex, SamplerState destSmp,float2 uv,float dx,float dy)
{
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);

	//	�ŏ�i
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 2 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 2 * dy)) * 1 / 256;

	//	1��i
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 1 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 1 * dy)) * 4 / 256;

	//	���i
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, 0 * dy)) * 36 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, 0 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, 0 * dy)) * 6 / 256;

	//	1���i
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, -1 * dy)) * 24 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, -1 * dy)) * 16 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, -1 * dy)) * 4 / 256;

	//	�ŉ��i
	ret += destTex.Sample(destSmp, uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
	ret += destTex.Sample(destSmp, uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(0 * dx, -2 * dy)) * 6 / 256;
	ret += destTex.Sample(destSmp, uv + float2(1 * dx, -2 * dy)) * 4 / 256;
	ret += destTex.Sample(destSmp, uv + float2(2 * dx, -2 * dy)) * 1 / 256;


	//	��f�̕��ϒl��Ԃ�
	return ret;
}

//	�K�E�V�A���ڂ�������
float4 GaussianBlur(Output input)
{
	float4 col = _tex.Sample(_smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	//ret += _bkweights[0] * col;

	//	0~7�̍��v�l
	for (int i = 0; i < 8; ++i)
	{
		ret += _bkweights[i >> 2][i % 4]
			* _tex.Sample(_smp, input.uv + float2(i * dx, 0));
		ret += _bkweights[i >> 2][i % 4]
			* _tex.Sample(_smp, input.uv + float2(-i * dx, 0));
	}

	//	x�l�����̃K�E�V�A���ڂ������̒l��Ԃ�
	return float4(ret.rgb,col.a);
}

//	�u���[������
float4 Bloom(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);
	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//	�k�����P�x�̌v�Z
	float4 bloomAccum = float4(0, 0, 0, 0);
	float2 uvSize = float2(1.0f, 0.5f);
	float2 uvOfst = float2(0, 0);
	for (int i = 0; i < 8; i++)
	{
		bloomAccum += DetailBlur(
			_shrinkHightLumTex, _smp, input.uv * uvSize + uvOfst, dx, dy
		);
		uvOfst.y += uvSize.y;
		uvSize *= 0.5f;
	}

	float4 pass1Blur = DetailBlur(_highLumTex, _smp, input.uv, dx, dy) * _bloomColor;	//	1���ڂ̍��P�x�e�N�X�`�����ڂ���
	float4 pass2Blur = saturate(bloomAccum) * _bloomColor;								//	�k���ڂ����ς�
	return pass1Blur + pass2Blur;
}

//	��ʊE�[�x�ɂ��ڂ�������
float4 Dof(Output input,float4 srcCol)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);
	//	��f��
	float dx = 1.0f / w;
	float dy = 1.0f / h;

	//	�^�񒆂���Ƃ����[�x�l�̍�
	float depthDiff = abs(_depthTex.Sample(_smp, float2(0.5f, 0.5f))
		- _depthTex.Sample(_smp, input.uv));
	depthDiff = pow(depthDiff, 0.5f);	//	�[�x�l�̋߂��l�ł��ω����o�₷���悤�ɂ���
	float2 uvSize = float2(1.0f, 0.5f);
	float2 uvOfst = float2(0, 0);

	float t = depthDiff * 8;
	float no;
	t = modf(t, no);	//	no:�����l�Aref:�����l
	float4 retColors[2];

	//	�ʏ�e�N�X�`��
	retColors[0] = srcCol;

	//	����������0�̏ꍇ
	if (no == 0.0f)
	{
		//	�ʏ�/2�T�C�Y�̂ڂ����e�N�X�`��
		retColors[1] = DetailBlur(_shrinkTex, _smp, input.uv * uvSize + uvOfst, dx, dy);
		//retColors[1] = _tex.Sample(_smp, input.uv);

	}
	//	����������0�ȊO�̏ꍇ
	else
	{
		//	�k���ʏ�̃e�N�X�`���[�����[�v
		for (int i = 0; i <= 8; ++i)
		{
			//	0�����̏ꍇ�X�L�b�v
			if (i - no < 0)
			{
				continue;
			}
			//	1���߂̏ꍇ�A���[�v�𔲂���
			if (i - no > 1)
			{
				break;
			}
			//	����̏k���e�N�X�`���̂ڂ������擾
			retColors[i - no] = DetailBlur(
				_shrinkTex, _smp, input.uv * uvSize + uvOfst, dx, dy);
			uvOfst.y += uvSize.y;
			uvSize *= 0.5f;
		}
	}
	return lerp(retColors[0], retColors[1], t);
}

//	�y���|���S���p�̃s�N�Z���V�F�[�_�[	//
float4 ps(Output input) : SV_Target
{
	//	�f�o�b�O�\��
	if (_DebugDispFlag)
	{
		//	�[�x�o��
		if (input.uv.x < 0.2f && input.uv.y < 0.2f)
		{
			float dep = _depthTex.Sample(_smp, input.uv * 5);
			dep = 1.0f - pow(dep, 20);
			return float4(dep, dep, dep, 1.0f);
		}
		//	SSAO
		else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
		{
			float ssao = _ssaoTex.Sample(_smp, (input.uv - float2(0.0f,0.2f)) * 5);
			return float4(ssao, ssao, ssao, 1.0f);
		}
		//	�@���o��
		else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
		{
			return _texNormal.Sample(_smp, (input.uv - float2(0, 0.4f)) * 5);
			//return _shrinkTex.Sample(_smp,(input.uv - float2(0,0.4f)) * 5);
		}
		//	���P�x�o��
		else if (input.uv.x < 0.2f && input.uv.y < 0.8f)
		{
			return _highLumTex.Sample(_smp, (input.uv - float2(0, 0.6f)) * 5);
		}
		//	���P�x�k���o�b�t�@�o��
		else if (input.uv.x < 0.2f && input.uv.y < 1.0f)
		{
			return _shrinkHightLumTex.Sample(_smp, (input.uv - float2(0, 0.8f)) * 5);
		}
	}
	
	float4 destCol = _tex.Sample(_smp,input.uv);	//	�ŏI�F

	//	SSAO
	if (_SSAOFlag)
	{
		destCol *= _ssaoTex.Sample(_smp, input.uv);		//	SSAO
	}
	//	��ʊE�[�x�ڂ�������	
	if (_nDof)
	{
		destCol = Dof(input, destCol);
	}
	//	�u���[��
	destCol += Bloom(input);
	//	���m�N��
	destCol = monokuro(destCol);
	//	���]
	if (_nReverse)
	{
		destCol = float4(1.0f - destCol.rgb, destCol.a);
	}

	return destCol;
	//	�K�E�V�A���ڂ���
	return GaussianBlur(input);
}

//	�ڂ����̃s�N�Z���V�F�[�_�[	//
float4 VerticalBokehPS(Output input) : SV_Target
{
	//	�ʏ�`��
	return _tex.Sample(_smp, input.uv);

	/*
	float2 normalTex = _effectTex.Sample(_smp,input.uv).xy;
	//	RGB���@���x�N�g���̕ϊ�
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTex�͈̔͂�-1~1�����A��1���e�N�X�`��1���̑傫���ł���A
	//	�c�݂����邽��0.1����Z����
	return _tex.Sample(_smp, input.uv + normalTex * 0.1f);
	*/

	//return _tex.Sample(_smp, input.uv);
	float4 col = _tex.Sample(_smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	_tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dy = 1.0f / h;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);

	//	x�����̍����H
	//	���Ȃ�_bkweights[0]�i0~3)�܂ł�col�ł����Ă���̂��H
	//ret += _bkweights[0] * col;
	//ret += _bkweights[1] * col;

	//	0~7�̍��v�l
	for (int i = 0; i < 8; ++i)
	{
		ret += _bkweights[i >> 2][i % 4]
			* _tex.Sample(_smp, input.uv + float2(0, dy * i));
		ret += _bkweights[i >> 2][i % 4]
			* _tex.Sample(_smp, input.uv + float2(0, -dy * i));
	}

	//	y�l�����̃K�E�V�A���ڂ������̒l��Ԃ�
	return float4(ret.rgb,col.a);
}

//	���C���e�N�X�`�����ڍׂڂ����s�N�Z���V�F�[�_�[
BlurOutput BlurPS(Output input)
{
	float w,h,miplevels;
	_tex.GetDimensions(0, w, h, miplevels);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	BlurOutput ret;
	ret.col = DetailBlur(_tex, _smp, input.uv, dx, dy);
	ret.highLum = DetailBlur(_highLumTex, _smp, input.uv, dx, dy);
	return ret;
}

//	�|�X�g�G�t�F�N�gPS
float4 PostEffectPS(Output input) : SV_Target
{
	return _tex.Sample(_smp, input.uv);

	float2 normalTex = _effectTex.Sample(_smp,input.uv).xy;
	//	RGB���@���x�N�g���̕ϊ�
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTex�͈̔͂�-1~1�����A��1���e�N�X�`��1���̑傫���ł���A
	//	�c�݂����邽��0.1����Z����
	return _tex.Sample(_smp, input.uv + normalTex * 0.1f);
}
