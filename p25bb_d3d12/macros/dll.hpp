#pragma once
#ifdef P25BBD3D12_EXPORTS
#define PECS_DLL_SHARED __declspec(dllexport)
#else
#define PECS_DLL_SHARED __declspec(dllimport)
#endif
