#include "Pipeline.h"

namespace Pipeline {

void createPSO(
	ComPtr<ID3D12Device> &mDevice, 
	ComPtr<ID3D12RootSignature> &mRootSignature, 
	ComPtr<ID3D12PipelineState> &outPSO,
	const wchar_t* vsAssetName,
	const wchar_t* psAssetName
)
{
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

	// For debugging, you might wantt o have the following flags:
	// D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
	// This way, if we use graphics debugging tools, we get fun symbols and stuff.
	int32_t compileFlags = 0;

	auto vsPath = GetAssetFullPath(vsAssetName);
	auto psPath = GetAssetFullPath(psAssetName);

	D3DCompileFromFile(vsPath.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
	D3DCompileFromFile(psPath.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

	D3D12_SHADER_BYTECODE vsByteCode = {};
	vsByteCode.pShaderBytecode = vertexShader->GetBufferPointer();
	vsByteCode.BytecodeLength = vertexShader->GetBufferSize();

	D3D12_SHADER_BYTECODE psByteCode = {};
	psByteCode.pShaderBytecode = pixelShader->GetBufferPointer();
	psByteCode.BytecodeLength = pixelShader->GetBufferSize();

	// Vertex shader specification data is hard-coded here, as specificed by the HelloTriangle example.
	// When we have more complex objects, we will refactor this accordingly.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12 + 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// PSO building time!
	// The root signature gets WAYYY more complicated when we start adding more shit to it.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = vsByteCode;
	psoDesc.PS = psByteCode;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&outPSO));

	if (outPSO == NULL) {
		// Problem detected, need to log this!
		return;
	}
}

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

}