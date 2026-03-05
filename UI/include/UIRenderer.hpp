#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

namespace UI {
    class FramebufferUI; // Forward declaration

    class UIRenderer {
    public:
        // Defines the static interface of the rendering mode
        static void renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW);
        static void renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);
    };
}

#endif
