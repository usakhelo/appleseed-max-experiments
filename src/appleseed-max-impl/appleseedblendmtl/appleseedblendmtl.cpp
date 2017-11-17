
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
#include "appleseedblendmtl.h"

// appleseed-max headers.
#include "appleseedblendmtl/datachunks.h"
#include "appleseedblendmtl/resource.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/material.h"
#include "renderer/api/scene.h"
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"
#include "renderer/modeling/shadergroup/shader.h"
#include "renderer/modeling/shadergroup/shaderconnection.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"

// 3ds Max headers.
#include <color.h>
#include <iparamm2.h>
#include <stdmat.h>
#include <strclass.h>

// Standard headers.
#include <string>

// Windows headers.
#include <tchar.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const wchar_t* AppleseedBlendMtlFriendlyClassName = L"appleseed Blend Material";
}

AppleseedBlendMtlClassDesc g_appleseed_blendmtl_classdesc;


//
// AppleseedBlendMtl class implementation.
//

namespace
{
    enum { ParamBlockIdBlendMtl };
    enum { ParamBlockRefBlendMtl };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdMtl              = 0,
        ParamIdMaskTex          = 1,
        ParamIdMaskAmount       = 2,
        ParamIdMixColor         = 3,
    };

    enum MtlId
    {
        // Changing these value WILL break compatibility.
        MaterialBase                = 0,
        MaterialCoat1               = 1,
        MaterialCoat2               = 2,
        MaterialCoat3               = 3,
        MaterialCoat4               = 4,
        MaterialCoat5               = 5,
        MaterialCoat6               = 6,
        MaterialCoat7               = 7,
        MaterialCoat8               = 8,
        MaterialCoat9               = 9,
        MaterialCoat10              = 10,
        MtlCount                 // keep last
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapMask1     = 0,
        TexmapMask2     = 1,
        TexmapMask3     = 2,
        TexmapMask4     = 3,
        TexmapMask5     = 4,
        TexmapMask6     = 5,
        TexmapMask7     = 6,
        TexmapMask8     = 7,
        TexmapMask9     = 8,
        TexmapMask10    = 9,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[10] =
    {
        L"Mask 1",
        L"Mask 2",
        L"Mask 3",
        L"Mask 4",
        L"Mask 5",
        L"Mask 6",
        L"Mask 7",
        L"Mask 8",
        L"Mask 9",
        L"Mask 10",
    };

    const MSTR g_material_slot_names[11] =
    {
        L"Base Material",
        L"Coat Material 1",
        L"Coat Material 2",
        L"Coat Material 3",
        L"Coat Material 4",
        L"Coat Material 5",
        L"Coat Material 6",
        L"Coat Material 7",
        L"Coat Material 8",
        L"Coat Material 9",
        L"Coat Material 10",
    };

    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        ParamBlockIdBlendMtl,                       // parameter block's ID
        L"appleseedBlendMtlParams",                 // internal parameter block's name
        0,                                          // ID of the localized name string
        &g_appleseed_blendmtl_classdesc,            // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        ParamBlockRefBlendMtl,                      // parameter block's reference number

        // --- P_AUTO_UI arguments for Blend rollup ---
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure
        
        // --- Parameters specifications for Blend rollup ---

        ParamIdMtl, L"material_list", TYPE_MTL_TAB, 3, P_SUBANIM | P_SHORT_LABELS, IDS_MATERIAL,
            p_submtlno, 0,
            p_ui,   TYPE_MTLBUTTON,     IDC_MTLBTN_BASE,IDC_MTLBTN_COAT_1, IDC_MTLBTN_COAT_2, 
                                        /*IDC_MTLBTN_COAT_3, IDC_MTLBTN_COAT_4, IDC_MTLBTN_COAT_5,
                                        IDC_MTLBTN_COAT_6, IDC_MTLBTN_COAT_7, IDC_MTLBTN_COAT_8,
                                        IDC_MTLBTN_COAT_9, IDC_MTLBTN_COAT_10,*/
        p_end,

        ParamIdMaskTex, L"mask_list", TYPE_TEXMAP_TAB, 2, P_SUBANIM | P_SHORT_LABELS, IDS_MASK_TEXTURE,
            p_subtexno, 0,
            p_ui,   TYPE_TEXMAPBUTTON,  IDC_TEXBTN_MASK_1, IDC_TEXBTN_MASK_2, /*IDC_TEXBTN_MASK_3,
                                        IDC_TEXBTN_MASK_4, IDC_TEXBTN_MASK_5, IDC_TEXBTN_MASK_6,
                                        IDC_TEXBTN_MASK_7, IDC_TEXBTN_MASK_8, IDC_TEXBTN_MASK_9,
                                        IDC_TEXBTN_MASK_10,*/
        p_end,

        
        ParamIdMaskAmount, L"texture_amount_list", TYPE_FLOAT_TAB, 2, P_ANIMATABLE, IDS_MASK_AMOUNT,
            p_default, 50.0f,
            p_range, 0.0f, 100.0f,
            p_ui,   TYPE_SPINNER, EDITTYPE_FLOAT,
                        IDC_EDIT_AMOUNT_1, IDC_SPIN_AMOUNT_1, 
                        IDC_EDIT_AMOUNT_2, IDC_SPIN_AMOUNT_2,
                        /*IDC_EDIT_AMOUNT_3, IDC_SPIN_AMOUNT_3,
                        IDC_EDIT_AMOUNT_4, IDC_SPIN_AMOUNT_4,
                        IDC_EDIT_AMOUNT_5, IDC_SPIN_AMOUNT_5,
                        IDC_EDIT_AMOUNT_6, IDC_SPIN_AMOUNT_6,
                        IDC_EDIT_AMOUNT_7, IDC_SPIN_AMOUNT_7,
                        IDC_EDIT_AMOUNT_8, IDC_SPIN_AMOUNT_8,
                        IDC_EDIT_AMOUNT_9, IDC_SPIN_AMOUNT_9,
                        IDC_EDIT_AMOUNT_10, IDC_SPIN_AMOUNT_10,*/
            0.1f,
        p_end,

        // --- The end ---
        p_end);
}

Class_ID AppleseedBlendMtl::get_class_id()
{
    return Class_ID(0x27752cf8, 0x5e8c6be3);
}

AppleseedBlendMtl::AppleseedBlendMtl()
  : m_pblock(nullptr)
{
    m_params_validity.SetEmpty();

    g_appleseed_blendmtl_classdesc.MakeAutoParamBlocks(this);
}

BaseInterface* AppleseedBlendMtl::GetInterface(Interface_ID id)
{
    return
        id == IAppleseedMtl::interface_id()
            ? static_cast<IAppleseedMtl*>(this)
            : Mtl::GetInterface(id);
}

void AppleseedBlendMtl::DeleteThis()
{
    delete this;
}

void AppleseedBlendMtl::GetClassName(TSTR& s)
{
    s = L"AppleseedBlendMtl";
}

SClass_ID AppleseedBlendMtl::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedBlendMtl::ClassID()
{
    return get_class_id();
}

int AppleseedBlendMtl::NumSubs()
{
    return NumRefs();
}

Animatable* AppleseedBlendMtl::SubAnim(int i)
{
    return GetReference(i);
}

TSTR AppleseedBlendMtl::SubAnimName(int i)
{
    return i == ParamBlockRefBlendMtl ? L"Parameters" : L"";
}

int AppleseedBlendMtl::SubNumToRefNum(int subNum)
{
    return subNum;
}

int AppleseedBlendMtl::NumParamBlocks()
{
    return 1;
}

IParamBlock2* AppleseedBlendMtl::GetParamBlock(int i)
{
    return i == ParamBlockRefBlendMtl ? m_pblock : nullptr;
}

IParamBlock2* AppleseedBlendMtl::GetParamBlockByID(BlockID id)
{
    return id == m_pblock->ID() ? m_pblock : nullptr;
}

int AppleseedBlendMtl::NumRefs()
{
    return 1;
}

RefTargetHandle AppleseedBlendMtl::GetReference(int i)
{
    return i == ParamBlockRefBlendMtl ? m_pblock : nullptr;
}

void AppleseedBlendMtl::SetReference(int i, RefTargetHandle rtarg)
{
    if (i == ParamBlockRefBlendMtl)
    {
        if (IParamBlock2* pblock = dynamic_cast<IParamBlock2*>(rtarg))
            m_pblock = pblock;
    }
}

RefResult AppleseedBlendMtl::NotifyRefChanged(
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
        if (hTarget == m_pblock)
            g_block_desc.InvalidateUI(m_pblock->LastNotifyParamID());
        break;
    }

    return REF_SUCCEED;
}

RefTargetHandle AppleseedBlendMtl::Clone(RemapDir& remap)
{
    AppleseedBlendMtl* clone = new AppleseedBlendMtl();
    *static_cast<MtlBase*>(clone) = *static_cast<MtlBase*>(this);
    clone->ReplaceReference(ParamBlockRefBlendMtl, remap.CloneRef(m_pblock));
    BaseClone(this, clone, remap);
    return clone;
}

int AppleseedBlendMtl::NumSubTexmaps()
{
    return 2; // TexmapCount;
}

Texmap* AppleseedBlendMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    m_pblock->GetValue(ParamIdMaskTex, 0, texmap, valid, i);

    return texmap;
}

void AppleseedBlendMtl::SetSubTexmap(int i, Texmap* texmap)
{
    m_pblock->SetValue(ParamIdMaskTex, 0, texmap, i);
}

int AppleseedBlendMtl::MapSlotType(int i)
{
    return MAPSLOT_TEXTURE;
}

MSTR AppleseedBlendMtl::GetSubTexmapSlotName(int i)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    return g_texmap_slot_names[texmap_id];
}

void AppleseedBlendMtl::Update(TimeValue t, Interval& valid)
{
    if (!m_params_validity.InInterval(t))
    {
        m_params_validity.SetInfinite();

        int mat_count = m_pblock->Count(ParamIdMtl);
        int tex_count = m_pblock->Count(ParamIdMaskTex);
        if (mat_count > 0)
        {
            Mtl* mat = nullptr;
            for (int i = 0; i < mat_count; ++i)
            {
                m_pblock->GetValue(ParamIdMtl, t, mat, valid, i);
                mat->Update(t, valid);
            }
        }
        if (tex_count > 0)
        {
            Texmap* tex = nullptr;
            for (int i = 0; i < mat_count; ++i)
            {
                m_pblock->GetValue(ParamIdMaskTex, t, tex, valid, i);
                tex->Update(t, valid);
            }
        }
        NotifyDependents(FOREVER, PART_ALL, REFMSG_DISPLAY_MATERIAL_CHANGE);
    }

    valid &= m_params_validity;
}

void AppleseedBlendMtl::Reset()
{
    g_appleseed_blendmtl_classdesc.Reset(this);

    m_params_validity.SetEmpty();
}

Interval AppleseedBlendMtl::Validity(TimeValue t)
{
    Interval valid = FOREVER;
    Update(t, valid);
    return valid;
}

ParamDlg* AppleseedBlendMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    ParamDlg* param_dialog = g_appleseed_blendmtl_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
    return param_dialog;
}

IOResult AppleseedBlendMtl::Save(ISave* isave)
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

IOResult AppleseedBlendMtl::Load(ILoad* iload)
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

Color AppleseedBlendMtl::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedBlendMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedBlendMtl::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedBlendMtl::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedBlendMtl::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedBlendMtl::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedBlendMtl::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedBlendMtl::SetDiffuse(Color c, TimeValue t)
{
}

void AppleseedBlendMtl::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedBlendMtl::SetShininess(float v, TimeValue t)
{
}

void AppleseedBlendMtl::Shade(ShadeContext& sc)
{
}

int AppleseedBlendMtl::NumSubMtls()
{
    return 3; // MtlCount;
}

Mtl* AppleseedBlendMtl::GetSubMtl(int i)
{
    Mtl* mtl = nullptr;
    m_pblock->GetValue(ParamIdMtl, 0, mtl, FOREVER, i);
    return mtl;
}

void AppleseedBlendMtl::SetSubMtl(int i, Mtl* m)
{
    m_pblock->SetValue(ParamIdMtl, 0, m, i);
}

MSTR AppleseedBlendMtl::GetSubMtlSlotName(int i)
{
    const auto mtl_id = static_cast<MtlId>(i);
    return g_material_slot_names[mtl_id];
}

int AppleseedBlendMtl::get_sides() const
{
    return asr::ObjectInstance::BothSides;
}

bool AppleseedBlendMtl::can_emit_light() const
{
    return false;
}

asf::auto_release_ptr<asr::Material> AppleseedBlendMtl::create_material(
    asr::Assembly&  assembly,
    const char*     name,
    const bool      use_max_procedural_maps)
{
    return
        use_max_procedural_maps
            ? create_builtin_material(assembly, name)
            : create_osl_material(assembly, name);
}

asf::auto_release_ptr<asr::Material> AppleseedBlendMtl::create_osl_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    Mtl* mtl1 = GetSubMtl(0);
    Mtl* mtl2 = GetSubMtl(1);
    
    if (!mtl1 || !mtl2)
        return asr::OSLMaterialFactory().create(name, asr::ParamArray());

    auto appleseed_mtl1 =
        static_cast<IAppleseedMtl*>(mtl1->GetInterface(IAppleseedMtl::interface_id()));

    auto appleseed_mtl2 =
        static_cast<IAppleseedMtl*>(mtl2->GetInterface(IAppleseedMtl::interface_id()));

    if (!appleseed_mtl1 || !appleseed_mtl2)
        return asr::OSLMaterialFactory().create(name, asr::ParamArray());

    std::string mtl1_name =
        make_unique_name(assembly.materials(), wide_to_utf8(mtl1->GetName()) + "_mat");
    auto proj_mtl1 = appleseed_mtl1->create_material(assembly, mtl1_name.c_str(), false);

    std::string mtl2_name =
        make_unique_name(assembly.materials(), wide_to_utf8(mtl2->GetName()) + "_mat");
    auto proj_mtl2 = appleseed_mtl2->create_material(assembly, mtl2_name.c_str(), false);

    asr::ShaderGroup* sh_grp_mtl1 = assembly.shader_groups().get_by_name(asf::format("{0}_{1}", mtl1_name, "shader_group").c_str());
    asr::ShaderGroup* sh_grp_mtl2 = assembly.shader_groups().get_by_name(asf::format("{0}_{1}", mtl2_name, "shader_group").c_str());

    if (!sh_grp_mtl1 || !sh_grp_mtl2)
        return asr::OSLMaterialFactory().create(name, asr::ParamArray());

    auto shader_group_name = make_unique_name(assembly.shader_groups(), std::string(name) + "_shader_group");
    auto shader_group = asr::ShaderGroupFactory::create(shader_group_name.c_str());

    for (const auto& shader : sh_grp_mtl1->shaders())
        shader_group->add_shader(shader.get_type(), shader.get_shader(), shader.get_layer(), shader.get_parameters());

    for (const auto& shader : sh_grp_mtl2->shaders())
        shader_group->add_shader(shader.get_type(), shader.get_shader(), shader.get_layer(), shader.get_parameters());

    for (const auto& conn : sh_grp_mtl1->shader_connections())
        shader_group->add_connection(conn.get_src_layer(), conn.get_src_param(), conn.get_dst_layer(), conn.get_dst_param());

    for (const auto& conn : sh_grp_mtl2->shader_connections())
        shader_group->add_connection(conn.get_src_layer(), conn.get_src_param(), conn.get_dst_layer(), conn.get_dst_param());

    // copy all the shaders from shader groups of mtl1 and mtl2 to blend shader group
    // add blender shader after that
    // copy all the connections from shader groups of mtl1 and mtl2 to blend shader group
    // add connection from mtl1 and mtl2 shader outputs to the blend shader input

    shader_group->add_connection(mtl1_name.c_str(), "Ci", name, "m1");

    shader_group->add_connection(mtl2_name.c_str(), "Ci", name, "m2");

    // Must come last.
    shader_group->add_shader("surface", "as_max_blend_material", name,
        asr::ParamArray()
            .insert("MixAmount", fmt_osl_expr(.5)));
    assembly.shader_groups().insert(shader_group);

    //
    // Material.
    //

    asr::ParamArray material_params;
    material_params.insert("osl_surface", shader_group_name);

    return asr::OSLMaterialFactory().create(name, material_params);
}

asf::auto_release_ptr<asr::Material> AppleseedBlendMtl::create_builtin_material(
    asr::Assembly&  assembly,
    const char*     name)
{
    return asr::GenericMaterialFactory().create(name, asr::ParamArray());
}


//
// AppleseedBlendMtlBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedBlendMtlBrowserEntryInfo::GetEntryName() const
{
    return AppleseedBlendMtlFriendlyClassName;
}

const MCHAR* AppleseedBlendMtlBrowserEntryInfo::GetEntryCategory() const
{
    return L"Materials\\appleseed";
}

Bitmap* AppleseedBlendMtlBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedBlendMtlClassDesc class implementation.
//

AppleseedBlendMtlClassDesc::AppleseedBlendMtlClassDesc()
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

int AppleseedBlendMtlClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedBlendMtlClassDesc::Create(BOOL loading)
{
    return new AppleseedBlendMtl();
}

const MCHAR* AppleseedBlendMtlClassDesc::ClassName()
{
    return AppleseedBlendMtlFriendlyClassName;
}

SClass_ID AppleseedBlendMtlClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedBlendMtlClassDesc::ClassID()
{
    return AppleseedBlendMtl::get_class_id();
}

const MCHAR* AppleseedBlendMtlClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedBlendMtlClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"AppleseedBlendMtl";
}

FPInterface* AppleseedBlendMtlClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}

HINSTANCE AppleseedBlendMtlClassDesc::HInstance()
{
    return g_module;
}

bool AppleseedBlendMtlClassDesc::IsCompatibleWithRenderer(ClassDesc& renderer_class_desc)
{
    // Before 3ds Max 2017, Class_ID::operator==() returned an int.
    return renderer_class_desc.ClassID() == AppleseedRenderer::get_class_id() ? true : false;
}
