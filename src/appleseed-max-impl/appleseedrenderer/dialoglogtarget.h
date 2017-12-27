
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Sergo Pogosyan, The appleseedhq Organization
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

#pragma once

// appleseed.foundation headers.
#include "foundation/utility/log.h"

// Standard headers.
#include <cstddef>
#include <string>
#include <vector>
#include <utility>

typedef std::vector<std::string> StringVec;
typedef foundation::LogMessage::Category MessageType;
typedef std::pair<MessageType, StringVec> MessagePair;

enum class LogDialogMode
{
    // Changing these value WILL break compatibility.
    Always  = 0,
    Never   = 1,
    Errors  = 2
};

class DialogLogTarget
  : public foundation::ILogTarget
{
  public:
    explicit DialogLogTarget(
        const LogDialogMode     open_mode);

    virtual void release() override;

    virtual void write(
        const MessageType       category,
        const char*             file,
        const size_t            line,
        const char*             header,
        const char*             message) override;

    void show_last_session_messages();

  private:
    std::vector<MessagePair>    m_session_messages;
    LogDialogMode               m_log_mode;

    void print_to_dialog();
};