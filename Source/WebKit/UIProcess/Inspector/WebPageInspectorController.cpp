/*
 * Copyright (C) 2018-2020 Apple Inc. All rights reserved.
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
#include "WebPageInspectorController.h"

#include "APINavigation.h"
#include "APIPageConfiguration.h"
#include "APIUIClient.h"
#include "InspectorBrowserAgent.h"
#include "InspectorDialogAgent.h"
#include "InspectorScreencastAgent.h"
#include "ProvisionalPageProxy.h"
#include "WebFrameInspectorTargetProxy.h"
#include "WebFrameProxy.h"
#include "WebPageInspectorAgentBase.h"
#include "WebPageInspectorEmulationAgent.h"
#include "WebPageInspectorInputAgent.h"
#include "WebPageInspectorTarget.h"
#include "WebPageInspectorTargetProxy.h"
#include "WebPageProxy.h"
#include "WebsiteDataStore.h"
#include <WebCore/ResourceError.h>
#include <WebCore/WindowFeatures.h>
#include <JavaScriptCore/InspectorAgentBase.h>
#include <JavaScriptCore/InspectorBackendDispatcher.h>
#include <JavaScriptCore/InspectorBackendDispatchers.h>
#include <JavaScriptCore/InspectorFrontendRouter.h>
#include <JavaScriptCore/InspectorTargetAgent.h>
#include <wtf/HashMap.h>
#include <wtf/TZoneMallocInlines.h>

namespace WebKit {

using namespace Inspector;

static String getTargetID(const ProvisionalPageProxy& provisionalPage)
{
    return WebPageInspectorTarget::toTargetID(provisionalPage.webPageID());
}

WTF_MAKE_TZONE_ALLOCATED_IMPL(WebPageInspectorController);

WebPageInspectorControllerObserver* WebPageInspectorController::s_observer = nullptr;

void WebPageInspectorController::setObserver(WebPageInspectorControllerObserver* observer)
{
    s_observer = observer;
}

WebPageInspectorControllerObserver* WebPageInspectorController::observer() {
    return s_observer;
}

WebPageInspectorController::WebPageInspectorController(WebPageProxy& inspectedPage)
    : m_frontendRouter(FrontendRouter::create())
    , m_backendDispatcher(BackendDispatcher::create(m_frontendRouter.copyRef()))
    , m_inspectedPage(inspectedPage)
{
}

WebPageInspectorController::~WebPageInspectorController() = default;

WeakRef<WebPageProxy> WebPageInspectorController::protectedInspectedPage()
{
    return m_inspectedPage;
}

void WebPageInspectorController::init()
{
<<<<<<< HEAD
    String pageTargetId = WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess());
    createWebPageInspectorTarget(pageTargetId, Inspector::InspectorTargetType::Page);
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
    String pageTargetId = WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess());
    createInspectorTarget(pageTargetId, Inspector::InspectorTargetType::Page);
=======
    auto targetAgent = makeUnique<InspectorTargetAgent>(m_frontendRouter.get(), m_backendDispatcher.get());
    m_targetAgent = targetAgent.get();
    m_agents.append(WTFMove(targetAgent));
    auto emulationAgent = makeUnique<WebPageInspectorEmulationAgent>(m_backendDispatcher.get(), m_inspectedPage);
    m_emulationAgent = emulationAgent.get();
    m_agents.append(WTFMove(emulationAgent));
    auto inputAgent = makeUnique<WebPageInspectorInputAgent>(m_backendDispatcher.get(), m_inspectedPage);
    m_inputAgent = inputAgent.get();
    m_agents.append(WTFMove(inputAgent));
    m_agents.append(makeUnique<InspectorDialogAgent>(m_backendDispatcher.get(), m_frontendRouter.get(), m_inspectedPage));
    auto screencastAgent = makeUnique<InspectorScreencastAgent>(m_backendDispatcher.get(), m_frontendRouter.get(), m_inspectedPage);
    m_screecastAgent = screencastAgent.get();
    m_agents.append(WTFMove(screencastAgent));
    if (s_observer)
        s_observer->didCreateInspectorController(m_inspectedPage);
}

void WebPageInspectorController::didInitializeWebPage()
{
    String pageTargetID = WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess());
    // Create target only after attaching to a Web Process first time. Before that
    // we cannot event establish frontend connection.
    if (m_targets.contains(pageTargetID))
        return;
    createInspectorTarget(pageTargetID, Inspector::InspectorTargetType::Page);
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
}

void WebPageInspectorController::pageClosed()
{
    String pageTargetId = WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess());
    destroyInspectorTarget(pageTargetId);

    disconnectAllFrontends();

    m_agents.discardValues();

    if (s_observer)
        s_observer->willDestroyInspectorController(m_inspectedPage);
}

bool WebPageInspectorController::pageCrashed(ProcessTerminationReason reason)
{
    if (reason != ProcessTerminationReason::Crash)
        return false;
    String targetId = WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess());
    auto it = m_targets.find(targetId);
    if (it == m_targets.end())
        return false;
    m_targetAgent->targetCrashed(*it->value);
    m_targets.remove(it);

    return m_targetAgent->isConnected();
}

void WebPageInspectorController::willCreateNewPage(const WebCore::WindowFeatures& features, const URL& url)
{
    if (s_observer)
        s_observer->willCreateNewPage(m_inspectedPage, features, url);
}

void WebPageInspectorController::didShowPage()
{
    if (m_frontendRouter->hasFrontends())
        m_emulationAgent->didShowPage();
}

void WebPageInspectorController::didProcessAllPendingKeyboardEvents()
{
    if (m_frontendRouter->hasFrontends())
        m_inputAgent->didProcessAllPendingKeyboardEvents();
}

void WebPageInspectorController::didProcessAllPendingMouseEvents()
{
    if (m_frontendRouter->hasFrontends())
        m_inputAgent->didProcessAllPendingMouseEvents();
}

void WebPageInspectorController::didProcessAllPendingWheelEvents()
{
    if (m_frontendRouter->hasFrontends())
        m_inputAgent->didProcessAllPendingWheelEvents();
}

bool WebPageInspectorController::hasLocalFrontend() const
{
    return m_frontendRouter->hasLocalFrontend();
}

void WebPageInspectorController::connectFrontend(Inspector::FrontendChannel& frontendChannel, bool, bool)
{
    createLazyAgents();

    bool connectingFirstFrontend = !m_frontendRouter->hasFrontends();

    // HACK: forcefully disconnect remote connections to show local inspector starting with initial
    // agents' state.
    if (frontendChannel.connectionType() == Inspector::FrontendChannel::ConnectionType::Local &&
        !connectingFirstFrontend && !m_frontendRouter->hasLocalFrontend()) {
        disconnectAllFrontends();
        connectingFirstFrontend = true;
    }

    m_frontendRouter->connectFrontend(frontendChannel);

    if (connectingFirstFrontend)
        m_agents.didCreateFrontendAndBackend();

    Ref inspectedPage = m_inspectedPage.get();
    inspectedPage->didChangeInspectorFrontendCount(m_frontendRouter->frontendCount());

#if ENABLE(REMOTE_INSPECTOR)
    if (hasLocalFrontend())
        inspectedPage->remoteInspectorInformationDidChange();
#endif
}

void WebPageInspectorController::disconnectFrontend(FrontendChannel& frontendChannel)
{
    m_frontendRouter->disconnectFrontend(frontendChannel);

    bool disconnectingLastFrontend = !m_frontendRouter->hasFrontends();
    if (disconnectingLastFrontend) {
        m_agents.willDestroyFrontendAndBackend(DisconnectReason::InspectorDestroyed);
        m_pendingNavigations.clear();
    }

    Ref inspectedPage = m_inspectedPage.get();
    inspectedPage->didChangeInspectorFrontendCount(m_frontendRouter->frontendCount());

#if ENABLE(REMOTE_INSPECTOR)
    if (disconnectingLastFrontend)
        inspectedPage->remoteInspectorInformationDidChange();
#endif
}

void WebPageInspectorController::disconnectAllFrontends()
{
    // FIXME: Handle a local inspector client.

    if (!m_frontendRouter->hasFrontends())
        return;

    // Notify agents first, since they may need to use InspectorBackendClient.
    m_agents.willDestroyFrontendAndBackend(DisconnectReason::InspectedTargetDestroyed);

    // Disconnect any remaining remote frontends.
    m_frontendRouter->disconnectAllFrontends();

    m_pendingNavigations.clear();

    Ref inspectedPage = m_inspectedPage.get();
    inspectedPage->didChangeInspectorFrontendCount(m_frontendRouter->frontendCount());

#if ENABLE(REMOTE_INSPECTOR)
    inspectedPage->remoteInspectorInformationDidChange();
#endif
}

void WebPageInspectorController::dispatchMessageFromFrontend(const String& message)
{
    m_backendDispatcher->dispatch(message);
}

#if ENABLE(REMOTE_INSPECTOR)
void WebPageInspectorController::setIndicating(bool indicating)
{
    Ref inspectedPage = m_inspectedPage.get();
#if !PLATFORM(IOS_FAMILY)
    inspectedPage->setIndicating(indicating);
#else
    if (indicating)
        inspectedPage->showInspectorIndication();
    else
        inspectedPage->hideInspectorIndication();
#endif
}
#endif

<<<<<<< HEAD
void WebPageInspectorController::createWebPageInspectorTarget(const String& targetId, Inspector::InspectorTargetType type)
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
void WebPageInspectorController::createInspectorTarget(const String& targetId, Inspector::InspectorTargetType type)
=======
#if USE(SKIA)
void WebPageInspectorController::didPaint(sk_sp<SkImage>&& surface)
{
    if (!m_frontendRouter->hasFrontends())
        return;

    m_screecastAgent->didPaint(WTFMove(surface));
}
#endif


void WebPageInspectorController::navigate(WebCore::ResourceRequest&& request, WebFrameProxy* frame, NavigationHandler&& completionHandler)
{
    auto navigation = m_inspectedPage->loadRequestForInspector(WTFMove(request), frame);
    if (!navigation) {
        completionHandler("Failed to navigate"_s, { });
        return;
    }

    m_pendingNavigations.set(navigation->navigationID(), WTFMove(completionHandler));
}

void WebPageInspectorController::didReceivePolicyDecision(WebCore::PolicyAction action, std::optional<WebCore::NavigationIdentifier> navigationID)
{
    if (!m_frontendRouter->hasFrontends())
        return;

    if (!navigationID)
        return;

    auto completionHandler = m_pendingNavigations.take(*navigationID);
    if (!completionHandler)
        return;

    if (action == WebCore::PolicyAction::Ignore)
        completionHandler("Navigation cancelled"_s, { });
    else
        completionHandler(String(), *navigationID);
}

void WebPageInspectorController::didDestroyNavigation(WebCore::NavigationIdentifier navigationID)
{
    if (!m_frontendRouter->hasFrontends())
        return;

    auto completionHandler = m_pendingNavigations.take(navigationID);
    if (!completionHandler)
        return;

    // Inspector initiated navigation is destroyed before policy check only when it
    // becomes a fragment navigation (which always reuses current navigation).
    completionHandler(String(), { });
}

void WebPageInspectorController::didFailProvisionalLoadForFrame(WebCore::NavigationIdentifier navigationID, const WebCore::ResourceError& error)
{
    if (s_observer)
        s_observer->didFailProvisionalLoad(m_inspectedPage, navigationID, error.localizedDescription());
}

void WebPageInspectorController::createInspectorTarget(const String& targetId, Inspector::InspectorTargetType type)
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
{
    addTarget(WebPageInspectorTargetProxy::create(protectedInspectedPage(), targetId, type));
}

void WebPageInspectorController::createWebFrameInspectorTarget(WebFrameProxy& frame, const String& targetId)
{
    addTarget(WebFrameInspectorTargetProxy::create(frame, targetId));
}

void WebPageInspectorController::destroyInspectorTarget(const String& targetId)
{
    auto it = m_targets.find(targetId);
    if (it == m_targets.end())
        return;
    checkedTargetAgent()->targetDestroyed(*it->value);
    m_targets.remove(it);
}

void WebPageInspectorController::sendMessageToInspectorFrontend(const String& targetId, const String& message)
{
    checkedTargetAgent()->sendMessageFromTargetToFrontend(targetId, message);
}

void WebPageInspectorController::setPauseOnStart(bool shouldPause)
{
    ASSERT(m_frontendRouter->hasFrontends());
    m_targetAgent->setPauseOnStart(shouldPause);
}

bool WebPageInspectorController::shouldPauseLoadRequest() const
{
    if (!m_frontendRouter->hasFrontends())
        return false;

    if (!m_inspectedPage->isPageOpenedByDOMShowingInitialEmptyDocument())
        return false;

    auto* target = m_targets.get(WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess()));
    // The method is expeted to be called only when the WebPage has already been
    // initilized, so the target must exist.
    ASSERT(target);
    return target->isPaused();
}

bool WebPageInspectorController::shouldPauseInInspectorWhenShown() const
{
    if (!m_frontendRouter->hasFrontends())
        return false;

    // Only pause if the page was opened by window.open() or new tab navigation.
    // We cannot use isPageOpenedByDOMShowingInitialEmptyDocument() here because
    // this method maybe called from WebPageProxy::initializeWebPage and setOpenedByDOM
    // is called after the page is initialized.
    if (!m_inspectedPage->configuration().windowFeatures())
        return false;

    // The method is called from WebPageProxy::initializePage and the
    // target is not created yet (it is created after the new page is
    //  initialized and attached to the process).
    return m_targetAgent->shouldPauseOnStart();
}

void WebPageInspectorController::setContinueLoadingCallback(WTF::Function<void()>&& callback)
{
    auto* target = m_targets.get(WebPageInspectorTarget::toTargetID(m_inspectedPage->webPageIDInMainFrameProcess()));
    ASSERT(target);
    target->setResumeCallback(WTFMove(callback));
}

bool WebPageInspectorController::shouldPauseLoading(const ProvisionalPageProxy& provisionalPage) const
{
    if (!m_frontendRouter->hasFrontends())
        return false;

    auto* target = m_targets.get(getTargetID(provisionalPage));
    ASSERT(target);
    return target->isPaused();
}

void WebPageInspectorController::setContinueLoadingCallback(const ProvisionalPageProxy& provisionalPage, WTF::Function<void()>&& callback)
{
    auto* target = m_targets.get(getTargetID(provisionalPage));
    ASSERT(target);
    target->setResumeCallback(WTFMove(callback));
}

void WebPageInspectorController::didCreateProvisionalPage(ProvisionalPageProxy& provisionalPage)
{
<<<<<<< HEAD
    addTarget(WebPageInspectorTargetProxy::create(provisionalPage, getTargetID(provisionalPage), Inspector::InspectorTargetType::Page));
||||||| parent of 956af7f55f35 (chore(webkit): bootstrap build #2214)
    addTarget(InspectorTargetProxy::create(provisionalPage, getTargetID(provisionalPage), Inspector::InspectorTargetType::Page));
=======
    addTarget(InspectorTargetProxy::create(provisionalPage, getTargetID(provisionalPage)));
>>>>>>> 956af7f55f35 (chore(webkit): bootstrap build #2214)
}

void WebPageInspectorController::willDestroyProvisionalPage(const ProvisionalPageProxy& provisionalPage)
{
    destroyInspectorTarget(getTargetID(provisionalPage));
}

void WebPageInspectorController::didCommitProvisionalPage(WebCore::PageIdentifier oldWebPageID, WebCore::PageIdentifier newWebPageID)
{
    String oldID = WebPageInspectorTarget::toTargetID(oldWebPageID);
    String newID = WebPageInspectorTarget::toTargetID(newWebPageID);
    auto newTarget = m_targets.take(newID);
    CheckedPtr targetAgent = m_targetAgent;
    ASSERT(newTarget);
    newTarget->didCommitProvisionalTarget();
    targetAgent->didCommitProvisionalTarget(oldID, newID);

    // We've disconnected from the old page and will not receive any message from it, so
    // we destroy everything but the new target here.
    // FIXME: <https://webkit.org/b/202937> do not destroy targets that belong to the committed page.
    for (auto& target : m_targets.values())
        targetAgent->targetDestroyed(*target);
    m_targets.clear();
    m_targets.set(newTarget->identifier(), WTFMove(newTarget));
}

InspectorBrowserAgent* WebPageInspectorController::enabledBrowserAgent() const
{
    return m_enabledBrowserAgent.get();
}

WebPageAgentContext WebPageInspectorController::webPageAgentContext()
{
    return {
        m_frontendRouter,
        m_backendDispatcher,
        m_inspectedPage,
    };
}

void WebPageInspectorController::createLazyAgents()
{
    if (m_didCreateLazyAgents)
        return;

    m_didCreateLazyAgents = true;

    auto webPageContext = webPageAgentContext();

    m_agents.append(makeUnique<InspectorBrowserAgent>(webPageContext));
}

void WebPageInspectorController::addTarget(std::unique_ptr<InspectorTargetProxy>&& target)
{
    checkedTargetAgent()->targetCreated(*target);
    m_targets.set(target->identifier(), WTFMove(target));
}

void WebPageInspectorController::setEnabledBrowserAgent(InspectorBrowserAgent* agent)
{
    if (m_enabledBrowserAgent == agent)
        return;

    m_enabledBrowserAgent = agent;

    Ref inspectedPage = m_inspectedPage.get();
    if (m_enabledBrowserAgent)
        inspectedPage->uiClient().didEnableInspectorBrowserDomain(inspectedPage);
    else
        inspectedPage->uiClient().didDisableInspectorBrowserDomain(inspectedPage);
}

void WebPageInspectorController::browserExtensionsEnabled(HashMap<String, String>&& extensionIDToName)
{
    if (CheckedPtr enabledBrowserAgent = m_enabledBrowserAgent)
        enabledBrowserAgent->extensionsEnabled(WTFMove(extensionIDToName));
}

void WebPageInspectorController::browserExtensionsDisabled(HashSet<String>&& extensionIDs)
{
    if (CheckedPtr enabledBrowserAgent = m_enabledBrowserAgent)
        enabledBrowserAgent->extensionsDisabled(WTFMove(extensionIDs));
}

} // namespace WebKit
