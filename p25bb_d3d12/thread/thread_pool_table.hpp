#pragma once
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include "../helpers/id_generator.hpp"
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <algorithm>

#include "../exceptions/invalid_operation.hpp"

namespace PameECS::Thread {
	// テンプレート毎にシングルトン(唯一のインスタンスのみ許可するクラス)であるが、グローバルで取得できないようにする
	template<bool ThreadSafe = true, size_t Id = 0>
	class ThreadPoolTable {
	public:
		ThreadPoolTable() {
			if (!m_there_is_no_thread_pool) {
				throw Exceptions::InvalidOperation("Only one instance of ThreadPoolTable is allowed.");
			}
			m_there_is_no_thread_pool = false;

#ifdef _DEBUG
			if constexpr (!ThreadSafe) {
				m_creation_thread_id = std::this_thread::get_id();
			}
#endif
		}

		~ThreadPoolTable() {
			m_there_is_no_thread_pool = true;
		}

		ThreadPoolTable(const ThreadPoolTable&) = delete;
		ThreadPoolTable& operator=(const ThreadPoolTable&) = delete;

		template<TemplateTypes::StringLiteral Name>
		std::shared_ptr<BS::thread_pool<0U>> GetThreadPool() {
#ifdef _DEBUG
			m_checkThreadSafety();
#endif
			const size_t id = m_id_generator.GetId<Name>();

			lockType lock(m_mutex);

			auto it = m_thread_pools.find(id);
			if (it != m_thread_pools.end()) {
				return it->second;
			}

			return nullptr;
		}

		template<TemplateTypes::StringLiteral Name>
		bool Allocate(size_t numThreads = std::thread::hardware_concurrency()) {
#ifdef _DEBUG
			m_checkThreadSafety();
#endif
			numThreads = std::max<size_t>(1, numThreads);

			const size_t id = m_id_generator.GetId<Name>();
			lockType lock(m_mutex);

			auto [it, inserted] = m_thread_pools.emplace(id, nullptr);
			if (it->second) return false;
			it->second = std::make_shared<BS::thread_pool<0U>>(numThreads);

			return true;
		}
	private:
#ifdef _DEBUG
		void m_checkThreadSafety() {
			if constexpr (!ThreadSafe) {
				if (std::this_thread::get_id() != m_creation_thread_id) {
					throw Exceptions::InvalidOperation(
						"ThreadPoolTable<ThreadSafe=false> is being accessed from a different thread than it was created on. "
						"This is not thread-safe. Use ThreadSafe=true if multi-threaded access is needed."
					);
				}
			}
		}

		std::thread::id m_creation_thread_id;
#endif

		using lockType = std::conditional_t<ThreadSafe, std::lock_guard<std::mutex>, DummyLock>;
		Helpers::IdGenerator<ThreadSafe, false, ThreadPoolTable<ThreadSafe, Id>> m_id_generator;
		std::unordered_map<size_t, std::shared_ptr<BS::thread_pool<0U>>> m_thread_pools;
		std::mutex m_mutex;

		static inline bool m_there_is_no_thread_pool = true;
	};
}
