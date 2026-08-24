#include "adapter.hpp"
#include "../helpers/container.hpp"
#include <mutex>
#include <atomic>
#include <vector>

extern "C" {
	PECS_DLL_SHARED bool GraphicsAdapterIsUsedTaskId(const PameECS::Graphics::Adapter* adapter, uint32_t id) {
		return adapter->IsUsedTaskId(id);
	}
	PECS_DLL_SHARED uint32_t GraphicsAdapterGetLowestAvailableTaskId(const PameECS::Graphics::Adapter* adapter) {
		return adapter->GetLowestAvailableTaskId();
	}
	PECS_DLL_SHARED bool GraphicsAdapterSetTask(
		PameECS::Graphics::Adapter* adapter,
		uint32_t id,
		PameECS::Graphics::AdapterTask* task,
		void(*deleter)(PameECS::Graphics::AdapterTask*)) {
		return adapter->SetTask(id, task, deleter);
	}
	PECS_DLL_SHARED PameECS::Graphics::AdapterTask* GraphicsAdapterGetTask(const PameECS::Graphics::Adapter* adapter, uint32_t id) {
		return adapter->GetTask(id);
	}
	PECS_DLL_SHARED void GraphicsAdapterEnqueueCommand(
		PameECS::Graphics::Adapter* adapter,
		uint32_t id,
		void* command,
		size_t commandSize) {
		adapter->EnqueueCommand(id, command, commandSize);
	}
	PECS_DLL_SHARED PameECS::Graphics::Renderer* GraphicsAdapterGetRenderer(const PameECS::Graphics::Adapter* adapter) {
		return adapter->GetRenderer();
	}
}

using PameECS::Graphics::Adapter;

struct Adapter::Impl {
	Renderer* renderer = nullptr;
	std::vector<std::shared_ptr<AdapterTask>> tasks;
	AdapterTask* currentTask = nullptr;
	std::set<uint32_t> usedTaskIds;
	uint32_t nextTaskId = 0;
	std::mutex mutex;

	template <bool IsPretreatment>
	void Enqueue(AdapterTask * task, AdapterTask::RenderCommandFunc func, void* dataPtr) {
		// RenderCommandFuncはRenderCommandに記録する関数ポインタの型
		// RenderCommandは実際のコマンドリスト等を格納する構造体
		renderer->EnqueueRenderTask<IsPretreatment>(
			[task, func, dataPtr](RendererTypes::RenderCommand renderCommand) -> RendererTypes::RenderCommand {
				if (func) {
					if (func(renderCommand, task, dataPtr)) {
						return renderCommand;
					}
				}

				throw Exceptions::RendererError("Failed to execute render command in adapter task.");
			}
		);
	};

	// 戻り値: CreateRenderCommandで実際にレンダーコマンドを1個排出できたか
	bool Submit(AdapterTask* task) {
		void* dataPtr = nullptr;
		bool produced = false;
		if (auto command = task->CreateRenderCommand(&dataPtr); command) {
			Enqueue<false>(task, command, dataPtr);
			produced = true;
		}
		if (auto command = task->CreatePretreatmentCommand(&dataPtr); command) {
			Enqueue<true>(task, command, dataPtr);
		}
		return produced;
	};

	// タスクを手放す時専用。末尾のバッチも含めて全部吐き出し切る。
	void SubmitAll(AdapterTask* task) {
		while (Submit(task)) {}
	};
};

Adapter::Adapter(Renderer* renderer) {
	m_impl = std::make_unique<Impl>();
	m_impl->renderer = renderer;
}

Adapter::~Adapter() {
}

bool Adapter::IsUsedTaskId(uint32_t id) const {
	std::lock_guard lock(m_impl->mutex);
	if (id >= m_impl->tasks.size() || !m_impl->tasks[id]) {
		return false;
	}
	if (m_impl->usedTaskIds.find(id) == m_impl->usedTaskIds.end()) {
		return false;
	}
	return true;
}

uint32_t Adapter::GetLowestAvailableTaskId() const {
	std::lock_guard lock(m_impl->mutex);
	auto it = m_impl->usedTaskIds.find(m_impl->nextTaskId);
	while (it != m_impl->usedTaskIds.end()) {
		++m_impl->nextTaskId;
		it = m_impl->usedTaskIds.find(m_impl->nextTaskId);
	}
	return m_impl->nextTaskId;
}

bool Adapter::SetTask(uint32_t id, AdapterTask* task, void(*deleter)(AdapterTask*)) {
	if (IsUsedTaskId(id)) {
		return false;
	}
	std::lock_guard lock(m_impl->mutex);
	m_impl->usedTaskIds.insert(id);
	if (id >= m_impl->tasks.size()) {
		Helpers::Container::ResizePow2(m_impl->tasks, static_cast<size_t>(id) + 1);
	}
	m_impl->tasks[id] = std::shared_ptr<AdapterTask>(task, deleter);
	return true;
}

PameECS::Graphics::AdapterTask* Adapter::GetTask(uint32_t id) const {
	if (!IsUsedTaskId(id)) {
		return nullptr;
	}

	std::lock_guard lock(m_impl->mutex);
	auto& task = m_impl->tasks[id];
	return task.get();
}

void Adapter::EnqueueCommand(uint32_t id, void* command, size_t commandSize) {
	if (!IsUsedTaskId(id)) {
		return;
	}

	std::lock_guard lock(m_impl->mutex);
	auto& task = m_impl->tasks[id];
	if (!task) {
		return;
	}

	if (task.get() != m_impl->currentTask && m_impl->currentTask) {
		// 別タスクに切り替わるので、直前のタスクの末尾のバッチも含めて全部吐き出す
		m_impl->SubmitAll(m_impl->currentTask);
	}
	// 新たに確定したバッチの数だけSubmitする。
	size_t newBatches = task->EnqueueCommand(command, commandSize);
	for (size_t i = 0; i < newBatches; ++i) {
		m_impl->Submit(task.get());
	}
	m_impl->currentTask = task.get();
}

// これは絶対に競合しないはず
// マルチスレッドで呼ばれるスクリプトから呼び出す手段が存在せず、
// ゲームエンジンの内部処理でも全ての処理が完了した後にメインスレッドでしか呼ばれないうえに、そのタイミングでは描画が始まっていないため
void Adapter::ExecuteLastTask() {
	if (m_impl->currentTask) {
		m_impl->SubmitAll(m_impl->currentTask);
		m_impl->currentTask = nullptr;
	}
}

PameECS::Graphics::Renderer* Adapter::GetRenderer() const {
	return m_impl->renderer;
}
