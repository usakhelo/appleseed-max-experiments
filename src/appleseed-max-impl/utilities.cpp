
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "utilities.h"

// appleseed-max headers.
#include "main.h"

// appleseed.renderer headers.
#include "renderer/api/color.h"
#include "renderer/api/source.h"
#include "renderer/api/texture.h"

// appleseed.foundation headers.
#include "foundation/utility/searchpaths.h"
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <AssetManagement/AssetUser.h>
#include <assert1.h>
#include <bitmap.h>
#include <imtl.h>
#include <plugapi.h>
#include <stdmat.h>
#include <maxapi.h>

// Windows headers.
#include <Shlwapi.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    //--- Shade Context
    class MaxShadeContext
        :public ShadeContext
    {
    public:
        TimeValue curtime;
        Point3 ltPos; // position of point in light space
        Point3 view;  // unit vector from light to point, in light space
        Point3 dp;
        Point2 uv, duv;
        IPoint2 scrpos;
        float curve;
        int projType;

        BOOL InMtlEditor() { return false; }
        LightDesc* Light(int n) { return NULL; }
        TimeValue CurTime() { return GetCOREInterface()->GetTime(); }
        int NodeID() { return -1; }
        int FaceNumber() { return 0; }
        int ProjType() { return projType; }
        Point3 Normal() { return Point3(0, 0, 0); }
        Point3 GNormal() { return Point3(0, 0, 0); }
        Point3 ReflectVector() { return Point3(0, 0, 0); }
        Point3 RefractVector(float ior) { return Point3(0, 0, 0); }
        Point3 CamPos() { return Point3(0, 0, 0); }
        Point3 V() { return view; }
        void SetView(Point3 v) { view = v; }
        Point3 P() { return ltPos; }
        Point3 DP() { return dp; }
        Point3 PObj() { return ltPos; }
        Point3 DPObj() { return Point3(0, 0, 0); }
        Box3 ObjectBox() { return Box3(Point3(-1, -1, -1), Point3(1, 1, 1)); }
        Point3 PObjRelBox() { return view; }
        Point3 DPObjRelBox() { return Point3(0, 0, 0); }
        void ScreenUV(Point2& UV, Point2 &Duv) { UV = uv; Duv = duv; }
        IPoint2 ScreenCoord() { return scrpos; }
        Point3 UVW(int chan) { return Point3(uv.x, uv.y, 0.0f); }
        Point3 DUVW(int chan) { return Point3(duv.x, duv.y, 0.0f); }
        void DPdUVW(Point3 dP[3], int chan) {}  // dont need bump vectors
        void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG = TRUE) {}   // returns Background color, bg transparency
        float Curve() { return curve; }

        // Transform to and from internal space
        Point3 PointTo(const Point3& p, RefFrame ito) { return p; }
        Point3 PointFrom(const Point3& p, RefFrame ifrom) { return p; }
        Point3 VectorTo(const Point3& p, RefFrame ito) { return p; }
        Point3 VectorFrom(const Point3& p, RefFrame ifrom) { return p; }
        MaxShadeContext() { doMaps = TRUE; curve = 0.0f; dp = Point3(0.0f, 0.0f, 0.0f); }
    };
}

namespace
{
    class MaxProceduralTextureSource
        : public asr::Source
    {
    public:
        MaxProceduralTextureSource(Texmap* texmap)
            : asr::Source(false)
            , m_texmap(texmap)
        {
        }

        virtual asf::uint64 compute_signature() const override
        {
            return 0;
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            float&                      scalar) const override
        {
            scalar = asf::luminance(evaluate_color(uv));
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asf::Color3f&               linear_rgb) const override
        {
            linear_rgb = evaluate_color(uv);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Spectrum&              spectrum) const override
        {
            DbgAssert(spectrum.size() == 3);
            const asf::Color3f linear_rgb = evaluate_color(uv);
            spectrum[0] = linear_rgb[0];
            spectrum[1] = linear_rgb[1];
            spectrum[2] = linear_rgb[2];
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Alpha&                 alpha) const override
        {
            alpha.set(1.0f);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asf::Color3f&               linear_rgb,
            asr::Alpha&                 alpha) const override
        {
            linear_rgb = evaluate_color(uv);
            alpha.set(1.0f);
        }

        virtual void evaluate(
            asr::TextureCache&          texture_cache,
            const asf::Vector2f&        uv,
            asr::Spectrum&              spectrum,
            asr::Alpha&                 alpha) const override
        {
            DbgAssert(spectrum.size() == 3);
            const asf::Color3f linear_rgb = evaluate_color(uv);
            spectrum[0] = linear_rgb[0];
            spectrum[1] = linear_rgb[1];
            spectrum[2] = linear_rgb[2];
            alpha.set(1.0f);
        }

    private:
        asf::Color3f evaluate_color(const asf::Vector2f& uv) const
        {
            MaxShadeContext maxsc;

            maxsc.mode = SCMODE_NORMAL;
            maxsc.projType = 0; // 0: perspective, 1: parallel
            maxsc.curtime = GetCOREInterface()->GetTime();
            //maxsc.curve = curve;
            //maxsc.ltPos = plt;
            //maxsc.view = FNormalize(Point3(plt.x, plt.y, 0.0f));
            maxsc.uv.x = uv.x;
            maxsc.uv.y = uv.y;
            //maxsc.scrpos.x = (int)(x + 0.5);
            //maxsc.scrpos.y = (int)(y + 0.5);
            maxsc.filterMaps = false;
            maxsc.mtlNum = 1;
            //maxsc.globContext = sc.globContext;

            AColor color = m_texmap->EvalColor(maxsc);

            return asf::Color3f(color.r, color.g, color.b);
            //return asf::Color3f(std::sin(uv[0]), std::sin(uv[1]), uv[0] * uv[1]);
        }

        Texmap*                 m_texmap;
    };

    class MaxProceduralTexture
        : public asr::Texture
    {
    public:
        explicit MaxProceduralTexture(const char* name, Texmap* texmap)
            : asr::Texture(name, asr::ParamArray())
            , m_texmap(texmap)
        {
        }

        virtual void release() override
        {
            delete this;
        }

        virtual const char* get_model() const override
        {
            return "max_procedural_texture";
        }

        virtual asf::ColorSpace get_color_space() const override
        {
            return asf::ColorSpaceLinearRGB;
        }

        virtual const asf::CanvasProperties& properties() override
        {
            return m_properties;
        }

        virtual asr::Source* create_source(
            const asf::UniqueID         assembly_uid,
            const asr::TextureInstance& texture_instance) override
        {
            return new MaxProceduralTextureSource(m_texmap);
        }

        virtual asf::Tile* load_tile(
            const size_t                tile_x,
            const size_t                tile_y) override
        {
            return nullptr;
        }

        virtual void unload_tile(
            const size_t                tile_x,
            const size_t                tile_y,
            const asf::Tile*            tile) override
        {
        }

    private:
        asf::CanvasProperties   m_properties;
        Texmap*                 m_texmap;
    };
}

std::string wide_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    const int wstr_size = static_cast<int>(wstr.size());
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, nullptr, 0, nullptr, nullptr);

    std::string result(result_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_size, &result[0], result_size, nullptr, nullptr);

    return result;
}

std::string wide_to_utf8(const wchar_t* wstr)
{
    const int result_size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

    std::string result(result_size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], result_size - 1, nullptr, nullptr);

    return result;
}

std::wstring utf8_to_wide(const std::string& str)
{
    if (str.empty())
        return std::wstring();

    const int str_size = static_cast<int>(str.size());
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, nullptr, 0);

    std::wstring result(result_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], str_size, &result[0], result_size);

    return result;
}

std::wstring utf8_to_wide(const char* str)
{
    const int result_size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);

    std::wstring result(result_size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &result[0], result_size - 1);

    return result;
}

bool is_bitmap_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    if (map->ClassID() != Class_ID(BMTEX_CLASS_ID, 0))
        return false;

    Bitmap* bitmap = static_cast<BitmapTex*>(map)->GetBitmap(0);

    if (bitmap == nullptr)
        return false;

    return true;
}

bool is_supported_texture(Texmap* map)
{
    if (map == nullptr)
        return false;

    auto part_a = map->ClassID().PartA();
    auto part_b = map->ClassID().PartB();

    switch (part_a)
    {
        // Not needed at the moment.
        //case MIRROR_CLASS_ID:       // Flat mirror.
        //case FALLOFF_CLASS_ID:      // Falloff texture.
        //case ACUBIC_CLASS_ID:       // Reflect/refract.
        //case PLATET_CLASS_ID:       // Plate glass texture.

    case 0x64035FB9:              // Tiles.
        if (part_b == 0x69664CDC)
            return true;
        break;
    case 0x1DEC5B86:              // Gradient Ramp.
        if (part_b == 0x43383A51)
            return true;
        break;
    case 0x72C8577F:              // Swirl.
        if (part_b == 0x39A00A1B)
            return true;
        break;
    case 0x23AD0AE9:              // Perlin Marble.
        if (part_b == 0x158D7A88)
            return true;
        break;
    case 0x243E22C6:              // Normal Bump.
        if (part_b == 0x63F6A014)
            return true;
        break;
    case 0x93A92749:              // Vector Map.
        if (part_b == 0x6B8D470A)
            return true;
        break;
    case CHECKER_CLASS_ID:
    case MARBLE_CLASS_ID:
    case MASK_CLASS_ID:
    case MIX_CLASS_ID:
    case NOISE_CLASS_ID:
    case GRADIENT_CLASS_ID:
    case TINT_CLASS_ID:
    case BMTEX_CLASS_ID:
    case COMPOSITE_CLASS_ID:
    case RGBMULT_CLASS_ID:
    case OUTPUT_CLASS_ID:
    case COLORCORRECTION_CLASS_ID:
    case 0x0000214:               // WOOD_CLASS_ID
    case 0x0000218:               // DENT_CLASS_ID
    case 0x46396cf1:              // PLANET_CLASS_ID
    case 0x7712634e:              // WATER_CLASS_ID
    case 0xa845e7c:               // SMOKE_CLASS_ID
    case 0x62c32b8a:              // SPECKLE_CLASS_ID
    case 0x90b04f9:               // SPLAT_CLASS_ID
    case 0x9312fbe:               // STUCCO_CLASS_ID
        return true;
        break;
    default:
        return false;
        break;
    }

    return true;
}

std::string get_root_path()
{
    wchar_t path[MAX_PATH];
    const DWORD path_length = sizeof(path) / sizeof(wchar_t);

    const auto result = GetModuleFileName(g_module, path, path_length);
    DbgAssert(result != 0);

    PathRemoveFileSpec(path);

    return wide_to_utf8(path);
}

void insert_color(asr::BaseGroup& base_group, const Color& color, const char* name)
{
    base_group.colors().insert(
        asr::ColorEntityFactory::create(
            name,
            asr::ParamArray()
                .insert("color_space", "linear_rgb")
                .insert("color", to_color3f(color))));
}

std::string insert_texture_and_instance(
    asr::BaseGroup& base_group,
    Texmap*         texmap,
    asr::ParamArray texture_params,
    asr::ParamArray texture_instance_params)
{
    const std::string texture_name = wide_to_utf8(texmap->GetName());
    if (is_bitmap_texture(texmap))
    {
        BitmapTex* bitmap_tex = static_cast<BitmapTex*>(texmap);

        const std::string filepath = wide_to_utf8(bitmap_tex->GetMap().GetFullFilePath());
        texture_params.insert("filename", filepath);

        if (!texture_params.strings().exist("color_space"))
        {
            if (asf::ends_with(filepath, ".exr"))
                texture_params.insert("color_space", "linear_rgb");
            else texture_params.insert("color_space", "srgb");
        }

        if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
        {
            base_group.textures().insert(
                asr::DiskTexture2dFactory::static_create(
                    texture_name.c_str(),
                    texture_params,
                    asf::SearchPaths()));
        }
    }
    else if (is_supported_texture(texmap))
    {
        texmap->Update(GetCOREInterface()->GetTime(), FOREVER);

        if (!texture_params.strings().exist("color_space"))
        {
            // todo: should check max's gamma settings here.
            texture_params.insert("color_space", "linear_rgb");
        }

        if (base_group.textures().get_by_name(texture_name.c_str()) == nullptr)
        {
            base_group.textures().insert(
                asf::auto_release_ptr<asr::Texture>(
                    new MaxProceduralTexture(
                        texture_name.c_str(), texmap)));
        }
    }
    const std::string texture_instance_name = texture_name + "_inst";
    if (base_group.texture_instances().get_by_name(texture_instance_name.c_str()) == nullptr)
    {
        base_group.texture_instances().insert(
            asr::TextureInstanceFactory::create(
                texture_instance_name.c_str(),
                texture_instance_params,
                texture_name.c_str()));
    }

    return texture_instance_name;
}

WStr replace_extension(const WStr& file_path, const WStr& new_ext)
{
    const int i = file_path.last(L'.');
    WStr new_file_path = i == -1 ? file_path : file_path.Substr(0, i);
    new_file_path.Append(new_ext);
    return new_file_path;
}
