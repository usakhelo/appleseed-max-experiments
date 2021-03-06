
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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
#include "appleseedrendererparamdlg.h"

// appleseed-max headers.
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedrenderer/renderersettings.h"
#include "appleseedrenderer/resource.h"
#include "appleseedrenderer/updatechecker.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>
#include <assert1.h>

// Windows headers.
#include <shellapi.h>
#include <tchar.h>

// Standard headers.
#include <future>
#include <memory>
#include <sstream>
#include <string>

namespace asf = foundation;

namespace
{
    // ------------------------------------------------------------------------------------------------
    // Utilities.
    // ------------------------------------------------------------------------------------------------

    void set_label_text(const HWND parent_hwnd, const int control_id, const wchar_t* text)
    {
        SetDlgItemText(parent_hwnd, control_id, text);

        const HWND control_hwnd = GetDlgItem(parent_hwnd, control_id);

        RECT rect;
        GetClientRect(control_hwnd, &rect);
        InvalidateRect(control_hwnd, &rect, TRUE);
        MapWindowPoints(control_hwnd, parent_hwnd, reinterpret_cast<POINT*>(&rect), 2);
        RedrawWindow(parent_hwnd, &rect, nullptr, RDW_ERASE | RDW_INVALIDATE);
    }

    bool get_save_project_filepath(HWND parent_hwnd, MSTR& filepath)
    {
        FilterList filter;
        filter.Append(L"Project Files (*.appleseed)");
        filter.Append(L"*.appleseed");
        filter.Append(L"All Files (*.*)");
        filter.Append(L"*.*");

        MSTR initial_dir;
        return
            GetCOREInterface14()->DoMaxSaveAsDialog(
                parent_hwnd,
                L"Save Project As...",
                filepath,
                initial_dir,
                filter);
    }

    // ------------------------------------------------------------------------------------------------
    // Base class for panels (rollups).
    // ------------------------------------------------------------------------------------------------

    struct PanelBase
    {
        virtual ~PanelBase() {}

        virtual void init(HWND hwnd) = 0;

        virtual INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) = 0;

        static INT_PTR CALLBACK dialog_proc_entry(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam)
        {
            switch (umsg)
            {
              case WM_INITDIALOG:
                {
                    PanelBase* panel = reinterpret_cast<PanelBase*>(lparam);
                    DLSetWindowLongPtr(hwnd, panel);
                    panel->init(hwnd);
                    return TRUE;
                }

              default:
                {
                    PanelBase* panel = DLGetWindowLongPtr<PanelBase*>(hwnd);
                    return panel->dialog_proc(hwnd, umsg, wparam, lparam);
                }
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // About panel.
    // ------------------------------------------------------------------------------------------------

    struct AboutPanel
      : public PanelBase
    {
        struct UpdateCheckData
        {
            bool            m_update_available;
            std::wstring    m_version_string;
            std::wstring    m_download_url;
        };

        IRendParams*                    m_rend_params;
        HWND                            m_rollup;
        HWND                            m_button_download_hwnd;
        ICustButton*                    m_button_download;
        std::auto_ptr<UpdateCheckData>  m_update_data;

        enum { WM_UPDATE_CHECK_DATA = WM_USER + 101 };

        explicit AboutPanel(
            IRendParams*        rend_params)
          : m_rend_params(rend_params)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_ABOUT),
                    &dialog_proc_entry,
                    L"About",
                    reinterpret_cast<LPARAM>(this));
        }

        ~AboutPanel() override
        {
            ReleaseICustButton(m_button_download);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        static void async_update_check(HWND hwnd)
        {
            std::auto_ptr<UpdateCheckData> data(new UpdateCheckData());
            data->m_update_available = false;

            std::string version_string, publication_date, download_url;
            if (check_for_update(version_string, publication_date, download_url))
            {
                const std::wstring wide_version_string = utf8_to_wide(version_string);
                if (wide_version_string > std::wstring(PluginVersionString))
                {
                    data->m_update_available = true;
                    data->m_version_string = wide_version_string;
                    data->m_download_url = utf8_to_wide(download_url);
                }
            }

            PostMessage(hwnd, WM_UPDATE_CHECK_DATA, reinterpret_cast<WPARAM>(data.release()), 0);
        }

        void init(HWND hwnd) override
        {
            m_button_download_hwnd = GetDlgItem(hwnd, IDC_BUTTON_DOWNLOAD);
            m_button_download = GetICustButton(m_button_download_hwnd);
            ShowWindow(m_button_download_hwnd, SW_HIDE);

            // Display the version strings of the plugin and of appleseed itself.
            GetICustStatus(GetDlgItem(hwnd, IDC_TEXT_PLUGIN_VERSION))->SetText(PluginVersionString);
            GetICustStatus(GetDlgItem(hwnd, IDC_TEXT_APPLESEED_VERSION))->SetText(
                utf8_to_wide(asf::Appleseed::get_lib_version()).c_str());

            // Asynchronously check if an update is available.
            std::async(std::launch::async, async_update_check, hwnd);
        }

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case WM_UPDATE_CHECK_DATA:
                {
                    m_update_data.reset(reinterpret_cast<UpdateCheckData*>(wparam));
                    if (m_update_data->m_update_available)
                    {
                        std::wstringstream sstr;
                        sstr << "UPDATE: Version " << m_update_data->m_version_string << " available.";
                        set_label_text(hwnd, IDC_STATIC_NEW_VERSION, sstr.str().c_str());
                        ShowWindow(m_button_download_hwnd, SW_SHOW);
                    }
                    else
                    {
                        set_label_text(hwnd, IDC_STATIC_NEW_VERSION, L"The plugin is up-to-date.");
                    }
                    return TRUE;
                }

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_BUTTON_DOWNLOAD:
                    {
                        DbgAssert(m_update_data.get());
                        DbgAssert(m_update_data->m_update_available);
                        ShellExecute(
                            hwnd,
                            L"open",
                            m_update_data->m_download_url.c_str(),
                            nullptr,            // application parameters
                            nullptr,            // working directory
                            SW_SHOWNORMAL);
                        return TRUE;
                    }

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Image Sampling panel.
    // ------------------------------------------------------------------------------------------------

    struct ImageSamplingPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_pixel_samples;
        ISpinnerControl*        m_spinner_pixel_samples;
        ICustEdit*              m_text_passes;
        ISpinnerControl*        m_spinner_passes;
        ICustEdit*              m_text_tile_size;
        ISpinnerControl*        m_spinner_tile_size;
        ICustEdit*              m_text_filter_size;
        ISpinnerControl*        m_spinner_filter_size;

        ImageSamplingPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_IMAGESAMPLING),
                    &dialog_proc_entry,
                    L"Image Sampling",
                    reinterpret_cast<LPARAM>(this));
        }

        ~ImageSamplingPanel() override
        {
            ReleaseISpinner(m_spinner_tile_size);
            ReleaseICustEdit(m_text_tile_size);
            ReleaseICustEdit(m_text_filter_size);
            ReleaseISpinner(m_spinner_filter_size);
            ReleaseISpinner(m_spinner_passes);
            ReleaseICustEdit(m_text_passes);
            ReleaseISpinner(m_spinner_pixel_samples);
            ReleaseICustEdit(m_text_pixel_samples);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        void init(HWND hwnd) override
        {
            // Pixel Samples.
            m_text_pixel_samples = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PIXEL_SAMPLES));
            m_spinner_pixel_samples = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_PIXEL_SAMPLES));
            m_spinner_pixel_samples->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_PIXEL_SAMPLES), EDITTYPE_INT);
            m_spinner_pixel_samples->SetLimits(1, 1000000, FALSE);
            m_spinner_pixel_samples->SetResetValue(RendererSettings::defaults().m_pixel_samples);
            m_spinner_pixel_samples->SetValue(m_settings.m_pixel_samples, FALSE);

            // Passes.
            m_text_passes = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PASSES));
            m_spinner_passes = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_PASSES));
            m_spinner_passes->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_PASSES), EDITTYPE_INT);
            m_spinner_passes->SetLimits(1, 1000000, FALSE);
            m_spinner_passes->SetResetValue(RendererSettings::defaults().m_passes);
            m_spinner_passes->SetValue(m_settings.m_passes, FALSE);

            // Tile size.
            m_text_tile_size = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_TILE_SIZE));
            m_spinner_tile_size = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_TILE_SIZE));
            m_spinner_tile_size->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_TILE_SIZE), EDITTYPE_INT);
            m_spinner_tile_size->SetLimits(1, 4096, FALSE);
            m_spinner_tile_size->SetResetValue(RendererSettings::defaults().m_tile_size);
            m_spinner_tile_size->SetValue(m_settings.m_tile_size, FALSE);

            // Pixel Filtering.
            static const wchar_t* FilterComboItems[] =
            {
                L"Blackman-Harris",
                L"Box",
                L"Catmull-Rom Spline",
                L"Cubic B-spline",
                L"Gaussian",
                L"Lanczos",
                L"Mitchell-Netravali",
                L"Triangle"
            };

            for (size_t i = 0; i < 8; i++)
                SendMessage(GetDlgItem(hwnd, IDC_COMBO_FILTER), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(FilterComboItems[i]));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO_FILTER), CB_SETCURSEL, static_cast<int>(m_settings.m_pixel_filter), 0);

            m_text_filter_size = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_FILTER_SIZE));
            m_spinner_filter_size = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_FILTER_SIZE));
            m_spinner_filter_size->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_FILTER_SIZE), EDITTYPE_FLOAT);
            m_spinner_filter_size->SetLimits(1, 20, FALSE);
            m_spinner_filter_size->SetResetValue(RendererSettings::defaults().m_pixel_filter_size);
            m_spinner_filter_size->SetValue(m_settings.m_pixel_filter_size, FALSE);
        }

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case CC_SPINNER_CHANGE:
                switch (LOWORD(wparam))
                {
                  case IDC_SPINNER_PIXEL_SAMPLES:
                    m_settings.m_pixel_samples = m_spinner_pixel_samples->GetIVal();
                    return TRUE;

                  case IDC_SPINNER_PASSES:
                    m_settings.m_passes = m_spinner_passes->GetIVal();
                    return TRUE;

                  case IDC_SPINNER_TILE_SIZE:
                    m_settings.m_tile_size = m_spinner_tile_size->GetIVal();
                    return TRUE;

                  case IDC_SPINNER_FILTER_SIZE:
                    m_settings.m_pixel_filter_size = m_spinner_filter_size->GetFVal();
                    return TRUE;

                  default:
                    return FALSE;
                }

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_COMBO_FILTER:
                    if (HIWORD(wparam) == CBN_SELCHANGE)
                    {
                        LRESULT sel_mode = SendDlgItemMessage(hwnd, IDC_COMBO_FILTER, CB_GETCURSEL, 0, 0);
                        if (sel_mode != CB_ERR)
                        {
                            m_settings.m_pixel_filter = static_cast<int>(sel_mode);
                        }
                    }
                    break;

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Lighting panel.
    // ------------------------------------------------------------------------------------------------

    struct LightingPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        HWND                    m_static_bounces;
        ICustEdit*              m_text_bounces;
        ISpinnerControl*        m_spinner_bounces;
        HWND                    m_check_caustics;
        HWND                    m_check_max_ray_intensity;
        ICustEdit*              m_text_max_ray_intensity;
        ISpinnerControl*        m_spinner_max_ray_intensity;
        ICustEdit*              m_text_background_alpha;
        ISpinnerControl*        m_spinner_background_alpha;

        LightingPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_LIGHTING),
                    &dialog_proc_entry,
                    L"Lighting",
                    reinterpret_cast<LPARAM>(this));
        }

        ~LightingPanel() override
        {
            ReleaseISpinner(m_spinner_background_alpha);
            ReleaseICustEdit(m_text_background_alpha);
            ReleaseISpinner(m_spinner_bounces);
            ReleaseICustEdit(m_text_bounces);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        void init(HWND hwnd) override
        {
            m_static_bounces = GetDlgItem(hwnd, IDC_STATIC_BOUNCES);
            m_text_bounces = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_BOUNCES));
            m_spinner_bounces = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_BOUNCES));
            m_spinner_bounces->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_BOUNCES), EDITTYPE_POS_INT);
            m_spinner_bounces->SetLimits(0, 100, FALSE);
            m_spinner_bounces->SetResetValue(RendererSettings::defaults().m_bounces);
            m_spinner_bounces->SetValue(m_settings.m_bounces, FALSE);

            CheckDlgButton(hwnd, IDC_CHECK_GI, m_settings.m_gi ? BST_CHECKED : BST_UNCHECKED);

            m_check_caustics = GetDlgItem(hwnd, IDC_CHECK_CAUSTICS);
            CheckDlgButton(hwnd, IDC_CHECK_CAUSTICS, m_settings.m_caustics ? BST_CHECKED : BST_UNCHECKED);

            m_check_max_ray_intensity = GetDlgItem(hwnd, IDC_CHECK_MAX_RAY_INTENSITY);
            CheckDlgButton(hwnd, IDC_CHECK_MAX_RAY_INTENSITY,
                m_settings.m_max_ray_intensity_set ? BST_CHECKED : BST_UNCHECKED);
            m_text_max_ray_intensity = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_MAX_RAY_INTENSITY));
            m_spinner_max_ray_intensity = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_MAX_RAY_INTENSITY));
            m_spinner_max_ray_intensity->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_MAX_RAY_INTENSITY), EDITTYPE_POS_FLOAT);
            m_spinner_max_ray_intensity->SetLimits(0.0f, 1000.0f, FALSE);
            m_spinner_max_ray_intensity->SetResetValue(RendererSettings::defaults().m_max_ray_intensity);
            m_spinner_max_ray_intensity->SetValue(m_settings.m_max_ray_intensity, FALSE);

            CheckDlgButton(hwnd, IDC_CHECK_BACKGROUND_EMITS_LIGHT,
                m_settings.m_background_emits_light ? BST_CHECKED : BST_UNCHECKED);

            m_text_background_alpha = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_BACKGROUND_ALPHA));
            m_spinner_background_alpha = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_BACKGROUND_ALPHA));
            m_spinner_background_alpha->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_BACKGROUND_ALPHA), EDITTYPE_POS_FLOAT);
            m_spinner_background_alpha->SetLimits(0.0f, 1.0f, FALSE);
            m_spinner_background_alpha->SetResetValue(RendererSettings::defaults().m_background_alpha);
            m_spinner_background_alpha->SetValue(m_settings.m_background_alpha, FALSE);

            CheckDlgButton(hwnd, IDC_CHECK_FORCE_OFF_DEFAULT_LIGHT,
                m_settings.m_force_off_default_lights ? BST_CHECKED : BST_UNCHECKED);

            enable_disable_controls();
        }

        void enable_disable_controls()
        {
            EnableWindow(m_check_caustics, m_settings.m_gi ? TRUE : FALSE);

            EnableWindow(m_check_max_ray_intensity, m_settings.m_gi ? TRUE : FALSE);
            m_text_max_ray_intensity->Enable(m_settings.m_gi && m_settings.m_max_ray_intensity_set);
            m_spinner_max_ray_intensity->Enable(m_settings.m_gi && m_settings.m_max_ray_intensity_set);

            // Fix wrong background color on label when it becomes enabled.
            RedrawWindow(m_static_bounces, nullptr, nullptr, RDW_INVALIDATE);
        }

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_CHECK_GI:
                    m_settings.m_gi = IsDlgButtonChecked(hwnd, IDC_CHECK_GI) == BST_CHECKED;
                    enable_disable_controls();
                    return TRUE;

                  case IDC_CHECK_CAUSTICS:
                    m_settings.m_caustics = IsDlgButtonChecked(hwnd, IDC_CHECK_CAUSTICS) == BST_CHECKED;
                    return TRUE;

                  case IDC_CHECK_MAX_RAY_INTENSITY:
                    m_settings.m_max_ray_intensity_set =
                        IsDlgButtonChecked(hwnd, IDC_CHECK_MAX_RAY_INTENSITY) == BST_CHECKED;
                    enable_disable_controls();
                    return TRUE;

                  case IDC_CHECK_BACKGROUND_EMITS_LIGHT:
                    m_settings.m_background_emits_light =
                        IsDlgButtonChecked(hwnd, IDC_CHECK_BACKGROUND_EMITS_LIGHT) == BST_CHECKED;
                    return TRUE;

                  case IDC_CHECK_FORCE_OFF_DEFAULT_LIGHT:
                    m_settings.m_force_off_default_lights =
                        IsDlgButtonChecked(hwnd, IDC_CHECK_FORCE_OFF_DEFAULT_LIGHT) == BST_CHECKED;
                    return TRUE;

                  default:
                    return FALSE;
                }

              case CC_SPINNER_CHANGE:
                switch (LOWORD(wparam))
                {
                  case IDC_SPINNER_BOUNCES:
                    m_settings.m_bounces = m_spinner_bounces->GetIVal();
                    return TRUE;

                  case IDC_SPINNER_MAX_RAY_INTENSITY:
                    m_settings.m_max_ray_intensity = m_spinner_max_ray_intensity->GetFVal();
                    return TRUE;

                  case IDC_SPINNER_BACKGROUND_ALPHA:
                    m_settings.m_background_alpha = m_spinner_background_alpha->GetFVal();
                    return TRUE;

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Output panel.
    // ------------------------------------------------------------------------------------------------

    struct OutputPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        HWND                    m_radio_render_only;
        HWND                    m_radio_save_only;
        HWND                    m_radio_save_and_render;
        HWND                    m_static_project_filepath;
        ICustEdit*              m_text_project_filepath;
        ICustButton*            m_button_browse;
        ICustEdit*              m_text_scale_multiplier;
        ISpinnerControl*        m_spinner_scale_multiplier;

        OutputPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_OUTPUT),
                    &dialog_proc_entry,
                    L"Output",
                    reinterpret_cast<LPARAM>(this));
        }

        ~OutputPanel() override
        {
            ReleaseISpinner(m_spinner_scale_multiplier);
            ReleaseICustEdit(m_text_scale_multiplier);
            ReleaseICustButton(m_button_browse);
            ReleaseICustEdit(m_text_project_filepath);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        void init(HWND hwnd) override
        {
            m_radio_render_only = GetDlgItem(hwnd, IDC_RADIO_RENDER);
            m_radio_save_only = GetDlgItem(hwnd, IDC_RADIO_SAVEPROJECT);
            m_radio_save_and_render = GetDlgItem(hwnd, IDC_RADIO_SAVEPROJECT_AND_RENDER);
            m_static_project_filepath = GetDlgItem(hwnd, IDC_STATIC_PROJECT_FILEPATH);
            m_text_project_filepath = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PROJECT_FILEPATH));
            m_text_project_filepath->SetText(m_settings.m_project_file_path);
            m_button_browse = GetICustButton(GetDlgItem(hwnd, IDC_BUTTON_BROWSE));

            CheckRadioButton(
                hwnd,
                IDC_RADIO_RENDER,
                IDC_RADIO_SAVEPROJECT_AND_RENDER,
                m_settings.m_output_mode == RendererSettings::OutputMode::RenderOnly ? IDC_RADIO_RENDER :
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ? IDC_RADIO_SAVEPROJECT :
                IDC_RADIO_SAVEPROJECT_AND_RENDER);

            // Scale multiplier.
            m_text_scale_multiplier = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_SCALE_MULTIPLIER));
            m_spinner_scale_multiplier = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_SCALE_MULTIPLIER));
            m_spinner_scale_multiplier->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_SCALE_MULTIPLIER), EDITTYPE_POS_FLOAT);
            m_spinner_scale_multiplier->SetLimits(1.0e-6f, 1.0e6f, FALSE);
            m_spinner_scale_multiplier->SetResetValue(RendererSettings::defaults().m_scale_multiplier);
            m_spinner_scale_multiplier->SetValue(m_settings.m_scale_multiplier, FALSE);

            enable_disable_controls(false);
        }

        void enable_disable_controls(const bool use_max_procedural_maps)
        {
            const bool save_project =
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ||
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender;

            EnableWindow(m_radio_render_only, use_max_procedural_maps ? FALSE : TRUE);
            EnableWindow(m_radio_save_only, use_max_procedural_maps ? FALSE : TRUE);
            EnableWindow(m_radio_save_and_render, use_max_procedural_maps ? FALSE : TRUE);

            EnableWindow(m_static_project_filepath, save_project && !use_max_procedural_maps ? TRUE : FALSE);
            m_text_project_filepath->Enable(save_project && !use_max_procedural_maps);
            m_button_browse->Enable(save_project && !use_max_procedural_maps);

            // Fix wrong background color on label when it becomes enabled.
            RedrawWindow(m_static_project_filepath, nullptr, nullptr, RDW_INVALIDATE);
        }

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case WM_CUSTEDIT_ENTER:
                switch (LOWORD(wparam))
                {
                  case IDC_TEXT_PROJECT_FILEPATH:
                    m_text_project_filepath->GetText(m_settings.m_project_file_path);
                    m_settings.m_project_file_path = replace_extension(m_settings.m_project_file_path, L".appleseed");
                    m_text_project_filepath->SetText(m_settings.m_project_file_path);
                    return TRUE;

                  default:
                    return FALSE;
                }

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_RADIO_RENDER:
                    m_settings.m_output_mode = RendererSettings::OutputMode::RenderOnly;
                    enable_disable_controls(false);
                    return TRUE;

                  case IDC_RADIO_SAVEPROJECT:
                    m_settings.m_output_mode = RendererSettings::OutputMode::SaveProjectOnly;
                    enable_disable_controls(false);
                    return TRUE;

                  case IDC_RADIO_SAVEPROJECT_AND_RENDER:
                    m_settings.m_output_mode = RendererSettings::OutputMode::SaveProjectAndRender;
                    enable_disable_controls(false);
                    return TRUE;

                  case IDC_BUTTON_BROWSE:
                    {
                        MSTR filepath;
                        if (get_save_project_filepath(hwnd, filepath))
                        {
                            m_settings.m_project_file_path = filepath;
                            m_text_project_filepath->SetText(filepath);
                        }
                        return TRUE;
                    }

                  default:
                    return FALSE;
                }

              case CC_SPINNER_CHANGE:
                switch (LOWORD(wparam))
                {
                  case IDC_SPINNER_SCALE_MULTIPLIER:
                    m_settings.m_scale_multiplier = m_spinner_scale_multiplier->GetFVal();
                    return TRUE;

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // System panel.
    // ------------------------------------------------------------------------------------------------

    struct SystemPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_renderingthreads;
        ISpinnerControl*        m_spinner_renderingthreads;
        ICustEdit*              m_text_render_stamp;
        AppleseedRenderer*      m_renderer;
        OutputPanel*            m_output_panel;

        SystemPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings,
            AppleseedRenderer*  renderer,
            OutputPanel*        output_panel)
          : m_rend_params(rend_params)
          , m_settings(settings)
          , m_renderer(renderer)
          , m_output_panel(output_panel)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_SYSTEM),
                    &dialog_proc_entry,
                    L"System",
                    reinterpret_cast<LPARAM>(this));
        }

        ~SystemPanel() override
        {
            ReleaseISpinner(m_spinner_renderingthreads);
            ReleaseICustEdit(m_text_renderingthreads);
            ReleaseICustEdit(m_text_render_stamp);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        void init(HWND hwnd) override
        {
            m_text_renderingthreads = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_RENDERINGTHREADS));
            m_spinner_renderingthreads = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_RENDERINGTHREADS));
            m_spinner_renderingthreads->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_RENDERINGTHREADS), EDITTYPE_INT);
            m_spinner_renderingthreads->SetLimits(-255, 256, FALSE);
            m_spinner_renderingthreads->SetResetValue(RendererSettings::defaults().m_rendering_threads);
            m_spinner_renderingthreads->SetValue(m_settings.m_rendering_threads, FALSE);

            static const wchar_t* LogComboItems[] = { L"Always", L"Never", L"On Error" };
            for (size_t i = 0; i < 3; i++)
                SendMessage(GetDlgItem(hwnd, IDC_COMBO_LOG), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(LogComboItems[i]));
            SendMessage(GetDlgItem(hwnd, IDC_COMBO_LOG), CB_SETCURSEL, static_cast<int>(m_settings.m_log_open_mode), 0);

            CheckDlgButton(hwnd, IDC_CHECK_LOW_PRIORITY_MODE, m_settings.m_low_priority_mode ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK_USE_MAX_PROCEDURAL_MAPS, m_settings.m_use_max_procedural_maps ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK_LOG_MATERIAL_EDITOR, m_settings.m_log_material_editor_messages ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECK_RENDER_STAMP, m_settings.m_enable_render_stamp? BST_CHECKED : BST_UNCHECKED);

            m_text_render_stamp = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_RENDER_STAMP));
            m_text_render_stamp->SetText(m_settings.m_render_stamp_format);

            enable_disable_controls();
        }

        void enable_disable_controls()
        {
            m_text_render_stamp->Enable(m_settings.m_enable_render_stamp);
            m_output_panel->enable_disable_controls(m_settings.m_use_max_procedural_maps);
        }

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case WM_CUSTEDIT_ENTER:
                switch (LOWORD(wparam))
                {
                  case IDC_TEXT_RENDER_STAMP:
                    m_text_render_stamp->GetText(m_settings.m_render_stamp_format);
                    return TRUE;

                  default:
                    return FALSE;
                }

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_CHECK_LOW_PRIORITY_MODE:
                    m_settings.m_low_priority_mode = IsDlgButtonChecked(hwnd, IDC_CHECK_LOW_PRIORITY_MODE) == BST_CHECKED;
                    return TRUE;

                  case IDC_CHECK_USE_MAX_PROCEDURAL_MAPS:
                    m_settings.m_use_max_procedural_maps = IsDlgButtonChecked(hwnd, IDC_CHECK_USE_MAX_PROCEDURAL_MAPS) == BST_CHECKED;
                    m_output_panel->enable_disable_controls(m_settings.m_use_max_procedural_maps);
                    return TRUE;

                  case IDC_CHECK_LOG_MATERIAL_EDITOR:
                    m_settings.m_log_material_editor_messages = IsDlgButtonChecked(hwnd, IDC_CHECK_LOG_MATERIAL_EDITOR) == BST_CHECKED;
                    save_system_setting(L"LogMaterialEditorMessages", m_settings.m_log_material_editor_messages);
                    return TRUE;

                  case IDC_BUTTON_LOG:
                    m_renderer->show_last_session_log();
                    return TRUE;
                  
                  case IDC_COMBO_LOG:
                    if (HIWORD(wparam) == CBN_SELCHANGE)
                    {
                        LRESULT sel_mode = SendDlgItemMessage(hwnd, IDC_COMBO_LOG, CB_GETCURSEL, 0, 0);
                        if (sel_mode != CB_ERR)
                        {
                            m_settings.m_log_open_mode = static_cast<DialogLogTarget::OpenMode>(sel_mode);
                            save_system_setting(L"LogOpenMode", static_cast<int>(m_settings.m_log_open_mode));
                        }
                    }
                    break;
                  
                  case IDC_CHECK_RENDER_STAMP:
                    m_settings.m_enable_render_stamp = IsDlgButtonChecked(hwnd, IDC_CHECK_RENDER_STAMP) == BST_CHECKED;
                    enable_disable_controls();
                    return TRUE;

                  default:
                    return FALSE;
                }

              case CC_SPINNER_CHANGE:
                switch (LOWORD(wparam))
                {
                  case IDC_SPINNER_RENDERINGTHREADS:
                    m_settings.m_rendering_threads = m_spinner_renderingthreads->GetIVal();
                    return TRUE;

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }
    };
}

struct AppleseedRendererParamDlg::Impl
{
    RendererSettings                    m_temp_settings;    // settings the dialog will modify
    RendererSettings&                   m_settings;         // output (accepted) settings

    std::auto_ptr<AboutPanel>           m_about_panel;
    std::auto_ptr<ImageSamplingPanel>   m_image_sampling_panel;
    std::auto_ptr<LightingPanel>        m_lighting_panel;
    std::auto_ptr<OutputPanel>          m_output_panel;
    std::auto_ptr<SystemPanel>          m_system_panel;

    Impl(
        IRendParams*        rend_params,
        BOOL                in_progress,
        RendererSettings&   settings,
        AppleseedRenderer*  renderer)
      : m_temp_settings(settings)
      , m_settings(settings)
    {
        if (!in_progress)
        {
            m_about_panel.reset(new AboutPanel(rend_params));
            m_image_sampling_panel.reset(new ImageSamplingPanel(rend_params, m_temp_settings));
            m_lighting_panel.reset(new LightingPanel(rend_params, m_temp_settings));
            m_output_panel.reset(new OutputPanel(rend_params, m_temp_settings));
            m_system_panel.reset(new SystemPanel(rend_params, m_temp_settings, renderer, m_output_panel.get()));
        }
    }
};

AppleseedRendererParamDlg::AppleseedRendererParamDlg(
    IRendParams*            rend_params,
    BOOL                    in_progress,
    RendererSettings&       settings,
    AppleseedRenderer*      renderer)
  : impl(new Impl(rend_params, in_progress, settings, renderer))
{
}

AppleseedRendererParamDlg::~AppleseedRendererParamDlg()
{
    delete impl;
}

void AppleseedRendererParamDlg::DeleteThis()
{
    delete this;
}

void AppleseedRendererParamDlg::AcceptParams()
{
    impl->m_settings = impl->m_temp_settings;
}
