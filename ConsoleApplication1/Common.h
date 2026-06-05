#pragma once

// Does this ideally include all of our necessary DirectX libraries?
#include "WinInclude.h" 

#include <wrl/client.h>
#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h> 
#include <string>
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

inline std::wstring GetAssetFullPath(LPCWSTR assetName)
{
	// Ugly hardcoding for now, but we work with it...
    return L"..\\shaders\\" + std::wstring(assetName);
}