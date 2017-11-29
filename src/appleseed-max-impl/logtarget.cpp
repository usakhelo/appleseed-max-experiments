
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016-2017 Francois Beaune, The appleseedhq Organization
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
#include "logtarget.h"

// appleseed-max headers.
#include "appleseedrenderer/resource.h"
#include "main.h"
#include "utilities.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/string.h"

// Boost headers.
#include "boost/thread/locks.hpp"
#include "boost/thread/mutex.hpp"

// 3ds Max headers.
#include <3dsmaxdlport.h>
#include <log.h>
#include <max.h>

// Standard headers.
#include <string>
#include <vector>

namespace asf = foundation;

struct LogTarget::LogWindow
{
    HWND edit_box;
    HWND m_log_window;

    LogWindow()
    {
        m_log_window = CreateDialogParam(
            g_module,
            MAKEINTRESOURCE(IDD_DIALOG_LOG),
            GetCOREInterface()->GetMAXHWnd(),
            dialog_proc,
            (LPARAM)this);

        edit_box = GetDlgItem(m_log_window, IDC_EDIT_LOG);
    }

    ~LogWindow()
    {
        DestroyWindow(m_log_window);
    }

    void append_text(const wchar_t* text)
    {
        // get the current selection
        DWORD start_pos, end_pos;
        SendMessage(edit_box, EM_GETSEL, reinterpret_cast<WPARAM>(&start_pos), reinterpret_cast<LPARAM>(&end_pos));

        // move the caret to the end of the text
        int outLength = GetWindowTextLength(edit_box);
        SendMessage(edit_box, EM_SETSEL, outLength, outLength);

        // insert the text at the new caret position
        SendMessage(edit_box, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));

        // restore the previous selection
        SendMessage(edit_box, EM_SETSEL, start_pos, end_pos);
    }

    static INT_PTR CALLBACK dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            CenterWindow(hWnd, GetParent(hWnd));
            ShowWindow(hWnd, SW_SHOW);
            break;

        case WM_CLOSE:
        {
            //LogWindow* dlg = DLGetWindowLongPtr<LogWindow*>(hWnd);
            //dlg->m_log_window = 0;
            DestroyWindow(hWnd);
        }
        break;

        default:
            return FALSE;
        }
        return TRUE;
    }
};

namespace
{
    typedef std::vector<std::string> StringVec;

    struct Message
    {
        DWORD               m_type;
        StringVec           m_lines;
    };

    boost::mutex            g_message_queue_mutex;
    std::vector<Message>    g_message_queue;
    LogTarget::LogWindow*   g_log_window = nullptr;

    void emit_message(const DWORD type, const StringVec& lines)
    {
        for (const auto& line : lines)
        {
            if (g_log_window)
            {
                g_log_window->append_text(utf8_to_wide(line).c_str());
                g_log_window->append_text(L"\n");
            }

            GetCOREInterface()->Log()->LogEntry(
                type,
                FALSE,
                L"appleseed",
                L"[appleseed] %s",
                utf8_to_wide(line).c_str());
        }
    }

    void push_message(const DWORD type, const StringVec& lines)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        Message message;
        message.m_type = type;
        message.m_lines = lines;
        g_message_queue.push_back(message);
    }

    void emit_pending_messages()
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        for (const auto& message : g_message_queue)
            emit_message(message.m_type, message.m_lines);

        g_message_queue.clear();
    }

    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;

    bool is_main_thread()
    {
        // There does not appear to be a GetCOREInterface15() function in the 3ds Max 2015 SDK.
        Interface15* interface15 =
            reinterpret_cast<Interface15*>(GetCOREInterface14()->GetInterface(Interface15::kInterface15InterfaceID));

        return interface15->GetMainThreadID() == GetCurrentThreadId();
    }
}

void LogTarget::release()
{
    delete m_log_window;
    delete this;
}

void LogTarget::write(
    const asf::LogMessage::Category category,
    const char*                     file,
    const size_t                    line,
    const char*                     header,
    const char*                     message)
{
    DWORD type;
    switch (category)
    {
      case asf::LogMessage::Debug: type = SYSLOG_DEBUG; break;
      case asf::LogMessage::Info: type = SYSLOG_INFO; break;
      case asf::LogMessage::Warning: type = SYSLOG_WARN; break;
      case asf::LogMessage::Error:
      case asf::LogMessage::Fatal:
      default:
        type = SYSLOG_ERROR;
        break;
    }

    // Open base on type and render settings
    if (!g_log_window)
        g_log_window = new LogTarget::LogWindow();

    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    if (is_main_thread())
        emit_message(type, lines);
    else
    {
        push_message(type, lines);
        PostMessage(
            GetCOREInterface()->GetMAXHWnd(),
            WM_TRIGGER_CALLBACK,
            reinterpret_cast<WPARAM>(emit_pending_messages),
            0);
    }
}

void LogTarget::open_log_window()
{
    // Open base on type and render settings
    if (!g_log_window)
        g_log_window = new LogTarget::LogWindow();
}
