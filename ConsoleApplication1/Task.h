#pragma once

#include "Common.h"

class Task
{
public:
	// Shader Resources
	ComPtr<ID3D12RootSignature> mRootSignature;
	// Pipelines
	ComPtr<ID3D12PipelineState> mPipelineState;

	virtual void execute(
		ComPtr<ID3D12GraphicsCommandList> &commandList, 
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		ComPtr<ID3D12Resource> &renderTarget,
		D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle
	) = 0;
};

class DrawTask : Task
{
public:
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mVertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	void execute(
		ComPtr<ID3D12GraphicsCommandList> &commandList, 
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		ComPtr<ID3D12Resource> &renderTarget,
		D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle
	) override;
};

