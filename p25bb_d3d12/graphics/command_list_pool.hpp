#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <unordered_set>
#include <set>
#include <queue>

#include "renderer_types.hpp"
#include "../exceptions/renderer_error.hpp"
#include "../helpers/errors/windows.hpp"

namespace PameECS::Graphics {
	// コマンドリストは自動でResetされない
	class CommandListPool {
	public:
		CommandListPool(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
			: m_device(std::move(device)), m_type(type) {
			if (!m_device) {
				throw Exceptions::RendererError("Device is null.");
			}
		}
		~CommandListPool() = default;

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList(ID3D12CommandAllocator* allocator);
		bool ReturnCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);
		bool ReturnCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_createCommandAllocator();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_createCommandList(ID3D12CommandAllocator* allocator);

		void m_handleError(HRESULT result, const std::string& message) {
			Helpers::Errors::Windows::HandleHRESULTError<Exceptions::RendererError>(result, message);
		}

		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		D3D12_COMMAND_LIST_TYPE m_type;

		std::queue<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocators;
		std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>> m_command_lists;

		std::unordered_set<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, RendererTypes::ComPtrHash, RendererTypes::ComPtrEqual> m_registered_allocators;
		std::unordered_set<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>, RendererTypes::ComPtrHash, RendererTypes::ComPtrEqual> m_registered_command_lists;
	};
}
