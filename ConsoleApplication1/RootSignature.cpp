#include "RootSignature.h"

namespace RootSignature 
{

// Root signatures address what resources the shader can see on the GPU
// I think, like, sadly this gets so much more freakingly complicated when we try to do stuff like 
// binding textures and what not. Eh, that's fine - we'll just get there when we get there, you know
// You'd link one root signature per pipeline essentially, this root signature defines what's linked to the shader
void createRootSignature(
	ComPtr<ID3D12Device> &mDevice,
	ComPtr<ID3D12RootSignature> &outRootSignature
)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// One SRV, if we have more then scale accordingly
	// Static flag means descriptor won't change dynamically, determined at compile time
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	// Only pixel? I wonder if this can be used for vert as well.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	// I should probably make some generic sampler builder class in the future, ya know?
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(
		_countof(rootParameters), 
		rootParameters, 
		1, 
		&sampler, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
		
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	// Converts root signature to a binary blob that the GPU can read
	// Not sure what a versioned root signature is though
	D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
	mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&outRootSignature));
}

// Different root signature for the compute pipeline
// TODO: There is a lot of duplicated code between this root-signature generation and the prior one.
void createPathTracingRootSignature(
	ComPtr<ID3D12Device> &mDevice,
	ComPtr<ID3D12RootSignature> &outRootSignature
)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Define two descriptor ranges for our RWTexture2D path-traced output (UAV), and one for our input (SRV).
	// SRV will store data such as camera info and scene geo.
	CD3DX12_DESCRIPTOR_RANGE1 UAVDescriptorRange = {};
	UAVDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE1 SRVDescriptorRange = {};
	SRVDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptorRanges = {UAVDescriptorRange, SRVDescriptorRange};

	CD3DX12_ROOT_PARAMETER1 rootParameter;
	rootParameter.InitAsDescriptorTable(2, descriptorRanges.data());

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init_1_1(1, &rootParameter);
	
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);

	if (FAILED(hr)) {
		return std::perror("[ERROR CS]: Cannot serialize versioned root signature");
	}

	hr = mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&outRootSignature));
	if (FAILED(hr)) {
		return std::perror("[ERROR CS]: Cannot create PT root signature");
	}
}

}