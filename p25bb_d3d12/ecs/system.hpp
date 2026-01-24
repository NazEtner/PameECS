#pragma once
#include <vector>
#include <string>
#include <memory>
#include <span>
#include <limits>
#include <cassert>
#include "../macros/dll.hpp"
#include "types.hpp"
#include "../services/services.hpp"

namespace PameECS::ECS {
	class ECSHost;
}

namespace PameECS::ECS::System {
	struct Context {
		ECSHost* ecsHost;
		Services::Services* services;
		const uint8_t* entityAliveFlags;
		size_t entityAliveFlagsCount;
		const uint64_t* entityGenerations;
		size_t entityGenerationsCount;
	};

	class Dependencies {
	public:
		Dependencies(const size_t* write, const size_t writeSize, const size_t* read, const size_t readSize);
		Dependencies(const std::vector<size_t>& write, const std::vector<size_t>& read);
		~Dependencies();
		Dependencies(const Dependencies&) = delete;
		Dependencies& operator=(const Dependencies&) = delete;
		Dependencies(Dependencies&&) = delete;
		Dependencies& operator=(Dependencies&&) = delete;

		const std::vector<size_t>& GetWriteDependencies() const;
		const std::vector<size_t>& GetReadDependencies() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};

	class Base {
	public:
		Base() = default;
		virtual ~Base() = default;
		void Update(const Context* context) noexcept {
			m_updateBegin(context);
			for (size_t i = 0; i < context->entityAliveFlagsCount; ++i) {
				if (!context->entityAliveFlags[i]) continue;
				assert(context->entityGenerationsCount > i);
				if (context->entityGenerationsCount <= i) [[unlikely]] continue;
				Types::Entity entity = {
					.id = i,
					.generation = context->entityGenerations[i]
				};

				m_entityUpdate(context, entity);
			}
			m_updateEnd(context);
		}
		virtual const Dependencies& GetDependencies(const ECSHost* ecsHost) const = 0;
	protected:
		virtual void m_entityUpdate(const Context* context, const Types::Entity& entity) noexcept {}
		virtual void m_updateBegin(const Context* context) noexcept {}
		virtual void m_updateEnd(const Context* context) noexcept {}
	};
}

extern "C" {
	PECS_DLL_SHARED PameECS::ECS::System::Dependencies* ECSSDependenciesConstruct(const size_t* write, const size_t writeSize, const size_t* read, const size_t readSize);
	PECS_DLL_SHARED void ECSSDependenciesDestruct(PameECS::ECS::System::Dependencies* dependencies);
}
