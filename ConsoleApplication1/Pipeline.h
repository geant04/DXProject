#pragma once

#include "Common.h"

namespace Pipeline {
	
void createPSO(
	ComPtr<ID3D12Device> &mDevice, 
	ComPtr<ID3D12RootSignature> &mRootSignature, 
	ComPtr<ID3D12PipelineState> &outPSO,
	const wchar_t* vsAssetName,
	const wchar_t* psAssetName
);

void createComputePSO(
	ComPtr<ID3D12Device> &mDevice, 
	ComPtr<ID3D12RootSignature> &mRootSignature, 
	ComPtr<ID3D12PipelineState> &outPSO,
	const wchar_t* csAssetName,
	const char* entryPoint
);

}
