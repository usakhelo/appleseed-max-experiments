
#pragma once

// appleseed.foundation headers.
#include "foundation/utility/log.h"

// Standard headers.
#include <cstddef>

class WindowLogTarget
    : public foundation::ILogTarget
{
public:
    virtual void release() override;

    virtual void write(
        const foundation::LogMessage::Category  category,
        const char*                             file,
        const size_t                            line,
        const char*                             header,
        const char*                             message) override;

    void open_log_window();
};
