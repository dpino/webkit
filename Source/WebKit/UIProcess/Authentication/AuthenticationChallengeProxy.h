/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#pragma once

#include "APIObject.h"
#include "IdentifierTypes.h"
#include <WebCore/AuthenticationChallenge.h>
#include <wtf/WeakPtr.h>

namespace IPC {
class Connection;
}

namespace WebKit {

class AuthenticationDecisionListener;
class WebCredential;
class WebProtectionSpace;

#if HAVE(SEC_KEY_PROXY)
class SecKeyProxyStore;
using WeakPtrSecKeyProxyStore = WeakPtr<SecKeyProxyStore>;
#else
using WeakPtrSecKeyProxyStore = std::nullptr_t;
#endif

class AuthenticationChallengeProxy : public API::ObjectImpl<API::Object::Type::AuthenticationChallenge> {
public:
    static Ref<AuthenticationChallengeProxy> create(WebCore::AuthenticationChallenge&& authenticationChallenge, AuthenticationChallengeIdentifier challengeID, Ref<IPC::Connection>&& connection, WeakPtrSecKeyProxyStore&& secKeyProxyStore)
    {
        return adoptRef(*new AuthenticationChallengeProxy(WTFMove(authenticationChallenge), challengeID, WTFMove(connection), WTFMove(secKeyProxyStore)));
    }

    virtual ~AuthenticationChallengeProxy();

    WebCredential* proposedCredential() const;
    RefPtr<WebCredential> protectedProposedCredential() const;

    WebProtectionSpace* protectionSpace() const;
    RefPtr<WebProtectionSpace> protectedProtectionSpace() const;

    AuthenticationDecisionListener& listener() const { return m_listener.get(); }
    const WebCore::AuthenticationChallenge& core() { return m_coreAuthenticationChallenge; }

private:
    AuthenticationChallengeProxy(WebCore::AuthenticationChallenge&&, AuthenticationChallengeIdentifier, Ref<IPC::Connection>&&, WeakPtrSecKeyProxyStore&&);

#if HAVE(SEC_KEY_PROXY)
    static void sendClientCertificateCredentialOverXpc(IPC::Connection&, SecKeyProxyStore&, AuthenticationChallengeIdentifier, const WebCore::Credential&);
#endif

    WebCore::AuthenticationChallenge m_coreAuthenticationChallenge;
    mutable RefPtr<WebCredential> m_webCredential;
    mutable RefPtr<WebProtectionSpace> m_webProtectionSpace;
    const Ref<AuthenticationDecisionListener> m_listener;
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_BEGIN(WebKit::AuthenticationChallengeProxy)
static bool isType(const API::Object& object) { return object.type() == API::Object::Type::AuthenticationChallenge; }
SPECIALIZE_TYPE_TRAITS_END()
