
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

    enum ParamMapId
    {
        ParamMapIdBlend,
    };

    enum ParamId
    {
        // Changing these value WILL break compatibility.
        ParamIdBaseMtl              = 0,
        ParamIdCoatMtl1             = 1,
        ParamIdCoatMixAmount1       = 2,
        ParamIdCoatMixColor1        = 3,
        ParamIdCoatMask1            = 4
    };

    enum MtlId
    {
        // Changing these value WILL break compatibility.
        MaterialBase                = 0,
        MaterialCoat1               = 1,
        MtlCount                 // keep last
    };

    enum TexmapId
    {
        // Changing these value WILL break compatibility.
        TexmapMask1 = 0,
        TexmapCount                 // keep last
    };

    const MSTR g_texmap_slot_names[TexmapCount] =
    {
        L"Mask1",
    };

    const ParamId g_texmap_id_to_param_id[TexmapCount] =
    {
        ParamIdCoatMask1,
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

        ParamIdBaseMtl, L"base_material", TYPE_MTL, P_SUBANIM, IDS_MATERIAL_BASE,
            p_submtlno, MaterialBase,
            p_ui, TYPE_MTLBUTTON, IDC_MTLBTN_BASE,
        p_end,

        ParamIdCoatMtl1, L"coat_material_1", TYPE_MTL, P_SUBANIM, IDS_MATERIAL_COAT1,
            p_submtlno, MaterialCoat1,
            p_ui, TYPE_MTLBUTTON, IDC_MTLBTN_COAT_1,
        p_end,

        ParamIdCoatMixAmount1, L"coat_mix_amount_1", TYPE_FLOAT, P_ANIMATABLE, IDS_MIX_AMOUNT1,
            p_default, 50.0f,
            p_range, 0.0f, 100.0f,
            p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_AMOUNT_1, IDC_SPIN_AMOUNT_1, 5.0f,
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
    return TexmapCount;
}

Texmap* AppleseedBlendMtl::GetSubTexmap(int i)
{
    Texmap* texmap;
    Interval valid;

    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->GetValue(param_id, 0, texmap, valid);

    return texmap;
}

void AppleseedBlendMtl::SetSubTexmap(int i, Texmap* texmap)
{
    const auto texmap_id = static_cast<TexmapId>(i);
    const auto param_id = g_texmap_id_to_param_id[texmap_id];
    m_pblock->SetValue(param_id, 0, texmap);
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

        NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
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
    return 2;
}

Mtl* AppleseedBlendMtl::GetSubMtl(int i)
{
    Mtl* mtl = nullptr;
    switch (i)
    {
    case 0:
        m_pblock->GetValue(ParamIdBaseMtl, 0, mtl, FOREVER);
        break;
    case 1:
        m_pblock->GetValue(ParamIdCoatMtl1, 0, mtl, FOREVER);
        break;
    }
    return mtl;
}

void AppleseedBlendMtl::SetSubMtl(int i, Mtl* m)
{
    switch (i)
    {
      case 0:
        m_pblock->SetValue(ParamIdBaseMtl, 0, m);
        break;
      case 1:
        m_pblock->SetValue(ParamIdCoatMtl1, 0, m);
        break;
    }
}

MSTR AppleseedBlendMtl::GetSubMtlSlotName(int i)
{
    switch (i)
    {
      case 0:
        return L"Base Material";
      case 1:
        return L"Coat Material 1";
      default:
        return L"";
    }
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
    return asr::OSLMaterialFactory().create(name, asr::ParamArray());
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
