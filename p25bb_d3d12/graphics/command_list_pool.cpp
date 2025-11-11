#include "command_list_pool.hpp"

using PameECS::Graphics::CommandListPool;

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandListPool::GetCommandAllocator() {
	if (m_allocators.empty()) {
		return m_createCommandAllocator();
	}

	auto allocator = m_allocators.front();
	m_allocators.pop();
	return allocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandListPool::GetCommandList(ID3D12CommandAllocator* allocator) {
	if (!allocator) {
		throw Exceptions::RendererError("Allocator is null.");
	}
	if (m_command_lists.empty()) {
		return m_createCommandList(allocator);
	}
	auto commandList = m_command_lists.front();
	m_command_lists.pop();
	return commandList;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandListPool::m_createCommandAllocator() {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
	m_handleError(
		m_device->CreateCommandAllocator(
			m_type,
			IID_PPV_ARGS(&allocator)
		),
		"Failed to create command allocator."
	);

	m_registered_allocators.insert(allocator);
	return allocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandListPool::m_createCommandList(ID3D12CommandAllocator* allocator) {
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
	m_handleError(
		m_device->CreateCommandList(
			0,
			m_type,
			allocator,
			nullptr,
			IID_PPV_ARGS(commandList.GetAddressOf())
		),
		"Failed to create command list."
	);
	m_handleError(
		commandList->Close(),
		"Failed to close command list after creation."
	);
	m_registered_command_lists.insert(commandList);
	return commandList;
}

bool CommandListPool::ReturnCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator) {
	if (m_registered_allocators.find(allocator) == m_registered_allocators.end()) {
		return false;
	}

	m_allocators.emplace(std::move(allocator));
	return true;
}

bool CommandListPool::ReturnCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList) {
	if (m_registered_command_lists.find(commandList) == m_registered_command_lists.end()) {
		return false;
	}

	m_command_lists.emplace(std::move(commandList));
	return true;
}
