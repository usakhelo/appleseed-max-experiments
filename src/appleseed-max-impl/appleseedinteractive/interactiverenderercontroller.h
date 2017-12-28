
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

// appleseed.renderer headers.
#include "renderer/api/rendering.h"

// appleseed-max headers.
#include "appleseedinteractive/appleseedinteractive.h"

// Standard headers.
#include <memory>

class INode;

class CameraUpdater
{
  public:
    CameraUpdater(AppleseedInteractiveRender* renderer, INode* camera)
        : m_renderer(renderer)
        , m_camera(camera)
    {}

    void update()
    {
        m_renderer->update_camera_parameters(m_camera);
    }

  private:
    AppleseedInteractiveRender* m_renderer;
    INode* m_camera;
};

class InteractiveRendererController
  : public renderer::DefaultRendererController
{
  public:
    InteractiveRendererController();

    virtual void on_rendering_begin() override;
    virtual Status get_status() const override;

    void set_status(const Status status);

    void schedule_udpate(std::unique_ptr<CameraUpdater> updater);

  private:
    std::unique_ptr<CameraUpdater> m_updater;
    Status m_status;
};
