/*
 * This file is part of the HTML widget for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef render_replaced_h
#define render_replaced_h

#include "RenderBox.h"

namespace WebCore {

class FrameView;
class Position;
class Widget;

class RenderReplaced : public RenderBox
{
public:
    RenderReplaced(Node*);

    virtual const char* renderName() const { return "RenderReplaced"; }

    virtual short lineHeight(bool firstLine, bool isRootLineBox = false) const;
    virtual short baselinePosition(bool firstLine, bool isRootLineBox = false) const;

    virtual void calcMinMaxWidth();

    bool shouldPaint(PaintInfo&, int& tx, int& ty);
    virtual void paint(PaintInfo&, int tx, int ty) = 0;

    virtual int intrinsicWidth() const { return m_intrinsicWidth; }
    virtual int intrinsicHeight() const { return m_intrinsicHeight; }

    void setIntrinsicWidth(int w) { m_intrinsicWidth = w; }
    void setIntrinsicHeight(int h) { m_intrinsicHeight = h; }

    virtual int caretMinOffset() const;
    virtual int caretMaxOffset() const;
    virtual unsigned caretMaxRenderedOffset() const;
    virtual VisiblePosition positionForCoordinates(int x, int y);
    
    virtual bool canBeSelectionLeaf() const { return true; }
    virtual SelectionState selectionState() const { return static_cast<SelectionState>(m_selectionState); }
    virtual void setSelectionState(SelectionState);
    virtual IntRect selectionRect();
    bool isSelected();
    virtual Color selectionColor(GraphicsContext*) const;

protected:
    int m_intrinsicWidth;
    int m_intrinsicHeight;
    
    unsigned m_selectionState : 3; // SelectionState
};


class RenderWidget : public RenderReplaced, public WidgetClient
{
public:
    RenderWidget(Node*);
    virtual ~RenderWidget();

    virtual void setStyle(RenderStyle*);

    virtual void paint(PaintInfo&, int tx, int ty);

    virtual bool isWidget() const { return true; };

    virtual void destroy();
    virtual void layout( );

    Widget* widget() const { return m_widget; }
    FrameView* view() const { return m_view; }

    RenderArena* ref() { ++m_refCount; return renderArena(); }
    void deref(RenderArena*);
    
    virtual void setSelectionState(SelectionState);

    virtual void updateWidgetPosition();

    virtual void setWidget(Widget*);

    using RenderReplaced::element;

private:
    virtual void focusIn(Widget*);
    virtual void focusOut(Widget*);
    virtual void scrollToVisible(Widget*);
    virtual Element* element(Widget*);
    virtual bool isVisible(Widget*);
    virtual void sendConsumedMouseUp(Widget*);

    void resizeWidget(Widget*, int w, int h);

    virtual void deleteWidget();

protected:
    Widget* m_widget;
    FrameView* m_view;
private:
    int m_refCount;
};

}

#endif
