
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Francois Beaune, The appleseedhq Organization
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
#include "oslutils.h"

// appleseed-max headers.
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/shadergroup.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/utility/string.h"

// 3ds Max Headers.
#include <maxapi.h>
#include <bitmap.h>
#include <stdmat.h>
#include "dbgprint.h"

namespace asf = foundation;
namespace asr = renderer;

std::string fmt_osl_expr(const std::string& s)
{
    return asf::format("string {0}", s);
}

std::string fmt_osl_expr(const float value)
{
    return asf::format("float {0}", value);
}

std::string fmt_osl_expr(const asf::Color3f& linear_rgb)
{
    return asf::format("color {0} {1} {2}", linear_rgb.r, linear_rgb.g, linear_rgb.b);
}

std::string fmt_osl_expr(Texmap* texmap)
{
    if (is_bitmap_texture(texmap))
    {
        const auto texture_filepath = wide_to_utf8(static_cast<BitmapTex*>(texmap)->GetMap().GetFullFilePath());
        return fmt_osl_expr(texture_filepath);
    }
    else return fmt_osl_expr(std::string());
}

void connect_float_texture(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const float         multiplier)
{
    auto layer_name = asf::format("{0}_{1}", material_node_name, material_input_name);

    shader_group.add_shader("shader", "as_max_float_texture", layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap))
            .insert("Multiplier", fmt_osl_expr(multiplier)));

    shader_group.add_connection(
        layer_name.c_str(), "FloatOut",
        material_node_name, material_input_name);
}

void connect_color_texture(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_input_name,
    Texmap*             texmap,
    const Color         multiplier)
{
    auto layer_name = asf::format("{0}_{1}", material_node_name, material_input_name);

    asr::ParamArray texture_params;
    texture_params.insert("Filename", fmt_osl_expr(texmap));
    texture_params.insert("Multiplier", fmt_osl_expr(to_color3f(multiplier)));
    if (texmap && texmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
    {
        StdUVGen* uv_gen = ((BitmapTex*)texmap)->GetUVGen();
        if (uv_gen) {
            TimeValue t = GetCOREInterface()->GetTime();
            DebugPrint(_T("GetUOffs: %f\n"), uv_gen->GetUOffs(t));
            DebugPrint(_T("GetVOffs: %f\n"), uv_gen->GetVOffs(t));
            DebugPrint(_T("GetUScl: %f\n"), uv_gen->GetUScl(t));
            DebugPrint(_T("GetVScl: %f\n"), uv_gen->GetVScl(t));
            texture_params.insert("U", fmt_osl_expr(uv_gen->GetUOffs(t)));
            texture_params.insert("V", fmt_osl_expr(uv_gen->GetVOffs(t)));
            //texture_params.insert("UWidth", fmt_osl_expr(uv_gen->GetUScl(t)));
            //texture_params.insert("VWidth", fmt_osl_expr(uv_gen->GetVScl(t)));
        }
    }
    shader_group.add_shader("shader", "as_max_color_texture", layer_name.c_str(), texture_params);
    //    asr::ParamArray()
    //        .insert("Filename", fmt_osl_expr(texmap))
    //        .insert("Multiplier", fmt_osl_expr(to_color3f(multiplier)))
    //);

    shader_group.add_connection(
        layer_name.c_str(), "ColorOut",
        material_node_name, material_input_name);
}

void connect_bump_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const float         amount)
{
    auto texture_layer_name = asf::format("{0}_bump_map_texture", material_node_name);
    shader_group.add_shader("shader", "as_max_float_texture", texture_layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap)));

    auto bump_map_layer_name = asf::format("{0}_bump_map", material_node_name);
    shader_group.add_shader("shader", "as_max_bump_map", bump_map_layer_name.c_str(),
        asr::ParamArray()
            .insert("Amount", fmt_osl_expr(amount)));

    shader_group.add_connection(
        texture_layer_name.c_str(), "FloatOut",
        bump_map_layer_name.c_str(), "Height");
    shader_group.add_connection(
        bump_map_layer_name.c_str(), "NormalOut",
        material_node_name, material_normal_input_name);
}

void connect_normal_map(
    asr::ShaderGroup&   shader_group,
    const char*         material_node_name,
    const char*         material_normal_input_name,
    const char*         material_tn_input_name,
    Texmap*             texmap,
    const int           up_vector)
{
    auto texture_layer_name = asf::format("{0}_normal_map_texture", material_node_name);
    shader_group.add_shader("shader", "as_max_color_texture", texture_layer_name.c_str(),
        asr::ParamArray()
            .insert("Filename", fmt_osl_expr(texmap)));

    auto normal_map_layer_name = asf::format("{0}_normal_map", material_node_name);
    shader_group.add_shader("shader", "as_max_normal_map", normal_map_layer_name.c_str(),
        asr::ParamArray()
            .insert("UpVector", fmt_osl_expr(up_vector == 0 ? "Green" : "Blue")));

    shader_group.add_connection(
        texture_layer_name.c_str(), "ColorOut",
        normal_map_layer_name.c_str(), "Color");
    shader_group.add_connection(
        normal_map_layer_name.c_str(), "NormalOut",
        material_node_name, material_normal_input_name);
    shader_group.add_connection(
        normal_map_layer_name.c_str(), "TangentOut",
        material_node_name, material_tn_input_name);
}
