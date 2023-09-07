//	インクルード
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

//	ライブラリリンク
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"winmm.lib")

//	定数
constexpr size_t pmdvertex_size = 38;				//	頂点一つ当たりのサイズ

namespace//	無名名前空間
{
	//	ボーン種別
	enum class BoneType
	{
		Rotation,		//	回転
		RotAndMove,		//	回転&移動
		IK,				//	IK
		Undefined,		//	未定義
		IKChile,		//	IK影響ボーン
		RotationChild,	//	回転影響ボーン
		IKDestination,	//	IK接続先
		Invisible		//	見えないボーン
	};
}

using namespace DirectX;
using namespace Microsoft::WRL;

//	コンストラクタ
PMDActor::PMDActor(Dx12Wrapper* pDxWrap, const char* modelpath, const char* vmdpath, DirectX::XMFLOAT3 position):
	_pDxWrap(pDxWrap), _position(position)
{
	//	PMDの読み込み
	LoadPMD(modelpath);
	//	VMDファイル読み込み
	LoadVMD(vmdpath);
}

//	初期化処理
void PMDActor::Init(void)
{
	//	頂点・インデックスバッファの作成
	CreateVertex_IdxView();

	//	位置座標バッファの作成
	CreateTransformView();

	//	マテリアルバッファの作成
	CreateMaterialView();

	//	アニメーションプレイ
	PlayAnimation();
}

//	更新処理
void PMDActor::Update(void)
{
	//	オブジェクトの回転処理
	//_angle += 0.001f;
	//_mappedMatrices[0] = XMMatrixRotationY(_angle);

	//	モーション更新処理
	MotionUpdate();
}

//	描画処理
void PMDActor::Draw(void)
{
	//	座標変換用ディスクリプタヒープの指定
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_BasicDescHeap.GetAddressOf()		//	座標変換用ディスクリプタヒープ
	);

	//	ルートパラメータとディスクリプタヒープの関連付け
	auto heapHandle = _BasicDescHeap->GetGPUDescriptorHandleForHeapStart();
	//	定数バッファ1ビュー用の指定
	_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
		1,			//	ルートパラメータインデックス
		heapHandle	//	ヒープアドレス
	);

	//	プリミティブポロジをセット
	_pDxWrap->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	頂点バッファをセット
	_pDxWrap->GetCmdList()->IASetVertexBuffers(
		0,			//	スロット番号
		1, 			//	頂点バッファビューの数
		&_vbView	//	頂点バッファビューオブジェ
	);

	//	インデックスバッファをセット
	_pDxWrap->GetCmdList()->IASetIndexBuffer(&_ibView);

	auto materialH = _materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;
	auto cbvsrvIncSize = _pDxWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	//	マテリアル用ディスクリプタヒープの指定
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_materialDescHeap.GetAddressOf()	//	マテリアル用用ディスクリプタヒープ
	);
	for (auto& m : _materials)
	{
		//	マテリアル、テクスチャービュー用の指定
		_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
			2,			//	ルートパラメータインデックス
			materialH	//	ヒープアドレス
		);
		//	ポリゴンの描画命令
		_pDxWrap->GetCmdList()->DrawIndexedInstanced(
			m.indicesNum,	//	インデックス数			
			//2,				//	インスタンス数（ポリゴン数）1:通常のモデル描画、2:地面影
			1,
			idxOffset,		//	インデックスのオフセット
			0,				//	頂点データのオフセット
			0				//	インスタンスのオフセット
		);
		//	ヒープポインターとインデックスを次に進める
		materialH.ptr += cbvsrvIncSize;	//	次のビューのため、2倍進める
		idxOffset += m.indicesNum;

	}
}

//	描画処理
void PMDActor::ShadowMapDraw(void)
{
	//	座標変換用ディスクリプタヒープの指定
	_pDxWrap->GetCmdList()->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_BasicDescHeap.GetAddressOf()		//	座標変換用ディスクリプタヒープ
	);

	//	ルートパラメータとディスクリプタヒープの関連付け
	auto heapHandle = _BasicDescHeap->GetGPUDescriptorHandleForHeapStart();
	//	定数バッファ1ビュー用の指定
	_pDxWrap->GetCmdList()->SetGraphicsRootDescriptorTable(
		1,			//	ルートパラメータインデックス
		heapHandle	//	ヒープアドレス
	);

	//	プリミティブポロジをセット
	_pDxWrap->GetCmdList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	頂点バッファをセット
	_pDxWrap->GetCmdList()->IASetVertexBuffers(
		0,			//	スロット番号
		1, 			//	頂点バッファビューの数
		&_vbView	//	頂点バッファビューオブジェ
	);

	//	インデックスバッファをセット
	_pDxWrap->GetCmdList()->IASetIndexBuffer(&_ibView);
	//	ポリゴンの描画命令
	_pDxWrap->GetCmdList()->DrawIndexedInstanced(
		_indicesNum,	//	インデックス数			
		1,				//	インスタンス数（ポリゴン数）
		0,		//	インデックスのオフセット
		0,				//	頂点データのオフセット
		0				//	インスタンスのオフセット
	);

}

//	オブジェクトの解放処理
void PMDActor::Release(void)
{

}

//	アニメーション開始処理
void PMDActor::PlayAnimation(void)
{
	_startTime = timeGetTime();	//	現在の時間取得
}

//	モーション更新処理
void PMDActor::MotionUpdate(void)
{
	DWORD elapsedTime = timeGetTime() - _startTime;	//	経過時間を測る
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);
	//	現在のフレーム数が最終フレーム超過なら
	if (frameNo > _duration)
	{
		_startTime = timeGetTime();
		frameNo = 0;
	}

	//	FK処理
	FKUpdate(frameNo);
	//	IK処理
	IKSolve(frameNo);

	//	更新された行列をコピー
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);	//	ボーン行列をコピー

}

void PMDActor::FKUpdate(const unsigned int& frameNo)
{
	//	行列情報クリア
	//	(クリアしていないと前フレームのポーズが重ね掛けされて
	//	モデルが壊れる）
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//	モーションデータ更新
	for (auto& bonemotion : _motiondata)
	{
		//	ボーンテーブル内に指定されたボーン名は存在していなければスキップ
		auto itBoneNode = _boneNodeTable.find(bonemotion.first);
		if (itBoneNode == _boneNodeTable.end())
		{
			continue;
		}

		auto node = _boneNodeTable[bonemotion.first];
		//	合致するものを探す
		auto motions = bonemotion.second;
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const KeyFrame& keyframe)
			{
				return keyframe.frameNo <= frameNo;
			}
		);

		//	合致するものがなければ処理をスキップ
		if (rit == motions.rend())
		{
			continue;
		}

		//	アニメーションの補間処理
		XMMATRIX rotation;								//	回転
		XMVECTOR offset = XMLoadFloat3(&rit->offset);	//	オフセット位置
		auto it = rit.base();	//	通常のイテレーター（元のデータ向き）に戻す
		if (it != motions.end())
		{
			//	現在のアニメーション影響度
			auto t = static_cast<float>(frameNo - rit->frameNo)		//	（現在のフレーム - 前回のフレーム）
				/ static_cast<float>(it->frameNo - rit->frameNo);	//	/　（次のフレーム - 前回のフレーム）
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);
			//	最終的な回転行列(線形補間）
			//rotation =
			//	XMMatrixRotationQuaternion(rit->quaternion) * (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion) * t;

			//	最終的な回転行列(球面線形補間）
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
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)	//	ボーン基準点を原点へ移動する
			* rotation									//	回転
			* XMMatrixTranslation(pos.x, pos.y, pos.z);	//	ボーンを元の基準点に戻す
		_boneMatrices[node.boneIdx] = mat * XMMatrixTranslationFromVector(offset);
	}
	//	各ボーンの行列を更新する
	RecusiveMatrixMultiply(&_boneNodeTable["センター"], XMMatrixIdentity());
}

//	IK処理
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
		//	ikが存在しているか
		if (it != _ikEnableData.rend())
		{
			//	IKボーンを取得
			auto ikEnableIt =
				it->ikEnableTable.find(_boneNameArray[ik.IKboneIdx]);
			//	ikボーンが存在しているか
			if (ikEnableIt != it->ikEnableTable.end())
			{
				//	フラグがOFFならスキップ
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
		case 1:	//	間のボーン数が1の時はLookAt
			SolveLookAt(ik);
			break;
		case 2:	//	間のボーン数が2の時は余弦定理IK
			SolveCosineIK(ik);
			break;
		default:	//	間のボーン数が3以上の時はCCD-IK
			SolveCCDIK(ik);
			break;
		}
	}
}

constexpr float epsilon = 0.0005f;

//	CCD-IKによりボーン方向を解決
//	@param ik 対象ikオブジェクト
void PMDActor::SolveCCDIK(const PMDIK& ik)
{
	//	ターゲット
	auto targetBoneNode = _boneNodeAddressArray[ik.IKboneIdx];
	auto targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	//	親座標を逆行列で無効にする
	auto& paremMat =
		_boneMatrices[_boneNodeAddressArray[ik.IKboneIdx]->ikParentBone];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, paremMat);
	auto targetNextPos = XMVector3Transform(
		targetOriginPos, _boneMatrices[ik.IKboneIdx] * invParentMat);

	//	末端ノード
	auto endPos = XMLoadFloat3(
		&_boneNodeAddressArray[ik.EndIdx]->startPos);
	
	//	中間ノード、rootノード
	std::vector<XMVECTOR> positions;
	for (auto& cidx : ik.nodeIdxes)
	{
		positions.emplace_back(
			XMLoadFloat3(&_boneNodeAddressArray[cidx]->startPos));
	}

	//	各ボーンの行列（末端以外）
	std::vector<XMMATRIX> mats(positions.size());	
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

	auto ikLimit = ik.limit * XM_PI;	//	ikの回転制限

	//	ikに設定されている試行回数だけ繰り返す
	for (int c = 0; c < ik.iterations; ++c)
	{
		//	ターゲットと末端がほぼ一致したら抜ける
		if (XMVector3Length(
			XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		//	それぞれのボーンをさかのぼりながら、
		//	角度制限に引っ掛からないように曲げていく

		//	末端の一つ前～ルートボーンまで
		for (unsigned int bidx = 0; bidx < positions.size(); ++bidx)
		{
			const auto& pos = positions[bidx];
			auto vecToEnd = XMVectorSubtract(endPos, pos);				//	対象ノードから末端ノードまでのベクトル
			auto vecToTarget = XMVectorSubtract(targetNextPos, pos);	//	対象ノードからターゲットノードまでのベクトル
			//	上のベクトル2つ正規化
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			//	ほぼ同じベクトルの場合、外積ができないためスキップ
			if (XMVector3Length(
				XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0]
				<= epsilon)
			{
				continue;
			}

			//	外積計算および角度計算
			auto cross = XMVector3Normalize(
				XMVector3Cross(vecToEnd, vecToTarget));	//	軸になる
			//	cos(内積値)なので、0~90°、0~-90°の区別がない
			float angle = XMVector3AngleBetweenVectors(
				vecToEnd, vecToTarget).m128_f32[0];
			//	回転限界を超えてしまったときは限界値に補正
			angle = min(angle, ikLimit);
			XMMATRIX rot =
				XMMatrixRotationAxis(cross, angle);	//	回転行列作成

			//	原点中心ではなく、pos中心に回転する行列を作成する
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);
			//	回転行列を保持する（乗算で回転重ね掛けを作っておく）
			mats[bidx] *= mat;

			//	対象となる点をすべて回転させる（現在の点からみて末端側を回転）
			//	自分は回転させる必要はない
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				positions[idx] =
					XMVector3Transform(positions[idx], mat);
			}
			endPos = XMVector3Transform(endPos, mat);

			//	ターゲットに近くなったらループを抜ける
			if (XMVector3Length(
				XMVectorSubtract(endPos, targetNextPos)).m128_f32[0]
				<= epsilon)
			{
				break;
			}
		}
	}

	//	mats配列は末端の一つ前ボーン～ルートボーン
	//	ik.nodeIdxesはボーンIDが格納されている
	int idx = 0;
	for (auto& cidx : ik.nodeIdxes)
	{
		_boneMatrices[cidx] = mats[idx];
		++idx;
	}

	auto rootNode = _boneNodeAddressArray[ik.nodeIdxes.back()];	//	ルートボーン取得
	RecusiveMatrixMultiply(rootNode, paremMat);
}

//	余弦定理IKによりボーン方向を解決
//	@param ik 対象ikオブジェクト
void PMDActor::SolveCosineIK(const PMDIK& ik)
{
	std::vector<XMVECTOR> positions;	//	IK構成点を保存
	std::array<float, 2> edgeLens;		//	IKのそれぞれのボーン間の距離を保存

	//	ターゲット(目標ボーンを取得)
	auto& targetNode = _boneNodeAddressArray[ik.IKboneIdx];
	auto targetPos = XMVector3Transform(
		XMLoadFloat3(&targetNode->startPos),
		_boneMatrices[ik.IKboneIdx]);

	//	末端ボーン取得
	auto endNode = _boneNodeAddressArray[ik.EndIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	//	中間及びルートボーン取得
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = _boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	//	現在、positionsが「末端～ルートボーン」になっているので、逆にする
	std::reverse(positions.begin(), positions.end());

	//	元の長さを測っておく
	edgeLens[0] = XMVector3Length(
		XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(
		XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	//	ルートボーン座標変換
	positions[0] = XMVector3Transform(
		positions[0], _boneMatrices[ik.nodeIdxes[1]]);

	//	真ん中のボーンは自動計算されるから計算しない

	//	先端ボーン
	positions[2] = XMVector3Transform(
		positions[2], _boneMatrices[ik.IKboneIdx]);

	//	rootから先端へのベクトルを作る
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float A = XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	//	rootから真ん中への角度計算
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));
	//	真ん中からターゲットへの角度計算
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	//	軸を求める
	//	※軸が零ベクトルになる可能性がある？
	XMVECTOR axis;
	//	「ひざ」ではない場合
	if (std::find(_kneeIdxes.begin(), _kneeIdxes.end(),
		ik.nodeIdxes[0]) == _kneeIdxes.end())
	{
		auto vm = XMVector3Normalize(
			XMVectorSubtract(positions[2], positions[0]));
		auto vt = XMVector3Normalize(
			XMVectorSubtract(targetPos, positions[0]));
		axis = XMVector3Cross(vt, vm);
	}
	//	真ん中が「ひざ」であった場合には強制的にX軸にする
	else
	{
		auto right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	//	注意点：IKチェーンはルートに向かってから数えられるため1がrootに近い
	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2-XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);
	
	_boneMatrices[ik.nodeIdxes[1]] *= mat1;	//	ルートボーン
	_boneMatrices[ik.nodeIdxes[0]] = mat2 * _boneMatrices[ik.nodeIdxes[1]];	//	中間ボーン
	//	※なぜ、末端ボーンに中間ボーンの行列情報をコピーしているのか？
	_boneMatrices[ik.EndIdx] = _boneMatrices[ik.nodeIdxes[0]];	//	末端ボーン
}

//	LookAt行列によりボーン方向を解決
//	@param ik 対象ikオブジェクト
void PMDActor::SolveLookAt(const PMDIK& ik)
{
	//	この関数に来た時点でノードは1つしかなく
	//	チェーンに入っているノード番号はIKのルートノードのものなので、
	//	このルートノードから末端に向かうベクトルを考える
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

//	特定のベクトルを特定の方向に向ける行列を返す関数
//	@param origin 特定のベクトル
//	@param lookat 向かせたい方向ベクトル
//	@param up 上ベクトル
//	@param right 右ベクトル
DirectX::XMMATRIX PMDActor::LookAtMatrix(
	const DirectX::XMVECTOR& origin,
	const DirectX::XMVECTOR& lookat, 
	DirectX::XMFLOAT3& up,
	DirectX::XMFLOAT3& right)
{
	return XMMatrixTranspose(LookAtMatrix(origin,up,right))
		* LookAtMatrix(lookat,up,right);
}
//	z軸を特定の方向に向ける行列を返す関数
//	@param lookaat 向かせたい方向ベクトル
//	@param up 上ベクトル
//	@param right 右ベクトル
DirectX::XMMATRIX PMDActor::LookAtMatrix(
	const DirectX::XMVECTOR& lookat,
	DirectX::XMFLOAT3& up, 
	DirectX::XMFLOAT3& right)
{
	//	向かせる方向(z軸)
	XMVECTOR vz = lookat;
	//	向かせたい方向を向かせたときの）仮のy軸
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));
	//	仮のy軸とz軸の外積ベクトル（x軸）
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	//	（向かせたい方向を向かせたときの）y軸
	vy = XMVector3Normalize(XMVector3Cross(vz, vx));

	//	LookAtとUpが同じ方向を向いていたらrightを基準にして作り直す
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		//	仮のx方向を定義
		vx = XMVector3Normalize(XMLoadFloat3(&right));
		//	向かせたい方向を向かせたときのy軸
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));
		//	真のx軸
		vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	}

	//	各軸を行列に変換
	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = vx;
	ret.r[1] = vy;
	ret.r[2] = vz;
	return ret;
}

//	頂点・インデックスバッファの作成
void PMDActor::CreateVertex_IdxView(void)
{
	//	ヒープ設定
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	リソース設定
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(_vertices.size());
	//	頂点バッファの作成
	auto result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&heapprop,							//	ヒープ設定
		D3D12_HEAP_FLAG_NONE,				//	ヒープフラグ
		&resDesc,							//	リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,	//	GPUからの読み取り専用
		nullptr,							//	特定のリソースのクリア操作
		IID_PPV_ARGS(_vertBuff.ReleaseAndGetAddressOf())				//	IDと頂点バッファ
	);

	//	頂点バッファへ頂点情報をコピーする
	unsigned char* vertMap = nullptr;
	result = _vertBuff->Map(0, nullptr, (void**)&vertMap);			//	頂点情報を頂点バッファに渡す
	std::copy(std::begin(_vertices), std::end(_vertices), vertMap);	//	頂点情報に頂点座標をコピーする
	_vertBuff->Unmap(0, nullptr);									//	マップを解除する

	//	頂点バッファビューの作成
	_vbView.BufferLocation = _vertBuff->GetGPUVirtualAddress();	//	バッファーの仮想アドレス
	_vbView.SizeInBytes = _vertices.size();						//	全バイト数
	_vbView.StrideInBytes = pmdvertex_size;					//	1頂点当たりのバイト数

	//	インデックスバッファの作成	//	
	CD3DX12_RESOURCE_DESC idxDesc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)_indices.size() * (UINT64)sizeof(_indices[0]));
	result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&idxDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_idxBuff.ReleaseAndGetAddressOf())
	);

	//	インデックスバッファへインデックス情報をコピーする
	unsigned short* mappedIdx = nullptr;
	//	インデックス情報をインデックスバッファに渡す
	_idxBuff->Map(
		0,
		nullptr,
		(void**)&mappedIdx
	);
	std::copy(std::begin(_indices), std::end(_indices), mappedIdx);	//	インデックス情報にインデックスをコピーする
	_idxBuff->Unmap(0, nullptr);										//	マップを開放する

	//	インデックスバッファビューを作成
	_ibView.BufferLocation = _idxBuff->GetGPUVirtualAddress();		//	バッファの仮想アドレス
	_ibView.Format = DXGI_FORMAT_R16_UINT;							//	フォーマット（unsigned short(16バイト）なのでR16_UINT）
	_ibView.SizeInBytes = _indices.size() * sizeof(_indices[0]);	//	全バイト数
}

//	位置座標バッファの作成
void PMDActor::CreateTransformView(void)
{
	//	ヒーププロパティー設定
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	座標変換用ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	シェーダから見えるように
	descHeapDesc.NodeMask = 0;										//	マスク
	descHeapDesc.NumDescriptors = 1;								//	定数バッファビュー（CBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	シェーダリソースビュー用
	auto result = _pDxWrap->GetDevice()->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_BasicDescHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		assert(SUCCEEDED(result));
	}

	//	定数バッファの作成	//
	//	定数バッファの生成（座標変換）
	auto buffSize = sizeof(XMMATRIX) * (1 + _boneMatrices.size());	//	「＋1]はワールド行列分
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

	//	位置座標の設定
	result = _constBuff->Map(0, nullptr, (void**)&_mappedMatrices);	//	マップ
	_mappedMatrices[0] = XMMatrixRotationY(0) * XMMatrixTranslation(_position.x,_position.y,_position.z);									//	ワールド行列の内容をコピー
	
	//	ボーン行列の設定
	for (auto& bonemotion : _motiondata)
	{
		auto node = _boneNodeTable[bonemotion.first];
		auto& pos = node.startPos;
		auto mat = 
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)	//	ボーン基準点を原点へ移動する
			* XMMatrixRotationQuaternion(bonemotion.second[0].quaternion)
			* XMMatrixTranslation(pos.x, pos.y, pos.z);	//	ボーンを元の基準点に戻す
		_boneMatrices[node.boneIdx] = mat;
	}

	RecusiveMatrixMultiply(&_boneNodeTable["センター"], XMMatrixIdentity());
	std::copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);	//	ボーン行列をコピー

	//	定数バッファビューの作成（座標変換）
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	auto basicHeapHandle = _BasicDescHeap->GetCPUDescriptorHandleForHeapStart();	//	ヒープの先頭を取得する
	_pDxWrap->GetDevice()->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
}

//	マテリアルバッファの作成
void PMDActor::CreateMaterialView(void)
{
	//	マテリアルバッファの作成	//
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;	//	アライメントに合わせる
	//	マテリアルバッファのヒープ詳細
	CD3DX12_HEAP_PROPERTIES materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	マテリアルバッファの設定
	CD3DX12_RESOURCE_DESC materialresDesc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)materialBuffSize * (UINT64)_materialNum);
	auto result = _pDxWrap->GetDevice()->CreateCommittedResource(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf())
	);
	//	マップ
	char* mapMaterial = nullptr;
	_materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto& m : _materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;	//	マテリアル情報のデータコピー
		mapMaterial += materialBuffSize;				//	次のアライメント位置まで進める（256の倍数）
	}
	_materialBuff->Unmap(0, nullptr);
	//	マテリアル用ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = _materialNum * 5;	//	マテリアル数×素材（マテリアル、テクスチャ、乗算スフィアマップ、加算スフィアマップ、トゥーンテクスチャ）のディスクリプタがある
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _pDxWrap->GetDevice()->CreateDescriptorHeap(
		&matDescHeapDesc,
		IID_PPV_ARGS(_materialDescHeap.ReleaseAndGetAddressOf())
	);
	//	マテリアル用定数バッファビュー設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = _materialBuff->GetGPUVirtualAddress();			//	バッファアドレス
	matCBVDesc.SizeInBytes = materialBuffSize;									//	バッファサイズ
	auto matDescHeapH = _materialDescHeap->GetCPUDescriptorHandleForHeapStart();	//	先頭の記録

	//	テクスチャーバッファの生成
	std::vector< ComPtr <ID3D12Resource>> textureResources(_materialNum);	//	テクスチャーバッファ
	std::vector<ComPtr <ID3D12Resource>> sphResources(_materialNum);		//	sphバッファ
	std::vector<ComPtr <ID3D12Resource>> spaResources(_materialNum);		//	spaバッファ
	std::vector<ComPtr <ID3D12Resource>> toonResources(_materialNum);		//	トゥーンバッファ
	
	for (unsigned int i = 0; i < _pmdMaterials.size(); i++)
	{
		//	トゥーンリソースの読み込み
		std::string tooonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(
			toonFileName,
			"toon%02d.bmp",					//	※%02~は2桁の幅で代入されない部分は0になる　5だったら05になる
			_pmdMaterials[i].toonIdx + 1
		);
		tooonFilePath += toonFileName;
		toonResources[i] = _pDxWrap->LoadTextureFromFile(tooonFilePath);

		std::string texFileName = _pmdMaterials[i].texFilePath;
		std::string spFileName = _pmdMaterials[i].texFilePath;
		//	テクスチャファイルパスがない場合
		if (strlen(texFileName.c_str()) == 0)
		{
			textureResources[i] = nullptr;
			sphResources[i] = nullptr;
			spaResources[i] = nullptr;
		}
		//	スプリッタがある場合
		else if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namepair = Helper::SplitFileName(texFileName);
			//	テクスチャーファイルの取得
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
		//	sphが含まれる場合、スフィアマップバッファ（乗算）を作成
		if (Helper::GetExtension(spFileName.c_str()) == "sph")
		{
			//	スフィアマップバッファの作成
			auto sphtexFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				spFileName.c_str()
			);
			sphResources[i] = _pDxWrap->LoadTextureFromFile(sphtexFilePath);
		}
		//	spaが含まれている場合、スフィアマップバッファ（加算）を作成
		else if (Helper::GetExtension(spFileName.c_str()) == "spa")
		{
			//	スフィアマップバッファの作成
			auto sphtexFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				spFileName.c_str()
			);
			spaResources[i] = _pDxWrap->LoadTextureFromFile(sphtexFilePath);
		}

		if (!(Helper::GetExtension(texFileName.c_str()) == "sph" ||
			Helper::GetExtension(texFileName.c_str()) == "spa"))
		{
			//	モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
			auto texFilePath = Helper::GetTexturePathFromModelAndTexPath(
				_strModelPath,
				texFileName.c_str()
			);

			textureResources[i] = _pDxWrap->LoadTextureFromFile(texFilePath);
		}
	}
	//	シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;								//	フォーマット

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;	//	データのRGBAのマッピング方法
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;						//	2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;											//	ミップマップは使用しないので1

	auto inc = _pDxWrap->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto whiteBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::WHITE);
	auto blackBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::BLACK);
	auto graBuff = _pDxWrap->GetNoneTexture(Dx12Wrapper::E_NONETEX::GRADUATION);
	for (unsigned int i = 0; i < _materialNum; ++i)
	{
		//	定数バッファ（マテリアル）ビュー
		_pDxWrap->GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;						//	次のメモリに移動
		matCBVDesc.BufferLocation += materialBuffSize;

		//	シェーダリソースビュー
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
		matDescHeapH.ptr += inc;					//	次のメモリに移動

		//	スフィアマップ(乗算）ビュー
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
		matDescHeapH.ptr += inc;					//	次のメモリに移動

		//	スフィアマップ（加算）ビュー
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
		matDescHeapH.ptr += inc;					//	次のメモリに移動

		//	トゥーンテクスチャビュー
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
		matDescHeapH.ptr += inc;					//	次のメモリに移動

	}

}

//	PMD読み込み処理
void PMDActor::LoadPMD(const char* modelpath)
{
	//	PMDヘッダー構造体
	struct PMDHeader
	{
		float version;			//	バージョン
		char model_name[20];	//	モデル名
		char comment[256];		//	コメント
	};
#pragma pack(1)	//	ここから1バイトパッキングとなり、アライメントは発生しない
	//	読み込み用ボーン構造体
	struct PMDBone
	{
		char boneName[20];			//	ボーン名
		unsigned short parentNo;	//	親ボーン番号
		unsigned short nextNo;		//	先端のボーン番号
		unsigned char type;			//	ボーン種別（回転だけ）
		unsigned short ikBoneNo;	//	IKボーン番号
		DirectX::XMFLOAT3 pos;				//	ボーンの基準点座標
	};	//	type後にアライメントが発生してしまうため、1バイトパッキングにする
#pragma pack()	//	パッキング指定を解除

	char signature[3] = {};	//	シグネチャ
	FILE* fp;
	_strModelPath = modelpath;
	//	ファイルの読み込み
	auto errow = fopen_s(&fp, modelpath, "rb");

	PMDHeader pmdheader;								//	PMDヘッダー
	std::vector<PMDBone> pmdBone;						//	ボーン情報

	if (errow == 0)	
	{
		Helper::DebugOutputFormatString("PMD読み込み成功\n");
		//	PMDヘッダー取得
		fread(signature, sizeof(signature), 1, fp);		//	シグネチャを取得
		fread(&pmdheader, sizeof(pmdheader), 1, fp);	//	PMDヘッダー取得
		//	PMD頂点情報取得
		fread(&_vertNum, sizeof(_vertNum), 1, fp);		//	頂点数取得
		_vertices.resize(_vertNum * pmdvertex_size);
		fread(_vertices.data(), _vertices.size(), 1, fp);	//	頂点情報取得
		//	PMDインデックス情報取得
		fread(&_indicesNum, sizeof(_indicesNum), 1, fp);	//	インデックス数取得
		_indices.resize(_indicesNum);
		fread(_indices.data(), _indices.size() * sizeof(_indices[0]), 1, fp);	//	インデックス情報取得
		//	PMDマテリアル情報取得
		fread(&_materialNum, sizeof(_materialNum), 1, fp);	//	マテリアル数取得
		_pmdMaterials.resize(_materialNum);
		fread(_pmdMaterials.data(), _pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);	//	マテリアル情報取得
		//	ボーン情報取得
		unsigned short boneNum = 0;
		fread(&boneNum, sizeof(boneNum), 1, fp);	//	ボーン数
		pmdBone.resize(boneNum);
		fread(pmdBone.data(), sizeof(PMDBone), boneNum, fp);	//	ボーン情報の取得
		//	IK情報取得
		uint16_t ikNum = 0;										//	IK数
		fread(&ikNum, sizeof(ikNum), 1, fp);
		_pmdIk.resize(ikNum);
		for (auto& ik : _pmdIk)
		{
			fread(&ik.IKboneIdx, sizeof(ik.IKboneIdx), 1, fp);
			fread(&ik.EndIdx, sizeof(ik.EndIdx), 1, fp);
			uint8_t chainLen = 0;	//	間にいくつノードがあるか
			fread(&chainLen, sizeof(chainLen), 1, fp);
			ik.nodeIdxes.resize(chainLen);
			fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
			fread(&ik.limit, sizeof(ik.limit), 1, fp);
			//	間のノード数が0ならばスキップ
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
		Helper::DebugOutputFormatString("PMD読み込みエラー\n");
	}
	_materials.resize(_pmdMaterials.size());
	//	マテリアル情報コピー
	for (unsigned int i = 0; i < _pmdMaterials.size(); i++)
	{
		_materials[i].indicesNum = _pmdMaterials[i].indicesNum;
		_materials[i].material.diffuse = _pmdMaterials[i].diffuse;
		_materials[i].material.alpha = _pmdMaterials[i].alpha;
		_materials[i].material.specular = _pmdMaterials[i].specular;
		_materials[i].material.specularity = _pmdMaterials[i].specularity;
		_materials[i].material.ambient = _pmdMaterials[i].ambient;
	}
	
	//	インデックスと名前の対応関係構築
	std::vector<std::string> boneNames(pmdBone.size());
	_boneNameArray.resize(pmdBone.size());
	_boneNodeAddressArray.resize(pmdBone.size());
	_kneeIdxes.clear();
	//	ボーンノードマップを作成
	for (unsigned int idx = 0; idx < pmdBone.size(); ++idx)
	{
		auto& pb = pmdBone[idx];
		boneNames[idx] = pb.boneName;
		auto& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.pos;
		//	インデックス検索がしやすいように
		_boneNameArray[idx] = pb.boneName;
		_boneNodeAddressArray[idx] = &node;
		//	膝の番号を収集
		std::string boneName = pb.boneName;
		if (boneName.find("ひざ") != std::string::npos)
		{
			_kneeIdxes.emplace_back(idx);
		}
	}

	//	親子関係を構築する
	for (auto& pb : pmdBone)
	{
		//	親インデックスをチェックする(ありえない番号なら飛ばす）
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
	//	ボーンをすべて初期化する
	std::fill(
		_boneMatrices.begin(),
		_boneMatrices.end(),
		XMMatrixIdentity());

	//	IKデバッグ用
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
		oss << "IKボーン番号 =" << ik.IKboneIdx << ":"
			<< getNameFromIdx(ik.IKboneIdx) << std::endl;
		oss << "ターゲット（末端）ボーン番号 =" << ik.EndIdx << ":"
			<< getNameFromIdx(ik.EndIdx) << std::endl;
		for (auto& node : ik.nodeIdxes)
		{
			oss << "\t ノードボーン =" << node
				<< ":" << getNameFromIdx(node) << std::endl;
		}
		Helper::DebugOutputFormatString(oss.str().c_str());
	}

}
//	VMDの読み込み
//	※のちにどこかでモーションは保管しておくと思われる
void PMDActor::LoadVMD(const char* vmdpath)
{
	//	VMDファイル読み込み用構造体
	struct VMDMotion
	{
		char boneName[15];			//	ボーン名
		//	※アライメントにより、ボーン名の後パディングが入っている
		unsigned int frameNo;		//	フレーム番号
		DirectX::XMFLOAT3 location;			//	位置
		DirectX::XMFLOAT4 quaternion;		//	クォータニオン（回転）
		unsigned char bezier[64];	//	[4][4][4]ベジュ補間パラメータ
	};

#pragma pack(1)
	//	表情データ（頂点モーフデータ）
	struct VMDMorph
	{
		char name[15];		//	名前（パディングする）
		uint32_t frameNo;	//	フレーム番号
		float weight;		//	ウェイト（0.0f～1.0f）
	};	//	全部で23バイトなので#pragma packで読む
#pragma pack()
#pragma pack(1)
	//	カメラ
	struct VMDCamera
	{
		uint32_t frameNo;			//	フレーム番号
		float distance;				//	距離
		XMFLOAT3 pos;				//	座標
		XMFLOAT3 eulerAngle;		//	オイラー角
		uint8_t Interpolation[24];	//	補間
		uint32_t fov;				//	視界角
		uint8_t persFlg;			//	パースフラグON / OFF
	};	//	61バイト
#pragma pack()
	//	ライト照明データ
	struct VMDLight
	{
		uint32_t frameNo;	//	フレーム番号
		uint8_t rgb;		//	ライト色
		XMFLOAT3 vec;		//	光線ベクトル（平行光線）
	};
#pragma pack(1)
	//	セルフ影データ
	struct VMDSelfShadow
	{
		uint32_t frameNo;	//	フレーム番号
		uint8_t mode;		//	影モード（0:影なし、1:モード1、2:モード2）
		float distance;		//	距離
	};
#pragma pack()

	FILE* fp;
	//	ファイルの読み込み
	auto errow = fopen_s(&fp, vmdpath, "rb");
	if (errow == 0)
	{
		Helper::DebugOutputFormatString("VMD読み込み成功\n");
		//	VMDファイル読み込み
		fseek(fp, 50, SEEK_SET);	//	最初の50バイトはスキップ
		unsigned int motionDataNum = 0;
		fread(&motionDataNum, sizeof(motionDataNum), 1, fp);
		std::vector<VMDMotion> vmdMotionData(motionDataNum);

		for (auto& motion : vmdMotionData)
		{
			//	※パディングが入っているため、ボーン名とほかの変数はわけて読み込んでいる
			fread(motion.boneName, sizeof(motion.boneName), 1, fp);	//	ボーン名
			fread(&motion.frameNo,
				sizeof(motion.frameNo)		//	フレーム番号
				+ sizeof(motion.location)	//	位置
				+ sizeof(motion.quaternion)	//	クォータニオン
				+ sizeof(motion.bezier),	//	補間ベジュデータ
				1, fp);

			_duration = std::max<unsigned int>(_duration, motion.frameNo);
		}
		//	VMDのモーションデータから、実際に使用するモーションテーブルへ変換
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
		//	モーションテーブルの各キーフレームをフレームの基準に昇順するようソート
		for (auto& motion : _motiondata)
		{
			std::sort(motion.second.begin(), motion.second.end(),
				[](const KeyFrame& lval, const KeyFrame& rval)
				{
					return lval.frameNo <= rval.frameNo;
				});
		}

		//	表情データ取得
		uint32_t morphCount = 0;
		fread(&morphCount, sizeof(morphCount), 1, fp);
		std::vector<VMDMorph> morphs(morphCount);
		fread(morphs.data(), sizeof(VMDMorph), morphCount, fp);

		//	カメラデータ取得
		uint32_t vmdCameraCount = 0;
		fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);
		std::vector<VMDCamera> cameraData(vmdCameraCount);
		fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, fp);

		//	ライト照明データ取得
		uint32_t vmdLightCount = 0;
		fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);
		std::vector<VMDLight> lights(vmdLightCount);
		fread(lights.data(), sizeof(VMDLight), vmdLightCount, fp);

		//	セルフ影データ取得
		uint32_t SelfShadowCount = 0; 
		fread(&SelfShadowCount, sizeof(SelfShadowCount), 1, fp);
		std::vector<VMDSelfShadow> selfShadowData(SelfShadowCount);
		fread(selfShadowData.data(), sizeof(VMDSelfShadow), SelfShadowCount, fp);

		//	IKオンオフデータ取得
		uint32_t ikSwitchCount = 0;
		fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);
		_ikEnableData.resize(ikSwitchCount);

		for (auto& ikEnable : _ikEnableData)
		{
			//	キーフレーム情報なのでまずはフレーム番号読みこみ
			fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);
			//	可視フラグの取得
			uint8_t visibleFlg = 0;
			fread(&visibleFlg, sizeof(visibleFlg), 1, fp);
			//	対象ボーン数読み込み
			uint32_t ikBoneCount = 0;
			fread(&ikBoneCount, sizeof(ikBoneCount), 1, fp);
			//	ループしつつ名前とON/OFF情報を取得
			for (unsigned int i = 0; i < ikBoneCount; ++i)
			{
				//	ボーン名取得
				char ikBoneName[20];
				fread(ikBoneName, _countof(ikBoneName), 1, fp);
				//	ON/OFF情報取得
				uint8_t flg = 0;
				fread(&flg, sizeof(flg), 1, fp);
				ikEnable.ikEnableTable[ikBoneName] = flg;
			}
		}

		fclose(fp);
	}
	else
	{
		Helper::DebugOutputFormatString("VMD読み込みエラー\n");
	}
}

//	ベジュ曲線のYを取得する処理
float PMDActor::GetYFromXOnBezier(
	float x,											//	変化量（x）
	const DirectX::XMFLOAT2& a, DirectX::XMFLOAT2& b,	//	中間コントロールポイント
	uint8_t n)											//	試行回数
{
	//	中間コントロールポイントの位置がxとyで同じの場合
	if (a.x == a.y && b.x == b.y)
	{
		return x;	//	計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	//	t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;		//	t^2の係数
	const float k2 = 3 * a.x;				//	tの係数

	//	誤差の範囲内かどうかに使用する定数
	constexpr float epsilon = 0.0005f;

	//	tを近似で求める
	for (int i = 0; i < n; ++i)
	{
		//	f(t)を求める
		auto ft = k0 * (t * t * t) + k1 * (t * t) + k2 * (t) - x;
		//	もし結果が0に近い（誤差の範囲内）なら打ち切る
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}
		t -= ft / 2;	//	刻む
	}

	//	求めたいtはすでに求めているのでyを計算する
	auto r = 1 - t;
	//	※こちらの式の立て方を考える
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

//	各ボーンに行列の変更を反映させる（再起処理）
void PMDActor::RecusiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	_boneMatrices[node->boneIdx] *= mat;
	for (auto& cnode : node->children)
	{
		RecusiveMatrixMultiply(cnode, _boneMatrices[node->boneIdx]);
	}
}

//	16バイト境界に確保する
void* PMDActor::Transform::operator new(size_t size)
{
	return _aligned_malloc(size,16);
}
