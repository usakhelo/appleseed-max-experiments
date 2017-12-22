
#pragma once

// appleseed.foundation headers.
#include "foundation/utility/log.h"

// Standard headers.
#include <cstddef>
#include <string>
#include <vector>

typedef std::vector<std::string> StringVec;
typedef foundation::LogMessage::Category MessageType;
typedef std::pair<MessageType, StringVec> MessagePair;

enum class LogDialogMode
{
    Always,
    Never,
    Errors
};

class WindowLogTarget
    : public foundation::ILogTarget
{
public:
    WindowLogTarget(LogDialogMode   open_mode);

    virtual void release() override;

    virtual void write(
        const MessageType           category,
        const char*                 file,
        const size_t                line,
        const char*                 header,
        const char*                 message) override;

    void print_to_window();
    void show_last_session_messages();

private:
    std::vector<MessagePair>        m_session_messages;
    LogDialogMode                   m_log_mode;
};
