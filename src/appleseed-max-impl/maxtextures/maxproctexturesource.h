
// appleseed.renderer headers.
#include "renderer/modeling/input/maxtexturesource.h"

// Forward declarations.
class Texmap;

namespace renderer
{
    class MaxProcTextureSource
        : public renderer::MaxTextureSource
    {
        public:
            // Constructor.
            MaxProcTextureSource(
                const TextureInstance&              texture_instance,
                Texmap*         max_texmap);

            virtual foundation::Color4f sample_max_texture(
                const foundation::Vector2f&         uv) const override;

        private:
            // Max texture
            Texmap* m_texmap;

            // ShaderContext
            class MaxSC;
    };
}