/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(SAFARI_THEME)

#include "PlatformScrollBar.h"

#include "ChromeClient.h"
#include "EventHandler.h"
#include "Frame.h"
#include "FrameView.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "Settings.h"
#include "SoftLinking.h"

#include <CoreGraphics/CoreGraphics.h>

// If you have an empty placeholder SafariThemeConstants.h, then include SafariTheme.h
// This is a workaround until a version of WebKitSupportLibrary is released with an updated SafariThemeConstants.h 
#include <SafariTheme/SafariThemeConstants.h>
#ifndef SafariThemeConstants_h
#include <SafariTheme/SafariTheme.h>
#endif

// FIXME: There are repainting problems due to Aqua scroll bar buttons' visual overflow.

using namespace std;

namespace WebCore {

using namespace SafariTheme;

// FIXME: We should get these numbers from SafariTheme
static int cHorizontalWidth[] = { 15, 11 };
static int cHorizontalHeight[] = { 15, 11 };
static int cVerticalWidth[] = { 15, 11 };
static int cVerticalHeight[] = { 15, 11 };
static int cRealButtonLength[] = { 28, 21 };
static int cButtonInset[] = { 14, 11 };
static int cButtonHitInset[] = { 3, 2 };
// cRealButtonLength - cButtonInset
static int cButtonLength[] = { 14, 10 };
static int cThumbWidth[] = { 15, 11 };
static int cThumbHeight[] = { 15, 11 };
static int cThumbMinLength[] = { 26, 20 };

#if !defined(NDEBUG) && defined(USE_DEBUG_SAFARI_THEME)
SOFT_LINK_DEBUG_LIBRARY(SafariTheme)
#else
SOFT_LINK_LIBRARY(SafariTheme)
#endif

SOFT_LINK(SafariTheme, paintThemePart, void, __stdcall, 
            (ThemePart part, CGContextRef context, const CGRect& rect, NSControlSize size, ThemeControlState state),
            (part, context, rect, size, state))

const double cInitialTimerDelay = 0.25;
const double cNormalTimerDelay = 0.05;

static ScrollbarControlState ScrollbarControlStateFromThemeState(ThemeControlState state)
{
    ScrollbarControlState s = 0;
    if (state & ActiveState)
        s |= ActiveScrollbarState;
    if (state & EnabledState)
        s |= EnabledScrollbarState;
    if (state & PressedState)
        s |= PressedScrollbarState;
    return s;
}

static Page* pageForScrollView(ScrollView* view)
{
    if (!view)
        return 0;
    if (!view->isFrameView())
        return 0;
    FrameView* frameView = static_cast<FrameView*>(view);
    if (!frameView->frame())
        return 0;
    return frameView->frame()->page();
}

PlatformScrollbar::PlatformScrollbar(ScrollbarClient* client, ScrollbarOrientation orientation, ScrollbarControlSize size)
    : Scrollbar(client, orientation, size)
{
    // Obtain the correct scrollbar sizes from the system.
    if (!cHorizontalWidth) {
        // FIXME: Get metics from SafariTheme
    }

    if (orientation == VerticalScrollbar)
        setFrameGeometry(IntRect(0, 0, cVerticalWidth[controlSize()], cVerticalHeight[controlSize()]));
    else
        setFrameGeometry(IntRect(0, 0, cHorizontalWidth[controlSize()], cHorizontalHeight[controlSize()]));
}

PlatformScrollbar::~PlatformScrollbar()
{
}

void PlatformScrollbar::updateThumbPosition()
{
    invalidateTrack();
}

void PlatformScrollbar::updateThumbProportion()
{
    invalidateTrack();
}

static IntRect trackRepaintRect(const IntRect& trackRect, ScrollbarOrientation orientation, ScrollbarControlSize controlSize)
{
    IntRect paintRect(trackRect);
    if (orientation == HorizontalScrollbar)
        paintRect.inflateX(cButtonLength[controlSize]);
    else
        paintRect.inflateY(cButtonLength[controlSize]);

    return paintRect;
}

static IntRect buttonRepaintRect(const IntRect& buttonRect, ScrollbarOrientation orientation, ScrollbarControlSize controlSize, bool start)
{
    IntRect paintRect(buttonRect);
    if (orientation == HorizontalScrollbar) {
        paintRect.setWidth(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setX(buttonRect.x() - (cRealButtonLength[controlSize] - buttonRect.width()));
    } else {
        paintRect.setHeight(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setY(buttonRect.y() - (cRealButtonLength[controlSize] - buttonRect.height()));
    }

    return paintRect;
}

void PlatformScrollbar::invalidateTrack()
{
    IntRect rect = trackRepaintRect(trackRect(), m_orientation, controlSize());
    rect.move(-x(), -y());
    invalidateRect(rect);
}

void PlatformScrollbar::invalidatePart(ScrollbarPart part)
{
    if (part == NoPart)
        return;

    IntRect result;    
    switch (part) {
        case BackButtonPart:
            result = buttonRepaintRect(backButtonRect(), m_orientation, controlSize(), true);
            break;
        case ForwardButtonPart:
            result = buttonRepaintRect(forwardButtonRect(), m_orientation, controlSize(), false);
            break;
        default: {
            IntRect beforeThumbRect, thumbRect, afterThumbRect;
            splitTrack(trackRect(), beforeThumbRect, thumbRect, afterThumbRect);
            if (part == BackTrackPart)
                result = beforeThumbRect;
            else if (part == ForwardTrackPart)
                result = afterThumbRect;
            else
                result = thumbRect;
        }
    }
    result.move(-x(), -y());
    invalidateRect(result);
}

void PlatformScrollbar::setRect(const IntRect& rect)
{
    // Get our window resizer rect and see if we overlap.  Adjust to avoid the overlap
    // if necessary.
    IntRect adjustedRect(rect);
    if (parent() && parent()->isFrameView()) {
        bool overlapsResizer = false;
        FrameView* view = static_cast<FrameView*>(parent());
        IntRect resizerRect = view->windowResizerRect();
        resizerRect.setLocation(view->convertFromContainingWindow(resizerRect.location()));
        if (rect.intersects(resizerRect)) {
            if (orientation() == HorizontalScrollbar) {
                int overlap = rect.right() - resizerRect.x();
                if (overlap > 0 && resizerRect.right() >= rect.right()) {
                    adjustedRect.setWidth(rect.width() - overlap);
                    overlapsResizer = true;
                }
            } else {
                int overlap = rect.bottom() - resizerRect.y();
                if (overlap > 0 && resizerRect.bottom() >= rect.bottom()) {
                    adjustedRect.setHeight(rect.height() - overlap);
                    overlapsResizer = true;
                }
            }
        }

        if (overlapsResizer != m_overlapsResizer) {
            m_overlapsResizer = overlapsResizer;
            view->adjustOverlappingScrollbarCount(m_overlapsResizer ? 1 : -1);
        }
    }

    setFrameGeometry(adjustedRect);
}

void PlatformScrollbar::setParent(ScrollView* parentView)
{
    if (!parentView && m_overlapsResizer && parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->adjustOverlappingScrollbarCount(-1);
    Widget::setParent(parentView);
}

void PlatformScrollbar::paint(GraphicsContext* graphicsContext, const IntRect& damageRect)
{
    if (graphicsContext->updatingControlTints()) {
        invalidate();
        return;
    }

    if (graphicsContext->paintingDisabled())
        return;

    // Don't paint anything if the scrollbar doesn't intersect the damage rect.
    if (!frameGeometry().intersects(damageRect))
        return;

    // Create the ScrollbarControlPartMask based on the damageRect
    ScrollbarControlPartMask scrollMask = NoPart;
 
    IntRect backButtonPaintRect;
    IntRect forwardButtonPaintRect;
    if (hasButtons()) {
        backButtonPaintRect = buttonRepaintRect(backButtonRect(), m_orientation, controlSize(), true);
        if (damageRect.intersects(backButtonPaintRect))
            scrollMask |= BackButtonPart;
        forwardButtonPaintRect = buttonRepaintRect(forwardButtonRect(), m_orientation, controlSize(), false);
        if (damageRect.intersects(forwardButtonPaintRect))
            scrollMask |= ForwardButtonPart;
    }

    IntRect track = trackRect();
    IntRect trackPaintRect = hasButtons() ? trackRepaintRect(track, m_orientation, controlSize()) : track;
    IntRect startTrackRect, thumbRect, endTrackRect;
    if (hasThumb()) {
        splitTrack(track, startTrackRect, thumbRect, endTrackRect);
        if (damageRect.intersects(thumbRect))
            scrollMask |= ThumbPart;
        if (damageRect.intersects(startTrackRect))
            scrollMask |= BackTrackPart;
        if (damageRect.intersects(endTrackRect))
            scrollMask |= ForwardTrackPart;
    } else if (damageRect.intersects(trackPaintRect)) {
        scrollMask |= BackTrackPart;
        scrollMask |= ForwardTrackPart;
    }

    NSControlSize size = controlSize() == SmallScrollbar ? NSSmallControlSize : NSRegularControlSize;
    ThemeControlState state = 0;
    if (m_client->isActive())
        state |= ActiveState;
    if (hasButtons())
        state |= EnabledState;

    float proportion = static_cast<float>(m_visibleSize) / m_totalSize;
    float value = static_cast<float>(m_currentPos) / static_cast<float>(m_totalSize - m_visibleSize);

    if (Page* page = pageForScrollView(parent())) {
        if (page->settings()->shouldPaintCustomScrollbars()) {
            if (page->chrome()->client()->paintCustomScrollbar(graphicsContext, frameGeometry(), 
                    controlSize(), ScrollbarControlStateFromThemeState(state), m_pressedPart, 
                    m_orientation == VerticalScrollbar, value, proportion, scrollMask))
                        return;
        }
    }

    if (!SafariThemeLibrary())
        return;

    // SafariTheme needs to repaint the track when painting the thumb so that the top and bottom of the thumb look right
    if (scrollMask & ThumbPart) {
        scrollMask |= BackTrackPart;
        scrollMask |= ForwardTrackPart;
    }

    // FIXME: We should switch to use STPaintScrollBar instead of paintThemePart.  Currently, STPaintScrollBar doesn't work correctly with the pressed buttons
    // when drawing from the part mask.
    if ((scrollMask & ForwardTrackPart) || (scrollMask & BackTrackPart))
        paintThemePart(m_orientation == VerticalScrollbar ? VScrollTrackPart : HScrollTrackPart, graphicsContext->platformContext(), trackPaintRect, size, state); 
    if (scrollMask & BackButtonPart)
        paintThemePart(m_orientation == VerticalScrollbar ? ScrollUpArrowPart : ScrollLeftArrowPart, graphicsContext->platformContext(),
                        backButtonPaintRect, size, m_pressedPart == BackButtonPart ? state | PressedState : state);
    if (scrollMask & ForwardButtonPart)
        paintThemePart(m_orientation == VerticalScrollbar ? ScrollDownArrowPart : ScrollRightArrowPart, graphicsContext->platformContext(),
                        forwardButtonPaintRect, size, m_pressedPart == ForwardButtonPart ? state | PressedState : state);
    if (scrollMask & ThumbPart)
        paintThemePart(m_orientation == VerticalScrollbar ? VScrollThumbPart : HScrollThumbPart, graphicsContext->platformContext(), 
                        thumbRect, size, m_pressedPart == ThumbPart ? state | PressedState : state);
}

bool PlatformScrollbar::hasButtons() const
{
    return isEnabled() && (m_orientation == HorizontalScrollbar ? width() : height()) >= 2 * (cRealButtonLength[controlSize()] - cButtonHitInset[controlSize()]);
}

bool PlatformScrollbar::hasThumb() const
{
    return isEnabled() && (m_orientation == HorizontalScrollbar ? width() : height()) >= 2 * cButtonInset[controlSize()] + cThumbMinLength[controlSize()] + 1;
}

IntRect PlatformScrollbar::backButtonRect() const
{
    // Our desired rect is essentially 17x17.
    
    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.
    if (m_orientation == HorizontalScrollbar)
        return IntRect(x(), y(), cButtonLength[controlSize()], cHorizontalHeight[controlSize()]);
    return IntRect(x(), y(), cVerticalWidth[controlSize()], cButtonLength[controlSize()]);
}

IntRect PlatformScrollbar::forwardButtonRect() const
{
    // Our desired rect is essentially 17x17.
    
    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.

    if (m_orientation == HorizontalScrollbar)
        return IntRect(x() + width() - cButtonLength[controlSize()], y(), cButtonLength[controlSize()], cHorizontalHeight[controlSize()]);
    return IntRect(x(), y() + height() - cButtonLength[controlSize()], cVerticalWidth[controlSize()], cButtonLength[controlSize()]);
}

IntRect PlatformScrollbar::trackRect() const
{
    if (m_orientation == HorizontalScrollbar) {
        if (!hasButtons())
            return IntRect(x(), y(), width(), cHorizontalHeight[controlSize()]);
        return IntRect(x() + cButtonLength[controlSize()], y(), width() - 2 * cButtonLength[controlSize()], cHorizontalHeight[controlSize()]);
    }

    if (!hasButtons())
        return IntRect(x(), y(), cVerticalWidth[controlSize()], height());
    return IntRect(x(), y() + cButtonLength[controlSize()], cVerticalWidth[controlSize()], height() - 2 * cButtonLength[controlSize()]);
}

IntRect PlatformScrollbar::thumbRect() const
{
    IntRect beforeThumbRect, thumbRect, afterThumbRect;
    splitTrack(trackRect(), beforeThumbRect, thumbRect, afterThumbRect);
    return thumbRect;
}

void PlatformScrollbar::splitTrack(const IntRect& trackRect, IntRect& beforeThumbRect, IntRect& thumbRect, IntRect& afterThumbRect) const
{
    // This function won't even get called unless we're big enough to have some combination of these three rects where at least
    // one of them is non-empty.
    int thumbPos = thumbPosition();
    if (m_orientation == HorizontalScrollbar) {
        thumbRect = IntRect(trackRect.x() + thumbPos, trackRect.y() + (trackRect.height() - cThumbHeight[controlSize()]) / 2, thumbLength(), cThumbHeight[controlSize()]);
        beforeThumbRect = IntRect(trackRect.x(), trackRect.y(), thumbPos, trackRect.height());
        afterThumbRect = IntRect(thumbRect.x() + thumbRect.width(), trackRect.y(), trackRect.right() - thumbRect.right(), trackRect.height());
    } else {
        thumbRect = IntRect(trackRect.x() + (trackRect.width() - cThumbWidth[controlSize()]) / 2, trackRect.y() + thumbPos, cThumbWidth[controlSize()], thumbLength());
        beforeThumbRect = IntRect(trackRect.x(), trackRect.y(), trackRect.width(), thumbPos);
        afterThumbRect = IntRect(trackRect.x(), thumbRect.y() + thumbRect.height(), trackRect.width(), trackRect.bottom() - thumbRect.bottom());
    }
}

int PlatformScrollbar::thumbPosition() const
{
    if (isEnabled())
        return (float)m_currentPos * (trackLength() - thumbLength()) / (m_totalSize - m_visibleSize);
    return 0;
}

int PlatformScrollbar::thumbLength() const
{
    if (!isEnabled())
        return 0;

    float proportion = (float)(m_visibleSize) / m_totalSize;
    int trackLen = trackLength();
    int length = proportion * trackLen;
    int minLength = cThumbMinLength[controlSize()];
    length = max(length, minLength);
    if (length > trackLen)
        length = 0; // Once the thumb is below the track length, it just goes away (to make more room for the track).
    return length;
}

int PlatformScrollbar::trackLength() const
{
    return (m_orientation == HorizontalScrollbar) ? trackRect().width() : trackRect().height();
}

ScrollbarPart PlatformScrollbar::hitTest(const PlatformMouseEvent& evt)
{
    if (!isEnabled())
        return NoPart;

    IntPoint mousePosition = convertFromContainingWindow(evt.pos());
    mousePosition.move(x(), y());

    if (hasButtons()) {
        if (backButtonRect().contains(mousePosition))
            return BackButtonPart;

        if (forwardButtonRect().contains(mousePosition))
            return ForwardButtonPart;
    }

    if (!hasThumb())
        return NoPart;

    IntRect track = trackRect();
    if (track.contains(mousePosition)) {
        IntRect beforeThumbRect, thumbRect, afterThumbRect;
        splitTrack(track, beforeThumbRect, thumbRect, afterThumbRect);
        if (beforeThumbRect.contains(mousePosition))
            return BackTrackPart;
        if (thumbRect.contains(mousePosition))
            return ThumbPart;
        return ForwardTrackPart;
    }
    return NoPart;
}

bool PlatformScrollbar::handleMouseMoveEvent(const PlatformMouseEvent& evt)
{
    if (m_pressedPart == ThumbPart) {
        // Drag the thumb.
        int thumbPos = thumbPosition();
        int thumbLen = thumbLength();
        int trackLen = trackLength();
        int maxPos = trackLen - thumbLen;
        int delta = 0;
        if (m_orientation == HorizontalScrollbar)
            delta = convertFromContainingWindow(evt.pos()).x() - m_pressedPos;
        else
            delta = convertFromContainingWindow(evt.pos()).y() - m_pressedPos;

        if (delta > 0)
            // The mouse moved down/right.
            delta = min(maxPos - thumbPos, delta);
        else if (delta < 0)
            // The mouse moved up/left.
            delta = max(-thumbPos, delta);

        if (delta != 0) {
            setValue((float)(thumbPos + delta) * (m_totalSize - m_visibleSize) / (trackLen - thumbLen));
            m_pressedPos += thumbPosition() - thumbPos;
        }
        
        return true;
    }

    if (m_pressedPart != NoPart)
        m_pressedPos = (m_orientation == HorizontalScrollbar ? convertFromContainingWindow(evt.pos()).x() : convertFromContainingWindow(evt.pos()).y());

    ScrollbarPart part = hitTest(evt);    
    if (part != m_hoveredPart) {
        if (m_pressedPart != NoPart) {
            if (part == m_pressedPart) {
                // The mouse is moving back over the pressed part.  We
                // need to start up the timer action again.
                startTimerIfNeeded(cNormalTimerDelay);
                invalidatePart(m_pressedPart);
            } else if (m_hoveredPart == m_pressedPart) {
                // The mouse is leaving the pressed part.  Kill our timer
                // if needed.
                stopTimerIfNeeded();
                invalidatePart(m_pressedPart);
            }
        } else {
            invalidatePart(part);
            invalidatePart(m_hoveredPart);
        }
        m_hoveredPart = part;
    } 

    return true;
}

bool PlatformScrollbar::handleMouseOutEvent(const PlatformMouseEvent& evt)
{
    invalidatePart(m_hoveredPart);
    m_hoveredPart = NoPart;

    return true;
}

bool PlatformScrollbar::handleMousePressEvent(const PlatformMouseEvent& evt)
{
    m_pressedPart = hitTest(evt);
    m_pressedPos = (m_orientation == HorizontalScrollbar ? convertFromContainingWindow(evt.pos()).x() : convertFromContainingWindow(evt.pos()).y());
    invalidatePart(m_pressedPart);
    autoscrollPressedPart(cInitialTimerDelay);
    return true;
}

bool PlatformScrollbar::handleMouseReleaseEvent(const PlatformMouseEvent& evt)
{
    invalidatePart(m_pressedPart);
    m_pressedPart = NoPart;
    m_pressedPos = 0;
    stopTimerIfNeeded();

    if (parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->frame()->eventHandler()->setMousePressed(false);

    return true;
}

bool PlatformScrollbar::thumbUnderMouse()
{
    // Construct a rect.
    IntRect thumb = thumbRect();
    thumb.move(-x(), -y());
    int begin = (m_orientation == HorizontalScrollbar) ? thumb.x() : thumb.y();
    int end = (m_orientation == HorizontalScrollbar) ? thumb.right() : thumb.bottom();
    return (begin <= m_pressedPos && m_pressedPos < end);
}

IntRect PlatformScrollbar::windowClipRect() const
{
    IntRect clipRect(0, 0, width(), height());
    clipRect = convertToContainingWindow(clipRect);
    if (m_client)
        clipRect.intersect(m_client->windowClipRect());
    return clipRect;
}

void PlatformScrollbar::paintGripper(HDC hdc, const IntRect& rect) const
{
}

IntRect PlatformScrollbar::gripperRect(const IntRect& thumbRect) const
{
    return IntRect();
}

}

#endif // USE(SAFARI_THEME)
