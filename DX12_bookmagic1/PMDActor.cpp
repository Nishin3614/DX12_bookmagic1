//	�C���N���[�h
#include "PMDActor.h"
#include <string>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "Win32Application.h"
#include "Dx12Wrapper.h"
#include <Windows.h>
#include <algorithm>
#include <sstream>
#include <array>
#include "helper.h"

//	���C�u���������N
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"winmm.lib")

//	�萔
constexpr size_t pmdvertex_size = 38;				//	���_�������̃T�C�Y

namespace//	�������O���
{
	//	�{�[�����
	enum class BoneType
	{
		Rotation,		//	��]
		RotAndMove,		//	��]&�ړ�
		IK,				//	IK
		Undefined,		//	����`
		IKChile,		//	IK�e���{�[��
		RotationChild,	//	��]�e���{�[��
		IKDestination,	//	IK�ڑ���
		Invisible		//	�����Ȃ��{�[��
	};
}

using namespace DirectX;
using namespace Microsoft::WRL;

//	�R���X�g���N�^
PMDActor::PMDActor(Dx12Wrapper* pDxWrap, const char* modelpath, const char* vmdpath, DirectX::XMFLOAT3 position):
	_pDxWrap(pDxWrap), _position(position)
{
	//	PMD�̓ǂݍ���
	LoadPMD(modelpath);
	//	VMD�t�@�C���ǂݍ���
	LoadVMD(vmdpath);
}

//	����������
void PMDActor::Init(void)
{
	//	���_�E�C���f�b�N�X�o�b�t�@�̍쐬
	CreateVertex_IdxView();

	//	�ʒu���W�o�b�t�@�̍쐬
	CreateTransformView();

	//	�}�e���A���o�b�t�@�̍쐬
	CreateMaterialView();

	//	�A�j���[�V�����v���C
	PlayAnimation();
}

//	�X�V����
void PMDActor::Update(void)
{
	//	�I�u�W�F�N�g�̉�]����
	//_angle += 0.001f;
	//_mappedMatrices[0] = XMMatrixRotationY(_angle);

	//	���[�V�����X�V����
	MotionUpdate();
}

//	�`�揈��
void PMDActor::Draw(void)
{
	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�̎w��
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_BasicDescHeap.GetAddressOf()		//	���W�ϊ��p�f�B�X�N���v�^�q�[�v
	);

	//	���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
	auto heapHandle = _BasicDescHeap->GetGPUDescriptorHandleForHeapStart();
	//	�萔�o�b�t�@1�r���[�p�̎w��
	_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
		1,			//	���[�g�p�����[�^�C���f�b�N�X
		heapHandle	//	�q�[�v�A�h���X
	);

	//	�v���~�e�B�u�|���W���Z�b�g
	_pDxWrap->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	���_�o�b�t�@���Z�b�g
	_pDxWrap->GetCmdList()->IASetVertexBuffers(
		0,			//	�X���b�g�ԍ�
		1, 			//	���_�o�b�t�@�r���[�̐�
		&_vbView	//	���_�o�b�t�@�r���[�I�u�W�F
	);

	//	�C���f�b�N�X�o�b�t�@���Z�b�g
	_pDxWrap->GetCmdList()->IASetIndexBuffer(&_ibView);

	auto materialH = _materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;
	auto cbvsrvIncSize = _pDxWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	//	�}�e���A���p�f�B�X�N���v�^�q�[�v�̎w��
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_materialDescHeap.GetAddressOf()	//	�}�e���A���p�p�f�B�X�N���v�^�q�[�v
	);
	for (auto& m : _materials)
	{
		//	�}�e���A���A�e�N�X�`���[�r���[�p�̎w��
		_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
			2,			//	���[�g�p�����[�^�C���f�b�N�X
			materialH	//	�q�[�v�A�h���X
		);
		//	�|���S���̕`�施��
		_pDxWrap->GetCmdList()->DrawIndexedInstanced(
			m.indicesNum,	//	�C���f�b�N�X��			
			//2,				//	�C���X�^���X���i�|���S�����j1:�ʏ�̃��f���`��A2:�n�ʉe
			1,
			idxOffset,		//	�C���f�b�N�X�̃I�t�Z�b�g
			0,				//	���_�f�[�^�̃I�t�Z�b�g
			0				//	�C���X�^���X�̃I�t�Z�b�g
		);
		//	�q�[�v�|�C���^�[�ƃC���f�b�N�X�����ɐi�߂�
		materialH.ptr += cbvsrvIncSize;	//	���̃r���[�̂��߁A2�{�i�߂�
		idxOffset += m.indicesNum;

	}
}

//	�`�揈��
void PMDActor::ShadowMapDraw(void)
{
	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�̎w��
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_BasicDescHeap.GetAddressOf()		//	���W�ϊ��p�f�B�X�N���v�^�q�[�v
	);

	//	���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
	auto heapHandle = _BasicDescHeap->GetGPUDescriptorHandleForHeapStart();
	//	�萔�o�b�t�@1�r���[�p�̎w��
	_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
		1,			//	���[�g�p�����[�^�C���f�b�N�X
		heapHandle	//	�q�[�v�A�h���X
	);

	//	�v���~�e�B�u�|���W���Z�b�g
	_pDxWrap->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	���_�o�b�t�@���Z�b�g
	_pDxWrap->GetCmdList()->IASetVertexBuffers(
		0,			//	�X���b�g�ԍ�
		1, 			//	���_�o�b�t�@�r���[�̐�
		&_vbView	//	���_�o�b�t�@�r���[�I�u�W�F
	);

	//	�C���f�b�N�X�o�b�t�@���Z�b�g
	_pDxWrap->GetCmdList()->IASetIndexBuffer(&_ibView);
	//	�|���S���̕`�施��
	_pDxWrap->GetCmdList()->DrawIndexedInstanced(
		_indicesNum,	//	�C���f�b�N�X��			
		1,				//	�C���X�^���X���i�|���S�����j
		0,		//	�C���f�b�N�X�̃I�t�Z�b�g
		0,				//	���_�f�[�^�̃I�t�Z�b�g
		0				//	�C���X�^���X�̃I�t�Z�b�g
	);

}

//	�I�u�W�F�N�g�̉������
void PMDActor::Release(void)
{

}

//	�A�j���[�V�����J�n����
void PMDActor::PlayAnimation(void)
{
	_startTime = timeGetTime();	//	���݂̎��Ԏ擾
}

//	���[�V�����X�V����
void PMDActor::MotionUpdate(void)
{
	DWORD elapsedTime = timeGetTime() - _startTime;	//	�o�ߎ��Ԃ𑪂�
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);
	//	���݂̃t���[�������ŏI�t���[�����߂Ȃ�
	if (frameNo > _duration)
	{
		_startTime = timeGetTime();
		frameNo = 0;
	}

	//	FK����
	FKUpdate(frameNo);
	//	IK����
	IKSolve(frameNo);

	//	�X�V���ꂽ�s����R�s�[
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);	//	�{�[���s����R�s�[

}

void PMDActor::FKUpdate(const unsigned int& frameNo)
{
	//	�s����N���A
	//	(�N���A���Ă��Ȃ��ƑO�t���[���̃|�[�Y���d�ˊ|�������
	//	���f��������j
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//	���[�V�����f�[�^�X�V
	for (auto& bonemotion : _motiondata)
	{
		//	�{�[���e�[�u�����Ɏw�肳�ꂽ�{�[�����͑��݂��Ă��Ȃ���΃X�L�b�v
		auto itBoneNode = _boneNodeTable.find(bonemotion.first);
		if (itBoneNode == _boneNodeTable.end())
		{
			continue;
		}

		auto node = _boneNodeTable[bonemotion.first];
		//	���v������̂�T��
		auto motions = bonemotion.second;
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const KeyFrame& keyframe)
			{
				return keyframe.frameNo <= frameNo;
			}
		);

		//	���v������̂��Ȃ���Ώ������X�L�b�v
		if (rit == motions.rend())
		{
			continue;
		}

		//	�A�j���[�V�����̕�ԏ���
		XMMATRIX rotation;								//	��]
		XMVECTOR offset = XMLoadFloat3(&rit->offset);	//	�I�t�Z�b�g�ʒu
		auto it = rit.base();	//	�ʏ�̃C�e���[�^�[�i���̃f�[�^�����j�ɖ߂�
		if (it != motions.end())
		{
			//	���݂̃A�j���[�V�����e���x
			auto t = static_cast<float>(frameNo - rit->frameNo)		//	�i���݂̃t���[�� - �O��̃t���[���j
				/ static_cast<float>(it->frameNo - rit->frameNo);	//	/�@�i���̃t���[�� - �O��̃t���[���j
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);
			//	�ŏI�I�ȉ�]�s��(���`��ԁj
			//rotation =
			//	XMMatrixRotationQuaternion(rit->quaternion) * (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion) * t;

			//	�ŏI�I�ȉ�]�s��(���ʐ��`��ԁj
			rotation = XMMatrixRotationQuaternion(
				XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
			offset = XMVectorLerp(offset, XMLoadFloat3(&it->offset), t);
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;
		auto mat =
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)	//	�{�[����_�����_�ֈړ�����
			* rotation									//	��]
			* XMMatrixTranslation(pos.x, pos.y, pos.z);	//	�{�[�������̊�_�ɖ߂�
		_boneMatrices[node.boneIdx] = mat * XMMatrixTranslationFromVector(offset);
	}
	//	�e�{�[���̍s����X�V����
	RecusiveMatrixMultiply(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());
}

//	IK����
void PMDActor::IKSolve(unsigned int frameNo)
{
	//	
	auto it = std::find_if(_ikEnableData.rbegin(),
		_ikEnableData.rend(),
		[frameNo](const VMDIKEnable& ikenable)
		{
			return ikenable.frameNo <= frameNo;
		});

	for (auto& ik : _pmdIk)
	{
		//	ik�����݂��Ă��邩
		if (it != _ikEnableData.rend())
		{
			//	IK�{�[�����擾
			auto ikEnableIt =
				it->ikEnableTable.find(_boneNameArray[ik.IKboneIdx]);
			//	ik�{�[�������݂��Ă��邩
			if (ikEnableIt != it->ikEnableTable.end())
			{
				//	�t���O��OFF�Ȃ�X�L�b�v
				if (!ikEnableIt->second)
				{
					continue;
				}
			}
		}
		auto childrenNodeCount = ik.nodeIdxes.size();

		switch (childrenNodeCount)
		{
		case 0:
			assert(0);
			continue;
		case 1:	//	�Ԃ̃{�[������1�̎���LookAt
			SolveLookAt(ik);
			break;
		case 2:	//	�Ԃ̃{�[������2�̎��͗]���藝IK
			SolveCosineIK(ik);
			break;
		default:	//	�Ԃ̃{�[������3�ȏ�̎���CCD-IK
			SolveCCDIK(ik);
			break;
		}
	}
}

constexpr float epsilon = 0.0005f;

//	CCD-IK�ɂ��{�[������������
//	@param ik �Ώ�ik�I�u�W�F�N�g
void PMDActor::SolveCCDIK(const PMDIK& ik)
{
	//	�^�[�Q�b�g
	auto targetBoneNode = _boneNodeAddressArray[ik.IKboneIdx];
	auto targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	//	�e���W���t�s��Ŗ����ɂ���
	auto& paremMat =
		_boneMatrices[_boneNodeAddressArray[ik.IKboneIdx]->ikParentBone];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, paremMat);
	auto targetNextPos = XMVector3Transform(
		targetOriginPos, _boneMatrices[ik.IKboneIdx] * invParentMat);

	//	���[�m�[�h
	auto endPos = XMLoadFloat3(
		&_boneNodeAddressArray[ik.EndIdx]->startPos);
	
	//	���ԃm�[�h�Aroot�m�[�h
	std::vector<XMVECTOR> positions;
	for (auto& cidx : ik.nodeIdxes)
	{
		positions.emplace_back(
			XMLoadFloat3(&_boneNodeAddressArray[cidx]->startPos));
	}

	//	�e�{�[���̍s��i���[�ȊO�j
	std::vector<XMMATRIX> mats(positions.size());	
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

	auto ikLimit = ik.limit * XM_PI;	//	ik�̉�]����

	//	ik�ɐݒ肳��Ă��鎎�s�񐔂����J��Ԃ�
	for (int c = 0; c < ik.iterations; ++c)
	{
		//	�^�[�Q�b�g�Ɩ��[���قڈ�v�����甲����
		if (XMVector3Length(
			XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		//	���ꂼ��̃{�[���������̂ڂ�Ȃ���A
		//	�p�x�����Ɉ����|����Ȃ��悤�ɋȂ��Ă���

		//	���[�̈�O�`���[�g�{�[���܂�
		for (unsigned int bidx = 0; bidx < positions.size(); ++bidx)
		{
			const auto& pos = positions[bidx];
			auto vecToEnd = XMVectorSubtract(endPos, pos);				//	�Ώۃm�[�h���疖�[�m�[�h�܂ł̃x�N�g��
			auto vecToTarget = XMVectorSubtract(targetNextPos, pos);	//	�Ώۃm�[�h����^�[�Q�b�g�m�[�h�܂ł̃x�N�g��
			//	��̃x�N�g��2���K��
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			//	�قړ����x�N�g���̏ꍇ�A�O�ς��ł��Ȃ����߃X�L�b�v
			if (XMVector3Length(
				XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0]
				<= epsilon)
			{
				continue;
			}

			//	�O�όv�Z����ъp�x�v�Z
			auto cross = XMVector3Normalize(
				XMVector3Cross(vecToEnd, vecToTarget));	//	���ɂȂ�
			//	cos(���ϒl)�Ȃ̂ŁA0~90���A0~-90���̋�ʂ��Ȃ�
			float angle = XMVector3AngleBetweenVectors(
				vecToEnd, vecToTarget).m128_f32[0];
			//	��]���E�𒴂��Ă��܂����Ƃ��͌��E�l�ɕ␳
			angle = min(angle, ikLimit);
			XMMATRIX rot =
				XMMatrixRotationAxis(cross, angle);	//	��]�s��쐬

			//	���_���S�ł͂Ȃ��Apos���S�ɉ�]����s����쐬����
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);
			//	��]�s���ێ�����i��Z�ŉ�]�d�ˊ|��������Ă����j
			mats[bidx] *= mat;

			//	�ΏۂƂȂ�_�����ׂĉ�]������i���݂̓_����݂Ė��[������]�j
			//	�����͉�]������K�v�͂Ȃ�
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				positions[idx] =
					XMVector3Transform(positions[idx], mat);
			}
			endPos = XMVector3Transform(endPos, mat);

			//	�^�[�Q�b�g�ɋ߂��Ȃ����烋�[�v�𔲂���
			if (XMVector3Length(
				XMVectorSubtract(endPos, targetNextPos)).m128_f32[0]
				<= epsilon)
			{
				break;
			}
		}
	}

	//	mats�z��͖��[�̈�O�{�[���`���[�g�{�[��
	//	ik.nodeIdxes�̓{�[��ID���i�[����Ă���
	int idx = 0;
	for (auto& cidx : ik.nodeIdxes)
	{
		_boneMatrices[cidx] = mats[idx];
		++idx;
	}

	auto rootNode = _boneNodeAddressArray[ik.nodeIdxes.back()];	//	���[�g�{�[���擾
	RecusiveMatrixMultiply(rootNode, paremMat);
}

//	�]���藝IK�ɂ��{�[������������
//	@param ik �Ώ�ik�I�u�W�F�N�g
void PMDActor::SolveCosineIK(const PMDIK& ik)
{
	std::vector<XMVECTOR> positions;	//	IK�\���_��ۑ�
	std::array<float, 2> edgeLens;		//	IK�̂��ꂼ��̃{�[���Ԃ̋�����ۑ�

	//	�^�[�Q�b�g(�ڕW�{�[�����擾)
	auto& targetNode = _boneNodeAddressArray[ik.IKboneIdx];
	auto targetPos = XMVector3Transform(
		XMLoadFloat3(&targetNode->startPos),
		_boneMatrices[ik.IKboneIdx]);

	//	���[�{�[���擾
	auto endNode = _boneNodeAddressArray[ik.EndIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	//	���ԋy�у��[�g�{�[���擾
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = _boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	//	���݁Apositions���u���[�`���[�g�{�[���v�ɂȂ��Ă���̂ŁA�t�ɂ���
	std::reverse(positions.begin(), positions.end());

	//	���̒����𑪂��Ă���
	edgeLens[0] = XMVector3Length(
		XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(
		XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	//	���[�g�{�[�����W�ϊ�
	positions[0] = XMVector3Transform(
		positions[0], _boneMatrices[ik.nodeIdxes[1]]);

	//	�^�񒆂̃{�[���͎����v�Z����邩��v�Z���Ȃ�

	//	��[�{�[��
	positions[2] = XMVector3Transform(
		positions[2], _boneMatrices[ik.IKboneIdx]);

	//	root�����[�ւ̃x�N�g�������
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float A = XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	//	root����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));
	//	�^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	//	�������߂�
	//	��������x�N�g���ɂȂ�\��������H
	XMVECTOR axis;
	//	�u�Ђ��v�ł͂Ȃ��ꍇ
	if (std::find(_kneeIdxes.begin(), _kneeIdxes.end(),
		ik.nodeIdxes[0]) == _kneeIdxes.end())
	{
		auto vm = XMVector3Normalize(
			XMVectorSubtract(positions[2], positions[0]));
		auto vt = XMVector3Normalize(
			XMVectorSubtract(targetPos, positions[0]));
		axis = XMVector3Cross(vt, vm);
	}
	//	�^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���ɂ���
	else
	{
		auto right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	//	���ӓ_�FIK�`�F�[���̓��[�g�Ɍ������Ă��琔�����邽��1��root�ɋ߂�
	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2-XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);
	
	_boneMatrices[ik.nodeIdxes[1]] *= mat1;	//	���[�g�{�[��
	_boneMatrices[ik.nodeIdxes[0]] = mat2 * _boneMatrices[ik.nodeIdxes[1]];	//	���ԃ{�[��
	//	���Ȃ��A���[�{�[���ɒ��ԃ{�[���̍s������R�s�[���Ă���̂��H
	_boneMatrices[ik.EndIdx] = _boneMatrices[ik.nodeIdxes[0]];	//	���[�{�[��
}

//	LookAt�s��ɂ��{�[������������
//	@param ik �Ώ�ik�I�u�W�F�N�g
void PMDActor::SolveLookAt(const PMDIK& ik)
{
	//	���̊֐��ɗ������_�Ńm�[�h��1�����Ȃ�
	//	�`�F�[���ɓ����Ă���m�[�h�ԍ���IK�̃��[�g�m�[�h�̂��̂Ȃ̂ŁA
	//	���̃��[�g�m�[�h���疖�[�Ɍ������x�N�g�����l����
	auto rootNode = _boneNodeAddressArray[ik.nodeIdxes[0]];
	auto targetNode = _boneNodeAddressArray[ik.EndIdx];

	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto rpos2 = XMVector3TransformCoord(
		rpos1, _boneMatrices[ik.nodeIdxes[0]]);
	auto tpos2 = XMVector3TransformCoord(
		tpos1, _boneMatrices[ik.IKboneIdx]);

	auto originVec = XMVectorSubtract(tpos1, rpos1);
	auto targetVec = XMVectorSubtract(tpos2, rpos2);
	XMFLOAT3 up = XMFLOAT3(0, 1, 0);
	XMFLOAT3 right = XMFLOAT3(1, 0, 0);

	originVec = XMVector3Normalize(originVec);
	targetVec = XMVector3Normalize(targetVec);
	_boneMatrices[ik.nodeIdxes[0]] =
		LookAtMatrix(originVec, targetVec,
			up, right);
}

//	����̃x�N�g�������̕����Ɍ�����s���Ԃ��֐�
//	@param origin ����̃x�N�g��
//	@param lookat ���������������x�N�g��
//	@param up ��x�N�g��
//	@param right �E�x�N�g��
DirectX::XMMATRIX PMDActor::LookAtMatrix(
	const DirectX::XMVECTOR& origin,
	const DirectX::XMVECTOR& lookat, 
	DirectX::XMFLOAT3& up,
	DirectX::XMFLOAT3& right)
{
	return XMMatrixTranspose(LookAtMatrix(origin,up,right))
		* LookAtMatrix(lookat,up,right);
}
//	z�������̕����Ɍ�����s���Ԃ��֐�
//	@param lookaat ���������������x�N�g��
//	@param up ��x�N�g��
//	@param right �E�x�N�g��
DirectX::XMMATRIX PMDActor::LookAtMatrix(
	const DirectX::XMVECTOR& lookat,
	DirectX::XMFLOAT3& up, 
	DirectX::XMFLOAT3& right)
{
	//	�����������(z��)
	XMVECTOR vz = lookat;
	//	�������������������������Ƃ��́j����y��
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));
	//	����y����z���̊O�σx�N�g���ix���j
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	//	�i�������������������������Ƃ��́jy��
	vy = XMVector3Normalize(XMVector3Cross(vz, vx));

	//	LookAt��Up�����������������Ă�����right����ɂ��č�蒼��
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		//	����x�������`
		vx = XMVector3Normalize(XMLoadFloat3(&right));
		//	�������������������������Ƃ���y��
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));
		//	�^��x��
		vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	}

	//	�e�����s��ɕϊ�
	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = vx;
	ret.r[1] = vy;
	ret.r[2] = vz;
	return ret;
}

//	���_�E�C���f�b�N�X�o�b�t�@�̍쐬
void PMDActor::CreateVertex_IdxView(void)
{
	//	�q�[�v�ݒ�
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(_vertices.size());
	//	���_�o�b�t�@�̍쐬
	auto result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&heapprop,							//	�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,				//	�q�[�v�t���O
		&resDesc,							//	���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,	//	GPU����̓ǂݎ���p
		nullptr,							//	����̃��\�[�X�̃N���A����
		IID_PPV_ARGS(_vertBuff.ReleaseAndGetAddressOf())				//	ID�ƒ��_�o�b�t�@
	);

	//	���_�o�b�t�@�֒��_�����R�s�[����
	unsigned char* vertMap = nullptr;
	result = _vertBuff->Map(0, nullptr, (void**)&vertMap);			//	���_���𒸓_�o�b�t�@�ɓn��
	std::copy(std::begin(_vertices), std::end(_vertices), vertMap);	//	���_���ɒ��_���W���R�s�[����
	_vertBuff->Unmap(0, nullptr);									//	�}�b�v����������

	//	���_�o�b�t�@�r���[�̍쐬
	_vbView.BufferLocation = _vertBuff->GetGPUVirtualAddress();	//	�o�b�t�@�[�̉��z�A�h���X
	_vbView.SizeInBytes = _vertices.size();						//	�S�o�C�g��
	_vbView.StrideInBytes = pmdvertex_size;					//	1���_������̃o�C�g��

	//	�C���f�b�N�X�o�b�t�@�̍쐬	//	
	CD3DX12_RESOURCE_DESC idxDesc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)_indices.size() * (UINT64)sizeof(_indices[0]));
	result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&idxDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_idxBuff.ReleaseAndGetAddressOf())
	);

	//	�C���f�b�N�X�o�b�t�@�փC���f�b�N�X�����R�s�[����
	unsigned short* mappedIdx = nullptr;
	//	�C���f�b�N�X�����C���f�b�N�X�o�b�t�@�ɓn��
	_idxBuff->Map(
		0,
		nullptr,
		(void**)&mappedIdx
	);
	std::copy(std::begin(_indices), std::end(_indices), mappedIdx);	//	�C���f�b�N�X���ɃC���f�b�N�X���R�s�[����
	_idxBuff->Unmap(0, nullptr);										//	�}�b�v���J������

	//	�C���f�b�N�X�o�b�t�@�r���[���쐬
	_ibView.BufferLocation = _idxBuff->GetGPUVirtualAddress();		//	�o�b�t�@�̉��z�A�h���X
	_ibView.Format = DXGI_FORMAT_R16_UINT;							//	�t�H�[�}�b�g�iunsigned short(16�o�C�g�j�Ȃ̂�R16_UINT�j
	_ibView.SizeInBytes = _indices.size() * sizeof(_indices[0]);	//	�S�o�C�g��
}

//	�ʒu���W�o�b�t�@�̍쐬
void PMDActor::CreateTransformView(void)
{
	//	�q�[�v�v���p�e�B�[�ݒ�
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;										//	�}�X�N
	descHeapDesc.NumDescriptors = 1;								//	�萔�o�b�t�@�r���[�iCBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	�V�F�[�_���\�[�X�r���[�p
	auto result = _pDxWrap->GetDevice()->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_BasicDescHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
	}

	//	�萔�o�b�t�@�̍쐬	//
	//	�萔�o�b�t�@�̐����i���W�ϊ��j
	auto buffSize = sizeof(XMMATRIX) * (1 + _boneMatrices.size());	//	�u�{1]�̓��[���h�s��
	buffSize = (buffSize + 0xff) & ~0xff;
	CD3DX12_RESOURCE_DESC constresDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);
	result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&constresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_constBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
	}

	//	�ʒu���W�̐ݒ�
	result = _constBuff->Map(0, nullptr, (void**)&_mappedMatrices);	//	�}�b�v
	_mappedMatrices[0] = XMMatrixRotationY(0) * XMMatrixTranslation(_position.x,_position.y,_position.z);									//	���[���h�s��̓��e���R�s�[
	
	//	�{�[���s��̐ݒ�
	for (auto& bonemotion : _motiondata)
	{
		auto node = _boneNodeTable[bonemotion.first];
		auto& pos = node.startPos;
		auto mat = 
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)	//	�{�[����_�����_�ֈړ�����
			* XMMatrixRotationQuaternion(bonemotion.second[0].quaternion)
			* XMMatrixTranslation(pos.x, pos.y, pos.z);	//	�{�[�������̊�_�ɖ߂�
		_boneMatrices[node.boneIdx] = mat;
	}

	RecusiveMatrixMultiply(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);	//	�{�[���s����R�s�[

	//	�萔�o�b�t�@�r���[�̍쐬�i���W�ϊ��j
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	auto basicHeapHandle = _BasicDescHeap->GetCPUDescriptorHandleForHeapStart();	//	�q�[�v�̐擪���擾����
	_pDxWrap->GetDevice()->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
}

//	�}�e���A���o�b�t�@�̍쐬
void PMDActor::CreateMaterialView(void)
{
	//	�}�e���A���o�b�t�@�̍쐬	//
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;	//	�A���C�����g�ɍ��킹��
	//	�}�e���A���o�b�t�@�̃q�[�v�ڍ�
	CD3DX12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	�}�e���A���o�b�t�@�̐ݒ�
	CD3DX12_RESOURCE_DESC materialresDesc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)materialBuffSize * (UINT64)_materialNum);
	auto result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf())
	);
	//	�}�b�v
	char* mapMaterial = nullptr;
	_materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : _materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;	//	�}�e���A�����̃f�[�^�R�s�[
		mapMaterial += materialBuffSize;				//	���̃A���C�����g�ʒu�܂Ői�߂�i256�̔{���j
	}
	_materialBuff->Unmap(0, nullptr);
	//	�}�e���A���p�f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = _materialNum * 5;	//	�}�e���A�����~�f�ށi�}�e���A���A�e�N�X�`���A��Z�X�t�B�A�}�b�v�A���Z�X�t�B�A�}�b�v�A�g�D�[���e�N�X�`���j�̃f�B�X�N���v�^������
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _pDxWrap->GetDevice()->CreateDescriptorHeap(
		&matDescHeapDesc,
		IID_PPV_ARGS(_materialDescHeap.ReleaseAndGetAddressOf())
	);
	//	�}�e���A���p�萔�o�b�t�@�r���[�ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = _materialBuff->GetGPUVirtualAddress();			//	�o�b�t�@�A�h���X
	matCBVDesc.SizeInBytes = materialBuffSize;									//	�o�b�t�@�T�C�Y
	auto matDescHeapH = _materialDescHeap->GetCPUDescriptorHandleForHeapStart();	//	�擪�̋L�^

	//	�e�N�X�`���[�o�b�t�@�̐���
	std::vector< ComPtr <ID3D12Resource>> textureResources(_materialNum);	//	�e�N�X�`���[�o�b�t�@
	std::vector<ComPtr <ID3D12Resource>> sphResources(_materialNum);		//	sph�o�b�t�@
	std::vector<ComPtr <ID3D12Resource>> spaResources(_materialNum);		//	spa�o�b�t�@
	std::vector<ComPtr <ID3D12Resource>> toonResources(_materialNum);		//	�g�D�[���o�b�t�@
	
	for (unsigned int i = 0; i < _pmdMaterials.size(); i++)
	{
		//	�g�D�[�����\�[�X�̓ǂݍ���
		std::string tooonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(
			toonFileName,
			"toon%02d.bmp",					//	��%02~��2���̕��ő������Ȃ�������0�ɂȂ�@5��������05�ɂȂ�
			_pmdMaterials[i].toonIdx + 1
		);
		tooonFilePath += toonFileName;
		toonResources[i] = _pDxWrap->LoadTextureFromFile(tooonFilePath);

		std::string texFileName = _pmdMaterials[i].texFilePath;
		std::string spFileName = _pmdMaterials[i].texFilePath;
		//	�e�N�X�`���t�@�C���p�X���Ȃ��ꍇ
		if (strlen(texFileName.c_str()) == 0)
		{
			textureResources[i] = nullptr;
			sphResources[i] = nullptr;
			spaResources[i] = nullptr;
		}
		//	�X�v���b�^������ꍇ
		else if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namepair = Helper::SplitFileName(texFileName);
			//	�e�N�X�`���[�t�@�C���̎擾
			if (Helper::GetExtension(namepair.first) == "sph" ||
				Helper::GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spFileName = namepair.first;
			}
			else
			{
				texFileName = namepair.first;
				spFileName = namepair.second;
			}
		}
		else {}
		//	sph���܂܂��ꍇ�A�X�t�B�A�}�b�v�o�b�t�@�i��Z�j���쐬
		if (Helper::GetExtension(spFileName.c_str()) == "sph")
		{
			//	�X�t�B�A�}�b�v�o�b�t�@�̍쐬
			auto sphtexFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				spFileName.c_str()
			);
			sphResources[i] = _pDxWrap->LoadTextureFromFile(sphtexFilePath);
		}
		//	spa���܂܂�Ă���ꍇ�A�X�t�B�A�}�b�v�o�b�t�@�i���Z�j���쐬
		else if (Helper::GetExtension(spFileName.c_str()) == "spa")
		{
			//	�X�t�B�A�}�b�v�o�b�t�@�̍쐬
			auto sphtexFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				spFileName.c_str()
			);
			spaResources[i] = _pDxWrap->LoadTextureFromFile(sphtexFilePath);
		}

		if (!(Helper::GetExtension(texFileName.c_str()) == "sph" ||
			Helper::GetExtension(texFileName.c_str()) == "spa"))
		{
			//	���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
			auto texFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				texFileName.c_str()
			);

			textureResources[i] = _pDxWrap->LoadTextureFromFile(texFilePath);
		}
	}
	//	�V�F�[�_���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;								//	�t�H�[�}�b�g

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;	//	�f�[�^��RGBA�̃}�b�s���O���@
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;						//	2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;											//	�~�b�v�}�b�v�͎g�p���Ȃ��̂�1

	auto inc = _pDxWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto whiteBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::WHITE);
	auto blackBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::BLACK);
	auto graBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::GRADUATION);
	for (unsigned int i = 0; i < _materialNum; ++i)
	{
		//	�萔�o�b�t�@�i�}�e���A���j�r���[
		_pDxWrap->GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;						//	���̃������Ɉړ�
		matCBVDesc.BufferLocation += materialBuffSize;

		//	�V�F�[�_���\�[�X�r���[
		if (textureResources[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				whiteBuff.Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				textureResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		matDescHeapH.ptr += inc;					//	���̃������Ɉړ�

		//	�X�t�B�A�}�b�v(��Z�j�r���[
		if (sphResources[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				whiteBuff.Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				sphResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		matDescHeapH.ptr += inc;					//	���̃������Ɉړ�

		//	�X�t�B�A�}�b�v�i���Z�j�r���[
		if (spaResources[i] == nullptr)
		{
			srvDesc.Format = blackBuff->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				blackBuff.Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				spaResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		matDescHeapH.ptr += inc;					//	���̃������Ɉړ�

		//	�g�D�[���e�N�X�`���r���[
		if (toonResources[i] == nullptr)
		{
			srvDesc.Format = graBuff->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				graBuff.Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_pDxWrap->GetDevice()->CreateShaderResourceView(
				toonResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		matDescHeapH.ptr += inc;					//	���̃������Ɉړ�

	}

}

//	PMD�ǂݍ��ݏ���
void PMDActor::LoadPMD(const char* modelpath)
{
	//	PMD�w�b�_�[�\����
	struct PMDHeader
	{
		float version;			//	�o�[�W����
		char model_name[20];	//	���f����
		char comment[256];		//	�R�����g
	};
#pragma pack(1)	//	��������1�o�C�g�p�b�L���O�ƂȂ�A�A���C�����g�͔������Ȃ�
	//	�ǂݍ��ݗp�{�[���\����
	struct PMDBone
	{
		char boneName[20];			//	�{�[����
		unsigned short parentNo;	//	�e�{�[���ԍ�
		unsigned short nextNo;		//	��[�̃{�[���ԍ�
		unsigned char type;			//	�{�[����ʁi��]�����j
		unsigned short ikBoneNo;	//	IK�{�[���ԍ�
		DirectX::XMFLOAT3 pos;				//	�{�[���̊�_���W
	};	//	type��ɃA���C�����g���������Ă��܂����߁A1�o�C�g�p�b�L���O�ɂ���
#pragma pack()	//	�p�b�L���O�w�������

	char signature[3] = {};	//	�V�O�l�`��
	FILE* fp;
	_strModelPath = modelpath;
	//	�t�@�C���̓ǂݍ���
	auto errow = fopen_s(&fp, modelpath, "rb");

	PMDHeader pmdheader;								//	PMD�w�b�_�[
	std::vector<PMDBone> pmdBone;						//	�{�[�����

	if (errow == 0)	
	{
		Helper::DebugOutputFormatString("PMD�ǂݍ��ݐ���\n");
		//	PMD�w�b�_�[�擾
		fread(signature, sizeof(signature), 1, fp);		//	�V�O�l�`�����擾
		fread(&pmdheader, sizeof(pmdheader), 1, fp);	//	PMD�w�b�_�[�擾
		//	PMD���_���擾
		fread(&_vertNum, sizeof(_vertNum), 1, fp);		//	���_���擾
		_vertices.resize(_vertNum * pmdvertex_size);
		fread(_vertices.data(), _vertices.size(), 1, fp);	//	���_���擾
		//	PMD�C���f�b�N�X���擾
		fread(&_indicesNum, sizeof(_indicesNum), 1, fp);	//	�C���f�b�N�X���擾
		_indices.resize(_indicesNum);
		fread(_indices.data(), _indices.size() * sizeof(_indices[0]), 1, fp);	//	�C���f�b�N�X���擾
		//	PMD�}�e���A�����擾
		fread(&_materialNum, sizeof(_materialNum), 1, fp);	//	�}�e���A�����擾
		_pmdMaterials.resize(_materialNum);
		fread(_pmdMaterials.data(), _pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);	//	�}�e���A�����擾
		//	�{�[�����擾
		unsigned short boneNum = 0;
		fread(&boneNum, sizeof(boneNum), 1, fp);	//	�{�[����
		pmdBone.resize(boneNum);
		fread(pmdBone.data(), sizeof(PMDBone), boneNum, fp);	//	�{�[�����̎擾
		//	IK���擾
		uint16_t ikNum = 0;										//	IK��
		fread(&ikNum, sizeof(ikNum), 1, fp);
		_pmdIk.resize(ikNum);
		for (auto& ik : _pmdIk)
		{
			fread(&ik.IKboneIdx, sizeof(ik.IKboneIdx), 1, fp);
			fread(&ik.EndIdx, sizeof(ik.EndIdx), 1, fp);
			uint8_t chainLen = 0;	//	�Ԃɂ����m�[�h�����邩
			fread(&chainLen, sizeof(chainLen), 1, fp);
			ik.nodeIdxes.resize(chainLen);
			fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
			fread(&ik.limit, sizeof(ik.limit), 1, fp);
			//	�Ԃ̃m�[�h����0�Ȃ�΃X�L�b�v
			if (chainLen == 0)
			{
				continue;
			}
			fread(ik.nodeIdxes.data(), sizeof(ik.nodeIdxes[0]), chainLen, fp);
		}
		fclose(fp);
	}
	else
	{
		Helper::DebugOutputFormatString("PMD�ǂݍ��݃G���[\n");
	}
	_materials.resize(_pmdMaterials.size());
	//	�}�e���A�����R�s�[
	for (unsigned int i = 0; i < _pmdMaterials.size(); i++)
	{
		_materials[i].indicesNum = _pmdMaterials[i].indicesNum;
		_materials[i].material.diffuse = _pmdMaterials[i].diffuse;
		_materials[i].material.alpha = _pmdMaterials[i].alpha;
		_materials[i].material.specular = _pmdMaterials[i].specular;
		_materials[i].material.specularity = _pmdMaterials[i].specularity;
		_materials[i].material.ambient = _pmdMaterials[i].ambient;
	}
	
	//	�C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z
	std::vector<std::string> boneNames(pmdBone.size());
	_boneNameArray.resize(pmdBone.size());
	_boneNodeAddressArray.resize(pmdBone.size());
	_kneeIdxes.clear();
	//	�{�[���m�[�h�}�b�v���쐬
	for (unsigned int idx = 0; idx < pmdBone.size(); ++idx)
	{
		auto& pb = pmdBone[idx];
		boneNames[idx] = pb.boneName;
		auto& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.pos;
		//	�C���f�b�N�X���������₷���悤��
		_boneNameArray[idx] = pb.boneName;
		_boneNodeAddressArray[idx] = &node;
		//	�G�̔ԍ������W
		std::string boneName = pb.boneName;
		if (boneName.find("�Ђ�") != std::string::npos)
		{
			_kneeIdxes.emplace_back(idx);
		}
	}

	//	�e�q�֌W���\�z����
	for (auto& pb : pmdBone)
	{
		//	�e�C���f�b�N�X���`�F�b�N����(���肦�Ȃ��ԍ��Ȃ��΂��j
		if (pb.parentNo >= pmdBone.size())
		{
			continue;
		}
		auto parentName = boneNames[pb.parentNo];
		_boneNodeTable[parentName].children.emplace_back(
			&_boneNodeTable[pb.boneName]
		);
	}
	_boneMatrices.resize(pmdBone.size());
	//	�{�[�������ׂď���������
	std::fill(
		_boneMatrices.begin(),
		_boneMatrices.end(),
		XMMatrixIdentity());

	//	IK�f�o�b�O�p
	auto getNameFromIdx = [&](uint16_t idx)->std::string
	{
		auto it = std::find_if(_boneNodeTable.begin(),
			_boneNodeTable.end(),
			[idx](const std::pair<std::string, BoneNode>& obj)
			{
				return obj.second.boneIdx == idx;
			});
		if (it != _boneNodeTable.end())
		{
			return it->first;
		}
		else
		{
			return "";
		}
	};

	for (auto& ik : _pmdIk)
	{
		std::ostringstream oss;
		oss << "IK�{�[���ԍ� =" << ik.IKboneIdx << ":"
			<< getNameFromIdx(ik.IKboneIdx) << std::endl;
		oss << "�^�[�Q�b�g�i���[�j�{�[���ԍ� =" << ik.EndIdx << ":"
			<< getNameFromIdx(ik.EndIdx) << std::endl;
		for (auto& node : ik.nodeIdxes)
		{
			oss << "\t �m�[�h�{�[�� =" << node
				<< ":" << getNameFromIdx(node) << std::endl;
		}
		Helper::DebugOutputFormatString(oss.str().c_str());
	}

}
//	VMD�̓ǂݍ���
//	���̂��ɂǂ����Ń��[�V�����͕ۊǂ��Ă����Ǝv����
void PMDActor::LoadVMD(const char* vmdpath)
{
	//	VMD�t�@�C���ǂݍ��ݗp�\����
	struct VMDMotion
	{
		char boneName[15];			//	�{�[����
		//	���A���C�����g�ɂ��A�{�[�����̌�p�f�B���O�������Ă���
		unsigned int frameNo;		//	�t���[���ԍ�
		DirectX::XMFLOAT3 location;			//	�ʒu
		DirectX::XMFLOAT4 quaternion;		//	�N�H�[�^�j�I���i��]�j
		unsigned char bezier[64];	//	[4][4][4]�x�W����ԃp�����[�^
	};

#pragma pack(1)
	//	�\��f�[�^�i���_���[�t�f�[�^�j
	struct VMDMorph
	{
		char name[15];		//	���O�i�p�f�B���O����j
		uint32_t frameNo;	//	�t���[���ԍ�
		float weight;		//	�E�F�C�g�i0.0f�`1.0f�j
	};	//	�S����23�o�C�g�Ȃ̂�#pragma pack�œǂ�
#pragma pack()
#pragma pack(1)
	//	�J����
	struct VMDCamera
	{
		uint32_t frameNo;			//	�t���[���ԍ�
		float distance;				//	����
		XMFLOAT3 pos;				//	���W
		XMFLOAT3 eulerAngle;		//	�I�C���[�p
		uint8_t Interpolation[24];	//	���
		uint32_t fov;				//	���E�p
		uint8_t persFlg;			//	�p�[�X�t���OON / OFF
	};	//	61�o�C�g
#pragma pack()
	//	���C�g�Ɩ��f�[�^
	struct VMDLight
	{
		uint32_t frameNo;	//	�t���[���ԍ�
		uint8_t rgb;		//	���C�g�F
		XMFLOAT3 vec;		//	�����x�N�g���i���s�����j
	};
#pragma pack(1)
	//	�Z���t�e�f�[�^
	struct VMDSelfShadow
	{
		uint32_t frameNo;	//	�t���[���ԍ�
		uint8_t mode;		//	�e���[�h�i0:�e�Ȃ��A1:���[�h1�A2:���[�h2�j
		float distance;		//	����
	};
#pragma pack()

	FILE* fp;
	//	�t�@�C���̓ǂݍ���
	auto errow = fopen_s(&fp, vmdpath, "rb");
	if (errow == 0)
	{
		Helper::DebugOutputFormatString("VMD�ǂݍ��ݐ���\n");
		//	VMD�t�@�C���ǂݍ���
		fseek(fp, 50, SEEK_SET);	//	�ŏ���50�o�C�g�̓X�L�b�v
		unsigned int motionDataNum = 0;
		fread(&motionDataNum, sizeof(motionDataNum), 1, fp);
		std::vector<VMDMotion> vmdMotionData(motionDataNum);

		for (auto& motion : vmdMotionData)
		{
			//	���p�f�B���O�������Ă��邽�߁A�{�[�����Ƃق��̕ϐ��͂킯�ēǂݍ���ł���
			fread(motion.boneName, sizeof(motion.boneName), 1, fp);	//	�{�[����
			fread(&motion.frameNo,
				sizeof(motion.frameNo)		//	�t���[���ԍ�
				+ sizeof(motion.location)	//	�ʒu
				+ sizeof(motion.quaternion)	//	�N�H�[�^�j�I��
				+ sizeof(motion.bezier),	//	��ԃx�W���f�[�^
				1, fp);

			_duration = std::max<unsigned int>(_duration, motion.frameNo);
		}
		//	VMD�̃��[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
		for (auto& vmdMotion : vmdMotionData)
		{
			XMVECTOR quaternion = XMLoadFloat4(&vmdMotion.quaternion);
			_motiondata[vmdMotion.boneName].emplace_back(
				KeyFrame(vmdMotion.frameNo,
					quaternion,
					vmdMotion.location,
					XMFLOAT2((float)vmdMotion.bezier[3] / 127.0f,(float)vmdMotion.bezier[7] / 127.0f),
					XMFLOAT2((float)vmdMotion.bezier[11] / 127.0f, (float)vmdMotion.bezier[15] / 127.0f))
			);
		}
		//	���[�V�����e�[�u���̊e�L�[�t���[�����t���[���̊�ɏ�������悤�\�[�g
		for (auto& motion : _motiondata)
		{
			std::sort(motion.second.begin(), motion.second.end(),
				[](const KeyFrame& lval, const KeyFrame& rval)
				{
					return lval.frameNo <= rval.frameNo;
				});
		}

		//	�\��f�[�^�擾
		uint32_t morphCount = 0;
		fread(&morphCount, sizeof(morphCount), 1, fp);
		std::vector<VMDMorph> morphs(morphCount);
		fread(morphs.data(), sizeof(VMDMorph), morphCount, fp);

		//	�J�����f�[�^�擾
		uint32_t vmdCameraCount = 0;
		fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);
		std::vector<VMDCamera> cameraData(vmdCameraCount);
		fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, fp);

		//	���C�g�Ɩ��f�[�^�擾
		uint32_t vmdLightCount = 0;
		fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);
		std::vector<VMDLight> lights(vmdLightCount);
		fread(lights.data(), sizeof(VMDLight), vmdLightCount, fp);

		//	�Z���t�e�f�[�^�擾
		uint32_t SelfShadowCount = 0; 
		fread(&SelfShadowCount, sizeof(SelfShadowCount), 1, fp);
		std::vector<VMDSelfShadow> selfShadowData(SelfShadowCount);
		fread(selfShadowData.data(), sizeof(VMDSelfShadow), SelfShadowCount, fp);

		//	IK�I���I�t�f�[�^�擾
		uint32_t ikSwitchCount = 0;
		fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);
		_ikEnableData.resize(ikSwitchCount);

		for (auto& ikEnable : _ikEnableData)
		{
			//	�L�[�t���[�����Ȃ̂ł܂��̓t���[���ԍ��ǂ݂���
			fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);
			//	���t���O�̎擾
			uint8_t visibleFlg = 0;
			fread(&visibleFlg, sizeof(visibleFlg), 1, fp);
			//	�Ώۃ{�[�����ǂݍ���
			uint32_t ikBoneCount = 0;
			fread(&ikBoneCount, sizeof(ikBoneCount), 1, fp);
			//	���[�v�����O��ON/OFF�����擾
			for (unsigned int i = 0; i < ikBoneCount; ++i)
			{
				//	�{�[�����擾
				char ikBoneName[20];
				fread(ikBoneName, _countof(ikBoneName), 1, fp);
				//	ON/OFF���擾
				uint8_t flg = 0;
				fread(&flg, sizeof(flg), 1, fp);
				ikEnable.ikEnableTable[ikBoneName] = flg;
			}
		}

		fclose(fp);
	}
	else
	{
		Helper::DebugOutputFormatString("VMD�ǂݍ��݃G���[\n");
	}
}

//	�x�W���Ȑ���Y���擾���鏈��
float PMDActor::GetYFromXOnBezier(
	float x,											//	�ω��ʁix�j
	const DirectX::XMFLOAT2& a, DirectX::XMFLOAT2& b,	//	���ԃR���g���[���|�C���g
	uint8_t n)											//	���s��
{
	//	���ԃR���g���[���|�C���g�̈ʒu��x��y�œ����̏ꍇ
	if (a.x == a.y && b.x == b.y)
	{
		return x;	//	�v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	//	t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;		//	t^2�̌W��
	const float k2 = 3 * a.x;				//	t�̌W��

	//	�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	//	t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i)
	{
		//	f(t)�����߂�
		auto ft = k0 * (t * t * t) + k1 * (t * t) + k2 * (t) - x;
		//	�������ʂ�0�ɋ߂��i�덷�͈͓̔��j�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}
		t -= ft / 2;	//	����
	}

	//	���߂���t�͂��łɋ��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	//	��������̎��̗��ĕ����l����
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

//	�e�{�[���ɍs��̕ύX�𔽉f������i�ċN�����j
void PMDActor::RecusiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	_boneMatrices[node->boneIdx] *= mat;
	for (auto& cnode : node->children)
	{
		RecusiveMatrixMultiply(cnode, _boneMatrices[node->boneIdx]);
	}
}

//	16�o�C�g���E�Ɋm�ۂ���
void* PMDActor::Transform::operator new(size_t size)
{
	return _aligned_malloc(size,16);
}
