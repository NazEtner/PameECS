#pragma once
#include "renderer_types.hpp"

namespace PameECS::Graphics {
	class AdapterTask {
	public:
		AdapterTask() = default;
		virtual ~AdapterTask() = default;

		using RenderCommandFunc = bool(*)(RendererTypes::RenderCommand, AdapterTask*);

		virtual RenderCommandFunc CreatePretreatmentCommand() = 0;

		// commandはnullptrではないはず
		// 想定される呼び出し順は2回以上のEnqueueCommandの後に1回のCreateRenderCommandの繰り返し
		// 最後にエンキューされたコマンドはCreateRenderCommandより前にEnqueueCommandがtrueを返していない場合以外に対応する呼び出しでは使用されるべきではない
		virtual RenderCommandFunc CreateRenderCommand() = 0;

		// 戻り値はそのコマンドを入力した直前までの状態でレンダリングコマンドを生成する必要があるか
		// 実際はEnqueueした直後にレンダリングコマンドを生成するので、描画時に新しいコマンドを使わないように注意
		virtual bool EnqueueCommand(void* command, size_t commandSize) = 0;
	};
}
