#pragma once
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include "../helpers/id_generator.hpp"
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <algorithm>

namespace PameECS::Thread {
	template<bool ThreadSafe = true>
	class ThreadPoolTable {
	public:
		template<TemplateTypes::StringLiteral Name>
		std::shared_ptr<BS::thread_pool<0U>> GetThreadPool() const {
			const size_t id = m_id_generator.GetId<Name>();

			lockType lock(m_mutex);

			auto it = m_thread_pools.find(id);
			if (it != m_thread_pools.end) {
				return it->second;
			}

			return nullptr;
		}

		template<TemplateTypes::StringLiteral Name>
		bool Allocate(size_t numThreads = std::thread::hardware_concurrency()) {
			numThreads = std::max<size_t>(1, numThreads);

			const size_t id = m_id_generator.GetId<Name>();
			lockType lock(m_mutex);

			auto [it, inserted] = m_thread_pools.emplace(id, nullptr);
			if (it->second) return false;
			it->second = std::make_shared<BS::thread_pool<0U>>(numThreads);

			return true;
		}
	private:
		using lockType = std::conditional_t<ThreadSafe, std::lock_guard<std::mutex>, DummyLock>;
		Helpers::IdGenerator<ThreadSafe> m_id_generator;
		std::unordered_map<size_t, std::shared_ptr<BS::thread_pool<0U>>> m_thread_pools;
		std::mutex m_mutex;
	};
}
