#pragma once

#include "Common.h"
#include "Task.h"

struct WindowProperties
{
	HWND g_hWnd;
	int mWidth;
	int mHeight;
	float aspectRatio;
};

class MinimalDXApp
{
	// Baseline boiler plate information storing everything needed for a simple DX12 app.
	// This includes the following:
	//  - DXGI Factory + Adapter + Device setup to allow our program to communicate to the GPU + locate GPU to work on
	//  - Command Queue + Command List to execute commands, in this case we only care about a command queue to draw
	//  - Command Allocator, ?? not sure what this does yet, but it's important
	//  - Swap Chain, this stores our framebuffers that we will query and draw to per frame?
	//  - Descriptor Heaps + RTVs, this stores a table on the GPU to access Render Target Views/Views, allowing us to see GPU memory
	//  - PSO... stores vertex/pixel shader information, providing proper layouts for the respective shaders
	//  - Fences/Synchronization to ensure we can execute commands on the GPU and run CPU code subsequently
public:
	void initializeDX();
	void loadAssets();
	void waitForPreviousFrame();
	void createWindow(HINSTANCE hInstnace, int nCmdShow);

	void render();
	void run();
	void addTask(std::unique_ptr<Task> task);

	// Something sus, but we roll with it for now.
	// Do something more advanced later, like multiple buffering?
	static const uint32_t mFrameCount = 2;
	WindowProperties mWindowProperties = {};

private:
	ComPtr<IDXGIFactory7> mFactory;
	ComPtr<IDXGIAdapter1> mAdapter;
	ComPtr<ID3D12Device> mDevice;
	
	// Buffer writing stuff
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<IDXGISwapChain4> mSwapChain;

	// Resources
	ComPtr<ID3D12DescriptorHeap> mRTVDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mSRVHeap;
	ComPtr<ID3D12DescriptorHeap> mTextureHeap;
	ComPtr<ID3D12Resource> mTextureUploadHeap;
	ComPtr<ID3D12Resource> mRenderTargets[mFrameCount];
	ComPtr<ID3D12Resource> mTexture;

	ComPtr<ID3D12Fence> mFence;
	HANDLE mFenceEvent;
	UINT64 mFenceValue = 0;

	// Miscellanious things
	std::vector<std::unique_ptr<Task>> mTasks;

	uint32_t mFrameIndex;
	uint32_t mRTVDescriptorSize;
};

