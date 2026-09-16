/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#include "config.h"
#include "SVGPointList.h"

#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringParsingBuffer.h>

namespace WebCore {

bool SVGPointList::parse(StringView value)
{
    clearItems();

    bool parsingSucceeded = readCharactersForParsing(value, [&](auto buffer) {
        skipOptionalSVGSpaces(buffer);

        bool delimParsed = false;
        while (buffer.hasCharactersRemaining()) {
            delimParsed = false;

            auto xPos = parseNumber(buffer);
            if (!xPos)
                return false;

            auto yPos = parseNumber(buffer, SuffixSkippingPolicy::DontSkip);
            if (!yPos) {
                skipOptionalSVGSpaces(buffer);
                if (buffer.hasCharactersRemaining())
                    return false;
                break;
            }

            skipOptionalSVGSpaces(buffer);

            if (skipExactly(buffer, ','))
                delimParsed = true;

            skipOptionalSVGSpaces(buffer);

            append(SVGPoint::create({ *xPos, *yPos }));
        }

        return !delimParsed;
    });
    if (!parsingSucceeded)
        clearItems();
    return parsingSucceeded;
}

uint64_t SVGPointList::nextItemsIdentifier()
{
    static uint64_t identifier = 0;
    return ++identifier;
}

Ref<SVGPoint> SVGPointList::insertAt(unsigned index, Ref<SVGPoint>&& newItem)
{
    itemsDidChangeStructurally();
    return Base::insertAt(index, WTF::move(newItem));
}

Ref<SVGPoint> SVGPointList::replaceAt(unsigned index, Ref<SVGPoint>&& newItem)
{
    itemDidChange(index);
    return Base::replaceAt(index, WTF::move(newItem));
}

Ref<SVGPoint> SVGPointList::removeAt(unsigned index)
{
    itemsDidChangeStructurally();
    return Base::removeAt(index);
}

Ref<SVGPoint> SVGPointList::append(Ref<SVGPoint>&& newItem)
{
    itemDidChange(size());
    return Base::append(WTF::move(newItem));
}

void SVGPointList::detachItems()
{
    // Called whenever the items are cleared, for example when parsing a new value.
    itemsDidChangeStructurally();
    Base::detachItems();
}

void SVGPointList::commitPropertyChange(SVGProperty* item)
{
    if (auto index = indexOfItem(item))
        itemDidChange(*index);
    else
        itemsDidChangeStructurally();
    Base::commitPropertyChange(item);
}

std::optional<unsigned> SVGPointList::indexOfItem(const SVGProperty* item) const
{
    unsigned size = m_items.size();
    if (!size)
        return std::nullopt;

    // Items usually change one after another, so start looking next to the last changed item.
    unsigned hint = std::min(m_lastChangedItemIndex, size - 1);
    for (unsigned distance = 0; distance < size; ++distance) {
        if (hint + distance < size && static_cast<const SVGProperty*>(m_items[hint + distance].ptr()) == item)
            return hint + distance;
        if (distance && distance <= hint && static_cast<const SVGProperty*>(m_items[hint - distance].ptr()) == item)
            return hint - distance;
    }
    return std::nullopt;
}

void SVGPointList::itemDidChange(unsigned index)
{
    m_lastChangedItemIndex = index;
    if (!m_changedItemRange) {
        m_changedItemRange = ItemRange { index, index };
        return;
    }
    m_changedItemRange->first = std::min(m_changedItemRange->first, index);
    m_changedItemRange->last = std::max(m_changedItemRange->last, index);
}

void SVGPointList::itemsDidChangeStructurally()
{
    m_itemsIdentifier = nextItemsIdentifier();
    m_changedItemRange = std::nullopt;
}

String SVGPointList::valueAsString() const
{
    StringBuilder builder;

    for (const auto& point : m_items) {
        if (builder.length())
            builder.append(' ');

        builder.append(point->x(), ' ', point->y());
    }

    return builder.toString();
}

}
