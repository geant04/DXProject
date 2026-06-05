#include "Mesh.h"

void Mesh::UploadMeshBuffer(
	ComPtr<ID3D12Device> &device,
	ComPtr<ID3D12Resource> &vertexBuffer,
	ComPtr<ID3D12Resource> &vertexBufferUpload,
	D3D12_VERTEX_BUFFER_VIEW &vertexBufferView,
	ComPtr<ID3D12CommandAllocator> &mCommandAllocator,
	ComPtr<ID3D12GraphicsCommandList> &mCommandList,
	ComPtr<ID3D12CommandQueue> &mCommandQueue,
	ComPtr<ID3D12PipelineState> &pipelineState
)
{
	// Upload CPU to GPU memory using defaultHeap
	// Should we share the same heap when uploading info?
	CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mVertexBufferSize);

	// Vertex buffer will reside on the GPU as a default heap
	device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	);
		
	// Upload heap on CPU, sends data to vertex buffer on GPU
	// The purpose for this is to prepare our buffer data on system RAM to be uploaded to the GPU later
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
		
	device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBufferUpload)
	);

	// Copy memory to upload heap
	// We're strictly writing, so no need to define a read range.
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange = {0, 0};
	vertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, mVertices.data(), mVertexBufferSize);
	vertexBufferUpload->Unmap(0, nullptr);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = mVertexBufferSize;

	// Execute the buffer upload
	mCommandAllocator->Reset();
	mCommandList->Reset(mCommandAllocator.Get(), pipelineState.Get());

	mCommandList->CopyBufferRegion(
		vertexBuffer.Get(), 0,
		vertexBufferUpload.Get(), 0,
		mVertexBufferSize
	);
		
	// We were originally D3D12_RESOURCE_STATE_COPY_DEST,
	// need to transition this to a vertex buffer so the shader can interpret the resource properly I assume
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		vertexBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
	);

	mCommandList->ResourceBarrier(1, &barrier);

	mCommandList->Close();

	ID3D12CommandList* ppCommandList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
}