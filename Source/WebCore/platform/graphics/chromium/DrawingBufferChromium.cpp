/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "DrawingBuffer.h"

#include "CanvasRenderingContext.h"
#include "Extensions3DChromium.h"
#include "GraphicsContext3D.h"
#include "WebGLLayerChromium.h"
#include "cc/CCProxy.h"
#include <algorithm>

using namespace std;

namespace WebCore {

static unsigned generateColorTexture(GraphicsContext3D* context, const IntSize& size)
{
    unsigned offscreenColorTexture = context->createTexture();
    if (!offscreenColorTexture)
        return 0;

    context->bindTexture(GraphicsContext3D::TEXTURE_2D, offscreenColorTexture);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texImage2DResourceSafe(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, size.width(), size.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE);

    return offscreenColorTexture;
}

DrawingBuffer::DrawingBuffer(GraphicsContext3D* context,
                             const IntSize& size,
                             bool multisampleExtensionSupported,
                             bool packedDepthStencilExtensionSupported,
                             PreserveDrawingBuffer preserve,
                             AlphaRequirement alpha)
    : m_preserveDrawingBuffer(preserve)
    , m_alpha(alpha)
    , m_scissorEnabled(false)
    , m_texture2DBinding(0)
    , m_activeTextureUnit(GraphicsContext3D::TEXTURE0)
    , m_context(context)
    , m_size(-1, -1)
    , m_multisampleExtensionSupported(multisampleExtensionSupported)
    , m_packedDepthStencilExtensionSupported(packedDepthStencilExtensionSupported)
    , m_fbo(0)
    , m_colorBuffer(0)
    , m_frontColorBuffer(0)
    , m_separateFrontTexture(m_preserveDrawingBuffer == Preserve || CCProxy::implThread())
    , m_depthStencilBuffer(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_multisampleFBO(0)
    , m_multisampleColorBuffer(0)
{
    initialize(size);
}

DrawingBuffer::~DrawingBuffer()
{
    if (m_platformLayer)
        m_platformLayer->setDrawingBuffer(0);

    if (!m_context)
        return;

    clear();
}

void DrawingBuffer::initialize(const IntSize& size)
{
    m_fbo = m_context->createFramebuffer();

    if (m_separateFrontTexture)
        m_frontColorBuffer = generateColorTexture(m_context.get(), size);

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_colorBuffer = generateColorTexture(m_context.get(), size);
    m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_colorBuffer, 0);
    createSecondaryBuffers();
    if (!reset(size)) {
        m_context.clear();
        return;
    }
}

#if USE(ACCELERATED_COMPOSITING)
void DrawingBuffer::prepareBackBuffer()
{
    m_context->makeContextCurrent();

    if (multisample())
        commit();

    if (m_preserveDrawingBuffer == Discard && m_separateFrontTexture) {
        swap(m_frontColorBuffer, m_colorBuffer);
        // It appears safe to overwrite the context's framebuffer binding in the Discard case since there will always be a
        // WebGLRenderingContext::clearIfComposited() call made before the next draw call which restores the framebuffer binding.
        // If this stops being true at some point, we should track the current framebuffer binding in the DrawingBuffer and restore
        // it after attaching the new back buffer here.
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
        m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_colorBuffer, 0);
    }

    if (multisample())
        bind();
}

bool DrawingBuffer::requiresCopyFromBackToFrontBuffer() const
{
    return m_separateFrontTexture && m_preserveDrawingBuffer == Preserve;
}

unsigned DrawingBuffer::frontColorBuffer() const
{
    return m_separateFrontTexture ? m_frontColorBuffer : m_colorBuffer;
}
#endif

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* DrawingBuffer::platformLayer()
{
    if (!m_platformLayer) {
        m_platformLayer = WebGLLayerChromium::create();
        m_platformLayer->setDrawingBuffer(this);
        m_platformLayer->setOpaque(m_alpha == Opaque);
    }

    return m_platformLayer.get();
}
#endif

Platform3DObject DrawingBuffer::framebuffer() const
{
    return m_fbo;
}

#if USE(ACCELERATED_COMPOSITING)
void DrawingBuffer::paintCompositedResultsToCanvas(CanvasRenderingContext* context)
{
    if (m_platformLayer)
        m_platformLayer->paintRenderedResultsToCanvas(context->canvas()->buffer());
}
#endif

}
