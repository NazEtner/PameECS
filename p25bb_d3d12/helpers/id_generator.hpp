#pragma once
#include "../macros/debug.hpp"
#include "../template_types/string_literal.hpp"
#include "../thread/dummy_lock.hpp"
#include "empty_type.hpp"
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>

namespace PameECS::Helpers {
	template<bool ThreadSafe = false, bool EnableRuntimeGenerate = false>
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

		size_t GetId(const std::string& name) {
			static_assert(EnableRuntimeGenerate);
			size_t id = std::numeric_limits<size_t>::max();
			if constexpr (EnableRuntimeGenerate) {
				{
					lockType lock(m_runtime_generate_mutex);
					auto it = m_runtime_generate_ids.find(name);
					if (it != m_runtime_generate_ids.end()) {
						id = it->second;
					}
					else {
						id = m_current_id++;
						m_runtime_generate_ids[name] = id;
					}
				}

#ifdef _DEBUG
				lockType lock(m_map_mutex);
				m_id_to_name_map[id] = std::string(name);
#endif
			}

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

		std::conditional_t<EnableRuntimeGenerate, std::unordered_map<std::string, size_t>, EmptyType> m_runtime_generate_ids;
		std::conditional_t<EnableRuntimeGenerate && ThreadSafe, std::mutex, EmptyType> m_runtime_generate_mutex;
#ifdef _DEBUG
		std::unordered_map<size_t, std::string> m_id_to_name_map;
		std::mutex m_map_mutex;
#endif
	};
}
