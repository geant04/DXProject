#include "MinimalDXApp.h"
#include "Pipeline.h"
#include "Mesh.h"
#include <iostream>

#define SIMPLE_TEST false

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_DESTROY: PostQuitMessage(0); return 0;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void MinimalDXApp::createWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Populate the WindowProperties struct with stuff copied from GPT.
	// Not gonna bother figuring out how the window is made. Not the priority here.
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DX12WindowClass";
	RegisterClassEx(&wc);

	mWindowProperties.mHeight = 720;
	mWindowProperties.mWidth = 1280;
	mWindowProperties.aspectRatio = static_cast<float>(mWindowProperties.mWidth) / static_cast<float>(mWindowProperties.mHeight);

	RECT rect = {0, 0, (LONG)mWindowProperties.mWidth, (LONG)mWindowProperties.mHeight};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	mWindowProperties.g_hWnd = CreateWindow(
		wc.lpszClassName,
		L"DirectX 12 Triangle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr, nullptr, hInstance, nullptr
	);

	ShowWindow(mWindowProperties.g_hWnd, nCmdShow);
}


/*
Very straightforward program, goes over the esssential basics to making sure our DX12 program can run smoothly.
The code works in a very sequential order, initializing everything important we need.
I think there should be a destructor of some sort implemented later on, but we'll worry about it later.
*/
void MinimalDXApp::initializeDX()
{
	// Begin with the DXGI Factory. 
	// DXGI is the bridge between GPU/driver and the code
	// Not sure what IID_PPV_ARGS does, but returns interface pointer... whatever that means
	CreateDXGIFactory2(0, IID_PPV_ARGS(&mFactory));

	// I do need to check that this isn't my integrated graphics, I might have multiple adapters
	mFactory->EnumAdapters1(0, &mAdapter);
	
	// Create the device
	D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));

	{
		// Create Command Queue, requires a description. This is pretty similar to Vulkan!
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};


		// D3D12 offers several different command list types:
		// https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles
		// - Direct: Capable of handling everything apaprently 3D (graphics, compute, copy)
		// - Compute: I assume this handles compute shaders, similar to a compute queue.
		// - Bundle: ? Not sure - consists of multiple API calls... 
		// - Copy: ? Not sure, just repsonsible for copying memory between resources on GPU
		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		mDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCommandQueue));
	}

	// Create swap chain
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = mFrameCount;
		swapChainDesc.Width = mWindowProperties.mWidth;
		swapChainDesc.Height = mWindowProperties.mHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// The frames in this swap chain have render target writing capability
		// Retrieved buffers are called back-buffers, written to and then later drawn to the screen
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		// This has to do with multisampling I think?
		swapChainDesc.SampleDesc.Count = 1;
	
		ComPtr<IDXGISwapChain1> swapChain;
		mFactory->CreateSwapChainForHwnd(
			mCommandQueue.Get(), 
			mWindowProperties.g_hWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		);

		swapChain.As(&mSwapChain);
	}

	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	// Create RTV Descriptor Heap + Texture Heap for our awesome texture
	{
		// I'm afraid this gets a bit complicated when we want to do stuff like
		// add textures, requiring SRVs and UAVs
		// SRV - Shader Resource View
		// UAV - Unordered Resource View
		// RTV - Render Target View
		D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc = {};
		RTVDescriptorHeapDesc.NumDescriptors = mFrameCount;
		RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		mDevice->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(&mRTVDescriptorHeap));
		
		mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// So yes, I was correct that this part gets complicated when we add SRV heaps. My question, though, is that if we 
		// want to bind multiple textures, does it just require one heap, or many?
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};

		// In theory, if I wanted multiple textures, then I'd adjust the number of descriptors.
		// This is overly simplistic, though! It does not scale at all.
		// To scale well, we'd instead need a bigger descriptor heap with, say, like 10000 descriptors.
		// I think this is pretty similar to the bind group in terms of defining visibility as well.

		// Descriptor is a metadata record. Stored in the descriptor heap
		// This should be called the textureHeap or something in the future, should I add more textures to it
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSRVHeap));
	}

	// Create frame resources in the swap chain, can we think of this as populating frame data per backbuffer in the swapchain?
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		for (uint32_t i = 0; i < mFrameCount; i++)
		{
			mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i]));
			mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += mRTVDescriptorSize;
		}
	}

	// Might need a reminder of mCommandAllocator's purpose?
	mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));

	// Create synchronization stage
	{
		mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
		mFenceValue = 1;
		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent == nullptr)
		{
			// TODO: Need some error handling/validation
		}
	}
}


// Responsible for PSO creation and compiling vert/pixel shaders
void MinimalDXApp::loadAssets()
{
	std::unique_ptr<DrawTask> drawHelloTriangle = std::make_unique<DrawTask>();

	Pipeline::createRootSignature(
		mDevice,
		drawHelloTriangle->mRootSignature
	);

	Pipeline::createPSO(
		mDevice,
		drawHelloTriangle->mRootSignature,
		drawHelloTriangle->mPipelineState,
		L"vsTriangle.hlsl",
		L"psTriangle.hlsl"
	);

	// We need to make the command list that'll execute based off of the PSO we made
	mDevice->CreateCommandList(
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		mCommandAllocator.Get(), 
		drawHelloTriangle->mPipelineState.Get(), 
		IID_PPV_ARGS(&mCommandList)
	);
	
	// Immediately close it since we're not recording anything
	mCommandList->Close();

	// Create helloTriangle mesh information
	{
		float aspectRatio = mWindowProperties.aspectRatio;

		std::vector<Vertex> triangleVertices =
        {
			{ { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f } },
			{ { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
        };

		const uint32_t vertexBufferSize = static_cast<uint32_t>(triangleVertices.size() * sizeof(Vertex));

		Mesh helloTriangleMesh = Mesh(triangleVertices, vertexBufferSize);

		// Vertex buffer data upload call
		helloTriangleMesh.UploadMeshBuffer(
			mDevice,
			drawHelloTriangle->mVertexBuffer,
			drawHelloTriangle->mVertexBufferUpload,
			drawHelloTriangle->mVertexBufferView,
			mCommandAllocator,
			mCommandList,
			mCommandQueue,
			drawHelloTriangle->mPipelineState
		);
		
		waitForPreviousFrame();

		// Reset this until we need to do another vertex buffer upload, not needed anymore since we copied relevant info over
		drawHelloTriangle->mVertexBufferUpload.Reset();
	}

	// Texture creation stage
	// By this point, we've properly setup our awesome umm everythings!
	// This includes a descriptor heap for the SRVs, and a modified root signature
	// that now proudly includes a slot to store our first SRV and 1 static sampler.
	// The heap only has one descriptor for our SRV. According to GPT, most production renderers have multiple.
	// Overall memory binding process from my understanding:
	// 1. Define descriptor heap for your resource, jotting down num of descriptors and what types of descriptors are there
	// 2. In your root signature param, init our SRV in the first slot of our descriptor heap
	// 3. Create root signature using our param, this now has our first SRV. I imagine for CBVs, you just init it and it's straight forward
	// 4. Bind any samplers necessary to the root signature
	// 5. Ensure that your PSO now uses this root signature so that it can actually do stuff
	// 6. CommandList goes brrr

	// However, I don't think we ever went over how to actually view the texture data, or I assume link the SRV to actual texture info
	// on the GPU, which I assume is what this section now covers in high detail.
	// Exciting! I think if this works, then I can set up the CBV and get a triangle spinning in no time.
	// Another goal past this point is maybe figuring out how to use a UAV to write to a texture next, and do some exciting post-processing.
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.MipLevels = 1;
		textureDesc.Width = 256;
		textureDesc.Height = 256;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;


		CD3DX12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		mDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&mTexture)
		);

		UINT64 uploadBufferSize = GetRequiredIntermediateSize(mTexture.Get(), 0, 1);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		mDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mTextureUploadHeap)
		);

		// Populate texture data, define it on host
		UINT rowPitch = 256 * 4;
		UINT cellPitch = rowPitch >> 3;
		UINT cellHeight = 256 >> 3;
		UINT textureSize = rowPitch * 256;

		std::vector<UINT8> texture(textureSize);
		UINT8* pData = &texture[0];

		for (UINT n = 0; n < textureSize; n += 4)
		{
			UINT x = n % rowPitch;
			UINT y = n / rowPitch;
			UINT i = x / cellPitch;
			UINT j = y / cellHeight;

			if (i % 2 == j % 2)
			{
				pData[n] = 0x00;
				pData[n+1] = 0x00;
				pData[n+2] = 0x00;
				pData[n+3] = 0x00;
			}
			else
			{
				pData[n] = 0xff;
				pData[n+1] = 0xff;
				pData[n+2] = 0xff;
				pData[n+3] = 0xff;
			}
		}

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &texture[0];
		textureData.RowPitch = 256 * 4;
		textureData.SlicePitch = textureData.RowPitch * 256;

		// Execute texture upload command
		mCommandAllocator->Reset();
		mCommandList->Reset(mCommandAllocator.Get(), drawHelloTriangle->mPipelineState.Get());
		
		UpdateSubresources(mCommandList.Get(), mTexture.Get(), mTextureUploadHeap.Get(), 0, 0, 1, &textureData);
		CD3DX12_RESOURCE_BARRIER mTexCopyToPSBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mCommandList->ResourceBarrier(1, &mTexCopyToPSBarrier);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		
		mDevice->CreateShaderResourceView(mTexture.Get(), &srvDesc, mSRVHeap->GetCPUDescriptorHandleForHeapStart());

		// Bombs away!
		mCommandList->Close();
		ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	// Sync
	{
		waitForPreviousFrame();
	}

	mTasks.push_back(std::move(drawHelloTriangle));
}

void MinimalDXApp::addTask(std::unique_ptr<Task> task)
{
	mTasks.push_back(std::move(task));
}

void MinimalDXApp::render()
{
    // Get the current back buffer handle
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex * mRTVDescriptorSize;

	// Reset command allocator & list
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), nullptr);

    // Set viewport and scissor rect
    D3D12_VIEWPORT viewport = {0.0f, 0.0f, static_cast<float>(mWindowProperties.mWidth), static_cast<float>(mWindowProperties.mHeight), 0.0f, 1.0f};
    D3D12_RECT scissorRect = {0, 0, mWindowProperties.mWidth, mWindowProperties.mHeight};
	mCommandList->RSSetViewports(1, &viewport);
    mCommandList->RSSetScissorRects(1, &scissorRect);

	// Transition the back buffer to render target
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		mRenderTargets[mFrameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
    mCommandList->ResourceBarrier(1, &barrier);

    // Clear the render target
    FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	for (auto& task : mTasks)
	{	
		task->execute(mCommandList, mCommandAllocator, mRenderTargets[mFrameIndex], rtvHandle);
	}
	
    // Transition back buffer to present
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mRenderTargets[mFrameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    mCommandList->ResourceBarrier(1, &barrier);

    // Close and execute
    mCommandList->Close();

	// ACTUAL rendering part
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present
    mSwapChain->Present(1, 0);

    // Wait for GPU to finish
    waitForPreviousFrame();
}

void MinimalDXApp::run()
{
	// When render is done, we call it like a helper function.
	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			render();
		}
	}
}

void MinimalDXApp::waitForPreviousFrame()
{
	const UINT64 fence = mFenceValue;
	mCommandQueue->Signal(mFence.Get(), fence);
	mFenceValue++;

	if ( mFence->GetCompletedValue() < fence)
	{
		mFence->SetEventOnCompletion(fence, mFenceEvent);
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}