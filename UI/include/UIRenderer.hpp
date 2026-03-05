#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

namespace UI {
    class FramebufferUI; // 前向声明

    class UIRenderer {
    public:
        // 定义渲染模式的静态接口
        static void renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW);
        static void renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);
    };
}
#endif