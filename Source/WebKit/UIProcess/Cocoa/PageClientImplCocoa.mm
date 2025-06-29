/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "PageClientImplCocoa.h"

#import "APIUIClient.h"
#import "RemoteLayerTreeTransaction.h"
#import "WKWebViewInternal.h"
#import "WebFullScreenManagerProxy.h"
#import "WebPageProxy.h"
#import <WebCore/AlternativeTextUIController.h>
#import <WebCore/FixedContainerEdges.h>
#import <WebCore/TextAnimationTypes.h>
#import <WebCore/WritingToolsTypes.h>
#import <WebKit/WKWebViewConfigurationPrivate.h>
#import <WebKit/WKWebViewPrivateForTesting.h>
#import <pal/spi/ios/BrowserEngineKitSPI.h>
#import <wtf/Vector.h>
#import <wtf/cocoa/VectorCocoa.h>
#import <wtf/text/WTFString.h>

#if ENABLE(SCREEN_TIME)
#import <pal/cocoa/ScreenTimeSoftLink.h>
#endif

namespace WebKit {

PageClientImplCocoa::PageClientImplCocoa(WKWebView *webView)
    : m_webView { webView }
    , m_alternativeTextUIController { makeUnique<WebCore::AlternativeTextUIController>() }
{
}

PageClientImplCocoa::~PageClientImplCocoa() = default;

void PageClientImplCocoa::obscuredContentInsetsDidChange()
{
    [m_webView _recalculateViewportSizesWithMinimumViewportInset:[m_webView minimumViewportInset] maximumViewportInset:[m_webView maximumViewportInset] throwOnInvalidInput:NO];
}

void PageClientImplCocoa::themeColorWillChange()
{
    [m_webView willChangeValueForKey:@"themeColor"];
}

void PageClientImplCocoa::themeColorDidChange()
{
    [m_webView didChangeValueForKey:@"themeColor"];
}

void PageClientImplCocoa::underPageBackgroundColorWillChange()
{
    [m_webView willChangeValueForKey:@"underPageBackgroundColor"];
}

void PageClientImplCocoa::underPageBackgroundColorDidChange()
{
    RetainPtr webView = this->webView();

    [webView didChangeValueForKey:@"underPageBackgroundColor"];
#if ENABLE(CONTENT_INSET_BACKGROUND_FILL)
    [webView _updateTopScrollPocketCaptureColor];
#endif
}

void PageClientImplCocoa::sampledPageTopColorWillChange()
{
    [m_webView willChangeValueForKey:@"_sampledPageTopColor"];
}

void PageClientImplCocoa::sampledPageTopColorDidChange()
{
    [m_webView didChangeValueForKey:@"_sampledPageTopColor"];
}

#if ENABLE(WEB_PAGE_SPATIAL_BACKDROP)
void PageClientImplCocoa::spatialBackdropSourceWillChange()
{
    [m_webView willChangeValueForKey:@"_spatialBackdropSource"];
}

void PageClientImplCocoa::spatialBackdropSourceDidChange()
{
    [m_webView _spatialBackdropSourceDidChange];
    [m_webView didChangeValueForKey:@"_spatialBackdropSource"];
}
#endif

void PageClientImplCocoa::isPlayingAudioWillChange()
{
    [m_webView willChangeValueForKey:NSStringFromSelector(@selector(_isPlayingAudio))];
}

void PageClientImplCocoa::isPlayingAudioDidChange()
{
    [m_webView didChangeValueForKey:NSStringFromSelector(@selector(_isPlayingAudio))];
}

bool PageClientImplCocoa::scrollingUpdatesDisabledForTesting()
{
    return [m_webView _scrollingUpdatesDisabledForTesting];
}

#if ENABLE(ATTACHMENT_ELEMENT)

void PageClientImplCocoa::didInsertAttachment(API::Attachment& attachment, const String& source)
{
    [m_webView _didInsertAttachment:attachment withSource:source.createNSString().get()];
}

void PageClientImplCocoa::didRemoveAttachment(API::Attachment& attachment)
{
    [m_webView _didRemoveAttachment:attachment];
}

void PageClientImplCocoa::didInvalidateDataForAttachment(API::Attachment& attachment)
{
    [m_webView _didInvalidateDataForAttachment:attachment];
}

NSFileWrapper *PageClientImplCocoa::allocFileWrapperInstance() const
{
    RetainPtr cls = [m_webView configuration]._attachmentFileWrapperClass ?: [NSFileWrapper class];
    return [cls.get() alloc];
}

NSSet *PageClientImplCocoa::serializableFileWrapperClasses() const
{
    RetainPtr<Class> defaultFileWrapperClass = NSFileWrapper.class;
    RetainPtr<Class> configuredFileWrapperClass = [m_webView configuration]._attachmentFileWrapperClass;
    if (configuredFileWrapperClass && configuredFileWrapperClass.get() != defaultFileWrapperClass.get())
        return [NSSet setWithObjects:configuredFileWrapperClass.get(), defaultFileWrapperClass.get(), nil];
    return [NSSet setWithObjects:defaultFileWrapperClass.get(), nil];
}

#endif

#if ENABLE(APP_HIGHLIGHTS)
void PageClientImplCocoa::storeAppHighlight(const WebCore::AppHighlight &highlight)
{
    [m_webView _storeAppHighlight:highlight];
}
#endif // ENABLE(APP_HIGHLIGHTS)

void PageClientImplCocoa::pageClosed()
{
    m_alternativeTextUIController->clear();
}

#if ENABLE(GPU_PROCESS)
void PageClientImplCocoa::gpuProcessDidFinishLaunching()
{
    [m_webView willChangeValueForKey:@"_gpuProcessIdentifier"];
    [m_webView didChangeValueForKey:@"_gpuProcessIdentifier"];
}

void PageClientImplCocoa::gpuProcessDidExit()
{
    [m_webView willChangeValueForKey:@"_gpuProcessIdentifier"];
    [m_webView didChangeValueForKey:@"_gpuProcessIdentifier"];
}
#endif

#if ENABLE(MODEL_PROCESS)
void PageClientImplCocoa::modelProcessDidFinishLaunching()
{
    [m_webView willChangeValueForKey:@"_modelProcessIdentifier"];
    [m_webView didChangeValueForKey:@"_modelProcessIdentifier"];
}

void PageClientImplCocoa::modelProcessDidExit()
{
    [m_webView willChangeValueForKey:@"_modelProcessIdentifier"];
    [m_webView didChangeValueForKey:@"_modelProcessIdentifier"];
}
#endif

std::optional<WebCore::DictationContext> PageClientImplCocoa::addDictationAlternatives(PlatformTextAlternatives *alternatives)
{
    return m_alternativeTextUIController->addAlternatives(alternatives);
}

void PageClientImplCocoa::replaceDictationAlternatives(PlatformTextAlternatives *alternatives, WebCore::DictationContext context)
{
    m_alternativeTextUIController->replaceAlternatives(alternatives, context);
}

void PageClientImplCocoa::removeDictationAlternatives(WebCore::DictationContext dictationContext)
{
    m_alternativeTextUIController->removeAlternatives(dictationContext);
}

Vector<String> PageClientImplCocoa::dictationAlternatives(WebCore::DictationContext dictationContext)
{
    return makeVector<String>(platformDictationAlternatives(dictationContext).alternativeStrings);
}

PlatformTextAlternatives *PageClientImplCocoa::platformDictationAlternatives(WebCore::DictationContext dictationContext)
{
    return m_alternativeTextUIController->alternativesForContext(dictationContext);
}

void PageClientImplCocoa::microphoneCaptureWillChange()
{
    [m_webView willChangeValueForKey:@"microphoneCaptureState"];
}

void PageClientImplCocoa::cameraCaptureWillChange()
{
    [m_webView willChangeValueForKey:@"cameraCaptureState"];
}

void PageClientImplCocoa::displayCaptureWillChange()
{
    [m_webView willChangeValueForKey:@"_displayCaptureState"];
}

void PageClientImplCocoa::displayCaptureSurfacesWillChange()
{
    [m_webView willChangeValueForKey:@"_displayCaptureSurfaces"];
}

void PageClientImplCocoa::systemAudioCaptureWillChange()
{
    [m_webView willChangeValueForKey:@"_systemAudioCaptureState"];
}

void PageClientImplCocoa::microphoneCaptureChanged()
{
    [m_webView didChangeValueForKey:@"microphoneCaptureState"];
}

void PageClientImplCocoa::cameraCaptureChanged()
{
    [m_webView didChangeValueForKey:@"cameraCaptureState"];
}

void PageClientImplCocoa::displayCaptureChanged()
{
    [m_webView didChangeValueForKey:@"_displayCaptureState"];
}

void PageClientImplCocoa::displayCaptureSurfacesChanged()
{
    [m_webView didChangeValueForKey:@"_displayCaptureSurfaces"];
}

void PageClientImplCocoa::systemAudioCaptureChanged()
{
    [m_webView didChangeValueForKey:@"_systemAudioCaptureState"];
}

WindowKind PageClientImplCocoa::windowKind()
{
    RetainPtr window = [m_webView window];
    if (!window)
        return WindowKind::Unparented;
    if ([window isKindOfClass:NSClassFromString(@"_SCNSnapshotWindow")])
        return WindowKind::InProcessSnapshotting;
    return WindowKind::Normal;
}

#if ENABLE(WRITING_TOOLS)
void PageClientImplCocoa::proofreadingSessionShowDetailsForSuggestionWithIDRelativeToRect(const WebCore::WritingTools::TextSuggestion::ID& replacementID, WebCore::IntRect selectionBoundsInRootView)
{
    [m_webView _proofreadingSessionShowDetailsForSuggestionWithUUID:replacementID.createNSUUID().get() relativeToRect:selectionBoundsInRootView];
}

void PageClientImplCocoa::proofreadingSessionUpdateStateForSuggestionWithID(WebCore::WritingTools::TextSuggestion::State state, const WebCore::WritingTools::TextSuggestion::ID& replacementID)
{
    [m_webView _proofreadingSessionUpdateState:state forSuggestionWithUUID:replacementID.createNSUUID().get()];
}

static NSString *writingToolsActiveKey = @"writingToolsActive";

void PageClientImplCocoa::writingToolsActiveWillChange()
{
    [m_webView willChangeValueForKey:writingToolsActiveKey];
}

void PageClientImplCocoa::writingToolsActiveDidChange()
{
    [m_webView didChangeValueForKey:writingToolsActiveKey];
}

void PageClientImplCocoa::didEndPartialIntelligenceTextAnimation()
{
    [m_webView _didEndPartialIntelligenceTextAnimation];
}

bool PageClientImplCocoa::writingToolsTextReplacementsFinished()
{
    return [m_webView _writingToolsTextReplacementsFinished];
}

void PageClientImplCocoa::addTextAnimationForAnimationID(const WTF::UUID& uuid, const WebCore::TextAnimationData& data)
{
    [m_webView _addTextAnimationForAnimationID:uuid.createNSUUID().get() withData:data];
}

void PageClientImplCocoa::removeTextAnimationForAnimationID(const WTF::UUID& uuid)
{
    [m_webView _removeTextAnimationForAnimationID:uuid.createNSUUID().get()];
}

#endif

#if ENABLE(SCREEN_TIME)
void PageClientImplCocoa::didChangeScreenTimeWebpageControllerURL()
{
    updateScreenTimeWebpageControllerURL(webView().get());
}

void PageClientImplCocoa::updateScreenTimeWebpageControllerURL(WKWebView *webView)
{
    if (!PAL::isScreenTimeFrameworkAvailable())
        return;

    RefPtr pageProxy = [webView _page].get();
    if (pageProxy && !pageProxy->preferences().screenTimeEnabled()) {
        [webView _uninstallScreenTimeWebpageController];
        return;
    }

    if ([webView window])
        [webView _installScreenTimeWebpageControllerIfNeeded];

    RetainPtr screenTimeWebpageController = [webView _screenTimeWebpageController];
    [screenTimeWebpageController setURL:[webView _mainFrameURL]];
}

void PageClientImplCocoa::setURLIsPictureInPictureForScreenTime(bool value)
{
    RetainPtr screenTimeWebpageController = [webView() _screenTimeWebpageController];
    if (!screenTimeWebpageController)
        return;

    [screenTimeWebpageController setURLIsPictureInPicture:value];
}

void PageClientImplCocoa::setURLIsPlayingVideoForScreenTime(bool value)
{
    RetainPtr screenTimeWebpageController = [webView() _screenTimeWebpageController];
    if (!screenTimeWebpageController)
        return;

    [screenTimeWebpageController setURLIsPlayingVideo:value];
}

#endif

void PageClientImplCocoa::viewIsBecomingVisible()
{
#if ENABLE(SCREEN_TIME)
    [m_webView _updateScreenTimeBasedOnWindowVisibility];
#endif
}

void PageClientImplCocoa::viewIsBecomingInvisible()
{
#if ENABLE(SCREEN_TIME)
    [m_webView _updateScreenTimeBasedOnWindowVisibility];
#endif
}

#if ENABLE(GAMEPAD)
void PageClientImplCocoa::setGamepadsRecentlyAccessed(GamepadsRecentlyAccessed gamepadsRecentlyAccessed)
{
    [m_webView _setGamepadsRecentlyAccessed:(gamepadsRecentlyAccessed == GamepadsRecentlyAccessed::No) ? NO : YES];
}

#if PLATFORM(VISION)
void PageClientImplCocoa::gamepadsConnectedStateChanged()
{
    [m_webView _gamepadsConnectedStateChanged];
}
#endif
#endif

void PageClientImplCocoa::hasActiveNowPlayingSessionChanged(bool hasActiveNowPlayingSession)
{
    if ([m_webView _hasActiveNowPlayingSession] == hasActiveNowPlayingSession)
        return;

    RELEASE_LOG(ViewState, "%p PageClientImplCocoa::hasActiveNowPlayingSessionChanged %d", m_webView.get().get(), hasActiveNowPlayingSession);

    [m_webView willChangeValueForKey:@"_hasActiveNowPlayingSession"];
    [m_webView _setHasActiveNowPlayingSession:hasActiveNowPlayingSession];
    [m_webView didChangeValueForKey:@"_hasActiveNowPlayingSession"];
}

void PageClientImplCocoa::videoControlsManagerDidChange()
{
    RELEASE_LOG(ViewState, "%p PageClientImplCocoa::videoControlsManagerDidChange %d", m_webView.get().get(), [m_webView _canEnterFullscreen]);
    [m_webView willChangeValueForKey:@"_canEnterFullscreen"];
    [m_webView didChangeValueForKey:@"_canEnterFullscreen"];
}

CocoaWindow *PageClientImplCocoa::platformWindow() const
{
    return [m_webView window];
}

void PageClientImplCocoa::processDidUpdateThrottleState()
{
    [m_webView willChangeValueForKey:@"_webProcessState"];
    [m_webView didChangeValueForKey:@"_webProcessState"];
}

#if ENABLE(FULLSCREEN_API)
void PageClientImplCocoa::setFullScreenClientForTesting(std::unique_ptr<WebFullScreenManagerProxyClient>&& client)
{
    m_fullscreenClientForTesting = WTFMove(client);
}
#endif

void PageClientImplCocoa::didCommitLayerTree(const RemoteLayerTreeTransaction& transaction)
{
    if (auto& edges = transaction.fixedContainerEdges())
        [webView() _updateFixedContainerEdges:*edges];
    [webView() _updateScrollGeometryWithContentOffset:transaction.scrollPosition() contentSize:transaction.scrollGeometryContentSize()];
}

} // namespace WebKit
