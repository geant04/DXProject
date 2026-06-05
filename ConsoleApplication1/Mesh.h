#pragma once

#include "Common.h"

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
	XMFLOAT2 uv;
};

class Mesh
{
public:
	std::vector<Vertex> mVertices;
	const uint32_t mVertexBufferSize;

	// Question: Should the buffer belong to the Mesh, or the DrawTask?
	// It may make more sense to have it belong to the DrawTask.
	void UploadMeshBuffer(
		ComPtr<ID3D12Device> &device,
		ComPtr<ID3D12Resource> &vertexBuffer,
		ComPtr<ID3D12Resource> &vertexBufferUpload,
		D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
		ComPtr<ID3D12CommandAllocator> &mCommandAllocator,
		ComPtr<ID3D12GraphicsCommandList> &mCommandList,
		ComPtr<ID3D12CommandQueue> &mCommandQueue,
		ComPtr<ID3D12PipelineState> &pipelineState
	);

	Mesh(
		std::vector<Vertex> vertices, 
		const uint32_t bufferSize
	) : mVertices(vertices), mVertexBufferSize(bufferSize)
	{};
};

