/*
 * Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "CoordinatedBackingStoreProxy.h"

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedBackingStoreProxyClient.h"
#include "GraphicsContext.h"
#include <wtf/CheckedArithmetic.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(CoordinatedBackingStoreProxy);

static const int defaultTileDimension = 512;

static IntPoint innerBottomRight(const IntRect& rect)
{
    // Actually, the rect does not contain rect.maxX(). Refer to IntRect::contain.
    return IntPoint(rect.maxX() - 1, rect.maxY() - 1);
}

CoordinatedBackingStoreProxy::CoordinatedBackingStoreProxy(CoordinatedBackingStoreProxyClient& client, float contentsScale)
    : m_client(client)
    , m_contentsScale(contentsScale)
    , m_tileSize(defaultTileDimension, defaultTileDimension)
{
}

CoordinatedBackingStoreProxy::~CoordinatedBackingStoreProxy() = default;

void CoordinatedBackingStoreProxy::createTilesIfNeeded(const IntRect& unscaledVisibleRect, const IntRect& unscaledContentsRect)
{
    IntRect contentsRect = mapFromContents(unscaledContentsRect);
    IntRect visibleRect = mapFromContents(unscaledVisibleRect);
    float coverAreaMultiplier = MemoryPressureHandler::singleton().isUnderMemoryPressure() ? 1.0f : 2.0f;

    bool didChange = m_visibleRect != visibleRect || m_contentsRect != contentsRect || m_coverAreaMultiplier != coverAreaMultiplier;
    if (didChange || m_pendingTileCreation)
        createTiles(visibleRect, contentsRect, coverAreaMultiplier);
}

void CoordinatedBackingStoreProxy::invalidate(const IntRect& contentsDirtyRect)
{
    IntRect dirtyRect(mapFromContents(contentsDirtyRect));
    IntRect keepRectFitToTileSize = tileRectForPosition(tilePositionForPoint(m_keepRect.location()));
    keepRectFitToTileSize.unite(tileRectForPosition(tilePositionForPoint(innerBottomRight(m_keepRect))));

    // Only iterate on the part of the rect that we know we might have tiles.
    IntRect coveredDirtyRect = intersection(dirtyRect, keepRectFitToTileSize);
    auto topLeft = tilePositionForPoint(coveredDirtyRect.location());
    auto bottomRight = tilePositionForPoint(innerBottomRight(coveredDirtyRect));

    for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
        for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
            auto* tile = m_tiles.get(IntPoint(x, y));
            if (!tile)
                continue;
            // Pass the full rect to each tile as coveredDirtyRect might not
            // contain them completely and we don't want partial tile redraws.
            tile->invalidate(dirtyRect);
        }
    }
}

Vector<std::reference_wrapper<CoordinatedBackingStoreProxyTile>> CoordinatedBackingStoreProxy::dirtyTiles()
{
    Vector<std::reference_wrapper<CoordinatedBackingStoreProxyTile>> tiles;
    for (auto& tile : m_tiles.values()) {
        if (tile->isDirty())
            tiles.append(*tile);
    }

    return tiles;
}

double CoordinatedBackingStoreProxy::tileDistance(const IntRect& viewport, const IntPoint& tilePosition) const
{
    if (viewport.intersects(tileRectForPosition(tilePosition)))
        return 0;

    IntPoint viewCenter = viewport.location() + IntSize(viewport.width() / 2, viewport.height() / 2);
    IntPoint centerPosition = tilePositionForPoint(viewCenter);

    return std::max(std::abs(centerPosition.y() - tilePosition.y()), std::abs(centerPosition.x() - tilePosition.x()));
}

void CoordinatedBackingStoreProxy::createTiles(const IntRect& visibleRect, const IntRect& scaledContentsRect, float coverAreaMultiplier)
{
    // Update our backing store geometry.
    m_contentsRect = scaledContentsRect;
    m_visibleRect = visibleRect;
    m_coverAreaMultiplier = coverAreaMultiplier;

    if (m_contentsRect.isEmpty()) {
        setCoverRect(IntRect());
        setKeepRect(IntRect());
        return;
    }

    /* We must compute cover and keep rects using the visibleRect, instead of the rect intersecting the visibleRect with m_contentsRect,
     * because TBS can be used as a backing store of GraphicsLayer and the visible rect usually does not intersect with m_contentsRect.
     * In the below case, the intersecting rect is an empty.
     *
     *  +----------------+
     *  |                |
     *  | m_contentsRect |
     *  |       +--------|----------------------+
     *  |       | HERE   |  cover or keep       |
     *  +----------------+      rect            |
     *          |         +---------+           |
     *          |         | visible |           |
     *          |         |  rect   |           |
     *          |         +---------+           |
     *          |                               |
     *          |                               |
     *          +-------------------------------+
     *
     * We must create or keep the tiles in the HERE region.
     */

    IntRect coverRect;
    IntRect keepRect;
    computeCoverAndKeepRect(m_visibleRect, coverRect, keepRect);

    setCoverRect(coverRect);
    setKeepRect(keepRect);

    if (coverRect.isEmpty())
        return;

    // Resize tiles at the edge in case the contents size has changed, but only do so
    // after having dropped tiles outside the keep rect.
    if (m_previousContentsRect != m_contentsRect) {
        m_previousContentsRect = m_contentsRect;
        resizeEdgeTiles();
    }

    // Search for the tile position closest to the viewport center that does not yet contain a tile.
    // Which position is considered the closest depends on the tileDistance function.
    double shortestDistance = std::numeric_limits<double>::infinity();
    Vector<IntPoint> tilesToCreate;
    unsigned requiredTileCount = 0;

    // Cover areas (in tiles) with minimum distance from the visible rect. If the visible rect is
    // not covered already it will be covered first in one go, due to the distance being 0 for tiles
    // inside the visible rect.
    auto topLeft = tilePositionForPoint(m_coverRect.location());
    auto bottomRight = tilePositionForPoint(innerBottomRight(m_coverRect));
    for (int y = topLeft.y(); y <= bottomRight.y(); ++y) {
        for (int x = topLeft.x(); x <= bottomRight.x(); ++x) {
            IntPoint position(x, y);
            if (m_tiles.contains(position))
                continue;

            ++requiredTileCount;
            double distance = tileDistance(m_visibleRect, position);
            if (distance > shortestDistance)
                continue;

            if (distance < shortestDistance) {
                tilesToCreate.clear();
                shortestDistance = distance;
            }
            tilesToCreate.append(position);
        }
    }

    // Now construct the tile(s) within the shortest distance.
    unsigned tilesToCreateCount = tilesToCreate.size();
    for (const auto& position : tilesToCreate)
        m_tiles.add(position, makeUnique<CoordinatedBackingStoreProxyTile>(*this, position, tileRectForPosition(position)));
    requiredTileCount -= tilesToCreateCount;

    // Re-call createTiles on a timer to cover the visible area with the newest shortest distance.
    m_pendingTileCreation = requiredTileCount;
    if (m_pendingTileCreation)
        m_client.tiledBackingStoreHasPendingTileCreation();
}

void CoordinatedBackingStoreProxy::adjustForContentsRect(IntRect& rect) const
{
    IntRect bounds = m_contentsRect;
    IntSize candidateSize = rect.size();

    rect.intersect(bounds);

    if (rect.size() == candidateSize)
        return;

    /*
     * In the following case, there is no intersection of the contents rect and the cover rect.
     * Thus the latter should not be inflated.
     *
     *  +----------------+
     *  | m_contentsRect |
     *  +----------------+
     *
     *          +-------------------------------+
     *          |          cover rect           |
     *          |         +---------+           |
     *          |         | visible |           |
     *          |         |  rect   |           |
     *          |         +---------+           |
     *          +-------------------------------+
     */
    if (rect.isEmpty())
        return;

    // Try to create a cover rect of the same size as the candidate, but within content bounds.
    int pixelsCovered = 0;
    if (!WTF::safeMultiply(candidateSize.width(), candidateSize.height(), pixelsCovered))
        pixelsCovered = std::numeric_limits<int>::max();

    if (rect.width() < candidateSize.width())
        rect.inflateY(((pixelsCovered / rect.width()) - rect.height()) / 2);
    if (rect.height() < candidateSize.height())
        rect.inflateX(((pixelsCovered / rect.height()) - rect.width()) / 2);

    rect.intersect(bounds);
}

void CoordinatedBackingStoreProxy::computeCoverAndKeepRect(const IntRect& visibleRect, IntRect& coverRect, IntRect& keepRect) const
{
    coverRect = visibleRect;
    keepRect = visibleRect;

    // If we cover more that the actual viewport we can be smart about which tiles we choose to render.
    if (m_coverAreaMultiplier > 1) {
        // The initial cover area covers equally in each direction, according to the coverAreaMultiplier.
        coverRect.inflateX(visibleRect.width() * (m_coverAreaMultiplier - 1) / 2);
        coverRect.inflateY(visibleRect.height() * (m_coverAreaMultiplier - 1) / 2);
        keepRect = coverRect;
        ASSERT(keepRect.contains(coverRect));
    }

    adjustForContentsRect(coverRect);

    // The keep rect is an inflated version of the cover rect, inflated in tile dimensions.
    keepRect.unite(coverRect);
    keepRect.inflateX(m_tileSize.width() / 2);
    keepRect.inflateY(m_tileSize.height() / 2);
    keepRect.intersect(m_contentsRect);

    ASSERT(coverRect.isEmpty() || keepRect.contains(coverRect));
}

void CoordinatedBackingStoreProxy::resizeEdgeTiles()
{
    Vector<IntPoint> tilesToRemove;
    for (auto& tile : m_tiles.values()) {
        IntPoint tilePosition = tile->position();
        IntRect tileRect = tile->rect();
        IntRect expectedTileRect = tileRectForPosition(tilePosition);
        if (expectedTileRect.isEmpty())
            tilesToRemove.append(tilePosition);
        else if (expectedTileRect != tileRect)
            tile->resize(expectedTileRect.size());
    }

    for (auto& positionToRemove : tilesToRemove)
        m_tiles.remove(positionToRemove);
}

void CoordinatedBackingStoreProxy::setKeepRect(const IntRect& keepRect)
{
    // Drop tiles outside the new keepRect.

    FloatRect keepRectF = keepRect;

    Vector<IntPoint> toRemove;
    for (auto& tile : m_tiles.values()) {
        FloatRect tileRect = tile->rect();
        if (!tileRect.intersects(keepRectF))
            toRemove.append(tile->position());
    }

    for (auto& positionToRemove : toRemove)
        m_tiles.remove(positionToRemove);

    m_keepRect = keepRect;
}

IntRect CoordinatedBackingStoreProxy::mapToContents(const IntRect& rect) const
{
    return enclosingIntRect(FloatRect(rect.x() / m_contentsScale,
        rect.y() / m_contentsScale,
        rect.width() / m_contentsScale,
        rect.height() / m_contentsScale));
}

IntRect CoordinatedBackingStoreProxy::mapFromContents(const IntRect& rect) const
{
    return enclosingIntRect(FloatRect(rect.x() * m_contentsScale,
        rect.y() * m_contentsScale,
        rect.width() * m_contentsScale,
        rect.height() * m_contentsScale));
}

IntRect CoordinatedBackingStoreProxy::tileRectForPosition(const IntPoint& position) const
{
    IntRect rect(position.x() * m_tileSize.width(),
        position.y() * m_tileSize.height(),
        m_tileSize.width(),
        m_tileSize.height());

    rect.intersect(m_contentsRect);
    return rect;
}

IntPoint CoordinatedBackingStoreProxy::tilePositionForPoint(const IntPoint& point) const
{
    int x = point.x() / m_tileSize.width();
    int y = point.y() / m_tileSize.height();
    return IntPoint(std::max(x, 0), std::max(y, 0));
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
