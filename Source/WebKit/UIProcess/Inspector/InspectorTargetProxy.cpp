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

#include "config.h"
#include "InspectorTargetProxy.h"

#include "MessageSenderInlines.h"
#include "ProvisionalPageProxy.h"
#include "WebPageInspectorController.h"
#include "WebPageInspectorTarget.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <JavaScriptCore/InspectorTarget.h>

namespace WebKit {

using namespace Inspector;

<<<<<<< HEAD
InspectorTargetProxy::InspectorTargetProxy(const String& targetId, Inspector::InspectorTargetType type)
    : m_identifier(targetId)
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
WTF_MAKE_TZONE_ALLOCATED_IMPL(InspectorTargetProxy);

std::unique_ptr<InspectorTargetProxy> InspectorTargetProxy::create(WebPageProxy& page, const String& targetId, Inspector::InspectorTargetType type)
{
    return makeUnique<InspectorTargetProxy>(page, targetId, type);
}

std::unique_ptr<InspectorTargetProxy> InspectorTargetProxy::create(ProvisionalPageProxy& provisionalPage, const String& targetId, Inspector::InspectorTargetType type)
{
    RefPtr page = provisionalPage.page();
    if (!page)
        return nullptr;

    auto target = InspectorTargetProxy::create(*page, targetId, type);
    target->m_provisionalPage = provisionalPage;
    return target;
}

InspectorTargetProxy::InspectorTargetProxy(WebPageProxy& page, const String& targetId, Inspector::InspectorTargetType type)
    : m_page(page)
    , m_identifier(targetId)
=======
WTF_MAKE_TZONE_ALLOCATED_IMPL(InspectorTargetProxy);

std::unique_ptr<InspectorTargetProxy> InspectorTargetProxy::create(WebPageProxy& page, const String& targetId, Inspector::InspectorTargetType type)
{
    return makeUnique<InspectorTargetProxy>(page, targetId, type);
}

std::unique_ptr<InspectorTargetProxy> InspectorTargetProxy::create(ProvisionalPageProxy& provisionalPage, const String& targetId)
{
    RefPtr page = provisionalPage.page();
    if (!page)
        return nullptr;

    auto target = InspectorTargetProxy::create(*page, targetId, Inspector::InspectorTargetType::Page);
    target->m_provisionalPage = provisionalPage;
    return target;
}

InspectorTargetProxy::InspectorTargetProxy(WebPageProxy& page, const String& targetId, Inspector::InspectorTargetType type)
    : m_page(page)
    , m_identifier(targetId)
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
    , m_type(type)
{
}

}
<<<<<<< HEAD
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)

void InspectorTargetProxy::disconnect()
{
    if (isPaused())
        resume();

    if (RefPtr provisionalPage = m_provisionalPage.get()) {
        provisionalPage->send(Messages::WebPage::DisconnectInspector(identifier()));
        return;
    }

    Ref page = m_page.get();
    if (page->hasRunningProcess())
        page->protectedLegacyMainFrameProcess()->send(Messages::WebPage::DisconnectInspector(identifier()), page->webPageIDInMainFrameProcess());
}

void InspectorTargetProxy::sendMessageToTargetBackend(const String& message)
{
    if (RefPtr provisionalPage = m_provisionalPage.get()) {
        provisionalPage->send(Messages::WebPage::SendMessageToTargetBackend(identifier(), message));
        return;
    }

    Ref page = m_page.get();
    if (page->hasRunningProcess())
        page->protectedLegacyMainFrameProcess()->send(Messages::WebPage::SendMessageToTargetBackend(identifier(), message), page->webPageIDInMainFrameProcess());
}

void InspectorTargetProxy::didCommitProvisionalTarget()
{
    m_provisionalPage = nullptr;
}

bool InspectorTargetProxy::isProvisional() const
{
    return !!m_provisionalPage;
}

} // namespace WebKit
=======

void InspectorTargetProxy::disconnect()
{
    if (isPaused())
        resume();

    if (RefPtr provisionalPage = m_provisionalPage.get()) {
        provisionalPage->send(Messages::WebPage::DisconnectInspector(identifier()));
        return;
    }

    Ref page = m_page.get();
    if (page->hasRunningProcess())
        page->protectedLegacyMainFrameProcess()->send(Messages::WebPage::DisconnectInspector(identifier()), page->webPageIDInMainFrameProcess());
}

void InspectorTargetProxy::sendMessageToTargetBackend(const String& message)
{
    if (RefPtr provisionalPage = m_provisionalPage.get()) {
        provisionalPage->send(Messages::WebPage::SendMessageToTargetBackend(identifier(), message));
        return;
    }

    Ref page = m_page.get();
    if (page->hasRunningProcess())
        page->protectedLegacyMainFrameProcess()->send(Messages::WebPage::SendMessageToTargetBackend(identifier(), message), page->webPageIDInMainFrameProcess());
}

void InspectorTargetProxy::didCommitProvisionalTarget()
{
    m_provisionalPage = nullptr;
}

void InspectorTargetProxy::willResume()
{
    if (m_page->hasRunningProcess())
        m_page->legacyMainFrameProcess().send(Messages::WebPage::ResumeInspectorIfPausedInNewWindow(), m_page->webPageIDInMainFrameProcess());
}

void InspectorTargetProxy::activate(String& error)
{
    if (m_type != Inspector::InspectorTargetType::Page)
        return InspectorTarget::activate(error);

    platformActivate(error);
}

void InspectorTargetProxy::close(String& error, bool runBeforeUnload)
{
    if (m_type != Inspector::InspectorTargetType::Page)
        return InspectorTarget::close(error, runBeforeUnload);

    if (runBeforeUnload)
        m_page->tryClose();
    else
        m_page->closePage();
}

bool InspectorTargetProxy::isProvisional() const
{
    return !!m_provisionalPage;
}

} // namespace WebKit
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
