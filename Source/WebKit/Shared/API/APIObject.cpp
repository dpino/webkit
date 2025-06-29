/*
 * Copyright (c) 2011 Motorola Mobility, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation and/or 
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may 
 * be used to endorse or promote products derived from this software without 
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "APIObject.h"

#include "WebKit2Initialize.h"

#include <wtf/ThreadSpecific.h>

#define DELEGATE_REF_COUNTING_TO_COCOA PLATFORM(COCOA)

namespace API {

#if DELEGATE_REF_COUNTING_TO_COCOA

HashMap<Object*, CFTypeRef>& Object::apiObjectsUnderConstruction()
{
    static LazyNeverDestroyed<ThreadSpecific<HashMap<Object*, CFTypeRef>>> s_apiObjectsUnderConstruction;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        s_apiObjectsUnderConstruction.construct();
    });
    return *s_apiObjectsUnderConstruction.get();
}

#endif

Object::Object()
#if DELEGATE_REF_COUNTING_TO_COCOA
    : m_wrapper(apiObjectsUnderConstruction().take(this))
#endif
{
#if DELEGATE_REF_COUNTING_TO_COCOA
    ASSERT(m_wrapper);
#endif
    WebKit::InitializeWebKit2();
}

} // namespace API
