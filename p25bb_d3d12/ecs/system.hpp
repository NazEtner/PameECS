#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../abi/shared_string_vector.hpp"

namespace PameECS::ECS {
	class ECSHost;
}

namespace PameECS::ECS::System {
	struct Context {
		ECSHost* ecsHost;
	};

	class Dependencies {
	public:
		Dependencies(const std::vector<size_t>& write, const std::vector<size_t>& read);
		Dependencies(const Dependencies&) = delete;
		Dependencies& operator=(const Dependencies&) = delete;
		Dependencies(const Dependencies&&) = delete;
		Dependencies& operator=(const Dependencies&&) = delete;

		const std::vector<size_t> GetWriteDependencies() const;
		const std::vector<size_t> GetReadDependencies() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};

	class Base {
	public:
		Base() = default;
		virtual ~Base() = default;
		virtual void Update(Context* context) noexcept = 0;
		virtual const Dependencies& GetDependencies(const ECSHost* ecsHost) const = 0;
	};
}
