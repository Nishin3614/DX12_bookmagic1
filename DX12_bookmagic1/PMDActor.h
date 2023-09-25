#pragma once

//	インクルード
#include <DirectXTex.h>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <memory>
#include <map>
#include <unordered_map>

//	ライブラリ
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")


//	クラスの前方宣言
class Dx12Wrapper;
//	クラス宣言
class PMDActor
{
public:

	//	関数	//
	//	コンストラクタ
	PMDActor(Dx12Wrapper* pDxWrap, const char* modelpath, const char* vmdpath, DirectX::XMFLOAT3 position = {0.0f,0.0f,0.0f});
	//	初期化処理
	void Init(void);
	//	更新処理
	void Update(void);
	//	描画処理
	void Draw(void);
	//	シャドウマップ描画
	void ShadowMapDraw(void);
	//	解放処理
	void Release(void);
	
	//	アニメーションを開始する
	void PlayAnimation(void);

	//	インスタンス数の設定
	static void SetInstance(const bool& bPlaneShadow);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	構造体	//
	//	シェーダー側に渡すためお基本的な行列データ
	struct Transform
	{
		//	内部に持っているXMMATRIXメンバーが16バイトアライメントであるため
		//	Transformをnewする際には16バイト境界に確保する
		void* operator new(size_t size);
		DirectX::XMMATRIX world;	//	ワールド行列
	};
	//	PMD頂点構造体
	/*
	struct PMDVertex
	{
		DirectX::XMFLOAT3 pos;	//	頂点座標：12バイト
		DirectX::XMFLOAT3 normal;	//	法線ベクトル：12バイト
		DirectX::XMFLOAT2 uv;		//	uv座標：8バイト
		unsigned short boneNo[2];	//	ボーン番号：4バイト
		unsigned char boneWeight;	//	ボーン影響力：1バイト
		unsigned char edgeflg;		//	輪郭線フラグ：1バイト
	};	//	×38バイト、〇40バイト（パディングが入るため）
	*/
#pragma pack(1)	//	ここから1バイトパッキングとなり、アライメントは発生しない
	//	PMDマテリアル構造体
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse;	//	ディフューズ色
		float alpha;				//	ディフューズα
		float specularity;			//	スペキュラの強さ（乗算値）
		DirectX::XMFLOAT3 specular;	//	スペキュラ色
		DirectX::XMFLOAT3 ambient;	//	アンビエント色
		unsigned char toonIdx;		//	トゥーン番号
		unsigned char edgeFlg;		//	マテリアルごとの輪郭線フラグ
		unsigned int indicesNum;	//	インデックス数
		char texFilePath[20];		//	テクスチャファイルパス+α
	};	//	パディングが発生しないため70バイト
#pragma pack()	//	パッキング指定を解除
	//	シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		MaterialForHlsl() :
			diffuse({0.0f,0.0f,0.0f}),
			alpha(0.0f),
			specular({ 0.0f,0.0f,0.0f }),
			specularity(0.0f),
			ambient({ 0.0f,0.0f,0.0f })
		{
		}
		DirectX::XMFLOAT3 diffuse;	//	ディフューズ色
		float alpha;		//	ディフューズα
		DirectX::XMFLOAT3 specular;	//	スペキュラ色
		float specularity;	//	スペキュラの強さ（乗算値）
		DirectX::XMFLOAT3 ambient;	//	アンビエント色
	};
	//	それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		AdditionalMaterial() :
			toonIdx(0),
			edgeFlg(false)
		{
		}
		std::string texPath;	//	テクスチャファイルパス
		int toonIdx;			//	トゥーン番号
		bool edgeFlg;			//	マテリアルごとの輪郭線フラグ
	};
	//	全体をまとめるデータ
	struct Material
	{
		Material() :
			indicesNum(0),
			material(),
			additional()
		{}
		unsigned int indicesNum;	//	インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	//	ボーンノード
	struct BoneNode
	{
		int boneIdx;						//	ボーン番号
		uint32_t boneType;					//	ボーン種別
		uint32_t ikParentBone;				//	IK親ボーン
		DirectX::XMFLOAT3 startPos;			//	ボーンの基準点（回転の中心）
		DirectX::XMFLOAT3 endPod;			//	ボーン先端点（実際のスキニングには利用しない）
		std::vector<BoneNode*> children;	//	子ノード
	};
	//	モーション構造体
	struct KeyFrame
	{
		unsigned int frameNo;
		DirectX::XMVECTOR quaternion;
		DirectX::XMFLOAT3 offset;		//	IKの初期座標からのオフセット情報
		DirectX::XMFLOAT2 p1, p2;		//	ベジュ曲線の中間コントロールポイント
		KeyFrame(unsigned int fno, 
			DirectX::XMVECTOR& q,
			DirectX::XMFLOAT3& ofst,
			const DirectX::XMFLOAT2& ip1, const DirectX::XMFLOAT2& ip2)
			: frameNo(fno), quaternion(q),offset(ofst),p1(ip1),p2(ip2)
		{}
	};
	//	PMDのIK読み込み用
	struct PMDIK
	{
		uint16_t IKboneIdx;					//	IK対象のボーンを示す
		uint16_t EndIdx;					//	ターゲットに近づけるためのボーンのID
		uint16_t iterations;				//	試行回数
		float limit;						//	1回あたりの回転制限
		std::vector<uint16_t> nodeIdxes;	//	間のノード番号
	};
	//	IKオン/オフデータ
	struct VMDIKEnable
	{
		//	キーフレームがあるフレーム番号
		uint32_t frameNo;
		//	名前とオン/オフフラグのマップ
		std::unordered_map<std::string, bool> ikEnableTable;
	};

	//	関数	//
	//	モーション更新処理
	void MotionUpdate(void);

	//	FK処理
	void FKUpdate(const unsigned int& frameNo);

	//	IK処理
	void IKSolve(unsigned int frameNo);
	//	CCD-IKによりボーン方向を解決
	//	@param ik 対象ikオブジェクト
	void SolveCCDIK(const PMDIK& ik);
	//	余弦定理IKによりボーン方向を解決
	//	@param ik 対象ikオブジェクト
	void SolveCosineIK(const PMDIK& ik);
	//	LookAt行列によりボーン方向を解決
	//	@param ik 対象ikオブジェクト
	void SolveLookAt(const PMDIK& ik);
	//	特定のベクトルを特定の方向に向ける行列を返す関数
	//	@param origin 特定のベクトル
	//	@param lookat 向かせたい方向ベクトル
	//	@param up 上ベクトル
	//	@param right 右ベクトル
	DirectX::XMMATRIX LookAtMatrix(
		const DirectX::XMVECTOR& origin,
		const DirectX::XMVECTOR& lookat,
		DirectX::XMFLOAT3& up,
		DirectX::XMFLOAT3& right);
	//	z軸を特定の方向に向ける行列を返す関数
	//	@param lookaat 向かせたい方向ベクトル
	//	@param up 上ベクトル
	//	@param right 右ベクトル
	DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& lookat,
		DirectX::XMFLOAT3& up,
		DirectX::XMFLOAT3& right);

	//	頂点バッファ、インデックスバッファの作成
	void CreateVertex_IdxView(void);
	//	位置座標バッファの作成
	void CreateTransformView(void);
	//	マテリアルバッファの作成
	void CreateMaterialView(void);

	//	PMDの読み込み
	void LoadPMD(const char* modelpath);
	//	VMDの読み込み
	void LoadVMD(const char* vmdpath);

	//	ベジュ曲線のYを取得する処理
	float GetYFromXOnBezier(
		float x, const DirectX::XMFLOAT2& a, DirectX::XMFLOAT2& b, uint8_t n);

	//	各ボーンに行列の変更を反映させる（再起処理）
	void RecusiveMatrixMultiply(
		BoneNode* node, const DirectX::XMMATRIX& mat);

	//	変数	//
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};				//	頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW _ibView = {};				//	インデックスバッファビュー
	ComPtr < ID3D12DescriptorHeap> _BasicDescHeap = nullptr;		//	ディスクリプタヒープ
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	深度バッファ用のディスクリプタヒープ
	ComPtr < ID3D12DescriptorHeap> _materialDescHeap = nullptr;	//	マテリアル用ディスクリプタヒープ
	ComPtr < ID3D12Resource> _vertBuff = nullptr;				//	頂点バッファ
	ComPtr < ID3D12Resource> _idxBuff = nullptr;				//	インデックスバッファ
	ComPtr < ID3D12Resource> _uploadbuff = nullptr;				//	アップロードバッファ
	ComPtr < ID3D12Resource> _texbuff = nullptr;				//	テクスチャバッファ
	ComPtr < ID3D12Resource> _constBuff = nullptr;				//	定数バッファ
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	深度バッファ
	ComPtr < ID3D12Resource> _materialBuff = nullptr;			//	マテリアルバッファ

	//	オブジェクトの行列、方向情報
	DirectX::XMMATRIX* _mappedMatrices = nullptr;					//	マップを示すポインタ
	DirectX::XMFLOAT3 _position;						//	位置
	float _angle;										//	方向
	DWORD _startTime;									//	アニメーション開始時のミリ秒
	DWORD _duration;									//	アニメーションの最終フレーム
	static unsigned int _nInstance;						//	インスタンス数(1:モデル描画のみ、2:モデル、地面影)

	//	PMDのデータ
	unsigned int _vertNum;								//	頂点数
	std::vector<unsigned char> _vertices;				//	頂点情報データの塊
	unsigned int _indicesNum;							//	インデックス数
	std::vector<unsigned short> _indices;				//	インデックス情報
	unsigned int _materialNum;							//	マテリアル数
	std::vector<PMDMaterial> _pmdMaterials;				//	読み込み用マテリアル情報
	std::vector<Material> _materials;					//	転送用マテリアル情報
	std::string _strModelPath;
	//	PMDのボーン情報関連データ
	std::vector<DirectX::XMMATRIX> _boneMatrices;		//	ボーン行列
	std::map<std::string, BoneNode> _boneNodeTable;		//	ボーンノードテーブル
	std::vector<std::string> _boneNameArray;			//	インデックスから名前を検索しやすいように
	std::vector<BoneNode*> _boneNodeAddressArray;		//	インデックスからノードを検索しやすいよう

	//	VMDファイルデータ
	std::unordered_map<std::string, std::vector<KeyFrame>> _motiondata;	//	モーションデータ

	//	IKファイルデータ
	std::vector<PMDIK> _pmdIk;							//	IK情報
	std::vector<uint32_t> _kneeIdxes;					//	ひざ
	std::vector<VMDIKEnable> _ikEnableData;				//	IKオンオフデータ

	//	他クラスのインスタンス化
	Dx12Wrapper* _pDxWrap;
};