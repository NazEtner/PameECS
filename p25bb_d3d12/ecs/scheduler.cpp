#include "scheduler.hpp"
#include "../helpers/container.hpp"

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

void Scheduler::Schedule() {
	m_commit();
	m_makePhases();

	// 以下、未実装
}

void Scheduler::m_commit() {
	if (m_next_system_index != m_last_committed) {
		m_phases_dirty = true;
	}
	m_last_committed = m_next_system_index;
	m_next_system_index = 0;
}

void Scheduler::m_makePhases() {
	// 未実装
}
