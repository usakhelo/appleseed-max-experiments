
// Interface header.
#include "logwindow.h"

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

namespace
{
    typedef std::vector<std::string> StringVec;
    typedef std::pair<asf::LogMessage::Category, StringVec> Message_Pair;

    boost::mutex                g_message_queue_mutex;
    std::vector<Message_Pair>   g_message_queue;
    HWND                        g_log_window = 0;

    void append_text(HWND edit_box, const wchar_t* text)
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

    static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
          case WM_INITDIALOG:
            {
                WindowLogTarget* log_window = reinterpret_cast<WindowLogTarget*>(lparam);
                DLSetWindowLongPtr(hwnd, log_window);
                //log_window->init(hwnd);

                HFONT hFont = CreateFont(20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas"));

                HFONT hFont1 = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
                if (hFont)
                    SendDlgItemMessage(g_log_window, IDC_EDIT_LOG, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            }
            break;
          case WM_CLOSE:
          case WM_DESTROY:
            {
                //LogWindow* dlg = DLGetWindowLongPtr<LogWindow*>(hWnd);
                GetCOREInterface14()->UnRegisterModelessRenderWindow(g_log_window);
                g_log_window = 0;
                DestroyWindow(hwnd);
            }
            break;
          
          case WM_SIZE:
              MoveWindow(GetDlgItem(g_log_window, IDC_EDIT_LOG),
                  0,
                  0,
                  LOWORD(lparam),
                  HIWORD(lparam),
                  true);

          default:
            return FALSE;
        }
        return TRUE;
    }

    void emit_message(const asf::LogMessage::Category type, const StringVec& lines)
    {
        for (const auto& line : lines)
        {
            if (g_log_window)
            {
                append_text(GetDlgItem(g_log_window, IDC_EDIT_LOG), utf8_to_wide(line + "\n").c_str());
            }
        }
    }

    void push_message(const asf::LogMessage::Category type, const StringVec& lines)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);
        g_message_queue.push_back(Message_Pair(type, lines));
    }

    void emit_pending_messages()
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        for (const auto& message : g_message_queue)
            emit_message(message.first, message.second);

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

void WindowLogTarget::release()
{
    delete this;
}

void WindowLogTarget::write(
    const asf::LogMessage::Category category,
    const char*                     file,
    const size_t                    line,
    const char*                     header,
    const char*                     message)
{
    // Open base on type and render settings
    if (true)
        open_log_window();

    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    //if (is_main_thread())
    //    emit_message(type, lines);
    //else
    //{
        push_message(category, lines);
        PostMessage(
            GetCOREInterface()->GetMAXHWnd(),
            WM_TRIGGER_CALLBACK,
            reinterpret_cast<WPARAM>(emit_pending_messages),
            0);
    //}
}

void WindowLogTarget::open_log_window()
{
    if (!g_log_window)
    {
        g_log_window = CreateDialogParam(
            g_module,
            MAKEINTRESOURCE(IDD_DIALOG_LOG),
            GetCOREInterface()->GetMAXHWnd(), 
            dialog_proc,
            (LPARAM)this);

        GetCOREInterface14()->RegisterModelessRenderWindow(g_log_window);
    }
}
