#pragma once
#include "adapter_task.hpp"
#include "renderer.hpp"
#include "../macros/debug.hpp"
#include "../macros/dll.hpp"
#include <memory>
#include <cassert>
#include <utility>

namespace PameECS::Graphics {
	class Adapter;
}

extern "C" {
	PECS_DLL_SHARED bool GraphicsAdapterIsUsedTaskId(const PameECS::Graphics::Adapter* adapter, uint32_t id);
	PECS_DLL_SHARED uint32_t GraphicsAdapterGetLowestAvailableTaskId(const PameECS::Graphics::Adapter* adapter);
	PECS_DLL_SHARED bool GraphicsAdapterSetTask(
		PameECS::Graphics::Adapter* adapter,
		uint32_t id,
		PameECS::Graphics::AdapterTask* task,
		void(*deleter)(PameECS::Graphics::AdapterTask*));
	PECS_DLL_SHARED void GraphicsAdapterEnqueueCommand(
		PameECS::Graphics::Adapter* adapter,
		uint32_t id,
		void* command,
		size_t commandSize);
}

namespace PameECS::Graphics {
	class Adapter {
	public:
		Adapter(Renderer* renderer);
		~Adapter();

		bool IsUsedTaskId(uint32_t id) const;

		uint32_t GetLowestAvailableTaskId() const;

		bool SetTask(uint32_t id, AdapterTask* task, void(*deleter)(AdapterTask*));
		uint32_t SetTask(AdapterTask* task, void(*deleter)(AdapterTask*)) {
			uint32_t id = GraphicsAdapterGetLowestAvailableTaskId(this);
			if (GraphicsAdapterSetTask(this, id, task, deleter)) {
				return id;
			}
			// 有効なIDを設定したはずなのに失敗した場合
			// まともな実装だったらここには来ない
#if _DEBUG
			assert(false);
#else
			std::unreachable();
#endif
			return 0;
		}

		template <typename T, typename ...Args>
			requires(std::is_base_of_v<AdapterTask, T>)
		bool SetTask(uint32_t id, Args... args) {
			return GraphicsAdapterSetTask(this, id, new T(std::forward<Args>(args)...), [](AdapterTask* task) -> void { delete task; });
		}

		template <typename T, typename ...Args> 
			requires(std::is_base_of_v<AdapterTask, T>)
		uint32_t SetTask(Args... args) {
			return SetTask(new T(std::forward<Args>(args)...), [](AdapterTask* task) -> void { delete task; });
		}

		void EnqueueCommand(uint32_t id, void* command, size_t commandSize);
		void Flush();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
