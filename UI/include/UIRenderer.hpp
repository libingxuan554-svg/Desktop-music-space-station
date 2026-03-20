#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

#include <vector>

namespace UI {
    class FramebufferUI; 
	struct UIButton; // Forward declaration

    class UIRenderer {
    public:
        // Defines the static interface of the rendering mode
        static void renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW);
        static void renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);
		// [新增] 渲染按钮组件的通用接口
		static void renderButtons(FramebufferUI* ui, const std::vector<UIButton>& buttons, float audioIntensity);
    };
}

#endif
