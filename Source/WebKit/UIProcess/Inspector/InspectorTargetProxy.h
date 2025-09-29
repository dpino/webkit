/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#pragma once

#include <JavaScriptCore/InspectorTarget.h>

namespace WebKit {

<<<<<<< HEAD
class InspectorTargetProxy : public Inspector::InspectorTarget {
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
class ProvisionalPageProxy;
class WebPageProxy;

// NOTE: This UIProcess side InspectorTarget doesn't care about the frontend channel, since
// any target -> frontend messages will be routed to the WebPageProxy with a targetId.

class InspectorTargetProxy final : public Inspector::InspectorTarget {
    WTF_MAKE_TZONE_ALLOCATED(InspectorTargetProxy);
    WTF_MAKE_NONCOPYABLE(InspectorTargetProxy);
=======
class ProvisionalPageProxy;
class WebPageProxy;

// NOTE: This UIProcess side InspectorTarget doesn't care about the frontend channel, since
// any target -> frontend messages will be routed to the WebPageProxy with a targetId.

class InspectorTargetProxy : public Inspector::InspectorTarget {
    WTF_MAKE_TZONE_ALLOCATED(InspectorTargetProxy);
    WTF_MAKE_NONCOPYABLE(InspectorTargetProxy);
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
public:
<<<<<<< HEAD
    virtual ~InspectorTargetProxy() = default;
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
    static std::unique_ptr<InspectorTargetProxy> create(WebPageProxy&, const String& targetId, Inspector::InspectorTargetType);
    static std::unique_ptr<InspectorTargetProxy> create(ProvisionalPageProxy&, const String& targetId, Inspector::InspectorTargetType);
    InspectorTargetProxy(WebPageProxy&, const String& targetId, Inspector::InspectorTargetType);
    ~InspectorTargetProxy() = default;
=======
    static std::unique_ptr<InspectorTargetProxy> create(WebPageProxy&, const String& targetId, Inspector::InspectorTargetType);
    static std::unique_ptr<InspectorTargetProxy> create(ProvisionalPageProxy&, const String& targetId);
    InspectorTargetProxy(WebPageProxy&, const String& targetId, Inspector::InspectorTargetType);
    ~InspectorTargetProxy() = default;
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)

    Inspector::InspectorTargetType type() const final { return m_type; }
    String identifier() const final { return m_identifier; }

    virtual void didCommitProvisionalTarget() = 0;

<<<<<<< HEAD
protected:
    InspectorTargetProxy(const String& targetId, Inspector::InspectorTargetType);
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
    void connect(Inspector::FrontendChannel::ConnectionType) override;
    void disconnect() override;
    void sendMessageToTargetBackend(const String&) override;
=======
    void connect(Inspector::FrontendChannel::ConnectionType) override;
    void disconnect() override;
    void sendMessageToTargetBackend(const String&) override;
    void activate(String& error) override;
    void close(String& error, bool runBeforeUnload) override;
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)

private:
<<<<<<< HEAD
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
    WeakRef<WebPageProxy> m_page;
=======
    void willResume() override;
    void platformActivate(String& error) const;

    WeakRef<WebPageProxy> m_page;
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
    String m_identifier;
    Inspector::InspectorTargetType m_type;
};

} // namespace WebKit
