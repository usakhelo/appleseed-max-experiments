
// appleseed.renderer headers.
#include "renderer/kernel/texturing/texturecache.h"
#include "renderer/modeling/input/maxtexturesource.h"
#include "renderer/modeling/scene/textureinstance.h"

namespace renderer
{
    extern foundation::LightingConditions g_std_lighting_conditions;

    class MaxProcTextureSource
        : public renderer::MaxTextureSource
    {
    public:
        MaxProcTextureSource(
            const renderer::TextureInstance& texture_instance)
            : renderer::MaxTextureSource(texture_instance)
        {
        }

        virtual void evaluate(
            renderer::TextureCache&             texture_cache,
            const foundation::Vector2f&         uv,
            Spectrum&                           spectrum) const override
        {
            const foundation::Color4f color(1.0, 1.0, 0.0, 1.0);
            spectrum.set(color.rgb(), g_std_lighting_conditions, Spectrum::Reflectance);
        }
    };
}