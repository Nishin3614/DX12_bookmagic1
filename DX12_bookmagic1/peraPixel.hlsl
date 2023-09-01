#include "peraHeader.hlsli"
//	�ʏ�`��
float4 Normal(Output input)
{
	return tex.Sample(smp,input.uv);
}

//	���m�N��������
float4 monokuro(float4 col)
{
	//	���m�N����
	float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	return float4(Y, Y, Y, col.a);

}

//	�F�̔��]����
float4 Reverse(float4 col)
{
	return float4(1.0 - col.rgb, col.a);
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
	tex.GetDimensions(
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
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy));//	����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy));	//	�E��
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));		//	��
	ret += tex.Sample(smp, input.uv);							//	����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0));		//	�E
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy));	//	����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy));		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy));	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret / 9.0f;
}

//	�G���{�X���H����
float4 Emboss(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
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
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 2;//	����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy));		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0));		//	��
	ret += tex.Sample(smp, input.uv);							//	����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -2;	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret;
}

//	�V���[�v�l�X�i�G�b�W�̋����j
float4 Sharpness(Output input)
{
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
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
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;		//	��
	ret += tex.Sample(smp, input.uv) * 5;							//	����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	�E��

	//	��f�̕��ϒl��Ԃ�
	return ret;
}

//	�֊s�����o�i�ȒP�j
float4 OutLine(Output input)
{
	float4 col = tex.Sample(smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
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
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 0;//	����
	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy)) * -1;		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 0;	//	�E��

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0)) * -1;		//	��
	ret += tex.Sample(smp, input.uv) * 4;							//	����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0)) * -1;		//	�E

	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 0;	//	����
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy)) * -1;		//	��
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * -0;	//	�E��

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
	float4 col = tex.Sample(smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
		0,			//	�~�b�v���x��
		w,			//	��
		h, 			//	����
		level		//	�~�b�v�}�b�v�̃��x����
	);

	//	��f��
	float dx = 1.0f / w;
	//	�ߖT�e�[�u���̍��v�l
	float4 ret = float4(0, 0, 0, 0);
	//ret += bkweights[0] * col;

	//	0~7�̍��v�l
	for (int i = 0; i < 8; ++i)
	{
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(i * dx, 0));
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(-i * dx, 0));
	}

	//	x�l�����̃K�E�V�A���ڂ������̒l��Ԃ�
	return float4(ret.rgb,col.a);
}

//	�y���|���S���p�̃s�N�Z���V�F�[�_�[	//
float4 ps(Output input) : SV_Target
{
	//	�[�x�o��
	if (input.uv.x < 0.2f && input.uv.y < 0.2f)
	{
		float dep = depthTex.Sample(smp, input.uv * 5);
		dep = 1.0f - pow(dep, 30);
		return float4(dep, dep, dep, 1.0f);
	}
	//	���C�g����̐[�x�o��
	else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
	{
		float lightdep = lightDepthTex.Sample(smp, (input.uv - float2(0.0f,0.2f)) * 5);
		lightdep = 1.0f - lightdep;
		return float4(lightdep, lightdep, lightdep, 1.0f);
	}
	//	�@���o��
	else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
	{
		return texNormal.Sample(smp,(input.uv - float2(0,0.4f)) * 5);
	}
	//	���P�x�o��
	else if (input.uv.x < 0.2f && input.uv.y < 0.8f)
	{
		return highLumTex.Sample(smp, (input.uv - float2(0, 0.6f)) * 5);
	}
	//	���P�x�k���o�b�t�@�o��
	else if (input.uv.x < 0.2f && input.uv.y < 1.0f)
	{
		return ShrinkHightLumTex.Sample(smp, (input.uv - float2(0, 0.8f)) * 5);
	}
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
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
			ShrinkHightLumTex, smp, input.uv * uvSize + uvOfst, dx, dy
		);
		uvOfst.y += uvSize.y;
		uvSize *= 0.5f;
	}

	//	�ʏ�`��
	return tex.Sample(smp, input.uv)
		+ DetailBlur(highLumTex, smp, input.uv, dx, dy)	//	1���ڂ̍��P�x�e�N�X�`�����ڂ���
		+ saturate(bloomAccum)							//	�k���ڂ����ς�
		;
	//	���m�N����
	return GaussianBlur(input);


	//	���C�g�f�v�X���ؗp
}

//	�ڂ����̃s�N�Z���V�F�[�_�[	//
float4 VerticalBokehPS(Output input) : SV_Target
{
	//	�ʏ�`��
	return tex.Sample(smp, input.uv);

	/*
	float2 normalTex = effectTex.Sample(smp,input.uv).xy;
	//	RGB���@���x�N�g���̕ϊ�
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTex�͈̔͂�-1~1�����A��1���e�N�X�`��1���̑傫���ł���A
	//	�c�݂����邽��0.1����Z����
	return tex.Sample(smp, input.uv + normalTex * 0.1f);
	*/

	//return tex.Sample(smp, input.uv);
	float4 col = tex.Sample(smp, input.uv);
	//	�e�N�X�`���̃T�C�Y���擾
	float w, h, level;
	tex.GetDimensions(
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
	//	���Ȃ�bkweights[0]�i0~3)�܂ł�col�ł����Ă���̂��H
	//ret += bkweights[0] * col;
	//ret += bkweights[1] * col;

	//	0~7�̍��v�l
	for (int i = 0; i < 8; ++i)
	{
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(0, dy * i));
		ret += bkweights[i >> 2][i % 4]
			* tex.Sample(smp, input.uv + float2(0, -dy * i));
	}

	//	y�l�����̃K�E�V�A���ڂ������̒l��Ԃ�
	return float4(ret.rgb,col.a);
}

//	���C���e�N�X�`�����ڍׂڂ����s�N�Z���V�F�[�_�[
float4 BlurPS(Output input) : SV_Target
{
	float w,h,miplevels;
	tex.GetDimensions(0, w, h, miplevels);
	return DetailBlur(tex, smp, input.uv, 1.0f / w, 1.0f / h);
}

//	�|�X�g�G�t�F�N�gPS
float4 PostEffectPS(Output input) : SV_Target
{
	return tex.Sample(smp, input.uv);

	float2 normalTex = effectTex.Sample(smp,input.uv).xy;
	//	RGB���@���x�N�g���̕ϊ�
	normalTex = normalTex * 2.0f - 1.0f;
	//	normalTex�͈̔͂�-1~1�����A��1���e�N�X�`��1���̑傫���ł���A
	//	�c�݂����邽��0.1����Z����
	return tex.Sample(smp, input.uv + normalTex * 0.1f);
}
