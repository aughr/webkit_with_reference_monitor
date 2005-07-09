/*
    Copyright (C) 2004 Nikolas Zimmermann <wildfox@kde.org>
				  2004 Rob Buis <buis@kde.org>

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

#include <qstringlist.h>

#include <kdom/impl/AttrImpl.h>

#include <kcanvas/KCanvas.h>
#include <kcanvas/KCanvasRegistry.h>
#include <kcanvas/KCanvasFilters.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "ksvg.h"
#include "svgattrs.h"
#include "SVGHelper.h"
#include "SVGRenderStyle.h"
#include "SVGFEBlendElementImpl.h"
#include "SVGAnimatedEnumerationImpl.h"
#include "SVGAnimatedStringImpl.h"
#include "SVGDOMImplementationImpl.h"

using namespace KSVG;

SVGFEBlendElementImpl::SVGFEBlendElementImpl(KDOM::DocumentImpl *doc, KDOM::NodeImpl::Id id, const KDOM::DOMString &prefix) : 
SVGFilterPrimitiveStandardAttributesImpl(doc, id, prefix)
{
	m_in1 = m_in2 = 0;
	m_mode = 0;
	m_filterEffect = 0;
}

SVGFEBlendElementImpl::~SVGFEBlendElementImpl()
{
	if(m_in1)
		m_in1->deref();
	if(m_in2)
		m_in2->deref();
	if(m_mode)
		m_mode->deref();
}

SVGAnimatedStringImpl *SVGFEBlendElementImpl::in1() const
{
	SVGStyledElementImpl *dummy = 0;
	return lazy_create<SVGAnimatedStringImpl>(m_in1, dummy);
}

SVGAnimatedStringImpl *SVGFEBlendElementImpl::in2() const
{
	SVGStyledElementImpl *dummy = 0;
	return lazy_create<SVGAnimatedStringImpl>(m_in2, dummy);
}

SVGAnimatedEnumerationImpl *SVGFEBlendElementImpl::mode() const
{
	SVGStyledElementImpl *dummy = 0;
	return lazy_create<SVGAnimatedEnumerationImpl>(m_mode, dummy);
}

void SVGFEBlendElementImpl::parseAttribute(KDOM::AttributeImpl *attr)
{
	int id = (attr->id() & NodeImpl_IdLocalMask);
	KDOM::DOMString value(attr->value());
	switch(id)
	{
		case ATTR_MODE:
		{
			if(value == "normal")
				mode()->setBaseVal(SVG_FEBLEND_MODE_NORMAL);
			else if(value == "multiply")
				mode()->setBaseVal(SVG_FEBLEND_MODE_MULTIPLY);
			else if(value == "screen")
				mode()->setBaseVal(SVG_FEBLEND_MODE_SCREEN);
			else if(value == "darken")
				mode()->setBaseVal(SVG_FEBLEND_MODE_DARKEN);
			else if(value == "lighten")
				mode()->setBaseVal(SVG_FEBLEND_MODE_LIGHTEN);
			break;
		}
		case ATTR_IN:
		{
			in1()->setBaseVal(value.implementation());
			break;
		}
		case ATTR_IN2:
		{
			in2()->setBaseVal(value.implementation());
			break;
		}
		default:
		{
			SVGFilterPrimitiveStandardAttributesImpl::parseAttribute(attr);
		}
	};
}

KCanvasItem *SVGFEBlendElementImpl::createCanvasItem(KCanvas *canvas, KRenderingStyle *style) const
{
	m_filterEffect = static_cast<KCanvasFEBlend *>(canvas->renderingDevice()->createFilterEffect(FE_BLEND));
	m_filterEffect->setBlendMode((KCBlendModeType)(mode()->baseVal()-1));
	m_filterEffect->setIn(KDOM::DOMString(in1()->baseVal()).string());
	m_filterEffect->setIn2(KDOM::DOMString(in2()->baseVal()).string());
	setStandardAttributes(m_filterEffect);
	return 0;
}

KCanvasFilterEffect *SVGFEBlendElementImpl::filterEffect() const
{
	return m_filterEffect;
}

// vim:ts=4:noet
