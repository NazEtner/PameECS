#pragma once
#include "renderer_types.hpp"

namespace PameECS::Graphics {
	class AdapterTask {
	public:
		AdapterTask() = default;
		virtual ~AdapterTask() = default;

		// commandはnullptrではないはず
		// 想定される呼び出し順は2回以上のEnqueueCommandの後に1回のExecuteの繰り返し
		// 最後にエンキューされたコマンドはキューの長さが1である、また、Executeより前にEnqueueCommandがtrueを返していない場合以外では使用されるべきではない
		virtual void Execute(RendererTypes::RenderCommand* command) = 0;

		// 戻り値はそのコマンドを入力した直前までの状態でExecuteする必要があるか
		// 実際はEnqueueした直後にExecuteされるので、描画時に新しいコマンドを使わないように注意
		virtual bool EnqueueCommand(void* command, size_t commandSize) = 0;
	};
}
