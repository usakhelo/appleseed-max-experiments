
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

#include <Richedit.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    boost::mutex                g_message_queue_mutex;
    std::vector<Message_Pair>   g_message_queue;
    HWND                        g_log_window = 0;

    void append_text(HWND edit_box, const wchar_t* text, COLORREF color)
    {
        int outLength = GetWindowTextLength(edit_box);
        SendMessage(edit_box, EM_SETSEL, outLength, outLength);

        CHARFORMAT char_format;
        SendMessage(edit_box, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));
        char_format.cbSize = sizeof(char_format);
        char_format.dwMask = CFM_COLOR;
        char_format.dwEffects = 0;
        char_format.crTextColor = color;
        SendMessage(edit_box, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));

        SendMessage(edit_box, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));
        SendMessage(edit_box, EM_LINESCROLL, 0, 1);
    }

    static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
          case WM_INITDIALOG:
            {
                WindowLogTarget* log_window = reinterpret_cast<WindowLogTarget*>(lparam);
                DLSetWindowLongPtr(hwnd, log_window);
            }
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

    void print_message(const asf::LogMessage::Category type, const StringVec& lines)
    {
        for (const auto& line : lines)
        {
            if (g_log_window)
            {
                typedef asf::LogMessage::Category message_type;
                COLORREF message_color;
                switch (type)
                {
                  case message_type::Error:
                  case message_type::Fatal:
                    message_color = RGB(220, 10, 10);
                    break;
                  case message_type::Warning:
                    message_color = RGB(220, 100, 10);
                    break;
                  case message_type::Debug:
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

    void push_message(Message_Pair message)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);
        g_message_queue.push_back(message);
    }

    void emit_pending_messages()
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        for (const auto& message : g_message_queue)
            print_message(message.first, message.second);

        g_message_queue.clear();
    }


    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;
}

WindowLogTarget::WindowLogTarget(
    std::vector<Message_Pair>*  messages,
    LogOpenMode                 open_mode)
    : m_saved_messages(messages)
    , m_open_mode(open_mode)
{
    asr::global_logger().add_target(this);
}

void WindowLogTarget::release()
{
    asr::global_logger().remove_target(this);
    delete this;
}

void WindowLogTarget::write(
    const asf::LogMessage::Category category,
    const char*                     file,
    const size_t                    line,
    const char*                     header,
    const char*                     message)
{
    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    if (m_saved_messages != nullptr)
        m_saved_messages->push_back(Message_Pair(category, lines));

    push_message(Message_Pair(category, lines));

    switch (category)
    {
      case asf::LogMessage::Category::Error:
      case asf::LogMessage::Category::Fatal:
      case asf::LogMessage::Category::Warning:
        if (m_open_mode != LogOpenMode::Never)
            open_log_window();
        break;
      case asf::LogMessage::Category::Info:
          if (m_open_mode == LogOpenMode::Always)
            open_log_window();
        break;
    }
}

void WindowLogTarget::show_saved_messages()
{
    if (m_saved_messages != nullptr)
    {
        for (const auto& message : *m_saved_messages)
            g_message_queue.push_back(message);

        if (g_log_window)
        {
            SetDlgItemText(g_log_window, IDC_EDIT_LOG, L"");
        }

        open_log_window();
    }
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
    
    PostMessage(
        GetCOREInterface()->GetMAXHWnd(),
        WM_TRIGGER_CALLBACK,
        reinterpret_cast<WPARAM>(emit_pending_messages),
        0);
}
