#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#include <wrl/client.h>
#include <functional>

namespace PameECS::Graphics::RendererTypes {
	struct RenderCommand {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	using RenderTask = std::function<RenderCommand(RenderCommand)>;

	struct ComPtrHash {
		size_t operator()(const Microsoft::WRL::ComPtr<IUnknown>& ptr) const {
			return std::hash<void*>()(ptr.Get());
		}
	};

	struct ComPtrEqual {
		bool operator()(const Microsoft::WRL::ComPtr<IUnknown>& lhs, const Microsoft::WRL::ComPtr<IUnknown>& rhs) const {
			return lhs.Get() == rhs.Get();
		}
	};
}
