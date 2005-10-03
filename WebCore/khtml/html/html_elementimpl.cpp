/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004 Apple Computer, Inc.
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
// -------------------------------------------------------------------------
//#define DEBUG
//#define DEBUG_LAYOUT
//#define PAR_DEBUG
//#define EVENT_DEBUG
//#define UNSUPPORTED_ATTR

#include "config.h"
#include "html/html_elementimpl.h"
#include "html/html_documentimpl.h"
#include "html/htmltokenizer.h"
#include "htmlfactory.h"

#include <kxmlcore/HashSet.h>
#include "editing/visible_text.h"

#include "khtmlview.h"
#include "khtml_part.h"

#include "dom/dom_exception.h"
#include "rendering/render_object.h"
#include "rendering/render_replaced.h"
#include "css/css_valueimpl.h"
#include "css/css_stylesheetimpl.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/css_ruleimpl.h"
#include "xml/dom_textimpl.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/EventNames.h"
#include "editing/markup.h"

#include <kdebug.h>

using namespace DOM;
using namespace DOM::EventNames;
using namespace HTMLNames;
using namespace khtml;

// ------------------------------------------------------------------

HTMLElementImpl::HTMLElementImpl(const QualifiedName& tagName, DocumentPtr *doc)
    : StyledElementImpl(tagName, doc)
{
}

HTMLElementImpl::~HTMLElementImpl()
{
}

DOMString HTMLElementImpl::nodeName() const
{
    // FIXME: Would be nice to have an atomicstring lookup based off uppercase chars that does not have to copy
    // the string on a hit in the hash.
    if (getDocument()->isHTMLDocument())
        return m_tagName.localName().impl()->upper();
    return ElementImpl::nodeName();
}
    
HTMLTagStatus HTMLElementImpl::endTagRequirement() const
{
    if (hasLocalName(dtTag) || hasLocalName(ddTag))
        return TagStatusOptional;

    // Same values as <span>.  This way custom tag name elements will behave like inline spans.
    return TagStatusRequired;
}

int HTMLElementImpl::tagPriority() const
{
    if (hasLocalName(addressTag) || hasLocalName(ddTag) || hasLocalName(dtTag) || hasLocalName(noscriptTag))
        return 3;
    if (hasLocalName(centerTag) || hasLocalName(nobrTag))
        return 5;
    if (hasLocalName(noembedTag) || hasLocalName(noframesTag))
        return 10;

    // Same values as <span>.  This way custom tag name elements will behave like inline spans.
    return 1;
}

NodeImpl *HTMLElementImpl::cloneNode(bool deep)
{
    HTMLElementImpl *clone = HTMLElementFactory::createHTMLElement(m_tagName.localName(), getDocument(), 0, false);
    if (!clone)
        return 0;

    if (namedAttrMap)
        *clone->attributes() = *namedAttrMap;

    if (m_inlineStyleDecl)
        *clone->getInlineStyleDecl() = *m_inlineStyleDecl;

    if (deep)
        cloneChildNodes(clone);

    return clone;
}

bool HTMLElementImpl::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == alignAttr ||
        attrName == contenteditableAttr ||
        attrName == dirAttr) {
        result = eUniversal;
        return false;
    }

    return StyledElementImpl::mapToEntry(attrName, result);
}
    
void HTMLElementImpl::parseMappedAttribute(MappedAttributeImpl *attr)
{
    if (attr->name() == idAttr || attr->name() == classAttr || attr->name() == styleAttr)
        return StyledElementImpl::parseMappedAttribute(attr);

    DOMString indexstring;
    if (attr->name() == alignAttr) {
        if (strcasecmp(attr->value(), "middle" ) == 0)
            addCSSProperty(attr, CSS_PROP_TEXT_ALIGN, "center");
        else
            addCSSProperty(attr, CSS_PROP_TEXT_ALIGN, attr->value());
    } else if (attr->name() == contenteditableAttr) {
        setContentEditable(attr);
    } else if (attr->name() == tabindexAttr) {
        indexstring = getAttribute(tabindexAttr);
        if (indexstring.length())
            setTabIndex(indexstring.toInt());
    } else if (attr->name() == langAttr) {
        // FIXME: Implement
    } else if (attr->name() == dirAttr) {
        addCSSProperty(attr, CSS_PROP_DIRECTION, attr->value());
        addCSSProperty(attr, CSS_PROP_UNICODE_BIDI, CSS_VAL_EMBED);
    }
// standard events
    else if (attr->name() == onclickAttr) {
        setHTMLEventListener(clickEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == oncontextmenuAttr) {
    	setHTMLEventListener(contextmenuEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondblclickAttr) {
	setHTMLEventListener(khtmlDblclickEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmousedownAttr) {
        setHTMLEventListener(mousedownEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmousemoveAttr) {
        setHTMLEventListener(mousemoveEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmouseoutAttr) {
        setHTMLEventListener(mouseoutEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmouseoverAttr) {
        setHTMLEventListener(mouseoverEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmouseupAttr) {
        setHTMLEventListener(mouseupEvent,
	                     getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onmousewheelAttr) {
        setHTMLEventListener(mousewheelEvent,
                            getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onfocusAttr) {
        setHTMLEventListener(DOMFocusInEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onkeydownAttr) {
        setHTMLEventListener(keydownEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onkeypressAttr) {
        setHTMLEventListener(keypressEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onkeyupAttr) {
        setHTMLEventListener(keyupEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onscrollAttr) {
        setHTMLEventListener(scrollEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onbeforecutAttr) {
        setHTMLEventListener(beforecutEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == oncutAttr) {
        setHTMLEventListener(cutEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onbeforecopyAttr) {
        setHTMLEventListener(beforecopyEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == oncopyAttr) {
        setHTMLEventListener(copyEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onbeforepasteAttr) {
        setHTMLEventListener(beforepasteEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onpasteAttr) {
        setHTMLEventListener(pasteEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragenterAttr) {
        setHTMLEventListener(dragenterEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragoverAttr) {
        setHTMLEventListener(dragoverEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragleaveAttr) {
        setHTMLEventListener(dragleaveEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondropAttr) {
        setHTMLEventListener(dropEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragstartAttr) {
        setHTMLEventListener(dragstartEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragAttr) {
        setHTMLEventListener(dragEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == ondragendAttr) {
        setHTMLEventListener(dragendEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } else if (attr->name() == onselectstartAttr) {
        setHTMLEventListener(selectstartEvent,
                             getDocument()->createHTMLEventListener(attr->value().qstring(), this));
    } 
}

DOMString HTMLElementImpl::innerHTML() const
{
    return createMarkup(this, ChildrenOnly);
}

DOMString HTMLElementImpl::outerHTML() const
{
    return createMarkup(this);
}

DOMString HTMLElementImpl::innerText() const
{
    // We need to update layout, since plainText uses line boxes in the render tree.
    getDocument()->updateLayout();
    return plainText(rangeOfContents(const_cast<HTMLElementImpl *>(this)).get());
}

DOMString HTMLElementImpl::outerText() const
{
    // Getting outerText is the same as getting innerText, only
    // setting is different. You would think this should get the plain
    // text for the outer range, but this is wrong, <br> for instance
    // would return different values for inner and outer text by such
    // a rule, but it doesn't in WinIE, and we want to match that.
    return innerText();
}

DocumentFragmentImpl *HTMLElementImpl::createContextualFragment(const DOMString &html)
{
    // the following is in accordance with the definition as used by IE
    if (endTagRequirement() == TagStatusForbidden)
        return 0;

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(styleTag) || hasLocalName(titleTag))
        return 0;

    DocumentFragmentImpl *fragment = new DocumentFragmentImpl(docPtr());
    fragment->ref();
    
    if (!getDocument()->isHTMLDocument()) {
        bool ret = parseXMLDocumentFragment(html, fragment, this);
        
        if (!ret) {
            // FIXME: We should propagate a syntax error exception out here.
            fragment->deref();
            return 0;
        }
    }
    else
    {
        HTMLTokenizer tok(docPtr(), fragment);
        tok.setForceSynchronous(true);            // disable asynchronous parsing
        tok.write( html.qstring(), true );
        tok.finish();
        assert(!tok.processingData());            // make sure we're done (see 3963151)
    }

    // Exceptions are ignored because none ought to happen here.
    int ignoredExceptionCode;

    // we need to pop <html> and <body> elements and remove <head> to
    // accommodate folks passing complete HTML documents to make the
    // child of an element.

    NodeImpl *nextNode;
    for (NodeImpl *node = fragment->firstChild(); node != NULL; node = nextNode) {
        nextNode = node->nextSibling();
	node->ref();
        if (node->hasTagName(htmlTag) || node->hasTagName(bodyTag)) {
	    NodeImpl *firstChild = node->firstChild();
            if (firstChild != NULL) {
                nextNode = firstChild;
            }
	    NodeImpl *nextChild;
            for (NodeImpl *child = firstChild; child != NULL; child = nextChild) {
		nextChild = child->nextSibling();
                child->ref();
                node->removeChild(child, ignoredExceptionCode);
		fragment->insertBefore(child, node, ignoredExceptionCode);
                child->deref();
	    }
            fragment->removeChild(node, ignoredExceptionCode);
	} else if (node->hasTagName(headTag))
	    fragment->removeChild(node, ignoredExceptionCode);

        // Important to do this deref after removeChild, because if the only thing
        // keeping a node around is a parent that is non-0, removeChild will not
        // delete the node. This works fine in JavaScript because there's always
        // a ref of the node, but here in C++ we need to do it explicitly.
        node->deref();
    }

    // Trick to get the fragment back to the floating state, with 0
    // refs but not destroyed.
    fragment->setParent(this);
    fragment->deref();
    fragment->setParent(0);

    return fragment;
}

void HTMLElementImpl::setInnerHTML(const DOMString &html, int &exception)
{
    DocumentFragmentImpl *fragment = createContextualFragment(html);
    if (fragment == NULL) {
	exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    removeChildren();
    appendChild(fragment, exception);
}

void HTMLElementImpl::setOuterHTML(const DOMString &html, int &exception)
{
    NodeImpl *p = parent();
    if (!p || !p->isHTMLElement()) {
	exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    HTMLElementImpl *parent = static_cast<HTMLElementImpl *>(p);
    DocumentFragmentImpl *fragment = parent->createContextualFragment(html);

    if (!fragment) {
	exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    
    ref();
    parent->replaceChild(fragment, this, exception);
    deref();
}


void HTMLElementImpl::setInnerText(const DOMString &text, int &exception)
{
    // following the IE specs.
    if (endTagRequirement() == TagStatusForbidden) {
	exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    removeChildren();
    appendChild(new TextImpl(docPtr(), text), exception);
}

void HTMLElementImpl::setOuterText(const DOMString &text, int &exception)
{
    // following the IE specs.
    if (endTagRequirement() == TagStatusForbidden) {
	exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    NodeImpl *parent = parentNode();

    if (!parent) {
        exception = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    TextImpl *t = new TextImpl(docPtr(), text);
    ref();
    parent->replaceChild(t, this, exception);
    deref();
    if (exception)
        return;

    // is previous node a text node? if so, merge into it
    NodeImpl *prev = t->previousSibling();
    if (prev && prev->isTextNode()) {
	TextImpl *textPrev = static_cast<TextImpl *>(prev);
	textPrev->appendData(t->data(), exception);
        if (exception)
            return;
        t->remove(exception);
        if (exception)
            return;
	t = textPrev;
    }

    // is next node a text node? if so, merge it in
    NodeImpl *next = t->nextSibling();
    if (next && next->isTextNode()) {
	TextImpl *textNext = static_cast<TextImpl *>(next);
	t->appendData(textNext->data(), exception);
        if (exception)
            return;
        textNext->remove(exception);
        if (exception)
            return;
    }
}

void HTMLElementImpl::addHTMLAlignment(MappedAttributeImpl* attr)
{
    //qDebug("alignment is %s", alignment.qstring().latin1() );
    // vertical alignment with respect to the current baseline of the text
    // right or left means floating images
    int propfloat = -1;
    int propvalign = -1;
    const AtomicString& alignment = attr->value();
    if ( strcasecmp( alignment, "absmiddle" ) == 0 ) {
        propvalign = CSS_VAL_MIDDLE;
    } else if ( strcasecmp( alignment, "absbottom" ) == 0 ) {
        propvalign = CSS_VAL_BOTTOM;
    } else if ( strcasecmp( alignment, "left" ) == 0 ) {
	propfloat = CSS_VAL_LEFT;
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "right" ) == 0 ) {
	propfloat = CSS_VAL_RIGHT;
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "top" ) == 0 ) {
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "middle" ) == 0 ) {
	propvalign = CSS_VAL__KHTML_BASELINE_MIDDLE;
    } else if ( strcasecmp( alignment, "center" ) == 0 ) {
	propvalign = CSS_VAL_MIDDLE;
    } else if ( strcasecmp( alignment, "bottom" ) == 0 ) {
	propvalign = CSS_VAL_BASELINE;
    } else if ( strcasecmp ( alignment, "texttop") == 0 ) {
	propvalign = CSS_VAL_TEXT_TOP;
    }
    
    if ( propfloat != -1 )
	addCSSProperty( attr, CSS_PROP_FLOAT, propfloat );
    if ( propvalign != -1 )
	addCSSProperty( attr, CSS_PROP_VERTICAL_ALIGN, propvalign );
}

bool HTMLElementImpl::isFocusable() const
{
    return isContentEditable() && parent() && !parent()->isContentEditable();
}

bool HTMLElementImpl::isContentEditable() const 
{
    if (getDocument()->part() && getDocument()->part()->isContentEditable())
        return true;

    getDocument()->updateRendering();

    if (!renderer()) {
        if (parentNode())
            return parentNode()->isContentEditable();
        else
            return false;
    }
    
    return renderer()->style()->userModify() == READ_WRITE;
}

DOMString HTMLElementImpl::contentEditable() const 
{
    getDocument()->updateRendering();

    if (!renderer())
        return "false";
    
    switch (renderer()->style()->userModify()) {
        case READ_WRITE:
            return "true";
        case READ_ONLY:
            return "false";
        default:
            return "inherit";
    }
    return "inherit";
}

void HTMLElementImpl::setContentEditable(MappedAttributeImpl* attr) 
{
    KHTMLPart *part = getDocument()->part();
    const AtomicString& enabled = attr->value();
    if (enabled.isEmpty() || strcasecmp(enabled, "true") == 0) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_READ_WRITE);
        if (part)
            part->applyEditingStyleToElement(this);    
    }
    else if (strcasecmp(enabled, "false") == 0) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_READ_ONLY);
        if (part)
            part->removeEditingStyleFromElement(this);    
    }
    else if (strcasecmp(enabled, "inherit") == 0) {
        addCSSProperty(attr, CSS_PROP__KHTML_USER_MODIFY, CSS_VAL_INHERIT);
        if (part)
            part->removeEditingStyleFromElement(this);    
    }
}

void HTMLElementImpl::setContentEditable(const DOMString &enabled) {
    if (enabled == "inherit") {
        int exceptionCode;
        removeAttribute(contenteditableAttr, exceptionCode);
    }
    else
        setAttribute(contenteditableAttr, enabled.isEmpty() ? "true" : enabled);
}

void HTMLElementImpl::click(bool sendMouseEvents, bool showPressedLook)
{
    int x = 0;
    int y = 0;
    RenderObject *r = renderer();
    if (r)
        r->absolutePosition(x,y);

    // send mousedown and mouseup before the click, if requested
    if (sendMouseEvents) {
        dispatchSimulatedMouseEvent(mousedownEvent);
        if (r)
            setActive(true, showPressedLook);
        dispatchSimulatedMouseEvent(mouseupEvent);
        if (r)
            setActive(false);
    } else if (r) {
        setActive(true, showPressedLook);
        setActive(false);
    }

    // always send click
    dispatchSimulatedMouseEvent(clickEvent);
}

// accessKeyAction is used by the accessibility support code
// to send events to elements that our JavaScript caller does
// does not.  The elements JS is interested in have subclasses
// that override this method to direct the click appropriately.
// Here in the base class, then, we only send the click if
// the caller wants it to go to any HTMLElementImpl, and we say
// to send the mouse events in addition to the click.
void HTMLElementImpl::accessKeyAction(bool sendToAnyElement)
{
    if (sendToAnyElement)
        click(true);
}

DOMString HTMLElementImpl::toString() const
{
    if (!hasChildNodes()) {
	DOMString result = openTagStartToString();
	result += ">";

	if (endTagRequirement() == TagStatusRequired) {
	    result += "</";
	    result += nodeName();
	    result += ">";
	}

	return result;
    }

    return ElementImpl::toString();
}

DOMString HTMLElementImpl::id() const
{
    return getAttribute(idAttr);
}

void HTMLElementImpl::setId(const DOMString &value)
{
    setAttribute(idAttr, value);
}

DOMString HTMLElementImpl::title() const
{
    return getAttribute(titleAttr);
}

void HTMLElementImpl::setTitle(const DOMString &value)
{
    setAttribute(titleAttr, value);
}

DOMString HTMLElementImpl::lang() const
{
    return getAttribute(langAttr);
}

void HTMLElementImpl::setLang(const DOMString &value)
{
    setAttribute(langAttr, value);
}

DOMString HTMLElementImpl::dir() const
{
    return getAttribute(dirAttr);
}

void HTMLElementImpl::setDir(const DOMString &value)
{
    setAttribute(dirAttr, value);
}

DOMString HTMLElementImpl::className() const
{
    return getAttribute(classAttr);
}

void HTMLElementImpl::setClassName(const DOMString &value)
{
    setAttribute(classAttr, value);
}

SharedPtr<HTMLCollectionImpl> HTMLElementImpl::children()
{
    return SharedPtr<HTMLCollectionImpl>(new HTMLCollectionImpl(this, HTMLCollectionImpl::NODE_CHILDREN));
}

// DOM Section 1.1.1
bool HTMLElementImpl::childAllowed(NodeImpl *newChild)
{
    if (!ElementImpl::childAllowed(newChild))
        return false;

    // For XML documents, we are non-validating and do not check against a DTD, even for HTML elements.
    if (!getDocument()->isHTMLDocument())
        return true;

    // Future-proof for XML content inside HTML documents (we may allow this some day).
    if (newChild->isElementNode() && !newChild->isHTMLElement())
        return true;

    // Elements with forbidden tag status can never have children
    if (endTagRequirement() == TagStatusForbidden)
        return false;

    // Comment nodes are always allowed.
    if (newChild->isCommentNode())
        return true;

    // Now call checkDTD.
    return checkDTD(newChild);
}

// DTD Stuff
// This unfortunate function is only needed when checking against the DTD.  Other languages (like SVG) won't need this.
bool HTMLElementImpl::isRecognizedTagName(const QualifiedName& tagName)
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        #define INSERT_TAG(name) tagList.insert(name##Tag.localName().impl());
        DOM_HTMLNAMES_FOR_EACH_TAG(INSERT_TAG)
    }
    return tagList.contains(tagName.localName().impl());
}

// The terms inline and block are used here loosely.  Don't make the mistake of assuming all inlines or all blocks
// need to be in these two lists.
HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> >* inlineTagList()
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        tagList.insert(ttTag.localName().impl());
        tagList.insert(iTag.localName().impl());
        tagList.insert(bTag.localName().impl());
        tagList.insert(uTag.localName().impl());
        tagList.insert(sTag.localName().impl());
        tagList.insert(strikeTag.localName().impl());
        tagList.insert(bigTag.localName().impl());
        tagList.insert(smallTag.localName().impl());
        tagList.insert(emTag.localName().impl());
        tagList.insert(strongTag.localName().impl());
        tagList.insert(dfnTag.localName().impl());
        tagList.insert(codeTag.localName().impl());
        tagList.insert(sampTag.localName().impl());
        tagList.insert(kbdTag.localName().impl());
        tagList.insert(varTag.localName().impl());
        tagList.insert(citeTag.localName().impl());
        tagList.insert(abbrTag.localName().impl());
        tagList.insert(acronymTag.localName().impl());
        tagList.insert(aTag.localName().impl());
        tagList.insert(canvasTag.localName().impl());
        tagList.insert(imgTag.localName().impl());
        tagList.insert(appletTag.localName().impl());
        tagList.insert(objectTag.localName().impl());
        tagList.insert(embedTag.localName().impl());
        tagList.insert(fontTag.localName().impl());
        tagList.insert(basefontTag.localName().impl());
        tagList.insert(brTag.localName().impl());
        tagList.insert(scriptTag.localName().impl());
        tagList.insert(mapTag.localName().impl());
        tagList.insert(qTag.localName().impl());
        tagList.insert(subTag.localName().impl());
        tagList.insert(supTag.localName().impl());
        tagList.insert(spanTag.localName().impl());
        tagList.insert(bdoTag.localName().impl());
        tagList.insert(iframeTag.localName().impl());
        tagList.insert(inputTag.localName().impl());
        tagList.insert(keygenTag.localName().impl());
        tagList.insert(selectTag.localName().impl());
        tagList.insert(textareaTag.localName().impl());
        tagList.insert(labelTag.localName().impl());
        tagList.insert(buttonTag.localName().impl());
        tagList.insert(insTag.localName().impl());
        tagList.insert(delTag.localName().impl());
        tagList.insert(nobrTag.localName().impl());
        tagList.insert(wbrTag.localName().impl());
    }
    return &tagList;
}

HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> >* blockTagList()
{
    static HashSet<DOMStringImpl*, PointerHash<DOMStringImpl*> > tagList;
    if (tagList.isEmpty()) {
        tagList.insert(pTag.localName().impl());
        tagList.insert(h1Tag.localName().impl());
        tagList.insert(h2Tag.localName().impl());
        tagList.insert(h3Tag.localName().impl());
        tagList.insert(h4Tag.localName().impl());
        tagList.insert(h5Tag.localName().impl());
        tagList.insert(h6Tag.localName().impl());
        tagList.insert(ulTag.localName().impl());
        tagList.insert(olTag.localName().impl());
        tagList.insert(dirTag.localName().impl());
        tagList.insert(menuTag.localName().impl());
        tagList.insert(preTag.localName().impl());
        tagList.insert(plaintextTag.localName().impl());
        tagList.insert(xmpTag.localName().impl());
        tagList.insert(dlTag.localName().impl());
        tagList.insert(divTag.localName().impl());
        tagList.insert(layerTag.localName().impl());
        tagList.insert(centerTag.localName().impl());
        tagList.insert(noscriptTag.localName().impl());
        tagList.insert(noframesTag.localName().impl());
        tagList.insert(noembedTag.localName().impl());
        tagList.insert(nolayerTag.localName().impl());
        tagList.insert(blockquoteTag.localName().impl());
        tagList.insert(formTag.localName().impl());
        tagList.insert(isindexTag.localName().impl());
        tagList.insert(hrTag.localName().impl());
        tagList.insert(tableTag.localName().impl());
        tagList.insert(fieldsetTag.localName().impl());
        tagList.insert(addressTag.localName().impl());
        tagList.insert(liTag.localName().impl());
        tagList.insert(ddTag.localName().impl());
        tagList.insert(dtTag.localName().impl());
        tagList.insert(marqueeTag.localName().impl());
    }
    return &tagList;
}

bool HTMLElementImpl::inEitherTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;
        
    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        if (inlineTagList()->contains(child->tagName().localName().impl()))
            return true;
        if (blockTagList()->contains(child->tagName().localName().impl()))
            return true;
        return !isRecognizedTagName(child->tagName()); // Accept custom html tags
    }

    return false;
}

bool HTMLElementImpl::inInlineTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;

    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        if (inlineTagList()->contains(child->tagName().localName().impl()))
            return true;
        return !isRecognizedTagName(child->tagName()); // Accept custom html tags
    }

    return false;
}

bool HTMLElementImpl::inBlockTagList(const NodeImpl* newChild)
{
    if (newChild->isTextNode())
        return true;
            
    if (newChild->isHTMLElement()) {
        const HTMLElementImpl* child = static_cast<const HTMLElementImpl*>(newChild);
        return (blockTagList()->contains(child->tagName().localName().impl()));
    }

    return false;
}

bool HTMLElementImpl::checkDTD(const NodeImpl* newChild)
{
    if (hasTagName(addressTag) && newChild->hasTagName(pTag))
        return true;
    return inEitherTagList(newChild);
}
