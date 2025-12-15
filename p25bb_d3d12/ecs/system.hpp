#pragma once

namespace PameECS::ECS {
	struct SystemContext {

	};

	class System {
	public:
		System(void(*updater)(SystemContext* context)) {
			m_updater = updater;
		}
		void Update(SystemContext* context) {
			if (m_updater) {
				m_updater(context);
			}
		}
	private:
		void(*m_updater)(SystemContext* context);
	};
}
