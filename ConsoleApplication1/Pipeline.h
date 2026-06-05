#pragma once

#include "Common.h"

namespace Pipeline {
	
void createPSO(
	ComPtr<ID3D12Device> &mDevice, 
	ComPtr<ID3D12RootSignature> &mRootSignature, 
	ComPtr<ID3D12PipelineState> &outPSO
);

void createRootSignature(
	ComPtr<ID3D12Device> &mDevice,
	ComPtr<ID3D12RootSignature> &outRootSignature
);

}
