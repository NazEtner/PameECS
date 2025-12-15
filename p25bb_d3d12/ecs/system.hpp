#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../abi/shared_string_vector.hpp"

namespace PameECS::ECS::System {
	struct Context {

	};

	class Dependencies {
	public:
		Dependencies(const std::vector<std::string>& write, const std::vector<std::string>& read);
		Dependencies(const Dependencies&) = delete;
		Dependencies& operator=(const Dependencies&) = delete;
		Dependencies(const Dependencies&&) = delete;
		Dependencies& operator=(const Dependencies&&) = delete;

		const std::vector<std::string> GetWriteDependencies() const;
		const std::vector<std::string> GetReadDependencies() const;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};

	class Base {
	public:
		Base() = default;
		virtual ~Base() = default;
		virtual void Update(Context* context) = 0;
		virtual const Dependencies& GetDependenceis() const = 0;
	};
}
