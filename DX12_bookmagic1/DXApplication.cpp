//	�C���N���[�h
#include "DXApplication.h"
#include "Dx12Wrapper.h"
#include "PMDActor.h"
#include "PMDRenderer.h"

using namespace DirectX;

//	�C���X�^���X���擾
DXApplication& DXApplication::Instance(void)
{
	static DXApplication instance;
	return instance;
}

//	�R���X�g���N�^
DXApplication::DXApplication() :
	_pDxWrap(nullptr),
	_pPmdAct(nullptr),
	_pPmdAct2(nullptr),
	_pPmdRender(nullptr)
{
}

//	����������
void DXApplication::OnInit(HWND hwnd, unsigned int window_width, unsigned int window_height)
{
	//	DirectX����̏���������
	_pDxWrap = new Dx12Wrapper;
	_pDxWrap->Init(hwnd);

	//	PMD���f���̏���������
	_pPmdAct = new PMDActor(_pDxWrap, "Model/�����~�N.pmd", "motion/pose.vmd");
	_pPmdAct->Init();

	_pPmdAct2 = new PMDActor(_pDxWrap, "Model/�㉹�n�N.pmd", "motion/motion.vmd", {15.0f,0.0f,5.0f});
	_pPmdAct2->Init();

	//	PMD�����_���[�̏���������
	_pPmdRender = new PMDRenderer(_pDxWrap);
	_pPmdRender->Init();
}

//	�`�揈��
void DXApplication::OnRender(void)
{
	//	�V���h�E�}�b�v�`��	//
	_pPmdRender->PreShadowDraw();
	_pDxWrap->ShadowDraw();
	_pPmdAct->ShadowMapDraw();
	_pPmdAct2->ShadowMapDraw();


	//	�������̕`����I���W�������_�[�^�[�Q�b�g�ɍs��	//
	//	�I���W�������_�[�^�[�Q�b�g���Z�b�g
	_pDxWrap->PreOriginDraw();
	//	PMD�����_���[�ɂāA���[�g�V�O�l�C�`���Ȃǂ��Z�b�g
	_pPmdRender->Draw();
	//	�V�[���r���[�̕`��Z�b�g
	_pDxWrap->CommandSet_SceneView();
	//	PMD���f���̕`�揈��
	_pPmdAct->Draw();
	_pPmdAct2->Draw();

	//	�I���W�������_�[�^�[�Q�b�g�̕`��I��
	_pDxWrap->EndOriginDraw();

	_pDxWrap->DrawShrinkTextureForBlur();
	//	���H�p�̃����_�[�^�[�Q�b�g�̕`��
	_pDxWrap->ProceDraw();


	//	�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�̃Z�b�g�y�сA�̃N���A
	_pDxWrap->Clear();
	//	�`��
	_pDxWrap->Draw();
	
	//	�t���b�v
	_pDxWrap->Flip();
}

//	�I�u�W�F�N�g�̉������
void DXApplication::OnRelease(void)
{
	//	DirectX����̉��
	delete _pDxWrap;
	_pDxWrap = nullptr;

	//	PMD���f���̉��
	delete _pPmdAct;
	_pPmdAct = nullptr;

	delete 	_pPmdAct2;
	_pPmdAct2 = nullptr;

	//	PMD�����_���[�̉��
	delete _pPmdRender;
	_pPmdRender = nullptr;
}

//	�X�V����
void DXApplication::OnUpdate(void)
{
	_pPmdAct->Update();
	_pPmdAct2->Update();
}

// //	�R�s�[�e�N�X�`���[���M�I���ł̃e�N�X�`���[�\��t��
// void DXApplication::ByCopyTextureRegion(void)
// {
// 	//	CopyTextureRegion�ɂ��A�e�N�X�`���[�\��t������	//
// //	WIC�iWindows Imaging Component)�e�N�X�`���̃��[�h
// //	��PNG,JPEG�Ȃǂ�`��ł���
// 	DirectX::TexMetadata metadate = {};		//	�e�N�X�`���̃��^�f�[�^�i�摜�t�@�C���Ɋւ���f�[�^�A���E�����E�t�H�[�}�b�g�Ȃǁj
// 	DirectX::ScratchImage scratchImg = {};	//	�摜�t�@�C���f�[�^
// 	//	�C���^�[�t�@�C�X�֘A�̃G���[���o������ꍇ�A������̃R�����g�����
// 	//result = CoInitializeEx(0, COINIT_MULTITHREADED);	//	COM���C�u���������������鏈��
// 	auto result = DirectX::LoadFromWICFile(
// 		//L"img/textest.png",	//	�t�@�C���p�X
// 		L"img/textest.png",	//	�t�@�C���p�X
// 
// 		WIC_FLAGS_NONE,		//	�ǂ̂悤�Ƀ��[�h���邩
// 		&metadate,			//	���^�f�[�^�̃|�C���^
// 		scratchImg			//	�摜�t�@�C���f�[�^
// 	);
// 	//	���f�[�^���o
// 	auto img = scratchImg.GetImage(
// 		0,	//	�~�b�v���x��
// 		0,	//	�e�N�X�`���z����g�p����ۂ̃C���f�b�N�X
// 		0	//	3D�e�N�X�`���[�ɂ�����[���i�X���C�X�j
// 	);
// 
// 	//	�A�b�v���[�h�o�b�t�@�̍쐬	//
// 	//	���ԃo�b�t�@�[�Ƃ��ẴA�b�v���[�h�q�[�v�ݒ�
// 	CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
// 	//	���\�[�X�ݒ�
// 	CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height);
// 	//	�A�b�v���[�h�p���\�[�X�̍쐬
// 	//ID3D12Resource* uploadbuff = nullptr;
// 	result = _dev->CreateCommittedResource(
// 		&uploadHeapProp,
// 		D3D12_HEAP_FLAG_NONE,
// 		&texresDesc,
// 		D3D12_RESOURCE_STATE_GENERIC_READ,
// 		nullptr,
// 		IID_PPV_ARGS(_uploadbuff.ReleaseAndGetAddressOf())
// 	);
// 
// 	//	�R�s�[�惊�\�[�X�̍쐬	//
// 	//	�e�N�X�`���̂��߂̃q�[�v�ݒ�
// 	CD3DX12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
// 	//	�e�N�X�`�����\�[�X�ݒ�
// 	texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
// 		metadate.format,
// 		metadate.width,
// 		metadate.height,
// 		(UINT16)metadate.arraySize,
// 		(UINT16)metadate.mipLevels
// 	);
// 
// 	//	�e�N�X�`���o�b�t�@�̍쐬
// 	result = _dev->CreateCommittedResource(
// 		&texHeapProp,
// 		D3D12_HEAP_FLAG_NONE,
// 		&texresDesc,
// 		D3D12_RESOURCE_STATE_COPY_DEST,	//	�R�s�[��
// 		nullptr,
// 		IID_PPV_ARGS(_texbuff.ReleaseAndGetAddressOf())
// 	);
// 
// 	//	�A�b�v���[�h���\�[�X�ւ̃}�b�v
// 	uint8_t* mapforImg = nullptr;								//	image->pixels�Ɠ����^�ɂ���
// 	result = _uploadbuff->Map(0, nullptr, (void**)&mapforImg);	//	�}�b�v
// 
// 	auto srcAddress = img->pixels;														//	���f�[�^�̃|�C���^���擾
// 	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);	//	1�s������̃T�C�Y�擾
// 	for (int y = 0; y < img->height; ++y)
// 	{
// 		std::copy_n(srcAddress, rowPitch, mapforImg);		//	1�s�R�s�[
// 		//	1�s���Ƃ̂��܂����킹��
// 		srcAddress += img->rowPitch;
// 		mapforImg += rowPitch;
// 	}
// 	_uploadbuff->Unmap(0, nullptr);								//	�A���}�b�v
// 
// 	//	�R�s�[���A�b�v���[�h�o�b�t�@����R�s�[��e�N�X�`���[�o�b�t�@�ɃR�s�[���鏈��	//
// 	//	�R�s�[���̐ݒ�
// 	D3D12_TEXTURE_COPY_LOCATION src = {};
// 	src.pResource = _uploadbuff.Get();									//	�A�b�v���[�h�o�b�t�@
// 	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		//	�t�b�g�v�����g�i��������L�̈�Ɋւ�����j
// 	src.PlacedFootprint.Offset = 0;								//	�������I�t�Z�b�g
// 	src.PlacedFootprint.Footprint.Width = metadate.width;		//	��
// 	src.PlacedFootprint.Footprint.Height = metadate.height;		//	����
// 	src.PlacedFootprint.Footprint.Depth = metadate.depth;		//	�[��
// 	src.PlacedFootprint.Footprint.RowPitch =					//	1�s������̃o�C�g��
// 		AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
// 	src.PlacedFootprint.Footprint.Format = img->format;			//	�t�H�[�}�b�g
// 
// 	//	�R�s�[��̐ݒ�
// 	D3D12_TEXTURE_COPY_LOCATION dst = {};
// 	dst.pResource = _texbuff.Get();									//	�e�N�X�`���[�o�b�t�@
// 	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		//	�C���f�b�N�X
// 	dst.SubresourceIndex = 0;									//	�R�s�[��̃C���f�b�N�X
// 
// 	//	GPU�ɑ΂��閽�ߏ���
// 	{
// 		//	�R�s�[�����R�s�[��ɃR�s�[���閽��
// 		_cmdList->CopyTextureRegion(
// 			&dst,		//	�R�s�[��̃|�C���^
// 			0,			//	�R�s�[��̈�J�nX
// 			0,			//	�R�s�[��̈�J�nY
// 			0,			//	�R�s�[��̈�J�nZ
// 			&src,		//	�R�s�[���̃|�C���^
// 			nullptr		//	�R�s�[���̈�{�b�N�X
// 		);
// 
// 		//	�o���A�̐ݒ�
// 		CD3DX12_RESOURCE_BARRIER BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
// 			_texbuff.Get(),
// 			D3D12_RESOURCE_STATE_COPY_DEST,
// 			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
// 		);
// 
// 		//	�o���A�̃Z�b�g
// 		_cmdList->ResourceBarrier(1, &BarrierDesc);
// 		_cmdList->Close();
// 
// 		//	�R�}���h���X�g�̎��s
// 		ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
// 		_cmdQueue->ExecuteCommandLists(1, cmdlists);
// 
// 		//	�t�F���X�̒l���X�V
// 		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
// 		//	GPU���Ŗ��߂��������Ă�����A�t�F���X�l����v����
// 		if (_fence->GetCompletedValue() != _fenceVal)
// 		{
// 			//	�C�x���g�n���h���̎擾
// 			auto event = CreateEvent(nullptr, false, false, nullptr);
// 			_fence->SetEventOnCompletion(_fenceVal, event);
// 			//	�C�x���g����������܂ő҂�������iINFINITE�j
// 			WaitForSingleObject(event, INFINITE);
// 			//	�C�x���g�n���h�������
// 			CloseHandle(event);
// 		}
// 		result = _cmdAllocator->Reset();
// 		_cmdList->Reset(_cmdAllocator.Get(), nullptr);
// 	}
// 
// 
// }
