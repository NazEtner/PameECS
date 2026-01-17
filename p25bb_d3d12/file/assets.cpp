#include "assets.hpp"
#include "file.hpp"
#include <future>
#include <sstream>
#include <mutex>
#include <unordered_map>

using PameECS::File::Assets;
using PameECS::File::AssetHandle;
using PameECS::File::AsyncTicket;

namespace {
	struct TicketInfo {
		std::future<std::stringstream> future;
	};
	
	struct HandleInfo {
		std::string path;
		uint32_t generation = 0;
		std::unordered_map<uint8_t*, std::unique_ptr<std::vector<uint8_t>>> loadedData;
		std::unordered_map<uint32_t, TicketInfo> tickets;
		bool active = true;
	};
}

struct Assets::Impl {
	std::mutex mutex;
	std::unordered_map<uint32_t, HandleInfo> handles;
};

Assets::Assets() {
	m_impl = std::make_unique<Impl>();
}

Assets::~Assets() {
}

bool Assets::IsValidHandle(const AssetHandle& handle) const {
	std::lock_guard lock(m_impl->mutex);

	auto it = m_impl->handles.find(handle.id);
	return it != m_impl->handles.end() && it->second.generation == handle.generation;
}

bool Assets::IsValidTicket(const AsyncTicket& ticket) const {
	std::lock_guard lock(m_impl->mutex);

	auto it = m_impl->handles.find(ticket.handle.id);
	if (it == m_impl->handles.end() || it->second.generation != ticket.handle.generation) {
		return false;
	}

	auto ticketIt = it->second.tickets.find(ticket.id);
	return ticketIt != it->second.tickets.end();
}

AssetHandle Assets::GetAssetHandle(const char* path, size_t pathSize) {
	std::lock_guard lock(m_impl->mutex);

	for (const auto& [id, handleInfo] : m_impl->handles) {
		if (handleInfo.path == std::string(path, pathSize) && handleInfo.active) {
			AssetHandle handle = {};
			handle.id = id;
			handle.generation = handleInfo.generation;
			return handle;
		}
	}

	uint32_t newId = static_cast<uint32_t>(m_impl->handles.size() + 1);
	m_impl->handles[newId] = HandleInfo{ std::string(path, pathSize), 1, {}, {}, true };

	AssetHandle handle = {};
	handle.id = newId;
	handle.generation = 1;
	return handle;
}

AsyncTicket Assets::Load(const AssetHandle& handle) {
	std::lock_guard lock(m_impl->mutex);

	auto it = m_impl->handles.find(handle.id);
	if (it == m_impl->handles.end() || it->second.generation != handle.generation) {
		return AsyncTicket{};
	}

	uint32_t newTicketId = static_cast<uint32_t>(it->second.tickets.size() + 1);

	auto file = File<0, 0>(); // ImplementIdとGlobalStateIdは0で固定
	auto future = file.LoadAsync(it->second.path);
	it->second.tickets[newTicketId] = TicketInfo{ std::move(future) };

	AsyncTicket ticket = {};
	ticket.id = newTicketId;
	ticket.handle = handle;
	return ticket;
}

uint8_t* Assets::GetData(AsyncTicket& ticket, size_t& outSize) {
	std::lock_guard lock(m_impl->mutex);

	auto handleIt = m_impl->handles.find(ticket.handle.id);
	if (handleIt == m_impl->handles.end() || handleIt->second.generation != ticket.handle.generation) {
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}

	auto& handleInfo = handleIt->second;
	auto ticketIt = handleInfo.tickets.find(ticket.id);
	if (ticketIt == handleInfo.tickets.end()) {
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}

	auto& ticketInfo = ticketIt->second;
	if (ticketInfo.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return nullptr;
	}
	try {
		auto ss = ticketInfo.future.get();
		handleInfo.tickets.erase(ticketIt);

		auto data = std::make_unique<std::vector<uint8_t>>(
			std::istreambuf_iterator<char>(ss),
			std::istreambuf_iterator<char>());

		outSize = data->size();
		uint8_t* dataPtr = data->data();
		handleInfo.loadedData[dataPtr] = std::move(data);

		ticket.id = 0;
		ticket.handle.generation = 0;

		return dataPtr;
	}
	catch (...) {
		handleInfo.tickets.erase(ticketIt);
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}
}

uint8_t* Assets::GetDataSync(AsyncTicket& ticket, size_t& outSize) {
	std::lock_guard lock(m_impl->mutex);

	auto handleIt = m_impl->handles.find(ticket.handle.id);
	if (handleIt == m_impl->handles.end() || handleIt->second.generation != ticket.handle.generation) {
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}

	auto& handleInfo = handleIt->second;
	auto ticketIt = handleInfo.tickets.find(ticket.id);
	if (ticketIt == handleInfo.tickets.end()) {
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}

	auto& ticketInfo = ticketIt->second;
	try {
		auto ss = ticketInfo.future.get();
		handleInfo.tickets.erase(ticketIt);

		auto data = std::make_unique<std::vector<uint8_t>>(
			std::istreambuf_iterator<char>(ss),
			std::istreambuf_iterator<char>());

		outSize = data->size();
		uint8_t* dataPtr = data->data();
		handleInfo.loadedData[dataPtr] = std::move(data);

		ticket.id = 0;
		ticket.handle.generation = 0;

		return dataPtr;
	}
	catch (...) {
		handleInfo.tickets.erase(ticketIt);
		ticket.id = 0;
		ticket.handle.generation = 0;
		return nullptr;
	}
}

void Assets::ReleaseData(const AssetHandle& handle, uint8_t* data) {
	std::lock_guard lock(m_impl->mutex);

	auto handleIt = m_impl->handles.find(handle.id);
	if (handleIt == m_impl->handles.end() || handleIt->second.generation != handle.generation) {
		return;
	}

	auto& handleInfo = handleIt->second;
	auto dataIt = handleInfo.loadedData.find(data);
	if (dataIt != handleInfo.loadedData.end()) {
		handleInfo.loadedData.erase(dataIt);
	}
}

void Assets::ReleaseHandle(const AssetHandle& handle) {
	std::lock_guard lock(m_impl->mutex);

	auto handleIt = m_impl->handles.find(handle.id);
	if (handleIt == m_impl->handles.end() || handleIt->second.generation != handle.generation) {
		return;
	}

	handleIt->second.active = false;
	handleIt->second.generation++;
	handleIt->second.loadedData.clear();
	handleIt->second.tickets.clear();
}

extern "C" {
	PECS_DLL_SHARED bool FileAssetsIsValidHandle(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle& handle) {
		return assets->IsValidHandle(handle);
	}
	PECS_DLL_SHARED bool FileAssetsIsValidTicket(
		PameECS::File::Assets* assets,
		const PameECS::File::AsyncTicket& ticket) {
		return assets->IsValidTicket(ticket);
	}
	PECS_DLL_SHARED void FileAssetsGetAssetHandle(
		PameECS::File::Assets* assets,
		PameECS::File::AssetHandle& outHandle,
		const char* path,
		size_t pathSize) {
		outHandle = assets->GetAssetHandle(path, pathSize);
	}
	PECS_DLL_SHARED void FileAssetsLoad(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket& outTicket,
		const PameECS::File::AssetHandle& handle) {
		outTicket = assets->Load(handle);
	}
	PECS_DLL_SHARED uint8_t* FileAssetsGetData(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket& ticket,
		size_t& outSize) {
		return assets->GetData(ticket, outSize);
	}
	PECS_DLL_SHARED uint8_t* FileAssetsGetDataSync(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket& ticket,
		size_t& outSize) {
		return assets->GetDataSync(ticket, outSize);
	}
	PECS_DLL_SHARED void FileAssetsReleaseData(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle& handle,
		uint8_t* data) {
		assets->ReleaseData(handle, data);
	}
	PECS_DLL_SHARED void FileAssetsReleaseHandle(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle& handle) {
		assets->ReleaseHandle(handle);
	}
}
