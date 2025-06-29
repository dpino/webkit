/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
#include "HTMLResourcePreloader.h"

#include "CachedResourceLoader.h"
#include "CrossOriginAccessControl.h"
#include "DefaultResourceLoadPriority.h"
#include "DocumentInlines.h"
#include "MediaQueryEvaluator.h"
#include "MediaQueryParser.h"
#include "NodeRenderStyle.h"
#include "RenderView.h"
#include "ScriptElementCachedScriptFetcher.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(PreloadRequest);
WTF_MAKE_TZONE_ALLOCATED_IMPL(HTMLResourcePreloader);

URL PreloadRequest::completeURL(Document& document)
{
    return document.completeURL(m_resourceURL, m_baseURL.isEmpty() ? document.baseURL() : m_baseURL);
}

CachedResourceRequest PreloadRequest::resourceRequest(Document& document)
{
    ASSERT(isMainThread());

    bool skipContentSecurityPolicyCheck = false;
    if (m_resourceType == CachedResource::Type::Script)
        skipContentSecurityPolicyCheck = document.checkedContentSecurityPolicy()->allowScriptWithNonce(m_nonceAttribute);
    else if (m_resourceType == CachedResource::Type::CSSStyleSheet)
        skipContentSecurityPolicyCheck = document.checkedContentSecurityPolicy()->allowStyleWithNonce(m_nonceAttribute);

    ResourceLoaderOptions options = CachedResourceLoader::defaultCachedResourceOptions();
    if (skipContentSecurityPolicyCheck)
        options.contentSecurityPolicyImposition = ContentSecurityPolicyImposition::SkipPolicyCheck;

    String crossOriginMode = m_crossOriginMode;
    if (m_scriptType == ScriptType::Module) {
        if (crossOriginMode.isNull())
            crossOriginMode = ScriptElementCachedScriptFetcher::defaultCrossOriginModeForModule;
    }
    if (m_resourceType == CachedResource::Type::Script || m_resourceType == CachedResource::Type::ImageResource)
        options.referrerPolicy = m_referrerPolicy;
    options.fetchPriority = m_fetchPriority;
    auto request = createPotentialAccessControlRequest(completeURL(document), WTFMove(options), document, crossOriginMode);
    request.setInitiatorType(m_initiatorType);

    if (m_scriptIsAsync && m_resourceType == CachedResource::Type::Script && m_scriptType == ScriptType::Classic)
        request.setPriority(DefaultResourceLoadPriority::asyncScript);

    return request;
}

void HTMLResourcePreloader::preload(PreloadRequestStream requests)
{
    for (auto& request : requests)
        preload(WTFMove(request));
}

void HTMLResourcePreloader::preload(std::unique_ptr<PreloadRequest> preload)
{
    Ref document = m_document.get();
    ASSERT(document->frame());
    ASSERT(document->renderView());

    auto queries = MQ::MediaQueryParser::parse(preload->media(), document->cssParserContext());
    if (!MQ::MediaQueryEvaluator { screenAtom(), document, document->renderStyle() }.evaluate(queries))
        return;

    document->protectedCachedResourceLoader()->preload(preload->resourceType(), preload->resourceRequest(document));
}

}
