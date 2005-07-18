/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
#include "html_listimpl.h"

#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "rendering/render_list.h"
#include "xml/dom_docimpl.h"

using namespace khtml;

namespace DOM {

bool HTMLUListElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == HTMLAttributes::type()) {
        result = eUnorderedList;
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLUListElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == HTMLAttributes::type())
        addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, attr->value());
    else
        HTMLElementImpl::parseMappedAttribute(attr);
}

bool HTMLUListElementImpl::compact() const
{
    return !getAttribute(HTMLAttributes::compact()).isNull();
}

void HTMLUListElementImpl::setCompact(bool b)
{
    setAttribute(HTMLAttributes::compact(), b ? "" : 0);
}

DOMString HTMLUListElementImpl::type() const
{
    return getAttribute(HTMLAttributes::type());
}

void HTMLUListElementImpl::setType(const DOMString &value)
{
    setAttribute(HTMLAttributes::type(), value);
}

// -------------------------------------------------------------------------

bool HTMLDirectoryElementImpl::compact() const
{
    return !getAttribute(HTMLAttributes::compact()).isNull();
}

void HTMLDirectoryElementImpl::setCompact(bool b)
{
    setAttribute(HTMLAttributes::compact(), b ? "" : 0);
}

// -------------------------------------------------------------------------

bool HTMLMenuElementImpl::compact() const
{
    return !getAttribute(HTMLAttributes::compact()).isNull();
}

void HTMLMenuElementImpl::setCompact(bool b)
{
    setAttribute(HTMLAttributes::compact(), b ? "" : 0);
}

// -------------------------------------------------------------------------

bool HTMLOListElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == HTMLAttributes::type()) {
        result = eListItem; // Share with <li>
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLOListElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == HTMLAttributes::type()) {
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ALPHA);
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ALPHA);
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ROMAN);
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ROMAN);
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_DECIMAL);
    } else if (attr->name() == HTMLAttributes::start()) {
        _start = !attr->isNull() ? attr->value().toInt() : 1;
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

bool HTMLOListElementImpl::compact() const
{
    return !getAttribute(HTMLAttributes::compact()).isNull();
}

void HTMLOListElementImpl::setCompact(bool b)
{
    setAttribute(HTMLAttributes::compact(), b ? "" : 0);
}

void HTMLOListElementImpl::setStart(long start)
{
    setAttribute(HTMLAttributes::start(), QString::number(start));
}

DOMString HTMLOListElementImpl::type() const
{
    return getAttribute(HTMLAttributes::type());
}

void HTMLOListElementImpl::setType(const DOMString &value)
{
    setAttribute(HTMLAttributes::type(), value);
}

// -------------------------------------------------------------------------

bool HTMLLIElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == HTMLAttributes::type()) {
        result = eListItem; // Share with <ol> since all the values are the same
        return false;
    }
    
    return HTMLElementImpl::mapToEntry(attrName, result);
}

void HTMLLIElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == HTMLAttributes::value()) {
        isValued = true;
        requestedValue = !attr->isNull() ? attr->value().toInt() : 0;

        if(m_render && m_render->isListItem())
        {
            RenderListItem *list = static_cast<RenderListItem *>(m_render);
            // ### work out what to do when attribute removed - use default of some sort?

            list->setValue(requestedValue);
        }
    } else if (attr->name() == HTMLAttributes::type()) {
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ALPHA);
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ALPHA);
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ROMAN);
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ROMAN);
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_DECIMAL);
        else
            addCSSProperty(attr, CSS_PROP_LIST_STYLE_TYPE, attr->value());
    } else
        HTMLElementImpl::parseMappedAttribute(attr);
}

void HTMLLIElementImpl::attach()
{
    assert(!attached());

    HTMLElementImpl::attach();

    if ( m_render && m_render->style()->display() == LIST_ITEM ) {
        RenderListItem *render = static_cast<RenderListItem *>(m_render);
        
        // Find the enclosing list node.
        NodeImpl *listNode = 0;
        NodeImpl *n = this;
        while (!listNode && (n = n->parentNode())) {
            if (n->hasTagName(HTMLTags::ul()) || n->hasTagName(HTMLTags::ol()))
                listNode = n;
        }
        
        // If we are not in a list, tell the renderer so it can position us inside.
        // We don't want to change our style to say "inside" since that would affect nested nodes.
        if (!listNode)
            render->setNotInList(true);

	// If we are first, and the OL has a start attr, set the value.
	if (listNode && listNode->hasTagName(HTMLTags::ol()) && !m_render->previousSibling()) {
	    HTMLOListElementImpl *ol = static_cast<HTMLOListElementImpl *>(listNode);
            render->setValue(ol->start());
	}

	// If we had a value attr.
	if (isValued)
	    render->setValue(requestedValue);
    }
}

DOMString HTMLLIElementImpl::type() const
{
    return getAttribute(HTMLAttributes::type());
}

void HTMLLIElementImpl::setType(const DOMString &value)
{
    setAttribute(HTMLAttributes::type(), value);
}

long HTMLLIElementImpl::value() const
{
    return getAttribute(HTMLAttributes::value()).toInt();
}

void HTMLLIElementImpl::setValue(long value)
{
    setAttribute(HTMLAttributes::value(), QString::number(value));
}

// -------------------------------------------------------------------------

bool HTMLDListElementImpl::compact() const
{
    return !getAttribute(HTMLAttributes::compact()).isNull();
}

void HTMLDListElementImpl::setCompact(bool b)
{
    setAttribute(HTMLAttributes::compact(), b ? "" : 0);
}

}
