/*
 * Copyright (C) 2009-2021 Apple Inc. All rights reserved.
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
#include "WebGLFramebuffer.h"

#if ENABLE(WEBGL)

#include "WebCoreOpaqueRootInlines.h"
#include "WebGLContextGroup.h"
#include "WebGLDrawBuffers.h"
#include "WebGLRenderingContextBase.h"
#include <JavaScriptCore/SlotVisitor.h>
#include <JavaScriptCore/SlotVisitorInlines.h>
#include <wtf/Lock.h>
#include <wtf/Locker.h>

namespace WebCore {

namespace {

    class WebGLRenderbufferAttachment : public WebGLFramebuffer::WebGLAttachment {
    public:
        static Ref<WebGLFramebuffer::WebGLAttachment> create(WebGLRenderbuffer*);

    private:
        WebGLRenderbufferAttachment(WebGLRenderbuffer*);
        WebGLSharedObject* getObject() const override;
        bool isSharedObject(WebGLSharedObject*) const override;
        bool isValid() const override;
        bool isInitialized() const override;
        void setInitialized() override;
        void onDetached(const AbstractLocker&, GraphicsContextGL*) override;
        void attach(GraphicsContextGL*, GCGLenum target, GCGLenum attachment) override;
        void unattach(GraphicsContextGL*, GCGLenum target, GCGLenum attachment) override;
        void addMembersToOpaqueRoots(const AbstractLocker&, JSC::AbstractSlotVisitor&) override;

        WebGLRenderbufferAttachment() { };

        RefPtr<WebGLRenderbuffer> m_renderbuffer;
    };

    Ref<WebGLFramebuffer::WebGLAttachment> WebGLRenderbufferAttachment::create(WebGLRenderbuffer* renderbuffer)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return adoptRef(*new WebGLRenderbufferAttachment(renderbuffer));
    }

    WebGLRenderbufferAttachment::WebGLRenderbufferAttachment(WebGLRenderbuffer* renderbuffer)
        : m_renderbuffer(renderbuffer)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    }


    WebGLSharedObject* WebGLRenderbufferAttachment::getObject() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return m_renderbuffer->object() ? m_renderbuffer.get() : 0;
    }

    bool WebGLRenderbufferAttachment::isSharedObject(WebGLSharedObject* object) const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return object == m_renderbuffer;
    }

    bool WebGLRenderbufferAttachment::isValid() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return m_renderbuffer->object();
    }

    bool WebGLRenderbufferAttachment::isInitialized() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return m_renderbuffer->object() && m_renderbuffer->isInitialized();
    }

    void WebGLRenderbufferAttachment::setInitialized()
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        if (m_renderbuffer->object())
            m_renderbuffer->setInitialized();
    }

    void WebGLRenderbufferAttachment::onDetached(const AbstractLocker& locker, GraphicsContextGL* context)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        m_renderbuffer->onDetached(locker, context);
    }

    void WebGLRenderbufferAttachment::attach(GraphicsContextGL* context, GCGLenum target, GCGLenum attachment)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        PlatformGLObject object = objectOrZero(m_renderbuffer.get());
        context->framebufferRenderbuffer(target, attachment, GraphicsContextGL::RENDERBUFFER, object);
    }

    void WebGLRenderbufferAttachment::unattach(GraphicsContextGL* context, GCGLenum target, GCGLenum attachment)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        context->framebufferRenderbuffer(target, attachment, GraphicsContextGL::RENDERBUFFER, 0);
    }

    void WebGLRenderbufferAttachment::addMembersToOpaqueRoots(const AbstractLocker&, JSC::AbstractSlotVisitor& visitor)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        addWebCoreOpaqueRoot(visitor, m_renderbuffer.get());
    }

    class WebGLTextureAttachment : public WebGLFramebuffer::WebGLAttachment {
    public:
        static Ref<WebGLFramebuffer::WebGLAttachment> create(WebGLTexture*, GCGLenum target, GCGLint level, GCGLint layer);

    private:
        WebGLTextureAttachment(WebGLTexture*, GCGLenum target, GCGLint level, GCGLint layer);
        WebGLSharedObject* getObject() const override;
        bool isSharedObject(WebGLSharedObject*) const override;
        bool isValid() const override;
        bool isInitialized() const override;
        void setInitialized() override;
        void onDetached(const AbstractLocker&, GraphicsContextGL*) override;
        void attach(GraphicsContextGL*, GCGLenum target, GCGLenum attachment) override;
        void unattach(GraphicsContextGL*, GCGLenum target, GCGLenum attachment) override;
        void addMembersToOpaqueRoots(const AbstractLocker&, JSC::AbstractSlotVisitor&) override;

        WebGLTextureAttachment() { };

        RefPtr<WebGLTexture> m_texture;
        GCGLenum m_target;
        GCGLint m_level;
        GCGLint m_layer;
    };

    Ref<WebGLFramebuffer::WebGLAttachment> WebGLTextureAttachment::create(WebGLTexture* texture, GCGLenum target, GCGLint level, GCGLint layer)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return adoptRef(*new WebGLTextureAttachment(texture, target, level, layer));
    }

    WebGLTextureAttachment::WebGLTextureAttachment(WebGLTexture* texture, GCGLenum target, GCGLint level, GCGLint layer)
        : m_texture(texture)
        , m_target(target)
        , m_level(level)
        , m_layer(layer)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    }

    WebGLSharedObject* WebGLTextureAttachment::getObject() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return m_texture->object() ? m_texture.get() : 0;
    }

    bool WebGLTextureAttachment::isSharedObject(WebGLSharedObject* object) const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return object == m_texture;
    }

    bool WebGLTextureAttachment::isValid() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        return m_texture->object();
    }

    bool WebGLTextureAttachment::isInitialized() const
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        // Textures are assumed to be initialized.
        return true;
    }

    void WebGLTextureAttachment::setInitialized()
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        // Textures are assumed to be initialized.
    }

    void WebGLTextureAttachment::onDetached(const AbstractLocker& locker, GraphicsContextGL* context)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        m_texture->onDetached(locker, context);
    }

    void WebGLTextureAttachment::attach(GraphicsContextGL* context, GCGLenum target, GCGLenum attachment)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        PlatformGLObject object = objectOrZero(m_texture.get());
        if (m_target == GraphicsContextGL::TEXTURE_3D || m_target == GraphicsContextGL::TEXTURE_2D_ARRAY)
            context->framebufferTextureLayer(target, attachment, object, m_level, m_layer);
        else
            context->framebufferTexture2D(target, attachment, m_target, object, m_level);
    }

    void WebGLTextureAttachment::unattach(GraphicsContextGL* context, GCGLenum target, GCGLenum attachment)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        // GL_DEPTH_STENCIL_ATTACHMENT attachment is valid in ES3.
        if (m_target == GraphicsContextGL::TEXTURE_3D || m_target == GraphicsContextGL::TEXTURE_2D_ARRAY)
            context->framebufferTextureLayer(target, attachment, 0, m_level, m_layer);
        else
            context->framebufferTexture2D(target, attachment, m_target, 0, m_level);
    }

    void WebGLTextureAttachment::addMembersToOpaqueRoots(const AbstractLocker&, JSC::AbstractSlotVisitor& visitor)
    {
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
        addWebCoreOpaqueRoot(visitor, m_texture.get());
    }

} // anonymous namespace

WebGLFramebuffer::WebGLAttachment::WebGLAttachment() = default;

WebGLFramebuffer::WebGLAttachment::~WebGLAttachment() = default;

Ref<WebGLFramebuffer> WebGLFramebuffer::create(WebGLRenderingContextBase& ctx)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    return adoptRef(*new WebGLFramebuffer(ctx));
}

#if ENABLE(WEBXR)

Ref<WebGLFramebuffer> WebGLFramebuffer::createOpaque(WebGLRenderingContextBase& ctx)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    auto framebuffer = adoptRef(*new WebGLFramebuffer(ctx));
    framebuffer->m_opaque = true;
    return framebuffer;
}

#endif

WebGLFramebuffer::WebGLFramebuffer(WebGLRenderingContextBase& ctx)
    : WebGLContextObject(ctx)
    , m_hasEverBeenBound(false)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    setObject(ctx.graphicsContextGL()->createFramebuffer());
}

WebGLFramebuffer::~WebGLFramebuffer()
{
    if (!context())
        return;

    runDestructor();
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GCGLenum target, GCGLenum attachment, GCGLenum texTarget, WebGLTexture* texture, GCGLint level, GCGLint layer)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    ASSERT(object());
    ASSERT(isBound(target));
    setAttachmentInternal(attachment, texTarget, texture, level, layer);
    if (context()->isWebGL2()) {
        GCGLuint textureID = objectOrZero(texture);
        // texTarget can be 0 if detaching using framebufferTextureLayer.
        ASSERT(texTarget || !textureID);
        switch (texTarget) {
        case 0:
        case GraphicsContextGL::TEXTURE_3D:
        case GraphicsContextGL::TEXTURE_2D_ARRAY:
            context()->graphicsContextGL()->framebufferTextureLayer(target, attachment, textureID, level, layer);
            break;
        default:
            ASSERT(!layer);
            context()->graphicsContextGL()->framebufferTexture2D(target, attachment, texTarget, textureID, level);
            break;
        }
    } else {
        ASSERT(!layer);
        context()->graphicsContextGL()->framebufferTexture2D(target, attachment, texTarget, objectOrZero(texture), level);
    }
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GCGLenum target, GCGLenum attachment, WebGLRenderbuffer* renderbuffer)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    ASSERT(object());
    ASSERT(isBound(target));
    setAttachmentInternal(attachment, renderbuffer);
    context()->graphicsContextGL()->framebufferRenderbuffer(target, attachment, GraphicsContextGL::RENDERBUFFER, objectOrZero(renderbuffer));
}

void WebGLFramebuffer::attach(GCGLenum target, GCGLenum attachment, GCGLenum attachmentPoint)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
#if ASSERT_ENABLED
    ASSERT(isBound(target));
#else
    UNUSED_PARAM(target);
#endif
    RefPtr<WebGLAttachment> attachmentObject = getAttachment(attachment);
    if (attachmentObject)
        attachmentObject->attach(context()->graphicsContextGL(), target, attachmentPoint);
}

WebGLSharedObject* WebGLFramebuffer::getAttachmentObject(GCGLenum attachment) const
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    if (!object())
        return 0;
    RefPtr<WebGLAttachment> attachmentObject = getAttachment(attachment);
    return attachmentObject ? attachmentObject->getObject() : 0;
}

WebGLFramebuffer::WebGLAttachment* WebGLFramebuffer::getAttachment(GCGLenum attachment) const
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    const AttachmentMap::const_iterator it = m_attachments.find(attachment);
    return (it != m_attachments.end()) ? it->value.get() : 0;
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(const AbstractLocker& locker, GCGLenum target, GCGLenum attachment)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    if (!context()) {
        // Context has been deleted - should not be calling this.
        return;
    }

#if ASSERT_ENABLED
    ASSERT(isBound(target));
#else
    UNUSED_PARAM(target);
#endif
    if (!object())
        return;

    RefPtr<WebGLAttachment> attachmentObject = getAttachment(attachment);
    if (attachmentObject) {
        attachmentObject->onDetached(locker, context()->graphicsContextGL());
        if (attachment == GraphicsContextGL::DEPTH_STENCIL_ATTACHMENT && context()->isWebGL2()) {
            m_attachments.remove(GraphicsContextGL::DEPTH_ATTACHMENT);
            m_attachments.remove(GraphicsContextGL::STENCIL_ATTACHMENT);
        }
        m_attachments.remove(attachment);
        drawBuffersIfNecessary(false);
    }
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(const AbstractLocker& locker, GCGLenum target, WebGLSharedObject* attachment)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    ASSERT(isBound(target));
    if (!object())
        return;
    if (!attachment)
        return;

    bool checkMore = true;
    do {
        checkMore = false;
        for (auto& entry : m_attachments) {
            RefPtr<WebGLAttachment> attachmentObject = entry.value.get();
            if (attachmentObject->isSharedObject(attachment)) {
                GCGLenum attachmentType = entry.key;
                attachmentObject->unattach(context()->graphicsContextGL(), target, attachmentType);
                removeAttachmentFromBoundFramebuffer(locker, target, attachmentType);
                checkMore = true;
                break;
            }
        }
    } while (checkMore);
}


bool WebGLFramebuffer::hasStencilBuffer() const
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    RefPtr<WebGLAttachment> attachment = getAttachment(GraphicsContextGL::STENCIL_ATTACHMENT);
    if (!attachment)
        attachment = getAttachment(GraphicsContextGL::DEPTH_STENCIL_ATTACHMENT);
    return attachment && attachment->isValid();
}

void WebGLFramebuffer::deleteObjectImpl(const AbstractLocker& locker, GraphicsContextGL* context3d, PlatformGLObject object)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    for (auto& attachment : m_attachments.values())
        attachment->onDetached(locker, context3d);

    context3d->deleteFramebuffer(object);
}

bool WebGLFramebuffer::isBound(GCGLenum target) const
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    return (context()->getFramebufferBinding(target) == this);
}

void WebGLFramebuffer::drawBuffers(const Vector<GCGLenum>& bufs)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    m_drawBuffers = bufs;
    m_filteredDrawBuffers.resize(m_drawBuffers.size());
    for (auto& buffer : m_filteredDrawBuffers)
        buffer = GraphicsContextGL::NONE;
    drawBuffersIfNecessary(true);
}

void WebGLFramebuffer::drawBuffersIfNecessary(bool force)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    if (context()->isWebGL2() || context()->m_webglDrawBuffers) {
        bool reset = force;
        // This filtering works around graphics driver bugs on macOS.
        for (size_t i = 0; i < m_drawBuffers.size(); ++i) {
            if (m_drawBuffers[i] != GraphicsContextGL::NONE && getAttachment(m_drawBuffers[i])) {
                if (m_filteredDrawBuffers[i] != m_drawBuffers[i]) {
                    m_filteredDrawBuffers[i] = m_drawBuffers[i];
                    reset = true;
                }
            } else {
                if (m_filteredDrawBuffers[i] != GraphicsContextGL::NONE) {
                    m_filteredDrawBuffers[i] = GraphicsContextGL::NONE;
                    reset = true;
                }
            }
        }
        if (reset) {
            if (context()->isWebGL2())
                context()->graphicsContextGL()->drawBuffers(m_filteredDrawBuffers);
            else
                context()->graphicsContextGL()->drawBuffersEXT(m_filteredDrawBuffers);
        }
    }
}

GCGLenum WebGLFramebuffer::getDrawBuffer(GCGLenum drawBuffer)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    int index = static_cast<int>(drawBuffer - GraphicsContextGL::DRAW_BUFFER0_EXT);
    ASSERT(index >= 0);
    if (index < static_cast<int>(m_drawBuffers.size()))
        return m_drawBuffers[index];
    if (drawBuffer == GraphicsContextGL::DRAW_BUFFER0_EXT)
        return GraphicsContextGL::COLOR_ATTACHMENT0;
    return GraphicsContextGL::NONE;
}

void WebGLFramebuffer::addMembersToOpaqueRoots(const AbstractLocker& locker, JSC::AbstractSlotVisitor& visitor)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    for (auto& entry : m_attachments)
        entry.value->addMembersToOpaqueRoots(locker, visitor);
}

void WebGLFramebuffer::setAttachmentInternal(GCGLenum attachment, GCGLenum texTarget, WebGLTexture* texture, GCGLint level, GCGLint layer)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    if (!context()) {
        // Context has been deleted - should not be calling this.
        return;
    }
    Locker locker { objectGraphLockForContext() };

    removeAttachmentInternal(locker, attachment);
    if (texture && texture->object()) {
        auto textureAttachment = WebGLTextureAttachment::create(texture, texTarget, level, layer);
        if (attachment == GraphicsContextGL::DEPTH_STENCIL_ATTACHMENT && context()->isWebGL2()) {
            m_attachments.set(GraphicsContextGL::DEPTH_ATTACHMENT, textureAttachment.ptr());
            m_attachments.set(GraphicsContextGL::STENCIL_ATTACHMENT, textureAttachment.ptr());
        }
        m_attachments.set(attachment, WTFMove(textureAttachment));
        drawBuffersIfNecessary(false);
        texture->onAttached();
    }
}

void WebGLFramebuffer::setAttachmentInternal(GCGLenum attachment, WebGLRenderbuffer* renderbuffer)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    if (!context()) {
        // Context has been deleted - should not be calling this.
        return;
    }
    Locker locker { objectGraphLockForContext() };

    removeAttachmentInternal(locker, attachment);
    if (renderbuffer && renderbuffer->object()) {
        auto renderbufferAttachment = WebGLRenderbufferAttachment::create(renderbuffer);
        if (attachment == GraphicsContextGL::DEPTH_STENCIL_ATTACHMENT && context()->isWebGL2()) {
            m_attachments.set(GraphicsContextGL::DEPTH_ATTACHMENT, renderbufferAttachment.ptr());
            m_attachments.set(GraphicsContextGL::STENCIL_ATTACHMENT, renderbufferAttachment.ptr());
        }
        m_attachments.set(attachment, WTFMove(renderbufferAttachment));
        drawBuffersIfNecessary(false);
        renderbuffer->onAttached();
    }
}

void WebGLFramebuffer::removeAttachmentInternal(const AbstractLocker& locker, GCGLenum attachment)
{
	fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __FUNCTION__);
    WebGLAttachment* attachmentObject = getAttachment(attachment);
    if (attachmentObject) {
        attachmentObject->onDetached(locker, context()->graphicsContextGL());
        if (attachment == GraphicsContextGL::DEPTH_STENCIL_ATTACHMENT && context()->isWebGL2()) {
            m_attachments.remove(GraphicsContextGL::DEPTH_ATTACHMENT);
            m_attachments.remove(GraphicsContextGL::STENCIL_ATTACHMENT);
        }
        m_attachments.remove(attachment);
        drawBuffersIfNecessary(false);
    }
}

}

#endif // ENABLE(WEBGL)
