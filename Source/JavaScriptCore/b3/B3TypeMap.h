/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#if ENABLE(B3_JIT)

#include "B3Type.h"
#include <wtf/PrintStream.h>

#if !ASSERT_ENABLED
IGNORE_RETURN_TYPE_WARNINGS_BEGIN
#endif

namespace JSC { namespace B3 {

template<typename T>
class TypeMap {
public:
    TypeMap() = default;
    
    T& at(Type type)
    {
        switch (type.kind()) {
        case Void:
            return m_void;
        case Int32:
            return m_int32;
        case Int64:
            return m_int64;
        case Float:
            return m_float;
        case Double:
            return m_double;
        case Tuple:
            return m_tuple;
        case V128:
            return m_vector;
        }
        ASSERT_NOT_REACHED();
    }
    
    const T& at(Type type) const
    {
        return std::bit_cast<TypeMap*>(this)->at(type);
    }
    
    T& operator[](Type type)
    {
        return at(type);
    }
    
    const T& operator[](Type type) const
    {
        return at(type);
    }
    
    void dump(PrintStream& out) const
    {
        out.print(
            "{void = ", m_void,
            ", int32 = ", m_int32,
            ", int64 = ", m_int64,
            ", float = ", m_float,
            ", double = ", m_double,
            ", vector = ", m_vector,
            ", tuple = ", m_tuple, "}");
    }
    
private:
    T m_void { };
    T m_int32 { };
    T m_int64 { };
    T m_float { };
    T m_double { };
    T m_vector { };
    T m_tuple { };
};

} } // namespace JSC::B3

#if !ASSERT_ENABLED
IGNORE_RETURN_TYPE_WARNINGS_END
#endif

#endif // ENABLE(B3_JIT)
