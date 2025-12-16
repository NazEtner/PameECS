#include "scheduler.hpp"
#include "../helpers/container.hpp"
#include "ecs_host.hpp"

using PameECS::ECS::Scheduler;

void Scheduler::Register(System::Base* system) {
	if (!system) return;
	if (m_systems_raw_vector.size() <= m_next_system_index) {
		Helper::Container::ResizePow2(m_systems_raw_vector, m_next_system_index + 1, nullptr);
	}
	else if (m_systems_raw_vector[m_next_system_index] == system) {
		return;
	}

	m_phases_dirty = true;
	m_systems_raw_vector[m_next_system_index] = system;
	m_next_system_index++;
}

void Scheduler::Schedule(System::Context* context) {
	m_commit();
	m_makePhases();
	if (!context || !context->ecsHost) return;

	context->ecsHost->LockAll();

	for (auto& phase : m_phases) {
		std::vector<std::future<void>> futures;
		futures.reserve(phase.systems.size());
		// ロックするのは取得できるかだけ
		// 取得した後の使い方が書き込みか読み込みかは知らない
		context->ecsHost->Unlock(phase.read);
		context->ecsHost->Unlock(phase.write);
		for (auto& system : phase.systems) {
			if (!system) continue;
			futures.emplace_back(
				m_thread_pool->submit_task(
					[system, context]() -> void {
						system->Update(context);
					}
				)
			);
		}
		for (auto& future : futures) {
			future.get();
		}
		context->ecsHost->LockAll();
	}

	context->ecsHost->UnlockAll();
}

void Scheduler::m_commit() {
	if (m_next_system_index != m_last_committed) {
		m_phases_dirty = true;
	}
	m_last_committed = m_next_system_index;
	m_next_system_index = 0;
}

void Scheduler::m_makePhases() {
	if (!m_phases_dirty) return;
	m_phases.clear();
	PhaseInfo nextPhase;
	for (size_t i = 0; i < m_last_committed; ++i) {
		auto system = m_systems_raw_vector[i];
		if (m_checkConflict(system, nextPhase.write, nextPhase.read)) {
			m_phases.emplace_back(std::move(nextPhase));
			nextPhase = {};
		}

		nextPhase.systems.emplace_back(system);
		m_updateDependenciesSet(system, nextPhase.write, nextPhase.read);
	}

	if (!nextPhase.systems.empty()) {
		m_phases.emplace_back(std::move(nextPhase));
	}

	m_phases_dirty = false;
}

bool Scheduler::m_checkConflict(const System::Base* system,
	const std::unordered_set<size_t>& writeSet, const std::unordered_set<size_t>& readSet) {
	if (!system) return false;
	const auto& dependencies = system->GetDependencies();
	for (const auto& write : dependencies.GetWriteDependencies()) {
		if (writeSet.contains(write)) return true;
		if (readSet.contains(write)) return true;
	}
	for (const auto& read : dependencies.GetReadDependencies()) {
		if (writeSet.contains(read)) return true;
	}

	return false;
}

void Scheduler::m_updateDependenciesSet(const System::Base* system, std::unordered_set<size_t>& writeSet, std::unordered_set<size_t>& readSet) {
	if (!system) return;
	const auto& dependencies = system->GetDependencies();
	for (const auto& write : dependencies.GetWriteDependencies()) {
		writeSet.insert(write);
	}
	for (const auto& read : dependencies.GetReadDependencies()) {
		readSet.insert(read);
	}
}
