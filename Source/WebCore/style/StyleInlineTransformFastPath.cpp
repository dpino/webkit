/*
 * Copyright (C) 2026 Igalia S.L.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StyleInlineTransformFastPath.h"

#include "CSSValue.h"
#include "ComputedStyleDependencies.h"
#include "Document.h"
#include "Element.h"
#include "ElementInlines.h"
#include "RenderLayer.h"
#include "RenderLayerModelObject.h"
#include "Settings.h"
#include "StyleBuilderState.h"
#include "StyleComputedStyle+GettersInlines.h"
#include "StyleComputedStyle+SettersInlines.h"
#include "StyleDocumentScope.h"
#include "StyleTransform.h"

namespace WebCore {
namespace Style {

// A recalc queued anywhere in the document is not the question. What the fast path needs is that
// this element is not itself about to be re-resolved, since that is what makes the render style it
// writes into, and the eligibility RenderLayer::styleChanged() recorded, still authoritative.
//
// The distinction is the difference between working and not working on a real page. An application
// that interleaves transform assignments with any other style mutation - a class, an attribute, an
// SVG geometry property - latches the document-wide flag on its first mutation of the frame, and
// every assignment after that one loses the fast path. Since a single fallback invalidates style
// and brings the whole recalc back, the assignments that did take it save nothing.
//
// Anything that can change what the cascade picks for this element marks the element itself: a
// sibling combinator, :has(), an attribute selector, an ancestor's class. So the chain this element
// inherits from is the whole question, and a recalc pending on an unrelated subtree is not.
static bool styleResolutionIsPending(const Element& element)
{
    Ref document = element.document();
    if (!document->hasPendingStyleRecalc())
        return false;

    // Neither marks the element, and both can change which declaration wins here.
    if (document->hasPendingFullStyleRebuild() || document->styleScope().hasPendingUpdate())
        return true;

    for (RefPtr ancestor = &element; ancestor; ancestor = ancestor->parentElementInComposedTree()) {
        if (ancestor->needsStyleRecalc())
            return true;
    }
    return false;
}

bool applyInlineTransformWithoutStyleRecalc(Element& element, const CSSValue& value)
{
    Ref document = element.document();
    if (!document->settings().inlineTransformFastPathEnabled())
        return false;

    if (styleResolutionIsPending(element))
        return false;

    CheckedPtr renderer = dynamicDowncast<RenderLayerModelObject>(element.renderer());
    CheckedPtr layer = renderer ? renderer->layer() : nullptr;
    if (!layer || !layer->canUseInlineTransformFastPath())
        return false;

    // Style::Resolver registers these on the document and the parent element, a bare BuilderState
    // cannot. Conversion sets the flags, so refuse first.
    auto dependencies = value.computedStyleDependencies();
    if (dependencies.containerDimensions || dependencies.viewportDimensions || dependencies.anchors)
        return false;

    // The same pick Style::Resolver::State makes, so a value with root relative units resolves
    // here the way it would after a recalc.
    RefPtr documentElement = document->documentElement();
    auto* rootElementStyle = documentElement && documentElement != &element ? documentElement->renderStyle() : nullptr;
    if (!rootElementStyle)
        rootElementStyle = document->initialContainingBlockStyle();

    auto& style = renderer->mutableStyle();
    auto builderState = BuilderState::create(style, BuilderContext {
        .document = document.copyRef(),
        .parentStyle = renderer->parentStyle() ? renderer->parentStyle() : &style,
        .rootElementStyle = rootElementStyle,
        .element = &element
    });

    auto transform = toStyleFromCSSValue<Transform>(builderState, value);
    if (transform.isNone())
        return false;

    // A function that fails conversion lands here as an identity matrix, not as unset.
    if (builderState->isCurrentPropertyInvalidAtComputedValueTime())
        return false;

    // attr() substitutions are registered on the document by Style::Resolver, which is bypassed here.
    if (!builderState->registeredSubstitutionAttributes().isEmpty())
        return false;

    if (transform == style.transform())
        return true;

    style.setTransform(WTF::move(transform));
    layer->inlineTransformDidChange();
    return true;
}

} // namespace Style
} // namespace WebCore
