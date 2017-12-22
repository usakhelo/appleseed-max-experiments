
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
#include <max.h>

// Standard headers.
#include <Richedit.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    boost::mutex                g_message_queue_mutex;
    std::vector<MessagePair>    g_message_queue;
    HWND                        g_log_window = 0;

    static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
          case WM_INITDIALOG:
            break;
          case WM_CLOSE:
          case WM_DESTROY:
            if (g_log_window != 0)
            {
                GetCOREInterface14()->UnRegisterModelessRenderWindow(g_log_window);
                g_log_window = 0;
                DestroyWindow(hwnd);
            }
            break;
          
          case WM_SIZE:
          {
              HWND edit_box = GetDlgItem(hwnd, IDC_EDIT_LOG);
              MoveWindow(
                  edit_box,
                  0,
                  0,
                  LOWORD(lparam),
                  HIWORD(lparam),
                  true);

              SendMessage(edit_box, EM_LINESCROLL, 0, 1);
              return TRUE;
          }
              break;

          default:
            return FALSE;
        }
        return TRUE;
    }

    void append_text(HWND edit_box, const wchar_t* text, COLORREF color)
    {
        const int text_length = GetWindowTextLength(edit_box);
        SendMessage(edit_box, EM_SETSEL, text_length, text_length);

        CHARFORMAT char_format;
        SendMessage(edit_box, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));
        char_format.cbSize = sizeof(char_format);
        char_format.dwMask = CFM_COLOR | CFM_FACE;
        char_format.dwEffects = 0;
        char_format.crTextColor = color;
        wcscpy(reinterpret_cast<WCHAR*>(char_format.szFaceName), L"Consolas");
        SendMessage(edit_box, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));

        SendMessage(edit_box, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
        SendMessage(edit_box, EM_SCROLLCARET, 0, 0);
    }

    void print_message(const MessageType type, const StringVec& lines)
    {
        for (const auto& line : lines)
        {
            if (g_log_window)
            {
                COLORREF message_color;
                switch (type)
                {
                  case MessageType::Error:
                  case MessageType::Fatal:
                    message_color = RGB(220, 10, 10);
                    break;
                  case MessageType::Warning:
                    message_color = RGB(220, 100, 10);
                    break;
                  case MessageType::Debug:
                    message_color = RGB(10, 150, 10);
                    break;
                  default:
                    message_color = RGB(10, 10, 10);
                    break;
                }
                append_text(GetDlgItem(g_log_window, IDC_EDIT_LOG), utf8_to_wide(line + "\n").c_str(), message_color);
            }
        }
    }

    void push_message(MessagePair message)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);
        g_message_queue.push_back(message);
    }

    // Runs in UI thread.
    void emit_pending_messages()
    {
        if (!g_log_window)
        {
            g_log_window = CreateDialogParam(
                g_module,
                MAKEINTRESOURCE(IDD_DIALOG_LOG),
                GetCOREInterface()->GetMAXHWnd(),
                dialog_proc,
                NULL);

            GetCOREInterface14()->RegisterModelessRenderWindow(g_log_window);
        }

        if (g_log_window != NULL)
        {
            boost::mutex::scoped_lock lock(g_message_queue_mutex);

            for (const auto& message : g_message_queue)
                print_message(message.first, message.second);

            g_message_queue.clear();
        }
    }

    // Runs in UI thread.
    void emit_saved_messages()
    {
        if (g_log_window)
            SetDlgItemText(g_log_window, IDC_EDIT_LOG, L"");

        emit_pending_messages();
    }

    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;
}

WindowLogTarget::WindowLogTarget(LogDialogMode open_mode)
  : m_log_mode(open_mode)
{
    m_session_messages.clear();
    g_message_queue.clear();
    asr::global_logger().add_target(this);
}

void WindowLogTarget::release()
{
    asr::global_logger().remove_target(this);
    delete this;
}

void WindowLogTarget::write(
    const MessageType       category,
    const char*             file,
    const size_t            line,
    const char*             header,
    const char*             message)
{
    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    m_session_messages.push_back(MessagePair(category, lines));

    push_message(MessagePair(category, lines));

    if (g_log_window)
        print_to_window();
    else
    {
        switch (category)
        {
          case asf::LogMessage::Category::Error:
          case asf::LogMessage::Category::Fatal:
          case asf::LogMessage::Category::Warning:
            if (m_log_mode != LogDialogMode::Never)
                print_to_window();
            break;
          case asf::LogMessage::Category::Debug:
          case asf::LogMessage::Category::Info:
            if (m_log_mode == LogDialogMode::Always)
                print_to_window();
            break;
        }
    }
}

void WindowLogTarget::show_last_session_messages()
{
    if (g_log_window)
        return;

    boost::mutex::scoped_lock lock(g_message_queue_mutex);
    g_message_queue.clear();
    for (const auto& message : m_session_messages)
        g_message_queue.push_back(message);

    PostMessage(
        GetCOREInterface()->GetMAXHWnd(),
        WM_TRIGGER_CALLBACK,
        reinterpret_cast<WPARAM>(emit_saved_messages),
        0);
}

void WindowLogTarget::print_to_window()
{
    PostMessage(
        GetCOREInterface()->GetMAXHWnd(),
        WM_TRIGGER_CALLBACK,
        reinterpret_cast<WPARAM>(emit_pending_messages),
        0);
}
