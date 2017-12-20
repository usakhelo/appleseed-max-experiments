
#pragma once

// appleseed.foundation headers.
#include "foundation/utility/log.h"

// Standard headers.
#include <cstddef>
#include <string>
#include <vector>

typedef std::vector<std::string> StringVec;
typedef std::pair<foundation::LogMessage::Category, StringVec> Message_Pair;

enum class LogOpenMode
{
    Always,
    Never,
    Errors
};

class WindowLogTarget
    : public foundation::ILogTarget
{
public:
    WindowLogTarget(
        std::vector<Message_Pair>*  session_messages,
        LogOpenMode                 open_mode);

    virtual void release() override;

    virtual void write(
        const foundation::LogMessage::Category  category,
        const char*                             file,
        const size_t                            line,
        const char*                             header,
        const char*                             message) override;

    void open_log_window();
    void show_saved_messages();

private:
    std::vector<Message_Pair>*      m_saved_messages;
    LogOpenMode                     m_open_mode;
};
