
// Interface
#include "maxproctexturesource.h"

namespace renderer
{
    MaxProcTextureSource::MaxProcTextureSource(const TextureInstance& texture_instance)
      : MaxTextureSource(texture_instance)
    {
    }

    foundation::Color4f MaxProcTextureSource::sample_max_texture(
        const foundation::Vector2f&         uv) const
    {
        return foundation::Color4f(1.0, 0.0, 1.0, 1.0);
    }
}