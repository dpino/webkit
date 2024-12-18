/*
 * Copyright (C) 2016 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AcceleratedSurface.h"

#include "WebPage.h"
#include <WebCore/PlatformDisplay.h>
#include <wtf/TZoneMallocInlines.h>

#if USE(WPE_RENDERER)
#include "AcceleratedSurfaceLibWPE.h"
#endif

#if (PLATFORM(GTK) || (PLATFORM(WPE) && ENABLE(WPE_PLATFORM)))
#include "AcceleratedSurfaceDMABuf.h"
#endif

namespace WebKit {
using namespace WebCore;

#define STRINGIFY(...) #__VA_ARGS__

static const char* vertexFullscreenQuadShader =
    STRINGIFY(
        precision highp float;

        attribute vec2 position;

        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    );

static const char* fragmentFullscreenQuadShader =
    STRINGIFY(
        precision highp float;

        void main()
        {
            gl_FragColor = vec4(0.0,0.0,0.0,0.0);
        }
    );

bool AcceleratedSurface::m_force_shader_clear;

WTF_MAKE_TZONE_ALLOCATED_IMPL(AcceleratedSurface);

std::unique_ptr<AcceleratedSurface> AcceleratedSurface::create(ThreadedCompositor& compositor, WebPage& webPage, Function<void()>&& frameCompleteHandler)
{
#if (PLATFORM(GTK) || (PLATFORM(WPE) && ENABLE(WPE_PLATFORM)))
#if USE(GBM)
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::GBM)
        return AcceleratedSurfaceDMABuf::create(compositor, webPage, WTFMove(frameCompleteHandler));
#endif
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::Surfaceless)
        return AcceleratedSurfaceDMABuf::create(compositor, webPage, WTFMove(frameCompleteHandler));
#endif
#if USE(WPE_RENDERER)
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::WPE)
        return AcceleratedSurfaceLibWPE::create(webPage, WTFMove(frameCompleteHandler));
#endif
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

AcceleratedSurface::AcceleratedSurface(WebPage& webPage, Function<void()>&& frameCompleteHandler)
    : m_webPage(webPage)
    , m_frameCompleteHandler(WTFMove(frameCompleteHandler))
    , m_isOpaque(!webPage.backgroundColor().has_value() || webPage.backgroundColor()->isOpaque())
{
}

bool AcceleratedSurface::resize(const IntSize& size)
{
    if (m_size == size)
        return false;

    m_size = size;
    return true;
}

bool AcceleratedSurface::backgroundColorDidChange()
{
    const auto& color = m_webPage->backgroundColor();
    auto isOpaque = !color.has_value() || color->isOpaque();
    if (m_isOpaque == isOpaque)
        return false;

    m_isOpaque = isOpaque;
    return true;
}

void AcceleratedSurface::clearIfNeeded()
{
    if (m_isOpaque)
        return;

    static bool stop_glClear = false;
    static std::once_flag glClear_flag;
    std::call_once(glClear_flag, []{
        const char* var = getenv("WEBKIT_STOP_CLEAR_COMPOSITING");
        if (var && !strcmp(var, "1"))
            stop_glClear = true;
    });

    if (!stop_glClear) {
        if (m_force_shader_clear) {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ZERO);
            glUseProgram(m_clearProgram);
            glBindVertexArray(m_vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            glUseProgram(0);
        } else {
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}

void AcceleratedSurface::didCreateGLContext()
{
    static std::once_flag shader_clear_flag;
    std::call_once(shader_clear_flag, checkClearShader);

    if (m_force_shader_clear) {
        m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_vertexShader, 1, &vertexFullscreenQuadShader, NULL);
        glCompileShader(m_vertexShader);

        m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_fragmentShader, 1, &fragmentFullscreenQuadShader, NULL);
        glCompileShader(m_fragmentShader);

        m_clearProgram = glCreateProgram();
        glAttachShader(m_clearProgram, m_vertexShader);
        glAttachShader(m_clearProgram, m_fragmentShader);
        glLinkProgram(m_clearProgram);

        float quad[] =
        {
            -1.0f,  1.0f,
            -1.0f, -1.0f,
            1.0f,  1.0f,
            1.0f, -1.0f
        };

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        GLint posAttrib = glGetAttribLocation(m_clearProgram, "position");
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(posAttrib);

        glBindVertexArray(0);
    }
}

void AcceleratedSurface::willDestroyGLContext()
{
    if (m_force_shader_clear) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDetachShader(m_clearProgram, m_vertexShader);
        glDeleteShader(m_vertexShader);
        glDetachShader(m_clearProgram, m_fragmentShader);
        glDeleteShader(m_fragmentShader);
        glDeleteProgram(m_clearProgram);
    }
}

void AcceleratedSurface::checkClearShader()
{
    const char* var = getenv("WEBKIT_FORCE_SHADER_CLEAR_COMPOSITING");
    if (var && !strcmp(var, "1")) {
        m_force_shader_clear = true;
    } else {
        m_force_shader_clear = false;
    }
}

void AcceleratedSurface::frameComplete() const
{
    m_frameCompleteHandler();
}

} // namespace WebKit
