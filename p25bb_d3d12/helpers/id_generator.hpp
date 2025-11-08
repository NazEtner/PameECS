#pragma once
#include "../macros/debug.hpp"
#include "../template_types/string_literal.hpp"
#include "../thread/dummy_lock.hpp"
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>

namespace PameECS::Helpers {
	template<bool ThreadSafe = false>
	class IdGenerator {
	public:
		template<TemplateTypes::StringLiteral Symbol>
		size_t GetId() {
			static const size_t id = m_current_id++;
#ifdef _DEBUG
			lockType lock(m_map_mutex);
			m_id_to_name_map[id] = std::string(Symbol.data);
#endif
			return id;
		}

		std::string IdToName(const size_t id) {
#ifdef _DEBUG
			lockType lock(m_map_mutex);
			auto it = m_id_to_name_map.find(id);
			if (it != m_id_to_name_map.end()) {
				return it->second;
			}
			else {
				return "Unknown ID";
			}
#else
			return "__OPTIMIZED__";
#endif
			return "__NON_STANDARD_COMPILER__"; // ここに来ることはあり得ないはず
		}
	private:
		using lockType = std::conditional_t<ThreadSafe, std::lock_guard<std::mutex>, Thread::DummyLock>;
		std::conditional_t<ThreadSafe, std::atomic_size_t, size_t> m_current_id{ 0 };
#ifdef _DEBUG
		std::unordered_map<size_t, std::string> m_id_to_name_map;
		std::mutex m_map_mutex;
#endif
	};
}
