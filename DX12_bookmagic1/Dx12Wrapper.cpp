//	�C���N���[�h
#include "Dx12Wrapper.h"
#include <d3dx12.h>
#include "Win32Application.h"
#include "helper.h"

//	���O���
using namespace Microsoft::WRL;
using namespace DirectX;

//	�萔��`
constexpr float shadow_difinition = 40.0f;	//	���C�g�f�v�X�̏c���T�C�Y
constexpr float CLSCLR[4] = { 0.5f,0.5f,0.5f,1.0f };		//	�����_�[�^�[�Q�b�g�N���A�J���[

//	�R���X�g���N�^
Dx12Wrapper::Dx12Wrapper() :
	_parallelLightVec(1,-1,1),
	_eye(0, 10, -15),
	_target(0, 10, 0),
	_up(0, 1, 0),
	_pMapSceneMtx(nullptr)
{
	//	�E�B���h�E�T�C�Y�̎擾
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_windowSize = WinApp.GetWindowSize();
}

/*	�������֘A�̏���	*/
//	����������
void Dx12Wrapper::Init(HWND hwnd)
{
#ifdef _DEBUG
	EnableDebugLayer();
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgifactory.ReleaseAndGetAddressOf()));
#else
	//	�t�@�N�g���[�쐬
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgifactory.ReleaseAndGetAddressOf()));
#endif // _DEBUG

	//	�f�o�C�X�̍쐬
	CreateDevice();
	
	//	�R�}���h���X�g�̍쐬
	CreateCommand();

	//	�X���b�v�`�F�C���̍쐬
	CreateSwapchain(hwnd);

	//	�t�F���X�̍쐬
	result = _dev->CreateFence(
		_fenceVal,				//	�������l
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())
	);

	//	�[�x�o�b�t�@�̍쐬
	CreateDepthView();

	//	�V�[���o�b�t�@�̍쐬
	CreateViewProjectionView();
	
	//	�e�N�X�`�����o�b�t�@�e�[�u�����쐬
	_noneTexTable[static_cast<int>(E_NONETEX::WHITE)] = CreateWhiteTexture();
	_noneTexTable[static_cast<int>(E_NONETEX::BLACK)] = CreateBlackTexture();
	_noneTexTable[static_cast<int>(E_NONETEX::GRADUATION)] = CreateGrayGradationTexture();
}

//	�f�o�b�O���C���[�L����
void Dx12Wrapper::EnableDebugLayer(void)
{
	ComPtr<ID3D12Debug> debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));

	debugLayer->EnableDebugLayer();	//	�f�o�b�O���C���[��L��������
}

//	�f�o�C�X�̍쐬
void Dx12Wrapper::CreateDevice(void)
{
	//	�A�_�v�^�[�̗񋓗p
	std::vector <IDXGIAdapter*> adapters;

	//	�����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;
	//	�A�_�v�^�[�I�u�W�F�N�g�����ׂĎ擾
	for (int i = 0; _dxgifactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	//	�ݒ肵�����A�_�v�^�[���T���o��
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	//	�A�_�v�^�[�̐����I�u�W�F�N�g�擾

		std::wstring strDesc = adesc.Description;

		//	�T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//	�f�o�C�X�쐬
	D3D_FEATURE_LEVEL levels[] =	//	�e�t�B�[�`���[���x���i�[
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,

	};
	D3D_FEATURE_LEVEL featureLevel;	//	�t�B�[�`���[���x��

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;	//	�����\�ȃo�[�W���������������̂Ń��[�v��ł��؂�
		}
	}
	//	�f�o�C�X���쐬�ł��Ȃ��ہA�֐��𔲂���
	if (_dev == nullptr)
	{
		Helper::DebugOutputFormatString("�f�o�C�X���쐬�ł��܂���ł����B");
		return;
	}
}

//	�R�}���h�쐬
void Dx12Wrapper::CreateCommand(void)
{	//	�R�}���h�A���P�[�^�[�̍쐬
	auto result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	//	�R�}���h���X�g�̍쐬
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	_cmdList->Close();
	//	���������������Ƀ��X�g���N���[�Y�ɂ��āA���s���ł͂Ȃ������邱�Ƃŉ�������
	result = _cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);	//	�ĂуR�}���h���X�g�����߂鏀��

	//	�R�}���h�L���[�̐ݒ�
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	//	�^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//	�A�_�v�^�[��1�����g��Ȃ�����0�ł悢
	cmdQueueDesc.NodeMask = 0;
	//	�v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//	�R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	//	�R�}���h�L���[�쐬
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));
}

//	�X���b�v�`�F�C���쐬
void Dx12Wrapper::CreateSwapchain(HWND hwnd)
{
	//	�X���b�v�`�F�C���̐ݒ�
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _windowSize.cx;
	swapchainDesc.Height = _windowSize.cy;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//	�o�b�N�o�b�t�@�͐L�яk�݉\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//	�t���b�v��͑��₩�ɔj��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//	���Ɏw��Ȃ�
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//	�E�B���h�E�̃t���X�N���[���؂�ւ��\
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//	�X���b�v�`�F�C���쐬
	auto result = _dxgifactory->CreateSwapChainForHwnd(
		_cmdQueue.Get(), hwnd,
		&swapchainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf());

	//	�f�B�X�N���v�^�q�[�v�̐ݒ�(�����_�[�^�[�Q�b�g�r���[	�j
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		//	�����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;						//	�\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	//	���Ɏw��Ȃ�
	//	�f�B�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));

	//	�X���b�v�`�F�[���̃������ƕR�Â���
	_backBuffers.resize(swapchainDesc.BufferCount);
	//	�擪�̃A�h���X���擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	//	�����_�[�^�[�Q�b�g�r���[�̐ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		//	�t�H�[�}�b�g
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		//	�t�H�[�}�b�g

	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	//	�e�N�X�`���[�̎���
	//	�����_�[�^�[�Q�b�g�r���[�̐���
	for (unsigned int idx = 0; idx < swapchainDesc.BufferCount; ++idx)
	{
		//	�X���b�v�`�F�[���̃��������擾
		_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		//	�����_�[�^�[�Q�b�g�r���[�쐬
		_dev->CreateRenderTargetView(
			_backBuffers[idx],	//	�o�b�N�o�b�t�@
			&rtvDesc,			//	�����_�[�^�[�Q�b�g�r���[�̐ݒ�
			handle);			//	�ǂ̃n���h�����琶�����邩
		//	�|�C���^�[�ړ�
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
}

//	�[�x�o�b�t�@�̍쐬
void Dx12Wrapper::CreateDepthView(void)
{
	//	�[�x�o�b�t�@�̐ݒ�
	CD3DX12_RESOURCE_DESC depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		//DXGI_FORMAT_D32_FLOAT,					//	�[�x�l�������ݗp�t�H�[�}�b�g
		DXGI_FORMAT_R32_TYPELESS,		//	�^���X�i�r���[���Ŏ��R�Ɍ^�����߂���j
		_windowSize.cx,							//	�����_�[�^�[�Q�b�g�Ɠ���
		_windowSize.cy,							//	�����_�[�^�[�Q�b�g�Ɠ���
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL	//	�f�v�X�X�e���V���Ƃ��Ďg�p
	);
	//	�[�x�l�p�q�[�v�v���p�e�B
	CD3DX12_HEAP_PROPERTIES depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//	�N���A�o�����[�̐ݒ�
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;	//	�[��1.0f�i�ő�l�j�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;	//	32�r�b�gfloat�l�Ƃ��ăN���A
	//	�[�x�o�b�t�@����
	auto result = _dev->CreateCommittedResource(
		&depthHeapProp,						//	�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,				//	�q�[�v�t���O
		&depthResDesc,						//	�[�x�o�b�t�@�̐ݒ�
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//	�[�x�l�̏������݂Ɏg�p
		&depthClearValue,					//	�N���A�o�����[
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf())			//	�o�b�t�@
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�[�x�o�b�t�@�쐬���s\n");
		return;
	}

	//	�V���h�E�}�b�v�p�[�x�o�b�t�@
	depthResDesc.Width = shadow_difinition;
	depthResDesc.Height = shadow_difinition;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,						//	�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,				//	�q�[�v�t���O
		&depthResDesc,						//	�[�x�o�b�t�@�̐ݒ�
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	//	�[�x�l�̏������݂Ɏg�p
		&depthClearValue,					//	�N���A�o�����[
		IID_PPV_ARGS(_lightDepthBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�V���h�E�}�b�v�p�[�x�o�b�t�@�쐬���s\n");
		return;
	}

	//	�[�x�o�b�t�@�r���[�̍쐬	//
	//	�f�B�X�N���v�^�q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 2;							//	�f�B�X�N���v�^��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;		//	�f�v�X�X�e���V���r���[�Ƃ��Ďg�p
	//	�[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));
	//	�[�x�o�b�t�@�r���[�̐ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;					//	�[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	//	2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;					//	�t���O�Ȃ�
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//	�f�B�X�N���v�^�q�[�v���ɐ[�x�o�b�t�@�r���[���쐬
	_dev->CreateDepthStencilView(
		_depthBuffer.Get(),
		&dsvDesc,
		handle
	);
	//	���C�g�f�v�X�p�̐[�x�o�b�t�@�r���[�쐬
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_dev->CreateDepthStencilView(
		_lightDepthBuffer.Get(),
		&dsvDesc,
		handle
	);

	//	�[�x�l�e�N�X�`���[�r���[�쐬	//
	//	�[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC SRVheapDesc = {};
	SRVheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SRVheapDesc.NodeMask = 0;
	SRVheapDesc.NumDescriptors = 2;
	SRVheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&SRVheapDesc, IID_PPV_ARGS(_depthSRVHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		Helper::DebugOutputFormatString("�[�x�l�e�N�X�`���[�p�̃f�B�X�N���v�^�q�[�v�쐬���s\n");
		return;
	}
	//	�ʏ�f�v�X���e�N�X�`���p
	D3D12_SHADER_RESOURCE_VIEW_DESC srvResDesc  = {};
	srvResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvResDesc.Texture2D.MipLevels = 1;
	srvResDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvResDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//	2D�e�N�X�`��
	handle = _depthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(
		_depthBuffer.Get(),
		&srvResDesc,
		handle
	);
	//	���C�g�f�v�X���e�N�X�`���p
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_dev->CreateShaderResourceView(
		_lightDepthBuffer.Get(),
		&srvResDesc,
		handle
	);
}

//	�r���[�v���W�F�N�V�����o�b�t�@�̍쐬
void Dx12Wrapper::CreateViewProjectionView(void)
{
	//	�f�B�X�N���v�^�q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	//	�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;										//	�}�X�N
	descHeapDesc.NumDescriptors = 1;								//	�萔�o�b�t�@�r���[�iCBV)
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		//	�V�F�[�_���\�[�X�r���[�p
	//	�f�B�X�N���v�^�q�[�v�̍쐬
	auto result = _dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(_ScenevHeap.ReleaseAndGetAddressOf())
	);
	//	�q�[�v�v���p�e�B�[�ݒ�
	CD3DX12_HEAP_PROPERTIES heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	�V�[���o�b�t�@�̐ݒ�
	UINT64 BufferSize = (sizeof(SceneMatrix) + 0xff) & ~0xff;	//	256�A���C�����g�ɂ��낦���T�C�Y
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
	//	�V�[���o�b�t�@�̍쐬
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_SceneBuffer.ReleaseAndGetAddressOf())
	);
	//	�V�[���o�b�t�@�r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _SceneBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = BufferSize;
	auto HeapHandle = _ScenevHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&cbvDesc, HeapHandle);

	//	�V�[���Ɏg�p����s��̐ݒ�
	result = _SceneBuffer->Map(0, nullptr, (void**)&_pMapSceneMtx);
	//	�r���[�s��ݒ�
	auto eyePos = XMLoadFloat3(&_eye);			//	���_
	auto targetPos = XMLoadFloat3(&_target);	//	�����_
	auto upPos = XMLoadFloat3(&_up);			//	��x�N�g��
	DirectX::XMFLOAT3 target(0, 10, 0);	//	�����_
	DirectX::XMFLOAT3 up(0, 1, 0);		//	��x�N�g��
	_pMapSceneMtx->eye = _eye;
	_pMapSceneMtx->view = DirectX::XMMatrixLookAtLH(
		eyePos,
		targetPos,
		upPos
	);
	//	�v���W�F�N�V�����s��ݒ�
	auto& WinApp = Win32Application::Instance();
	SIZE rec = WinApp.GetWindowSize();
	_pMapSceneMtx->proj = DirectX::XMMatrixPerspectiveFovLH(	//	�������e����(�p�[�X����j
		DirectX::XM_PIDIV2,
		static_cast<float>(rec.cx) / static_cast<float>(rec.cy),
		1.0f,
		100.0f
	);

	//	���C�g�r���[�v���W�F�N�V����
	XMVECTOR lightPos = targetPos + XMVector3Normalize(-XMLoadFloat3(&_parallelLightVec))
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_pMapSceneMtx->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upPos)
		* XMMatrixOrthographicLH(	//	���s���e�����i�p�[�X�Ȃ��j
			shadow_difinition,		//	���E�͈̔�
			shadow_difinition,		//	�㉺�͈̔�
			1.0f,	//	near
			100.0f	//	for
		);
	//	�e�s��ݒ�
	XMFLOAT4 planeVec(0, 1, 0, 0);	//	���ʂ̕�����
	_pMapSceneMtx->shadow = XMMatrixShadow(
		XMLoadFloat4(&planeVec),
		-XMLoadFloat3(&_parallelLightVec)
	);
}

//	�����e�N�X�`���[�𐶐����鏈��
ComPtr < ID3D12Resource> Dx12Wrapper::CreateWhiteTexture(void)
{
	//	�v���p�e�B�̐ݒ�
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	//	���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4,
		1,
		1
	);

	//	�e�N�X�`���[�o�b�t�@�̐���
	ComPtr < ID3D12Resource> whiteBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	���Ɏw��Ȃ�
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	�e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);	//	���ׂĂ̒l��255�ɓ��ꂷ��

	//	�e�N�X�`���o�b�t�@�̓]��
	result = whiteBuff->WriteToSubresource(
		0,								//	�T�u���\�[�X�C���f�b�N�X
		nullptr,						//	�������ݗ̈�̎w��inullptr = �S�̈�ւ̃R�s�[�j
		data.data(),					//	�������݂����f�[�^�̃A�h���X
		4 * 4,					//	1�s������̃f�[�^�T�C�Y
		data.size()					//	�X���C�X������̃f�[�^�T�C�Y
	);

	return whiteBuff;
}

//	���e�N�X�`���𐶐����鏈��
ComPtr < ID3D12Resource> Dx12Wrapper::CreateBlackTexture(void)
{
	//	�v���p�e�B�̐ݒ�
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	//	���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4,
		1,
		1
	);

	//	�e�N�X�`���[�o�b�t�@�̐���
	ComPtr < ID3D12Resource> blackBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	���Ɏw��Ȃ�
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	�e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(blackBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);	//	���ׂĂ̒l��0�ɓ��ꂷ��

	//	�e�N�X�`���o�b�t�@�̓]��
	result = blackBuff->WriteToSubresource(
		0,								//	�T�u���\�[�X�C���f�b�N�X
		nullptr,						//	�������ݗ̈�̎w��inullptr = �S�̈�ւ̃R�s�[�j
		data.data(),					//	�������݂����f�[�^�̃A�h���X
		4 * 4,					//	1�s������̃f�[�^�T�C�Y
		data.size()					//	�X���C�X������̃f�[�^�T�C�Y
	);

	return blackBuff;
}

//	�f�t�H���g�O���f�[�V�����e�N�X�`���[
ComPtr < ID3D12Resource> Dx12Wrapper::CreateGrayGradationTexture(void)
{
	//	�v���p�e�B�̐ݒ�
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);
	//	���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		256,
		1,
		1
	);
	//	�e�N�X�`���[�o�b�t�@�̐���
	ComPtr < ID3D12Resource> gradBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	���Ɏw��Ȃ�
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	�e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	//	�オ�����Ă����������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);
		--c;
	}

	//	�e�N�X�`���o�b�t�@�̓]��
		//	���܂�ɂ��̏��������s���ɐi�s��~����
	result = gradBuff->WriteToSubresource(
		0,								//	�T�u���\�[�X�C���f�b�N�X
		nullptr,						//	�������ݗ̈�̎w��inullptr = �S�̈�ւ̃R�s�[�j
		data.data(),					//	�������݂����f�[�^�̃A�h���X
		4 * sizeof(unsigned int),					//	1�s������̃f�[�^�T�C�Y
		sizeof(unsigned int) * data.size()				//	�X���C�X������̃f�[�^�T�C�Y
	);

	return gradBuff;
}

/*	�`��֘A�̏���	*/
//	�����_�[�^�[�Q�b�g���Z�b�g���鏈��
void Dx12Wrapper::ShadowDraw(void)
{	
	//	�[�x�o�b�t�@�p�f�B�X�N���v�^�q�[�v�n���h���擾
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//	�����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(0, nullptr, false, &handle);
	//	�[�x�o�b�t�@�r���[���N���A
	_cmdList->ClearDepthStencilView(handle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�̎w��
	_cmdList->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_ScenevHeap.GetAddressOf()		//	���W�ϊ��p�f�B�X�N���v�^�q�[�v
	);

	//	���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	�萔�o�b�t�@0�r���[�p�̎w��
	_cmdList->SetGraphicsRootDescriptorTable(
		0,			//	���[�g�p�����[�^�C���f�b�N�X
		heapHandle	//	�q�[�v�A�h���X
	);

	D3D12_VIEWPORT vp = 
		CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);//�r���[�|�[�g

	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`
}

//	�o�b�N�o�b�t�@�̃N���A
void Dx12Wrapper::Clear(void)
{
	//	�o�b�N�o�b�t�@�̃C���f�b�N�X���擾����
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	�o�b�N�o�b�t�@�̃o���A�ݒ�
	SetBarrier(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,		//	���O��PRESENT���
		D3D12_RESOURCE_STATE_RENDER_TARGET);	//	�����烌���_�[�^�[�Q�b�g���

	//	�o�b�N�o�b�t�@�̃����_�[�^�[�Q�b�g���Z�b�g
	auto rtvHeapPointer = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//	�o�b�N�o�b�t�@���N���A
	_cmdList->ClearRenderTargetView(rtvHeapPointer, CLSCLR, 0, nullptr);
}

//	�t���b�v����
void Dx12Wrapper::Flip(void)
{
	//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾����
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	//	�o�b�N�o�b�t�@�̃o���A�ݒ�
	SetBarrier(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,		//	���O��PRESENT���
		D3D12_RESOURCE_STATE_PRESENT);			//	�����烌���_�[�^�[�Q�b�g���

	//	���߂̃N���[�Y
	_cmdList->Close();
	//	�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(
		1,			//	���s����R�}���h���X�g��
		cmdlists);	//	�R�}���h���X�g�z��̐擪�A�h���X

	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		//	�C�x���g�n���h���̎擾
		auto event = CreateEvent(nullptr, false, false, nullptr);
		if (event != NULL)
		{
			_fence->SetEventOnCompletion(_fenceVal, event);
			//	�C�x���g����������܂ő҂�������iINFINITE�j
			WaitForSingleObject(event, INFINITE);
			//	�C�x���g�n���h�������
			CloseHandle(event);
		}
	}

	//	�R�}���h�A���P�[�^�[�ƃR�}���h���X�g�����Z�b�g����
	auto result = _cmdAllocator->Reset();	//	�L���[���N���A
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);	//	�ĂуR�}���h���X�g�����߂鏀��

	_swapchain->Present(1, 0);
}

//	�V�[���r���[�̃Z�b�g����
void Dx12Wrapper::CommandSet_SceneView(void)
{
	//	���W�ϊ��p�f�B�X�N���v�^�q�[�v�̎w��
	_cmdList->SetDescriptorHeaps(
		1,					//	�f�B�X�N���v�^�q�[�v��
		_ScenevHeap.GetAddressOf()		//	���W�ϊ��p�f�B�X�N���v�^�q�[�v
	);

	//	���[�g�p�����[�^�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
	auto heapHandle = _ScenevHeap->GetGPUDescriptorHandleForHeapStart();
	//	�萔�o�b�t�@0�r���[�p�̎w��
	_cmdList->SetGraphicsRootDescriptorTable(
		0,			//	���[�g�p�����[�^�C���f�b�N�X
		heapHandle	//	�q�[�v�A�h���X
	);

	//	�[�xSRV���Z�b�g
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	auto handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_cmdList->SetGraphicsRootDescriptorTable(3, handle);

	D3D12_VIEWPORT vp =
		CD3DX12_VIEWPORT(0.0f, 0.0f, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetViewports(1, &vp);//�r���[�|�[�g

	CD3DX12_RECT rc(0, 0, _windowSize.cx, _windowSize.cy);
	_cmdList->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`
}

//	�o���A�ݒ�
void Dx12Wrapper::SetBarrier(ID3D12Resource* res, D3D12_RESOURCE_STATES pre, D3D12_RESOURCE_STATES dest)
{
	auto resBarri =
		CD3DX12_RESOURCE_BARRIER::Transition(res,
			pre,
			dest);
	_cmdList->ResourceBarrier(1,
		&resBarri);

}

//	�e�N�X�`���̓ǂݍ��ݏ���
ComPtr<ID3D12Resource> Dx12Wrapper::LoadTextureFromFile(std::string& texPath)
{
	//	�e�N�X�`���[�p�X������̏ꍇ
	if (texPath.empty() || Helper::GetExtension(texPath) == "")
	{
		return nullptr;
	}
	//	�e�[�u�����ɓ����\�[�X����������A�}�b�v���̃��\�[�X��Ԃ�
	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end())
	{
		//	���\�[�X��Ԃ�
		return it->second;
	}

	DirectX::TexMetadata metadate = {};		//	�e�N�X�`���̃��^�f�[�^�i�摜�t�@�C���Ɋւ���f�[�^�A���E�����E�t�H�[�}�b�g�Ȃǁj
	DirectX::ScratchImage scratchImg = {};	//	�摜�t�@�C���f�[�^
	//	�C���^�[�t�@�C�X�֘A�̃G���[���o������ꍇ�A������̃R�����g�����
	//result = CoInitializeEx(0, COINIT_MULTITHREADED);	//	COM���C�u���������������鏈��

	//	�e�e�N�X�`���t�@�C���̊g���q�ɍ��킹���֐����Z�b�g
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	//	WIC�t�@�C���̓ǂݍ��ݗp�֐�
	loadLambdaTable["sph"]														//	�g���q��
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	�}�����郉���_���֐�
		-> HRESULT																//	�����_���֐��̕Ԃ�l
	{
		return DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, meta, img);
	};
	//	tga�̓ǂݍ��ݗp�֐�
	loadLambdaTable["tga"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	�}�����郉���_���֐�
		-> HRESULT																//	�����_���֐��̕Ԃ�l
	{
		return DirectX::LoadFromTGAFile(path.c_str(), meta, img);
	};
	//	dds�̓ǂݍ��ݗp�֐�
	loadLambdaTable["dds"]
		= [](const std::wstring& path, DirectX::TexMetadata* meta, DirectX::ScratchImage& img)	//	�}�����郉���_���֐�
		-> HRESULT																//	�����_���֐��̕Ԃ�l
	{
		return DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, meta, img);
	};
	auto wtexpath = Helper::GetWideStringFromString(texPath);	//	���C�h������ɕϊ�
	auto ext = Helper::GetExtension(texPath);					//	�g���q���擾

	//	�e�N�X�`���[�t�@�C���̓ǂݍ���
	auto result = loadLambdaTable[ext](
		wtexpath.c_str(),
		&metadate,
		scratchImg
		);

	if (FAILED(result))
	{
		return nullptr;
	}

	//	���f�[�^���o
	auto img = scratchImg.GetImage(
		0,	//	�~�b�v���x��
		0,	//	�e�N�X�`���z����g�p����ۂ̃C���f�b�N�X
		0	//	3D�e�N�X�`���[�ɂ�����[���i�X���C�X�j
	);

	//	�e�N�X�`���o�b�t�@�̍쐬	//
	//	WriteToSubresource�œ]�����邽�߂̃q�[�v�ݒ�
	CD3DX12_HEAP_PROPERTIES tex_heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,//	���C�g�o�b�N
		D3D12_MEMORY_POOL_L0);			   //	CPU������]�����s��
	//	���\�[�X�ݒ�
	CD3DX12_RESOURCE_DESC tex_resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadate.format,
		metadate.width,
		metadate.height,
		(UINT16)metadate.arraySize,
		(UINT16)metadate.mipLevels
	);

	//	�e�N�X�`���[�o�b�t�@�̐���
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&tex_heapprop,
		D3D12_HEAP_FLAG_NONE,						//	���Ɏw��Ȃ�
		&tex_resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	//	�e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	//	�e�N�X�`���o�b�t�@�̓]��
	result = texbuff->WriteToSubresource(
		0,								//	�T�u���\�[�X�C���f�b�N�X
		nullptr,						//	�������ݗ̈�̎w��inullptr = �S�̈�ւ̃R�s�[�j
		img->pixels,					//	�������݂����f�[�^�̃A�h���X
		img->rowPitch,					//	1�s������̃f�[�^�T�C�Y
		img->slicePitch					//	�X���C�X������̃f�[�^�T�C�Y
	);
	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = texbuff;
	return texbuff;
}