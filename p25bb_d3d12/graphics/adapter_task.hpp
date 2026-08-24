#pragma once
#include "renderer_types.hpp"

namespace PameECS::Graphics {
	class AdapterTask {
	public:
		AdapterTask() = default;
		virtual ~AdapterTask() = default;

		using RenderCommandFunc = bool(*)(RendererTypes::RenderCommand, AdapterTask*, void*);

		virtual RenderCommandFunc CreatePretreatmentCommand(void**) = 0;

		// commandはnullptrではないはず
		// 想定される呼び出し順は2回以上のEnqueueCommandの後に1回のCreateRenderCommandの繰り返し
		// 最後にエンキューされたコマンドはCreateRenderCommandより前にEnqueueCommandがtrueを返していない場合以外に対応する呼び出しでは使用されるべきではない
		virtual RenderCommandFunc CreateRenderCommand(void**) = 0;

		// 戻り値: このEnqueueCommand呼び出しで新たに確定したバッチの個数。呼び出し側はこの数だけCreateRenderCommandを呼ぶ。
		virtual size_t EnqueueCommand(void* command, size_t commandSize) = 0;
	};
}
