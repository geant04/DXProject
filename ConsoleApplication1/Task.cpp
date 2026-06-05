#include "Task.h"

void DrawTask::execute(
	ComPtr<ID3D12GraphicsCommandList> &commandList, 
	ComPtr<ID3D12CommandAllocator> &commandAllocator,
    ComPtr<ID3D12Resource> &renderTarget,
    D3D12_CPU_DESCRIPTOR_HANDLE &rtvHandle
)
{
	// Bind the root signature
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
	commandList->SetPipelineState(mPipelineState.Get());

    // Bind RTV
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Bind vertex buffer and draw
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);
}
