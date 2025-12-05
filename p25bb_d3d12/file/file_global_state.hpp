#pragma once
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <memory>

namespace PameECS::File {
	template<size_t UniqueId = 0>
	class FileGlobalState final {
	public:
		std::shared_ptr<BS::thread_pool<0U>>& GetThreadPool() {
			return m_getState()->threadPool;
		}
	private:
		struct State {
			std::shared_ptr<BS::thread_pool<0U>> threadPool;
		};
		State* m_getState() {
			static State state = {};
			return &state;
		}
	};
}
