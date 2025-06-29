/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
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
#include "ScrollTypes.h"

#include "ScrollBehavior.h"
#include <wtf/text/TextStream.h>

namespace WebCore {

TextStream& operator<<(TextStream& ts, ScrollType scrollType)
{
    switch (scrollType) {
    case ScrollType::User: ts << "user"_s; break;
    case ScrollType::Programmatic: ts << "programmatic"_s; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, NativeScrollbarVisibility scrollBarHidden)
{
    switch (scrollBarHidden) {
    case NativeScrollbarVisibility::Visible: ts << 0; break;
    case NativeScrollbarVisibility::HiddenByStyle: ts << 1; break;
    case NativeScrollbarVisibility::ReplacedByCustomScrollbar: ts << 2; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollClamping clamping)
{
    switch (clamping) {
    case ScrollClamping::Unclamped: ts << "unclamped"_s; break;
    case ScrollClamping::Clamped: ts << "clamped"_s; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollBehaviorForFixedElements behavior)
{
    switch (behavior) {
    case ScrollBehaviorForFixedElements::StickToDocumentBounds:
        ts << 0;
        break;
    case ScrollBehaviorForFixedElements::StickToViewportBounds:
        ts << 1;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollBehavior behavior)
{
    switch (behavior) {
    case ScrollBehavior::Auto: ts << "auto"_s; break;
    case ScrollBehavior::Instant: ts << "instant"_s; break;
    case ScrollBehavior::Smooth: ts << "smooth"_s; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollElasticity behavior)
{
    switch (behavior) {
    case ScrollElasticity::Automatic:
        ts << 0;
        break;
    case ScrollElasticity::None:
        ts << 1;
        break;
    case ScrollElasticity::Allowed:
        ts << 2;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, RubberBandingBehavior behavior)
{
    switch (behavior) {
    case RubberBandingBehavior::Always: ts << "always"_s; break;
    case RubberBandingBehavior::Never: ts << "never"_s; break;
    case RubberBandingBehavior::BasedOnSize: ts << "based on size"_s; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollbarMode behavior)
{
    switch (behavior) {
    case ScrollbarMode::Auto:
        ts << 0;
        break;
    case ScrollbarMode::AlwaysOff:
        ts << 1;
        break;
    case ScrollbarMode::AlwaysOn:
        ts << 2;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, OverflowAnchor behavior)
{
    switch (behavior) {
    case OverflowAnchor::Auto:
        ts << 0;
        break;
    case OverflowAnchor::None:
        ts << 1;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollDirection direction)
{
    switch (direction) {
    case ScrollDirection::ScrollUp:
        ts << "up"_s;
        break;
    case ScrollDirection::ScrollDown:
        ts << "down"_s;
        break;
    case ScrollDirection::ScrollLeft:
        ts << "left"_s;
        break;
    case ScrollDirection::ScrollRight:
        ts << "right"_s;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollGranularity granularity)
{
    switch (granularity) {
    case ScrollGranularity::Line:
        ts << "line"_s;
        break;
    case ScrollGranularity::Page:
        ts << "page"_s;
        break;
    case ScrollGranularity::Document:
        ts << "document"_s;
        break;
    case ScrollGranularity::Pixel:
        ts << "pixel"_s;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollbarWidth width)
{
    switch (width) {
    case ScrollbarWidth::Auto:
        ts << "auto"_s;
        break;
    case ScrollbarWidth::Thin:
        ts << "thin"_s;
        break;
    case ScrollbarWidth::None:
        ts << "none"_s;
        break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollPositionChangeOptions options)
{
    ts.dumpProperty("type"_s, options.type);
    ts.dumpProperty("clamping"_s, options.clamping);
    ts.dumpProperty("animated"_s, options.animated == ScrollIsAnimated::Yes);
    ts.dumpProperty("snap point selection method"_s, options.snapPointSelectionMethod);
    ts.dumpProperty("original scroll delta"_s, options.originalScrollDelta ? *options.originalScrollDelta : FloatSize());

    return ts;
}

TextStream& operator<<(TextStream& ts, ScrollSnapPointSelectionMethod option)
{
    switch (option) {
    case ScrollSnapPointSelectionMethod::Directional:
        ts << "Directional"_s;
        break;
    case ScrollSnapPointSelectionMethod::Closest:
        ts << "Closest"_s;
        break;
    }
    return ts;
}

} // namespace WebCore
