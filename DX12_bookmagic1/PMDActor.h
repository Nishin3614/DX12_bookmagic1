#pragma once

//	�C���N���[�h
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

//	���C�u����
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")


//	�N���X�̑O���錾
class Dx12Wrapper;
//	�N���X�錾
class PMDActor
{
public:

	//	�֐�	//
	//	�R���X�g���N�^
	PMDActor(Dx12Wrapper* pDxWrap, const char* modelpath, const char* vmdpath, DirectX::XMFLOAT3 position = {0.0f,0.0f,0.0f});
	//	����������
	void Init(void);
	//	�X�V����
	void Update(void);
	//	�`�揈��
	void Draw(void);
	//	�V���h�E�}�b�v�`��
	void ShadowMapDraw(void);
	//	�������
	void Release(void);
	
	//	�A�j���[�V�������J�n����
	void PlayAnimation(void);

	//	�C���X�^���X���̐ݒ�
	static void SetInstance(const bool& bPlaneShadow);

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//	�\����	//
	//	�V�F�[�_�[���ɓn�����߂���{�I�ȍs��f�[�^
	struct Transform
	{
		//	�����Ɏ����Ă���XMMATRIX�����o�[��16�o�C�g�A���C�����g�ł��邽��
		//	Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���
		void* operator new(size_t size);
		DirectX::XMMATRIX world;	//	���[���h�s��
	};
	//	PMD���_�\����
	/*
	struct PMDVertex
	{
		DirectX::XMFLOAT3 pos;	//	���_���W�F12�o�C�g
		DirectX::XMFLOAT3 normal;	//	�@���x�N�g���F12�o�C�g
		DirectX::XMFLOAT2 uv;		//	uv���W�F8�o�C�g
		unsigned short boneNo[2];	//	�{�[���ԍ��F4�o�C�g
		unsigned char boneWeight;	//	�{�[���e���́F1�o�C�g
		unsigned char edgeflg;		//	�֊s���t���O�F1�o�C�g
	};	//	�~38�o�C�g�A�Z40�o�C�g�i�p�f�B���O�����邽�߁j
	*/
#pragma pack(1)	//	��������1�o�C�g�p�b�L���O�ƂȂ�A�A���C�����g�͔������Ȃ�
	//	PMD�}�e���A���\����
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse;	//	�f�B�t���[�Y�F
		float alpha;				//	�f�B�t���[�Y��
		float specularity;			//	�X�y�L�����̋����i��Z�l�j
		DirectX::XMFLOAT3 specular;	//	�X�y�L�����F
		DirectX::XMFLOAT3 ambient;	//	�A���r�G���g�F
		unsigned char toonIdx;		//	�g�D�[���ԍ�
		unsigned char edgeFlg;		//	�}�e���A�����Ƃ̗֊s���t���O
		unsigned int indicesNum;	//	�C���f�b�N�X��
		char texFilePath[20];		//	�e�N�X�`���t�@�C���p�X+��
	};	//	�p�f�B���O���������Ȃ�����70�o�C�g
#pragma pack()	//	�p�b�L���O�w�������
	//	�V�F�[�_�[���ɓ�������}�e���A���f�[�^
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
		DirectX::XMFLOAT3 diffuse;	//	�f�B�t���[�Y�F
		float alpha;		//	�f�B�t���[�Y��
		DirectX::XMFLOAT3 specular;	//	�X�y�L�����F
		float specularity;	//	�X�y�L�����̋����i��Z�l�j
		DirectX::XMFLOAT3 ambient;	//	�A���r�G���g�F
	};
	//	����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial
	{
		AdditionalMaterial() :
			toonIdx(0),
			edgeFlg(false)
		{
		}
		std::string texPath;	//	�e�N�X�`���t�@�C���p�X
		int toonIdx;			//	�g�D�[���ԍ�
		bool edgeFlg;			//	�}�e���A�����Ƃ̗֊s���t���O
	};
	//	�S�̂��܂Ƃ߂�f�[�^
	struct Material
	{
		Material() :
			indicesNum(0),
			material(),
			additional()
		{}
		unsigned int indicesNum;	//	�C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	//	�{�[���m�[�h
	struct BoneNode
	{
		int boneIdx;						//	�{�[���ԍ�
		uint32_t boneType;					//	�{�[�����
		uint32_t ikParentBone;				//	IK�e�{�[��
		DirectX::XMFLOAT3 startPos;			//	�{�[���̊�_�i��]�̒��S�j
		DirectX::XMFLOAT3 endPod;			//	�{�[����[�_�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
		std::vector<BoneNode*> children;	//	�q�m�[�h
	};
	//	���[�V�����\����
	struct KeyFrame
	{
		unsigned int frameNo;
		DirectX::XMVECTOR quaternion;
		DirectX::XMFLOAT3 offset;		//	IK�̏������W����̃I�t�Z�b�g���
		DirectX::XMFLOAT2 p1, p2;		//	�x�W���Ȑ��̒��ԃR���g���[���|�C���g
		KeyFrame(unsigned int fno, 
			DirectX::XMVECTOR& q,
			DirectX::XMFLOAT3& ofst,
			const DirectX::XMFLOAT2& ip1, const DirectX::XMFLOAT2& ip2)
			: frameNo(fno), quaternion(q),offset(ofst),p1(ip1),p2(ip2)
		{}
	};
	//	PMD��IK�ǂݍ��ݗp
	struct PMDIK
	{
		uint16_t IKboneIdx;					//	IK�Ώۂ̃{�[��������
		uint16_t EndIdx;					//	�^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[����ID
		uint16_t iterations;				//	���s��
		float limit;						//	1�񂠂���̉�]����
		std::vector<uint16_t> nodeIdxes;	//	�Ԃ̃m�[�h�ԍ�
	};
	//	IK�I��/�I�t�f�[�^
	struct VMDIKEnable
	{
		//	�L�[�t���[��������t���[���ԍ�
		uint32_t frameNo;
		//	���O�ƃI��/�I�t�t���O�̃}�b�v
		std::unordered_map<std::string, bool> ikEnableTable;
	};

	//	�֐�	//
	//	���[�V�����X�V����
	void MotionUpdate(void);

	//	FK����
	void FKUpdate(const unsigned int& frameNo);

	//	IK����
	void IKSolve(unsigned int frameNo);
	//	CCD-IK�ɂ��{�[������������
	//	@param ik �Ώ�ik�I�u�W�F�N�g
	void SolveCCDIK(const PMDIK& ik);
	//	�]���藝IK�ɂ��{�[������������
	//	@param ik �Ώ�ik�I�u�W�F�N�g
	void SolveCosineIK(const PMDIK& ik);
	//	LookAt�s��ɂ��{�[������������
	//	@param ik �Ώ�ik�I�u�W�F�N�g
	void SolveLookAt(const PMDIK& ik);
	//	����̃x�N�g�������̕����Ɍ�����s���Ԃ��֐�
	//	@param origin ����̃x�N�g��
	//	@param lookat ���������������x�N�g��
	//	@param up ��x�N�g��
	//	@param right �E�x�N�g��
	DirectX::XMMATRIX LookAtMatrix(
		const DirectX::XMVECTOR& origin,
		const DirectX::XMVECTOR& lookat,
		DirectX::XMFLOAT3& up,
		DirectX::XMFLOAT3& right);
	//	z�������̕����Ɍ�����s���Ԃ��֐�
	//	@param lookaat ���������������x�N�g��
	//	@param up ��x�N�g��
	//	@param right �E�x�N�g��
	DirectX::XMMATRIX LookAtMatrix(const DirectX::XMVECTOR& lookat,
		DirectX::XMFLOAT3& up,
		DirectX::XMFLOAT3& right);

	//	���_�o�b�t�@�A�C���f�b�N�X�o�b�t�@�̍쐬
	void CreateVertex_IdxView(void);
	//	�ʒu���W�o�b�t�@�̍쐬
	void CreateTransformView(void);
	//	�}�e���A���o�b�t�@�̍쐬
	void CreateMaterialView(void);

	//	PMD�̓ǂݍ���
	void LoadPMD(const char* modelpath);
	//	VMD�̓ǂݍ���
	void LoadVMD(const char* vmdpath);

	//	�x�W���Ȑ���Y���擾���鏈��
	float GetYFromXOnBezier(
		float x, const DirectX::XMFLOAT2& a, DirectX::XMFLOAT2& b, uint8_t n);

	//	�e�{�[���ɍs��̕ύX�𔽉f������i�ċN�����j
	void RecusiveMatrixMultiply(
		BoneNode* node, const DirectX::XMMATRIX& mat);

	//	�ϐ�	//
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};				//	���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW _ibView = {};				//	�C���f�b�N�X�o�b�t�@�r���[
	ComPtr < ID3D12DescriptorHeap> _BasicDescHeap = nullptr;		//	�f�B�X�N���v�^�q�[�v
	ComPtr < ID3D12DescriptorHeap> _dsvHeap = nullptr;			//	�[�x�o�b�t�@�p�̃f�B�X�N���v�^�q�[�v
	ComPtr < ID3D12DescriptorHeap> _materialDescHeap = nullptr;	//	�}�e���A���p�f�B�X�N���v�^�q�[�v
	ComPtr < ID3D12Resource> _vertBuff = nullptr;				//	���_�o�b�t�@
	ComPtr < ID3D12Resource> _idxBuff = nullptr;				//	�C���f�b�N�X�o�b�t�@
	ComPtr < ID3D12Resource> _uploadbuff = nullptr;				//	�A�b�v���[�h�o�b�t�@
	ComPtr < ID3D12Resource> _texbuff = nullptr;				//	�e�N�X�`���o�b�t�@
	ComPtr < ID3D12Resource> _constBuff = nullptr;				//	�萔�o�b�t�@
	ComPtr < ID3D12Resource> _depthBuffer = nullptr;			//	�[�x�o�b�t�@
	ComPtr < ID3D12Resource> _materialBuff = nullptr;			//	�}�e���A���o�b�t�@

	//	�I�u�W�F�N�g�̍s��A�������
	DirectX::XMMATRIX* _mappedMatrices = nullptr;					//	�}�b�v�������|�C���^
	DirectX::XMFLOAT3 _position;						//	�ʒu
	float _angle;										//	����
	DWORD _startTime;									//	�A�j���[�V�����J�n���̃~���b
	DWORD _duration;									//	�A�j���[�V�����̍ŏI�t���[��
	static unsigned int _nInstance;						//	�C���X�^���X��(1:���f���`��̂݁A2:���f���A�n�ʉe)

	//	PMD�̃f�[�^
	unsigned int _vertNum;								//	���_��
	std::vector<unsigned char> _vertices;				//	���_���f�[�^�̉�
	unsigned int _indicesNum;							//	�C���f�b�N�X��
	std::vector<unsigned short> _indices;				//	�C���f�b�N�X���
	unsigned int _materialNum;							//	�}�e���A����
	std::vector<PMDMaterial> _pmdMaterials;				//	�ǂݍ��ݗp�}�e���A�����
	std::vector<Material> _materials;					//	�]���p�}�e���A�����
	std::string _strModelPath;
	//	PMD�̃{�[�����֘A�f�[�^
	std::vector<DirectX::XMMATRIX> _boneMatrices;		//	�{�[���s��
	std::map<std::string, BoneNode> _boneNodeTable;		//	�{�[���m�[�h�e�[�u��
	std::vector<std::string> _boneNameArray;			//	�C���f�b�N�X���疼�O���������₷���悤��
	std::vector<BoneNode*> _boneNodeAddressArray;		//	�C���f�b�N�X����m�[�h���������₷���悤

	//	VMD�t�@�C���f�[�^
	std::unordered_map<std::string, std::vector<KeyFrame>> _motiondata;	//	���[�V�����f�[�^

	//	IK�t�@�C���f�[�^
	std::vector<PMDIK> _pmdIk;							//	IK���
	std::vector<uint32_t> _kneeIdxes;					//	�Ђ�
	std::vector<VMDIKEnable> _ikEnableData;				//	IK�I���I�t�f�[�^

	//	���N���X�̃C���X�^���X��
	Dx12Wrapper* _pDxWrap;
};