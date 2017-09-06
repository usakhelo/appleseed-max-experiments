
// appleseed.renderer headers.
#include "renderer/modeling/input/maxtexturesource.h"

namespace renderer
{
    class MaxProcTextureSource
        : public renderer::MaxTextureSource
    {
    public:
        // Constructor.
        MaxProcTextureSource(
            const TextureInstance&              texture_instance);

        virtual foundation::Color4f sample_max_texture(
            const foundation::Vector2f&         uv) const override;
    };
}