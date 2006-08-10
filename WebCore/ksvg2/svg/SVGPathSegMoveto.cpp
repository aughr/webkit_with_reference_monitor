/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#ifdef SVG_SUPPORT
#include "SVGPathSegMoveto.h"
#include "SVGStyledElement.h"

using namespace WebCore;

SVGPathSegMovetoAbs::SVGPathSegMovetoAbs(const SVGStyledElement *context)
: SVGPathSeg(context)
{
    m_x = m_y = 0.0;
}

SVGPathSegMovetoAbs::~SVGPathSegMovetoAbs()
{
}

void SVGPathSegMovetoAbs::setX(double x)
{
    m_x = x;

    if(m_context)
        m_context->notifyAttributeChange();
}

double SVGPathSegMovetoAbs::x() const
{
    return m_x;
}

void SVGPathSegMovetoAbs::setY(double y)
{
    m_y = y;

    if(m_context)
        m_context->notifyAttributeChange();
}

double SVGPathSegMovetoAbs::y() const
{
    return m_y;
}




SVGPathSegMovetoRel::SVGPathSegMovetoRel(const SVGStyledElement *context)
: SVGPathSeg(context)
{
    m_x = m_y = 0.0;
}

SVGPathSegMovetoRel::~SVGPathSegMovetoRel()
{
}

void SVGPathSegMovetoRel::setX(double x)
{
    m_x = x;

    if(m_context)
        m_context->notifyAttributeChange();
}

double SVGPathSegMovetoRel::x() const
{
    return m_x;
}

void SVGPathSegMovetoRel::setY(double y)
{
    m_y = y;

    if(m_context)
        m_context->notifyAttributeChange();
}

double SVGPathSegMovetoRel::y() const
{
    return m_y;
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

