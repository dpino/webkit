/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2005, 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2009 Jeff Schiller <codedread@gmail.com>
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
 * Copyright (C) 2011 University of Szeged
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
#include "LegacyRenderSVGPath.h"

#include "Gradient.h"
#include "LegacyRenderSVGShapeInlines.h"
#if USE(SKIA)
#include "PathSkia.h"
#endif
#include "SVGElementTypeHelpers.h"
#include "SVGPathElement.h"
#include "SVGPointList.h"
#include "SVGPolylineElement.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGSubpathData.h"
#include "StyleComputedStyle+GettersInlines.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(LegacyRenderSVGPath);

LegacyRenderSVGPath::LegacyRenderSVGPath(SVGGraphicsElement& element, Style::ComputedStyle&& style)
    : LegacyRenderSVGShape(Type::LegacySVGPath, element, WTF::move(style))
{
    ASSERT(isLegacyRenderSVGPath());
}

LegacyRenderSVGPath::~LegacyRenderSVGPath() = default;

void LegacyRenderSVGPath::updateShapeFromElement()
{
    clearPath();
    m_shapeType = ShapeType::Empty;
    m_fillBoundingBox = ensurePath().boundingRect();
    m_strokeBoundingBox = std::nullopt;
    m_approximateStrokeBoundingBox = std::nullopt;
    // Remember the points of a polyline, to find out later which of them changed.
    m_polylinePointsIdentifier = 0;
    if (RefPtr polyline = dynamicDowncast<SVGPolylineElement>(graphicsElement())) {
        m_polylinePoints = std::as_const(*polyline).points().items().map([](auto& point) { return point->value(); });
        if (!polyline->isAnimatingPoints()) {
            // The changes the point list recorded so far are part of the remembered points.
            auto& pointList = polyline->points();
            pointList.takeChangedItemRange();
            m_polylinePointsIdentifier = pointList.itemsIdentifier();
        }
    } else
        m_polylinePoints.clear();
    processMarkerPositions();
    updateZeroLengthSubpaths();

    ASSERT(hasPath());
    if (path().isEmpty())
        return;
    if (path().definitelySingleLine())
        m_shapeType = ShapeType::Line;
    else
        m_shapeType = ShapeType::Path;

    // FIXME: This should not exist. However, currently SVG is relying on ordering of calculation of SVG2 strokeBoundingBox via layout() function
    // for recursive SVGs (markers are pointing to each other recursively). If we move to SVG2 computation, we no longer need this since SVG2 strokeBoundingBox
    // does not include markers rect (so we do not need to have this in LSBE). Right now, this exists only for RepaintRectCalculation::Accurate, and it should be removed once
    // 1. We fix checkInsertion / checkEnclosure implementations. Currently they are not aligned to what the spec requires.
    // 2. We move our RenderTreeAsText to avoid dumping Accurate repaint rect. We should dump strokeBoundingBox or actual repaintBoundingBox instead.
    // We fall back to path-based eager strokeBoundingBox only when there are markers and it is not SVG2.
    // There are several cases we use approximate repaintBoundingBox. But only LegacyRenderSVGPath can reference to the other approximate repaintBoundingBox via markers.
    // The other resources including maskers, clippers etc. are already computing bounding rect via Accurate eagerly. So they do not matter.
    // https://bugs.webkit.org/show_bug.cgi?id=263348
    if (!m_markerPositions.isEmpty())
        strokeBoundingBox();
}

static FloatRect boundingBoxOfPoints(const Vector<FloatPoint>& points, size_t startIndex, size_t endIndex)
{
    float minX = points[startIndex].x();
    float minY = points[startIndex].y();
    float maxX = minX;
    float maxY = minY;
    for (size_t i = startIndex + 1; i <= endIndex; ++i) {
        minX = std::min(minX, points[i].x());
        minY = std::min(minY, points[i].y());
        maxX = std::max(maxX, points[i].x());
        maxY = std::max(maxY, points[i].y());
    }
    return { minX, minY, maxX - minX, maxY - minY };
}

static Path polylinePath(const Vector<FloatPoint>& points)
{
#if USE(SKIA)
    // Build the Skia path right away, instead of path segments that would be converted into it whenever the
    // polyline is painted. A polyline with two points keeps its single segment, so it is still recognized as a line.
    if (points.size() > 2) {
        SkPathBuilder builder;
        builder.moveTo(SkFloatToScalar(points[0].x()), SkFloatToScalar(points[0].y()));
        for (size_t i = 1; i < points.size(); ++i)
            builder.lineTo(SkFloatToScalar(points[i].x()), SkFloatToScalar(points[i].y()));
        return Path { PathSkia::create(WTF::move(builder)) };
    }
#endif

    Vector<PathSegment> segments;
    segments.reserveInitialCapacity(points.size());
    segments.append(PathSegment { PathMoveTo { points[0] } });
    for (size_t i = 1; i < points.size(); ++i)
        segments.append(PathSegment { PathLineTo { points[i] } });
    return Path { WTF::move(segments) };
}

std::optional<FloatRect> LegacyRenderSVGPath::updateChangedShapeFromElement()
{
    RefPtr polyline = dynamicDowncast<SVGPolylineElement>(graphicsElement());
    if (!polyline || !hasPath() || m_polylinePoints.size() < 2 || shouldGenerateMarkerPositions())
        return std::nullopt;

    auto& points = std::as_const(*polyline).points().items();
    size_t oldPointCount = m_polylinePoints.size();
    size_t newPointCount = points.size();
    if (newPointCount < oldPointCount)
        return std::nullopt;

    // The point list records which points were replaced or appended, as long as no points were inserted or removed.
    // Animations change the animated point list without recording it, so all points are compared while animating.
    size_t firstIndexToCompare = 0;
    size_t endIndexToCompare = oldPointCount;
    if (polyline->isAnimatingPoints()) {
        if (m_polylinePointsIdentifier)
            return std::nullopt;
    } else {
        auto& pointList = polyline->points();
        if (!m_polylinePointsIdentifier || pointList.itemsIdentifier() != m_polylinePointsIdentifier)
            return std::nullopt;
        auto changedItemRange = pointList.takeChangedItemRange();
        if (!changedItemRange)
            return FloatRect { };
        firstIndexToCompare = std::min<size_t>(changedItemRange->first, oldPointCount);
        endIndexToCompare = std::min<size_t>(changedItemRange->last + 1, oldPointCount);
    }

    // Find the range of points that were replaced or appended.
    std::optional<size_t> firstChangedIndex;
    size_t lastChangedIndex = 0;
    for (size_t i = firstIndexToCompare; i < endIndexToCompare; ++i) {
        if (points[i]->value() == m_polylinePoints[i])
            continue;
        if (!firstChangedIndex)
            firstChangedIndex = i;
        lastChangedIndex = i;
    }
    if (newPointCount > oldPointCount) {
        if (!firstChangedIndex)
            firstChangedIndex = oldPointCount;
        lastChangedIndex = newPointCount - 1;
    }
    if (!firstChangedIndex)
        return FloatRect { };

    // The lines and joins next to a changed point render differently as well.
    size_t startIndex = *firstChangedIndex ? *firstChangedIndex - 1 : 0;
    auto changedGeometryBoundingBox = boundingBoxOfPoints(m_polylinePoints, startIndex, std::min(lastChangedIndex + 1, oldPointCount - 1));

    // Replaced points shrink the bounding box of the shape, if they were the ones it touched. Appended points can only extend it.
    bool mayShrinkFillBoundingBox = false;
    if (*firstChangedIndex < oldPointCount) {
        auto replacedBoundingBox = boundingBoxOfPoints(m_polylinePoints, *firstChangedIndex, std::min(lastChangedIndex, oldPointCount - 1));
        mayShrinkFillBoundingBox = replacedBoundingBox.x() <= m_fillBoundingBox.x() || replacedBoundingBox.y() <= m_fillBoundingBox.y()
            || replacedBoundingBox.maxX() >= m_fillBoundingBox.maxX() || replacedBoundingBox.maxY() >= m_fillBoundingBox.maxY();
    }

    m_polylinePoints.resize(newPointCount);
    for (size_t i = *firstChangedIndex; i <= lastChangedIndex; ++i)
        m_polylinePoints[i] = points[i]->value();
    auto newGeometryBoundingBox = boundingBoxOfPoints(m_polylinePoints, startIndex, std::min(lastChangedIndex + 1, newPointCount - 1));
    changedGeometryBoundingBox.uniteEvenIfEmpty(newGeometryBoundingBox);

    if (*firstChangedIndex == oldPointCount) {
        for (size_t i = oldPointCount; i < newPointCount; ++i)
            path().addLineTo(m_polylinePoints[i]);
    } else {
        // Replace the changed points of the existing path, which not every kind of path can do.
        bool updatedPathInPlace = true;
        for (size_t i = *firstChangedIndex; i <= lastChangedIndex && updatedPathInPlace; ++i) {
            if (i < oldPointCount)
                updatedPathInPlace = path().setPointAtIndex(i, m_polylinePoints[i]);
            else
                path().addLineTo(m_polylinePoints[i]);
        }
        if (!updatedPathInPlace)
            setPath(polylinePath(m_polylinePoints));
    }

    if (mayShrinkFillBoundingBox)
        m_fillBoundingBox = boundingBoxOfPoints(m_polylinePoints, 0, newPointCount - 1);
    else
        m_fillBoundingBox.uniteEvenIfEmpty(newGeometryBoundingBox);
    m_strokeBoundingBox = std::nullopt;
    m_approximateStrokeBoundingBox = std::nullopt;

    // A polyline is a single subpath. It has a non-zero length once two of its points differ.
    if (m_fillBoundingBox.width() || m_fillBoundingBox.height())
        m_zeroLengthLinecapLocations.clear();
    else
        updateZeroLengthSubpaths();

    m_shapeType = path().definitelySingleLine() ? ShapeType::Line : ShapeType::Path;
    return changedGeometryBoundingBox;
}

FloatRect LegacyRenderSVGPath::adjustStrokeBoundingBoxForMarkersAndZeroLengthLinecaps(RepaintRectCalculation repaintRectCalculation, FloatRect strokeBoundingBox) const
{
    bool hasMarkers = !m_markerPositions.isEmpty();
    bool hasZeroLengthCaps = !style().stroke().isNone() && !m_zeroLengthLinecapLocations.isEmpty();
    if (!hasMarkers && !hasZeroLengthCaps)
        return strokeBoundingBox;

    float strokeWidth = this->strokeWidth();

    if (hasMarkers) {
        auto markerRect = this->markerRect(repaintRectCalculation, strokeWidth);
        if (!markerRect.isNaN())
            strokeBoundingBox.unite(markerRect);
    }

    if (hasZeroLengthCaps) {
        // FIXME: zero-length subpaths do not respect vector-effect = non-scaling-stroke.
        for (auto& zeroLengthLinecapLocation : m_zeroLengthLinecapLocations) {
            auto subpathRect = zeroLengthSubpathRect(zeroLengthLinecapLocation, strokeWidth);
            if (!subpathRect.isNaN())
                strokeBoundingBox.unite(subpathRect);
        }
    }

    return strokeBoundingBox;
}

static void legacyUseStrokeStyleToFill(GraphicsContext& context)
{
    if (RefPtr gradient = context.strokeGradient())
        context.setFillGradient(*gradient, context.strokeGradientSpaceTransform());
    else if (RefPtr pattern = context.strokePattern())
        context.setFillPattern(*pattern);
    else
        context.setFillColor(context.strokeColor());
}

void LegacyRenderSVGPath::strokeShape(GraphicsContext& context) const
{
    if (style().stroke().isNone() || !style().strokeWidth().isPossiblyPositive())
        return;

    // This happens only if the layout was never been called for this element.
    if (!hasPath())
        return;

    LegacyRenderSVGShape::strokeShape(context);
    strokeZeroLengthSubpaths(context);
}

bool LegacyRenderSVGPath::shapeDependentStrokeContains(const FloatPoint& point, PointCoordinateSpace pointCoordinateSpace)
{
    if (LegacyRenderSVGShape::shapeDependentStrokeContains(point, pointCoordinateSpace))
        return true;

    ASSERT(m_zeroLengthLinecapLocations.isEmpty() || !style().stroke().isNone());
    float strokeWidth = this->strokeWidth();
    bool isSquareCap = style().capStyle() == LineCap::Square;
    for (auto& linecapLocation : m_zeroLengthLinecapLocations) {
        if (isSquareCap) {
            if (zeroLengthSubpathRect(linecapLocation, strokeWidth).contains(point))
                return true;
        } else {
            ASSERT(style().capStyle() == LineCap::Round);
            FloatPoint radiusVector(point.x() - linecapLocation.x(), point.y() - linecapLocation.y());
            if (radiusVector.lengthSquared() < strokeWidth * strokeWidth * .25f)
                return true;
        }
    }
    return false;
}

bool LegacyRenderSVGPath::shouldStrokeZeroLengthSubpath() const
{
    // Spec(11.4): Any zero length subpath shall not be stroked if the "stroke-linecap" property has a value of butt
    // but shall be stroked if the "stroke-linecap" property has a value of round or square
    return !style().stroke().isNone() && style().capStyle() != LineCap::Butt;
}

FloatRect LegacyRenderSVGPath::zeroLengthSubpathRect(const FloatPoint& linecapPosition, float strokeWidth) const
{
    return FloatRect(linecapPosition.x() - strokeWidth / 2, linecapPosition.y() - strokeWidth / 2, strokeWidth, strokeWidth);
}

void LegacyRenderSVGPath::updateZeroLengthSubpaths()
{
    m_zeroLengthLinecapLocations.clear();

    if (!strokeWidth() || !shouldStrokeZeroLengthSubpath())
        return;

    SVGSubpathData subpathData(m_zeroLengthLinecapLocations);
    path().applyElements([&subpathData](const PathElement& pathElement) {
        SVGSubpathData::updateFromPathElement(subpathData, pathElement);
    });
    subpathData.pathIsDone();
}

void LegacyRenderSVGPath::strokeZeroLengthSubpaths(GraphicsContext& context) const
{
    if (m_zeroLengthLinecapLocations.isEmpty())
        return;

    AffineTransform nonScalingTransform;
    if (hasNonScalingStroke())
        nonScalingTransform = nonScalingStrokeTransform();

    GraphicsContextStateSaver stateSaver(context, true);
    legacyUseStrokeStyleToFill(context);

    float strokeWidth = this->strokeWidth();
    bool isSquareCap = style().capStyle() == LineCap::Square;
    for (auto& linecapLocation : m_zeroLengthLinecapLocations) {
        // The linecap location is path geometry, not stroke geometry. So when
        // vector-effect: non-scaling-stroke is in effect, the transform must be
        // applied to the position where the cap is drawn -- not to the generated
        // cap shape, which would otherwise be distorted by the transform.
        auto position = hasNonScalingStroke() ? nonScalingTransform.mapPoint(linecapLocation) : linecapLocation;
        auto subpathRect = zeroLengthSubpathRect(position, strokeWidth);
        if (isSquareCap)
            context.fillRect(subpathRect);
        else
            context.fillEllipse(subpathRect);
    }
}

static inline LegacyRenderSVGResourceMarker* NODELETE markerForType(SVGMarkerType type, LegacyRenderSVGResourceMarker* markerStart, LegacyRenderSVGResourceMarker* markerMid, LegacyRenderSVGResourceMarker* markerEnd)
{
    switch (type) {
    case SVGMarkerType::Start:
        return markerStart;
    case SVGMarkerType::Middle:
        return markerMid;
    case SVGMarkerType::End:
        return markerEnd;
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

bool LegacyRenderSVGPath::shouldGenerateMarkerPositions() const
{
    if (!style().hasMarkers())
        return false;

    if (!protect(graphicsElement())->supportsMarkers())
        return false;

    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(*this);
    if (!resources)
        return false;

    return resources->markerStart() || resources->markerMid() || resources->markerEnd();
}

void LegacyRenderSVGPath::drawMarkers(PaintInfo& paintInfo)
{
    if (m_markerPositions.isEmpty())
        return;

    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(*this);
    if (!resources)
        return;

    LegacyRenderSVGResourceMarker* markerStart = resources->markerStart();
    LegacyRenderSVGResourceMarker* markerMid = resources->markerMid();
    LegacyRenderSVGResourceMarker* markerEnd = resources->markerEnd();
    if (!markerStart && !markerMid && !markerEnd)
        return;

    float strokeWidth = this->strokeWidthForMarkerUnits();
    unsigned size = m_markerPositions.size();
    for (unsigned i = 0; i < size; ++i) {
        if (auto* marker = markerForType(m_markerPositions[i].type, markerStart, markerMid, markerEnd)) {
            auto& context = paintInfo.context();
            GraphicsContextStateSaver stateSaver(context);

            context.setLineDash(DashArray(), 0);
            marker->draw(paintInfo, marker->markerTransformation(m_markerPositions[i].origin, m_markerPositions[i].angle, strokeWidth));
        }
    }
}

FloatRect LegacyRenderSVGPath::markerRect(RepaintRectCalculation repaintRectCalculation, float strokeWidth) const
{
    ASSERT(!m_markerPositions.isEmpty());

    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(*this);
    ASSERT(resources);

    auto* markerStart = resources->markerStart();
    auto* markerMid = resources->markerMid();
    auto* markerEnd = resources->markerEnd();
    ASSERT(markerStart || markerMid || markerEnd);

    FloatRect boundaries;
    unsigned size = m_markerPositions.size();
    for (unsigned i = 0; i < size; ++i) {
        if (auto* marker = markerForType(m_markerPositions[i].type, markerStart, markerMid, markerEnd))
            boundaries.unite(marker->markerBoundaries(repaintRectCalculation, marker->markerTransformation(m_markerPositions[i].origin, m_markerPositions[i].angle, strokeWidth)));
    }
    return boundaries;
}

void LegacyRenderSVGPath::processMarkerPositions()
{
    m_markerPositions.clear();

    if (!shouldGenerateMarkerPositions())
        return;

    ASSERT(hasPath());

    SVGMarkerData markerData(m_markerPositions, SVGResourcesCache::cachedResourcesForRenderer(*this)->markerReverseStart());
    path().applyElements([&markerData](const PathElement& pathElement) {
        SVGMarkerData::updateFromPathElement(markerData, pathElement);
    });
    markerData.pathIsDone();
}

bool LegacyRenderSVGPath::isRenderingDisabled() const
{
    // For a polygon, polyline or path, rendering is disabled if there is no path data.
    // No path data is possible in the case of a missing or empty 'd' or 'points' attribute.
    return !hasPath() || path().isEmpty();
}

void LegacyRenderSVGPath::styleDidChange(Style::Difference diff, const Style::ComputedStyle* oldStyle)
{
    if (oldStyle && oldStyle->hasMarkers() && !style().hasMarkers())
        m_markerPositions.clear();
    if (RefPtr pathElement = dynamicDowncast<SVGPathElement>(graphicsElement())) {
        if (!oldStyle || style().d() != oldStyle->d())
            pathElement->pathDidChange();
    }

    LegacyRenderSVGShape::styleDidChange(diff, oldStyle);
}

}
