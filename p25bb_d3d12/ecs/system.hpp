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
		PECS_DLL_SHARED Dependencies(const size_t* write, const size_t writeSize, const size_t* read, const size_t readSize);
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
		Impl* m_impl;
	};

	class Base {
	public:
		Base() = default;
		virtual ~Base() = default;
		virtual void Update(Context* context) noexcept = 0;
		virtual const Dependencies& GetDependencies(const ECSHost* ecsHost) const = 0;
	};
}
