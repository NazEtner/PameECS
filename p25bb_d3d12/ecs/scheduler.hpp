#pragma once
#include <BS_thread_pool.hpp/BS_thread_pool.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_set>
#include "system.hpp"

namespace PameECS::ECS {
	class Scheduler {
	public:
		Scheduler(std::shared_ptr<BS::thread_pool<0U>> threadPool) : m_thread_pool(threadPool) {}
		void Register(System::Base* system);
		void Schedule(System::Context* context);

		void ShowDebug(const ECSHost* ecsHost);
	private:
		void m_commit();
		void m_makePhases(const ECSHost* ecsHost);
		bool m_checkConflict(
			const ECSHost* ecsHost,
			System::Base* system,
			const std::unordered_set<size_t>& write, const std::unordered_set<size_t>& read);
		void m_updateDependenciesSet(
			const ECSHost* ecsHost,
			System::Base* system,
			std::unordered_set<size_t>& write, std::unordered_set<size_t>& read);
		size_t m_next_system_index = 0; // フェーズの再作成をすべきかを判定するためのもの
		size_t m_last_committed = 0;
		std::vector<System::Base*> m_systems_raw_vector; // あった方が再作成の判定がしやすいので
		bool m_phases_dirty = false;
		struct PhaseInfo {
			std::vector<System::Base*> systems;
			std::unordered_set<size_t> write;
			std::unordered_set<size_t> read;
		};
		std::vector<PhaseInfo> m_phases;
		std::shared_ptr<BS::thread_pool<0U>> m_thread_pool;
	};
}
