#pragma once
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "file_implementation.hpp"

namespace PameECS::File {
	template<size_t UniqueId = 0>
	class FileGlobalState final {
	public:
		std::shared_ptr<BS::thread_pool<0U>>& GetThreadPool() {
			return m_getState()->threadPool;
		}

		template<size_t ImplementId>
		FileImplementation* GetFileImplementation() {
			std::lock_guard lock(m_mutex);
			auto& ret = m_getState()->implementations[ImplementId];
			if (!ret) {
				ret = std::make_unique<FileImplementation>(m_getState()->threadPool);
			}
			return ret.get();
		}

		void Reset() {
			m_getState()->threadPool.reset();
			m_getState()->implementations.clear();
		}
	private:
		std::mutex m_mutex;
		struct State {
			std::shared_ptr<BS::thread_pool<0U>> threadPool;
			std::unordered_map<size_t, std::unique_ptr<FileImplementation>> implementations;
		};
		State* m_getState() {
			static State state = {};
			return &state;
		}
	};
}
