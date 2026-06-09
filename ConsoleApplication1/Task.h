#pragma once

#include "Common.h"

class Task
{
public:
	virtual void execute(
		ComPtr<ID3D12GraphicsCommandList> &commandList, 
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		D3D12_CPU_DESCRIPTOR_HANDLE *rtvHandle
	) = 0;

	// Shader Resources
	ComPtr<ID3D12RootSignature> mRootSignature;
	// Pipelines
	ComPtr<ID3D12PipelineState> mPipelineState;
};

class DrawTask : public Task
{
public:
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mVertexBufferUpload;
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap = {};
	D3D12_GPU_DESCRIPTOR_HANDLE mSRVTableHandle = {};
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};

	void execute(
		ComPtr<ID3D12GraphicsCommandList> &commandList, 
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		D3D12_CPU_DESCRIPTOR_HANDLE *rtvHandle
	) override;
};

// Camera buffer information - this should be separately stored outside of compute task...
struct CameraConstants {
	XMFLOAT4X4 viewProjInverse;
	XMFLOAT4 cameraPosition;
	uint32_t frameNumber;
	uint32_t padding0[3];
};

class ComputeTask : public Task
{
public:
	// These resources shouldn't exist for all compute tasks, but alas.
	ComPtr<ID3D12Resource> mOutputTexture;
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE mOutputUAVHandle = {};

	// Camera buffer information - this should be separately stored outside of compute task...
	ComPtr<ID3D12Resource> mCameraConstantBuffer;
	UINT8* mCameraConstantBufferData;
	CameraConstants mCameraConstants;

	// Dispatch info
	uint32_t mDispatchWidth, mDispatchHeight;

	void execute(
		ComPtr<ID3D12GraphicsCommandList> &commandList, 
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		D3D12_CPU_DESCRIPTOR_HANDLE *rtvHandle = nullptr
	) override;
};

