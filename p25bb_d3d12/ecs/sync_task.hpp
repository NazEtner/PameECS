#pragma once
#include "../macros/dll.hpp"
#include "system.hpp"
#include <cassert>

namespace PameECS::ECS {
	class SyncTask {
	public:
		explicit SyncTask(void(*task)(System::Context* context)) noexcept : m_sync_task(task) {}
		~SyncTask() {
			// nothing to do(このクラスの場合、どこで実行しても変わらないので)
		}
		void Execute(System::Context* context) noexcept {
#if defined(_DEBUG)
			assert(m_sync_task);
#elif
			if (!m_sync_Task) return;
#endif
			m_sync_task(context);
		}
	private:
		void(*m_sync_task)(System::Context* context) = nullptr;
	};
}
