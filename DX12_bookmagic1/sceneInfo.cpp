#include "sceneInfo.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "Dx12Wrapper.h"

namespace//	定数定義
{
	constexpr float shadow_difinition = 40.0f;	//	ライトデプスの縦横サイズ
}

using namespace DirectX;

//	コンストラクター
SceneInfo::SceneInfo(Dx12Wrapper* pWrap) : 
	_pWrap(pWrap),
	_eye(0,10,-15),
	_target(0,10,0),
	_up(0,1,0),
	_pMapSceneMtx(nullptr)
{
}

//	初期化処理
void SceneInfo::Init(void)
{
	//	シーンバッファの作成
	CreateViewProjectionView();
}

//	シーン情報更新処理
void SceneInfo::SetSceneInfo(void)
{
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();

	//	画角の変更
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	透視投影処理(パースあり）
		_fov,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);

	//	光源ベクトルの変更
	_pMapSceneMtx->lightVec.x = _lightVec.x;
	_pMapSceneMtx->lightVec.y = _lightVec.y;
	_pMapSceneMtx->lightVec.z = _lightVec.z;
	//	ライトビュープロジェクション
	auto eyePos = XMLoadFloat3(&_eye);			//	視点
	auto targetPos = XMLoadFloat3(&_target);	//	注視点
	auto upPos = XMLoadFloat3(&_up);			//	上ベクトル
	XMVECTOR lightVec = -XMLoadFloat3(&_lightVec);
	XMVECTOR lightPos = targetPos
		+ XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	平行投影処理（パースなし）
			shadow_difinition,		//	左右の範囲
			shadow_difinition,		//	上下の範囲
			1.0f,	//	near
			100.0f	//	for
		);
	//	影行列設定
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	平面の方程式
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		lightVec
	);


	//	シャドウマップOnOffの変更
	_pMapSceneMtx->bSelfShadow = _bSelfShadow;
}

//	ビュー・プロジェクション行列バッファの作成
void SceneInfo::CreateViewProjectionView(void)
{
	//	ディスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	シェーダから見えるように
	descHeapDesc.NodeMask = 0;										//	マスク
	descHeapDesc.NumDescriptors = 1;								//	定数バッファビュー（CBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	シェーダリソースビュー用
	//	ディスクリプタヒープの作成
	auto dev = _pWrap->GetDevice();
	auto result = dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_ScenevHeap.ReleaseAndGetAddressOf())
	);
	//	ヒーププロパティー設定
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	シーンバッファの設定
	UINT64 u_64BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256アライメントにそろえたサイズ
	UINT BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256アライメントにそろえたサイズ

	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(u_64BufferSize);
	//	シーンバッファの作成
	result = dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_SceneBuffer.ReleaseAndGetAddressOf())
	);
	//	シーンバッファビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _SceneBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = BufferSize;
	auto HeapHandle = _ScenevHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&cbvDesc, HeapHandle);

	//	シーンに使用する行列の設定
	result = _SceneBuffer->Map(0, nullptr, (void**)&_pMapSceneMtx);
	//	ビュー行列設定
	auto eyePos = XMLoadFloat3(&_eye);			//	視点
	auto targetPos = XMLoadFloat3(&_target);	//	注視点
	auto upPos = XMLoadFloat3(&_up);			//	上ベクトル
	_pMapSceneMtx->eye = _eye;
	_pMapSceneMtx->view = DirectX::XMMatrixLookAtLH(
		eyePos,
		targetPos,
		upPos
	);
	//	プロジェクション行列設定
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_fov = DirectX::XM_PIDIV2;
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	透視投影処理(パースあり）
		_fov,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);
	//	逆プロジェクション行列設定
	DirectX::XMVECTOR det;
	_pMapSceneMtx->invproj = DirectX::XMMatrixInverse(
		&det,				//	逆行列が取得できない際「0」が入る
		_pMapSceneMtx->proj);

	//	ライトビュープロジェクション
	XMVECTOR lightVec = -XMLoadFloat3(&_lightVec);
	XMVECTOR lightPos = targetPos + XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	平行投影処理（パースなし）
			shadow_difinition,		//	左右の範囲
			shadow_difinition,		//	上下の範囲
			1.0f,	//	near
			100.0f	//	for
		);
	//	影行列設定
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	平面の方程式
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		lightVec
	);
}

//	シーンビューのセット命令
void SceneInfo::CommandSet_SceneView(UINT rootPramIdx)
{
	auto cmdList = _pWrap->GetCmdList();
	//	座標変換用ディスクリプタヒープの指定
	cmdList->SetDescriptorHeaps(
		1,					//	ディスクリプタヒープ数
		_ScenevHeap.GetAddressOf()		//	座標変換用ディスクリプタヒープ
	);

	//	ルートパラメータとディスクリプタヒープの関連付け
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	定数バッファ0ビュー用の指定
	cmdList->SetGraphicsRootDescriptorTable(
		rootPramIdx,			//	ルートパラメータインデックス
		heapHandle	//	ヒープアドレス
	);
}

//	画角設定
void SceneInfo::SetFov(float& fov)
{
	_fov = fov;
}

//	光源ベクトル設定
void SceneInfo::SetLightVec(float vec[3])
{
	_lightVec.x = vec[0];
	_lightVec.y = vec[1];
	_lightVec.z = vec[2];
}

//	シャドウマップOnOff設定
void SceneInfo::SetSelfShadow(bool bShadow)
{
	_bSelfShadow = bShadow;
}
