
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2016 Francois Beaune, The appleseedhq Organization
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
#include "appleseeddisneymtl.h"

// appleseed-max headers.
#include "disneymtl/appleseeddisneymtlparamdlg.h"
#include "disneymtl/datachunks.h"
#include "disneymtl/resource.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/material.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"

// 3ds Max headers.
#include <assert1.h>
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>

// Windows headers.
#include <tchar.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const TCHAR* AppleseedDisneyMtlFriendlyClassName = _T("appleseed Disney Material");
}

AppleseedDisneyMtlClassDesc g_appleseed_disneymtl_classdesc;


//
// AppleseedDisneyMtl class implementation.
//

namespace
{
    enum { ParamBlockIdDisneyMtl };
    enum { ParamBlockRefDisneyMtl };

    enum ParamId
    {
        ParamIdBaseColor,
        ParamIdBaseColorTexmap,
        ParamIdMetallic,
        ParamIdMetallicTexmap,
        ParamIdSpecular,
        ParamIdSpecularTexmap,
        ParamIdSpecularTint,
        ParamIdSpecularTintTexmap,
        ParamIdAnisotropic,
        ParamIdAnisotropicTexmap,
        ParamIdRoughness,
        ParamIdRoughnessTexmap,
        ParamIdClearcoat,
        ParamIdClearcoatTexmap,
        ParamIdClearcoatGloss,
        ParamIdClearcoatGlossTexmap
    };

    enum TexmapId
    {
        TexmapIdBase,
        TexmapIdMetallic,
        TexmapIdSpecular,
        TexmapIdSpecularTint,
        TexmapIdAnisotropic,
        TexmapIdRoughness,
        TexmapIdClearcoat,
        TexmapIdClearcoatGloss,
        TexmapCount
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        _T("Base Color"),
        _T("Metallic"),
        _T("Specular"),
        _T("Specular Tint"),
        _T("Anisotropic"),
        _T("Roughness"),
        _T("Clearcoat"),
        _T("Clearcoat Gloss")
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdBaseColorTexmap,
        ParamIdMetallicTexmap,
        ParamIdSpecularTexmap,
        ParamIdSpecularTintTexmap,
        ParamIdAnisotropicTexmap,
        ParamIdRoughnessTexmap,
        ParamIdClearcoatTexmap,
        ParamIdClearcoatGlossTexmap
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdDisneyMtl,                      // parameter block's ID
        _T("appleseedDisneyMtlParams"),             // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_disneymtl_classdesc,           // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefDisneyMtl,                     // parameter block's reference number

        // --- P_AUTO_UI arguments ---
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications ---

        ParamIdBaseColor, _T("base_color"), TYPE_RGBA, P_ANIMATABLE, IDS_BASE_COLOR,
            p_default, Color(0.9f, 0.9f, 0.9f),
            p_ui, TYPE_COLORSWATCH, IDC_SWATCH_BASE_COLOR,
        p_end,
        ParamIdBaseColorTexmap, _T("base_color_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_BASE_COLOR,
            p_subtexno, TexmapIdBase,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_BASE_COLOR,
        p_end,

        ParamIdMetallic, _T("metallic"), TYPE_FLOAT, P_ANIMATABLE, IDS_METALLIC,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_METALLIC, IDC_SLIDER_METALLIC, 10.0f,
        p_end,
        ParamIdMetallicTexmap, _T("metallic_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_METALLIC,
            p_subtexno, TexmapIdMetallic,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_METALLIC,
        p_end,

        ParamIdSpecular, _T("specular"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR, IDC_SLIDER_SPECULAR, 10.0f,
        p_end,
        ParamIdSpecularTexmap, _T("specular_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR,
            p_subtexno, TexmapIdSpecular,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR,
        p_end,

        ParamIdSpecularTint, _T("specular_tint"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPECULAR_TINT,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_SPECULAR_TINT, IDC_SLIDER_SPECULAR_TINT, 10.0f,
        p_end,
        ParamIdSpecularTintTexmap, _T("specular_tint_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_SPECULAR_TINT,
            p_subtexno, TexmapIdSpecularTint,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_SPECULAR_TINT,
        p_end,

        ParamIdAnisotropic, _T("anisotropic"), TYPE_FLOAT, P_ANIMATABLE, IDS_ANISOTROPIC,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ANISOTROPIC, IDC_SLIDER_ANISOTROPIC, 10.0f,
        p_end,
        ParamIdAnisotropicTexmap, _T("anisotropic_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_ANISOTROPIC,
            p_subtexno, TexmapIdAnisotropic,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ANISOTROPIC,
        p_end,

        ParamIdRoughness, _T("roughness"), TYPE_FLOAT, P_ANIMATABLE, IDS_ROUGHNESS,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_ROUGHNESS, IDC_SLIDER_ROUGHNESS, 10.0f,
        p_end,
        ParamIdRoughnessTexmap, _T("roughness_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_ROUGHNESS,
            p_subtexno, TexmapIdRoughness,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_ROUGHNESS,
        p_end,

        ParamIdClearcoat, _T("clearcoat"), TYPE_FLOAT, P_ANIMATABLE, IDS_CLEARCOAT,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_CLEARCOAT, IDC_SLIDER_CLEARCOAT, 10.0f,
        p_end,
        ParamIdClearcoatTexmap, _T("clearcoat_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_CLEARCOAT,
            p_subtexno, TexmapIdClearcoat,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_CLEARCOAT,
        p_end,

        ParamIdClearcoatGloss, _T("clearcoat_gloss"), TYPE_FLOAT, P_ANIMATABLE, IDS_CLEARCOAT_GLOSS,
            p_default, 0.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_CLEARCOAT_GLOSS, IDC_SLIDER_CLEARCOAT_GLOSS, 10.0f,
        p_end,
        ParamIdClearcoatGlossTexmap, _T("clearcoat_gloss_texmap"), TYPE_TEXMAP, 0, IDS_TEXMAP_CLEARCOAT_GLOSS,
            p_subtexno, TexmapIdClearcoatGloss,
            p_ui, TYPE_TEXMAPBUTTON, IDC_TEXMAP_CLEARCOAT_GLOSS,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedDisneyMtl::get_class_id()
{
    return Class_ID(0x331b1ff7, 0x16381b67);
}

AppleseedDisneyMtl::AppleseedDisneyMtl()
  : m_pblock(nullptr)
  , m_base_color(0.0f, 0.0f, 0.0f)
  , m_base_color_texmap(nullptr)
  , m_metallic(0.0f)
  , m_metallic_texmap(nullptr)
  , m_specular(0.0f)
  , m_specular_texmap(nullptr)
  , m_specular_tint(0.0f)
  , m_specular_tint_texmap(nullptr)
  , m_anisotropic(0.0f)
  , m_anisotropic_texmap(nullptr)
  , m_roughness(0.0f)
  , m_roughness_texmap(nullptr)
  , m_clearcoat(0.0f)
  , m_clearcoat_texmap(nullptr)
  , m_clearcoat_gloss(0.0f)
  , m_clearcoat_gloss_texmap(nullptr)
{
    m_params_validity.SetEmpty();

    g_appleseed_disneymtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedDisneyMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedDisneyMtl::DeleteThis()
{
    delete this;
}

void AppleseedDisneyMtl::GetClassName(TSTR& s)
{
    s = _T("appleseedDisneyMtl");
}

SClass_ID AppleseedDisneyMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedDisneyMtl::ClassID()
{
    return get_class_id();
}

int AppleseedDisneyMtl::NumSubs()
{
    return 1;
}

Animatable* AppleseedDisneyMtl::SubAnim(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

TSTR AppleseedDisneyMtl::SubAnimName(int i)
{
    return i == 0 ? _T("Parameters") : _T("");
}

int AppleseedDisneyMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedDisneyMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedDisneyMtl::GetParamBlock(int i)
{
    return i == 0 ? m_pblock : nullptr;
}

IParamBlock2* AppleseedDisneyMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedDisneyMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedDisneyMtl::GetReference(int i)
{
    return i == ParamBlockRefDisneyMtl ? m_pblock : nullptr;
}

void AppleseedDisneyMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefDisneyMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedDisneyMtl::NotifyRefChanged(
    const Interval&     changeInt,
    RefTargetHandle     hTarget,
    PartID&             partID,
    RefMessage          message,
    BOOL                propagate)
{
    switch (message)
    {
      case REFMSG_CHANGE:
        m_params_validity.SetEmpty();
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle AppleseedDisneyMtl::Clone(RemapDir& remap)
{
    AppleseedDisneyMtl* clone = new AppleseedDisneyMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(0, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedDisneyMtl::NumSubTexmaps()
{
    return TexmapCount;
}

Texmap* AppleseedDisneyMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedDisneyMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
}

int AppleseedDisneyMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedDisneyMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedDisneyMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        m_pblock->GetValue(ParamIdBaseColor, t, m_base_color, m_params_validity);
        m_pblock->GetValue(ParamIdBaseColorTexmap, t, m_base_color_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdMetallic, t, m_metallic, m_params_validity);
        m_pblock->GetValue(ParamIdMetallicTexmap, t, m_metallic_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecular, t, m_specular, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularTexmap, t, m_specular_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdSpecularTint, t, m_specular_tint, m_params_validity);
        m_pblock->GetValue(ParamIdSpecularTintTexmap, t, m_specular_tint_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdAnisotropic, t, m_anisotropic, m_params_validity);
        m_pblock->GetValue(ParamIdAnisotropicTexmap, t, m_anisotropic_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdRoughness, t, m_roughness, m_params_validity);
        m_pblock->GetValue(ParamIdRoughnessTexmap, t, m_roughness_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdClearcoat, t, m_clearcoat, m_params_validity);
        m_pblock->GetValue(ParamIdClearcoatTexmap, t, m_clearcoat_texmap, m_params_validity);

        m_pblock->GetValue(ParamIdClearcoatGloss, t, m_clearcoat_gloss, m_params_validity);
        m_pblock->GetValue(ParamIdClearcoatGlossTexmap, t, m_clearcoat_gloss_texmap, m_params_validity);

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedDisneyMtl::Reset()
{
    g_appleseed_disneymtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedDisneyMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedDisneyMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    return g_appleseed_disneymtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
}

IOResult AppleseedDisneyMtl::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, FileFormatVersion);
    isave->EndChunk();

    isave->BeginChunk(ChunkMtlBase);
    success &= MtlBase::Save(isave) == IO_OK;
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult AppleseedDisneyMtl::Load(ILoad* iload)
{
    IOResult result = IO_OK;

    while (true)
    {
        result = iload->OpenChunk();
        if (result == IO_END)
            return IO_OK;
        if (result != IO_OK)
            break;

        switch (iload->CurChunkID())
        {
          case ChunkFileFormatVersion:
            {
                USHORT version;
                result = read<USHORT>(iload, &version);
            }
            break;

          case ChunkMtlBase:
            result = MtlBase::Load(iload);
            break;
        }

        if (result != IO_OK)
            break;

        result = iload->CloseChunk();
        if (result != IO_OK)
            break;
    }

    return result;
}

Color AppleseedDisneyMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedDisneyMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return m_base_color;
}

Color AppleseedDisneyMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedDisneyMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedDisneyMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedDisneyMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedDisneyMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedDisneyMtl::SetDiffuse(Color c, TimeValue t)
{
}

void AppleseedDisneyMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedDisneyMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedDisneyMtl::Shade(ShadeContext& sc)
{
}

namespace
{
    bool is_bitmap(Texmap* texmap)
    {
        if (texmap == nullptr)
            return false;

        if (texmap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0))
            return false;

        Bitmap* bitmap = static_cast<BitmapTex*>(texmap)->GetBitmap(0);

        if (bitmap == nullptr)
            return false;

        return true;
    }

    std::string format_value(BitmapTex* bitmap_tex)
    {
        DbgAssert(bitmap_tex != nullptr);
        const auto filepath = wide_to_utf8(bitmap_tex->GetMapName());

        Bitmap* bitmap = bitmap_tex->GetBitmap(0);
        DbgAssert(bitmap != nullptr);

        const int width = bitmap->Width();
        const int height = bitmap->Height();

        return asf::format("texture(\"{0}\", $u % {1}, $v % {2})", filepath, width, height);
    }

    std::string format_value(const float scalar, Texmap* map)
    {
        auto value = asf::to_string(scalar / 100.0f);

        if (is_bitmap(map))
            value += asf::format(" * {0}", format_value(static_cast<BitmapTex*>(map)));

        return value;
    }
}

asf::auto_release_ptr<asr::Material> AppleseedDisneyMtl::create_material(asr::Assembly& assembly, const char* name)
{
    auto material = asr::DisneyMaterialFactory::static_create(name, asr::ParamArray());
    auto values = asr::DisneyMaterialLayer::get_default_values();

    // The Disney material expects sRGB colors, so we have to convert input colors to sRGB.

    if (is_bitmap(m_base_color_texmap))
        values.insert("base_color", format_value(static_cast<BitmapTex*>(m_base_color_texmap)));
    else values.insert("base_color", fmt_color_expr(asf::linear_rgb_to_srgb(to_color3f(m_base_color))));

    values.insert("metallic", format_value(m_metallic, m_metallic_texmap));
    values.insert("specular", format_value(m_specular, m_specular_texmap));
    values.insert("specular_tint", format_value(m_specular_tint, m_specular_tint_texmap));
    values.insert("anisotropic", format_value(m_anisotropic, m_anisotropic_texmap));
    values.insert("roughness", format_value(m_roughness, m_roughness_texmap));
    values.insert("clearcoat", format_value(m_clearcoat, m_clearcoat_texmap));
    values.insert("clearcoat_gloss", format_value(m_clearcoat_gloss, m_clearcoat_gloss_texmap));

    auto disney_material = static_cast<asr::DisneyMaterial*>(material.get());
    disney_material->add_layer(values);

    return material;
}


//
// AppleseedDisneyMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedDisneyMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedDisneyMtlFriendlyClassName;
}

const MCHAR* AppleseedDisneyMtlBrowserEntryInfo::GetEntryCategory() const
{
    return _T("Materials\\appleseed");
}

Bitmap* AppleseedDisneyMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedDisneyMtlClassDesc class implementation.
//

int AppleseedDisneyMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedDisneyMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedDisneyMtl();
}

const MCHAR* AppleseedDisneyMtlClassDesc::ClassName()
{
    return AppleseedDisneyMtlFriendlyClassName;
}

SClass_ID AppleseedDisneyMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedDisneyMtlClassDesc::ClassID()
{
    return AppleseedDisneyMtl::get_class_id();
}

const MCHAR* AppleseedDisneyMtlClassDesc::Category()
{
    return _T("");
}

const MCHAR* AppleseedDisneyMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return _T("appleseed_disneymtl");
}

FPInterface* AppleseedDisneyMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedDisneyMtlClassDesc::HInstance()
{
    return g_module;
}
