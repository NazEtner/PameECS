#include "scheduler.hpp"
#include "../helpers/container.hpp"
#include "ecs_host.hpp"

using PameECS::ECS::Scheduler;

void Scheduler::Register(System::Base* system) {
	if (!system) return;
	auto index = m_next_system_index++;
	if (m_systems_raw_vector.size() <= index) {
		Helpers::Container::ResizePow2(m_systems_raw_vector, index + 1, nullptr);
	}
	else if (m_systems_raw_vector[index] == system) {
		return;
	}

	m_phases_dirty = true;
	m_systems_raw_vector[index] = system;
}

void Scheduler::Schedule(System::Context* context) {
	m_commit();

	if (!context || !context->ecsHost) return;
	m_makePhases(context->ecsHost);

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

void Scheduler::m_makePhases(const ECSHost* ecsHost) {
	if (!m_phases_dirty) return;
	m_phases.clear();
	PhaseInfo nextPhase;
	for (size_t i = 0; i < m_last_committed; ++i) {
		auto system = m_systems_raw_vector[i];
		if (m_checkConflict(ecsHost, system, nextPhase.write, nextPhase.read)) {
			m_phases.emplace_back(std::move(nextPhase));
			nextPhase = {};
		}

		nextPhase.systems.emplace_back(system);
		m_updateDependenciesSet(ecsHost, system, nextPhase.write, nextPhase.read);
	}

	if (!nextPhase.systems.empty()) {
		m_phases.emplace_back(std::move(nextPhase));
	}

	m_phases_dirty = false;
}

bool Scheduler::m_checkConflict(
	const ECSHost* ecsHost,
	System::Base* system,
	const std::unordered_set<size_t>& writeSet, const std::unordered_set<size_t>& readSet) {
	if (!system) return false;
	const auto& dependencies = system->GetDependencies(ecsHost);
	for (const auto& write : dependencies.GetWriteDependencies()) {
		if (writeSet.contains(write)) return true;
		if (readSet.contains(write)) return true;
	}
	for (const auto& read : dependencies.GetReadDependencies()) {
		if (writeSet.contains(read)) return true;
	}

	return false;
}

void Scheduler::m_updateDependenciesSet(
	const ECSHost* ecsHost,
	System::Base* system,
	std::unordered_set<size_t>& writeSet,
	std::unordered_set<size_t>& readSet) {
	if (!system) return;
	const auto& dependencies = system->GetDependencies(ecsHost);
	for (const auto& write : dependencies.GetWriteDependencies()) {
		writeSet.insert(write);
	}
	for (const auto& read : dependencies.GetReadDependencies()) {
		readSet.insert(read);
	}
}

void Scheduler::ShowDebug(ECSHost* ecsHost, DebugTools::DebugGUIHost* guiHost) {
	// 1. スケジューラーの統計情報
	if (ImGui::TreeNodeEx("Scheduler Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Total Systems: %zu", m_last_committed);
		ImGui::Text("Phase Count: %zu", m_phases.size());

		if (m_phases_dirty) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), " [Dirty]");
		}
		ImGui::TreePop();
	}

	ImGui::Separator();

	// 2. フェーズごとの詳細表示
	for (size_t i = 0; i < m_phases.size(); ++i) {
		auto& phase = m_phases[i];

		// std::format で動的にラベルを生成
		std::string phaseLabel = std::format("Phase {} (Systems: {})", i, phase.systems.size());

		if (ImGui::TreeNode(phaseLabel.c_str())) {
			//ImGui::Indent();

			for (size_t sysIdx = 0; sysIdx < phase.systems.size(); ++sysIdx) {
				auto* system = phase.systems[sysIdx];
				if (!system) continue;

				// システムのアドレスを識別子としてツリーを作成
				std::string sysName = std::format("System: {:p}", static_cast<void*>(system));

				if (ImGui::TreeNode(sysName.c_str())) {
					const auto& deps = system->GetDependencies(ecsHost);
					const auto& writes = deps.GetWriteDependencies();
					const auto& reads = deps.GetReadDependencies();

					// Table機能でRead/Writeを左右に並べて表示
					if (ImGui::BeginTable("DepsTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("Write Access (Conflict Source)");
						ImGui::TableSetupColumn("Read Access (Shared)");
						ImGui::TableHeadersRow();

						ImGui::TableNextRow();

						// --- Write Column (Red-ish) ---
						ImGui::TableSetColumnIndex(0);
						if (writes.empty()) {
							ImGui::TextDisabled("No Write Access");
						}
						else {
							for (auto w : writes) {
								ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ID: %zu", w);
							}
						}

						// --- Read Column (Blue-ish) ---
						ImGui::TableSetColumnIndex(1);
						if (reads.empty()) {
							ImGui::TextDisabled("No Read Access");
						}
						else {
							for (auto r : reads) {
								ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ID: %zu", r);
							}
						}

						ImGui::EndTable();
					}
					if (auto debugger = system->GetDebugger(); debugger) {
						ImGui::Separator();
						debugger(guiHost->GetContext(), system, ecsHost);
					}
					ImGui::TreePop();
				}
			}
			//ImGui::Unindent();
			ImGui::TreePop();
		}
	}
}
