
// appleseed.renderer headers.
#include "renderer/modeling/input/maxtexturesource.h"
#include "renderer/modeling/scene/textureinstance.h"


class MaxProcTextureSource
  : public renderer::MaxTextureSource
{
  public:
    MaxProcTextureSource(
        const renderer::TextureInstance& texture_instance)
        : renderer::MaxTextureSource(texture_instance)
    {
    }
};
