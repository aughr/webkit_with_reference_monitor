/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2012 Apple Inc. All rights reserved.
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
#include "CSSStyleSheet.h"

#include "CSSCharsetRule.h"
#include "CSSFontFaceRule.h"
#include "CSSImportRule.h"
#include "CSSNamespace.h"
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CachedCSSStyleSheet.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "MediaList.h"
#include "Node.h"
#include "SVGNames.h"
#include "SecurityOrigin.h"
#include "StylePropertySet.h"
#include "StyleRule.h"
#include <wtf/Deque.h>

namespace WebCore {

class StyleSheetCSSRuleList : public CSSRuleList {
public:
    StyleSheetCSSRuleList(CSSStyleSheet* sheet) : m_styleSheet(sheet) { }
    
private:
    virtual void ref() { m_styleSheet->ref(); }
    virtual void deref() { m_styleSheet->deref(); }
    
    virtual unsigned length() const { return m_styleSheet->length(); }
    virtual CSSRule* item(unsigned index) const { return m_styleSheet->item(index); }
    
    virtual CSSStyleSheet* styleSheet() const { return m_styleSheet; }
    
    CSSStyleSheet* m_styleSheet;
};

#if !ASSERT_DISABLED
static bool isAcceptableCSSStyleSheetParent(Node* parentNode)
{
    // Only these nodes can be parents of StyleSheets, and they need to call clearOwnerNode() when moved out of document.
    return !parentNode
        || parentNode->isDocumentNode()
        || parentNode->hasTagName(HTMLNames::linkTag)
        || parentNode->hasTagName(HTMLNames::styleTag)
#if ENABLE(SVG)
        || parentNode->hasTagName(SVGNames::styleTag)
#endif
        || parentNode->nodeType() == Node::PROCESSING_INSTRUCTION_NODE;
}
#endif

StyleSheetInternal::StyleSheetInternal(Node* parentNode, const String& originalURL, const KURL& finalURL, const String& charset)
    : m_ownerNode(parentNode)
    , m_ownerRule(0)
    , m_originalURL(originalURL)
    , m_finalURL(finalURL)
    , m_loadCompleted(false)
    , m_isUserStyleSheet(false)
    , m_hasSyntacticallyValidCSSHeader(true)
    , m_didLoadErrorOccur(false)
    , m_usesRemUnits(false)
    , m_parserContext(parentNode->document())
{
    ASSERT(isAcceptableCSSStyleSheetParent(parentNode));

    updateBaseURL();
    m_parserContext.charset = charset;
}

StyleSheetInternal::StyleSheetInternal(StyleRuleImport* ownerRule, const String& originalURL, const KURL& finalURL, const String& charset)
    : m_ownerNode(0)
    , m_ownerRule(ownerRule)
    , m_originalURL(originalURL)
    , m_finalURL(finalURL)
    , m_loadCompleted(false)
    , m_hasSyntacticallyValidCSSHeader(true)
    , m_didLoadErrorOccur(false)
    , m_usesRemUnits(false)
    , m_parserContext((ownerRule && ownerRule->parentStyleSheet()) ? ownerRule->parentStyleSheet()->parserContext() : CSSStrictMode)
{
    StyleSheetInternal* parentSheet = ownerRule ? ownerRule->parentStyleSheet() : 0;
    m_isUserStyleSheet = parentSheet && parentSheet->isUserStyleSheet();

    updateBaseURL();
    m_parserContext.charset = charset;
}

StyleSheetInternal::~StyleSheetInternal()
{
    clearRules();
}

void StyleSheetInternal::parserAppendRule(PassRefPtr<StyleRuleBase> rule)
{
    ASSERT(!rule->isCharsetRule());
    if (rule->isImportRule()) {
        // Parser enforces that @import rules come before anything else except @charset.
        ASSERT(m_childRules.isEmpty());
        m_importRules.append(static_cast<StyleRuleImport*>(rule.get()));
        m_importRules.last()->setParentStyleSheet(this);
        m_importRules.last()->requestStyleSheet();
        return;
    }
    m_childRules.append(rule);
}

PassRefPtr<CSSRule> StyleSheetInternal::createChildRuleCSSOMWrapper(unsigned index, CSSStyleSheet* parentWrapper)
{
    ASSERT(index < ruleCount());
    
    unsigned childVectorIndex = index;
    if (hasCharsetRule()) {
        if (index == 0)
            return CSSCharsetRule::create(parentWrapper, m_encodingFromCharsetRule);
        --childVectorIndex;
    }
    if (childVectorIndex < m_importRules.size())
        return m_importRules[childVectorIndex]->createCSSOMWrapper(parentWrapper);
    
    childVectorIndex -= m_importRules.size();
    return m_childRules[childVectorIndex]->createCSSOMWrapper(parentWrapper);
}

unsigned StyleSheetInternal::ruleCount() const
{
    unsigned result = 0;
    result += hasCharsetRule() ? 1 : 0;
    result += m_importRules.size();
    result += m_childRules.size();
    return result;
}

void StyleSheetInternal::clearCharsetRule()
{
    m_encodingFromCharsetRule = String();
}

void StyleSheetInternal::clearRules()
{
    for (unsigned i = 0; i < m_importRules.size(); ++i) {
        ASSERT(m_importRules.at(i)->parentStyleSheet() == this);
        m_importRules[i]->clearParentStyleSheet();
    }
    m_importRules.clear();
    m_childRules.clear();
    clearCharsetRule();
}

void StyleSheetInternal::parserSetEncodingFromCharsetRule(const String& encoding)
{
    // Parser enforces that there is ever only one @charset.
    ASSERT(m_encodingFromCharsetRule.isNull());
    m_encodingFromCharsetRule = encoding; 
}

bool StyleSheetInternal::wrapperInsertRule(PassRefPtr<StyleRuleBase> rule, unsigned index)
{
    ASSERT(index <= ruleCount());
    // Parser::parseRule doesn't currently allow @charset so we don't need to deal with it.
    ASSERT(!rule->isCharsetRule());
    
    unsigned childVectorIndex = index;
    // m_childRules does not contain @charset which is always in index 0 if it exists.
    if (hasCharsetRule()) {
        if (childVectorIndex == 0) {
            // Nothing can be inserted before @charset.
            return false;
        }
        --childVectorIndex;
    }
    
    if (childVectorIndex < m_importRules.size() || (childVectorIndex == m_importRules.size() && rule->isImportRule())) {
        // Inserting non-import rule before @import is not allowed.
        if (!rule->isImportRule())
            return false;
        m_importRules.insert(childVectorIndex, static_cast<StyleRuleImport*>(rule.get()));
        m_importRules[childVectorIndex]->setParentStyleSheet(this);
        m_importRules[childVectorIndex]->requestStyleSheet();
        // FIXME: Stylesheet doesn't actually change meaningfully before the imported sheets are loaded.
        styleSheetChanged();
        return true;
    }
    // Inserting @import rule after a non-import rule is not allowed.
    if (rule->isImportRule())
        return false;
    childVectorIndex -= m_importRules.size();
 
    m_childRules.insert(childVectorIndex, rule);
    
    styleSheetChanged();
    return true;
}

void StyleSheetInternal::wrapperDeleteRule(unsigned index)
{
    ASSERT(index < ruleCount());

    unsigned childVectorIndex = index;
    if (hasCharsetRule()) {
        if (childVectorIndex == 0) {
            clearCharsetRule();
            styleSheetChanged();
            return;
        }
        --childVectorIndex;
    }
    if (childVectorIndex < m_importRules.size()) {
        m_importRules[childVectorIndex]->clearParentStyleSheet();
        m_importRules.remove(childVectorIndex);
        styleSheetChanged();
        return;
    }
    childVectorIndex -= m_importRules.size();

    m_childRules.remove(childVectorIndex);
    styleSheetChanged();
}

void StyleSheetInternal::addNamespace(CSSParser* p, const AtomicString& prefix, const AtomicString& uri)
{
    if (uri.isNull())
        return;

    m_namespaces = adoptPtr(new CSSNamespace(prefix, uri, m_namespaces.release()));

    if (prefix.isEmpty())
        // Set the default namespace on the parser so that selectors that omit namespace info will
        // be able to pick it up easily.
        p->m_defaultNamespace = uri;
}

const AtomicString& StyleSheetInternal::determineNamespace(const AtomicString& prefix)
{
    if (prefix.isNull())
        return nullAtom; // No namespace. If an element/attribute has a namespace, we won't match it.
    if (prefix == starAtom)
        return starAtom; // We'll match any namespace.
    if (m_namespaces) {
        if (CSSNamespace* namespaceForPrefix = m_namespaces->namespaceForPrefix(prefix))
            return namespaceForPrefix->uri;
    }
    return nullAtom; // Assume we won't match any namespaces.
}

bool StyleSheetInternal::parseString(const String &string)
{
    return parseStringAtLine(string, 0);
}

bool StyleSheetInternal::parseStringAtLine(const String& string, int startLineNumber)
{
    CSSParser p(parserContext());
    p.parseSheet(this, string, startLineNumber);
    return true;
}

bool StyleSheetInternal::isLoading() const
{
    for (unsigned i = 0; i < m_importRules.size(); ++i) {
        if (m_importRules[i]->isLoading())
            return true;
    }
    return false;
}

void StyleSheetInternal::checkLoaded()
{
    if (isLoading())
        return;

    // Avoid |this| being deleted by scripts that run via
    // ScriptableDocumentParser::executeScriptsWaitingForStylesheets().
    // See <rdar://problem/6622300>.
    RefPtr<StyleSheetInternal> protector(this);
    if (StyleSheetInternal* styleSheet = parentStyleSheet())
        styleSheet->checkLoaded();

    RefPtr<Node> owner = ownerNode();
    if (!owner)
        m_loadCompleted = true;
    else {
        m_loadCompleted = owner->sheetLoaded();
        if (m_loadCompleted)
            owner->notifyLoadedSheetAndAllCriticalSubresources(m_didLoadErrorOccur);
    }
}

void StyleSheetInternal::notifyLoadedSheet(const CachedCSSStyleSheet* sheet)
{
    ASSERT(sheet);
    m_didLoadErrorOccur |= sheet->errorOccurred();
}

void StyleSheetInternal::startLoadingDynamicSheet()
{
    if (Node* owner = ownerNode())
        owner->startLoadingDynamicSheet();
}

Node* StyleSheetInternal::findStyleSheetOwnerNode() const
{
    for (const StyleSheetInternal* sheet = this; sheet; sheet = sheet->parentStyleSheet()) {
        if (Node* ownerNode = sheet->ownerNode())
            return ownerNode;
    }
    return 0;
}

Document* StyleSheetInternal::findDocument()
{
    Node* ownerNode = findStyleSheetOwnerNode();

    return ownerNode ? ownerNode->document() : 0;
}

void StyleSheetInternal::setMediaQueries(PassRefPtr<MediaQuerySet> mediaQueries)
{
    m_mediaQueries = mediaQueries;
}

void StyleSheetInternal::styleSheetChanged()
{
    StyleSheetInternal* rootSheet = this;
    while (StyleSheetInternal* parent = rootSheet->parentStyleSheet())
        rootSheet = parent;

    /* FIXME: We don't need to do everything updateStyleSelector does,
     * basically we just need to recreate the document's selector with the
     * already existing style sheets.
     */
    if (Document* documentToUpdate = rootSheet->findDocument())
        documentToUpdate->styleSelectorChanged(DeferRecalcStyle);
}

void StyleSheetInternal::updateBaseURL()
{
    if (!m_finalURL.isNull()) {
        m_parserContext.baseURL = m_finalURL;
        return;
    }
    if (StyleSheetInternal* parentSheet = parentStyleSheet()) {
        m_parserContext.baseURL = parentSheet->baseURL();
        return;
    }
    if (m_ownerNode) {
        m_parserContext.baseURL = m_ownerNode->document()->baseURL();
        return;
    }
    m_parserContext.baseURL = KURL();
}

KURL StyleSheetInternal::completeURL(const String& url) const
{
    return CSSParser::completeURL(m_parserContext, url);
}

void StyleSheetInternal::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    Deque<StyleSheetInternal*> styleSheetQueue;
    styleSheetQueue.append(this);

    while (!styleSheetQueue.isEmpty()) {
        StyleSheetInternal* styleSheet = styleSheetQueue.takeFirst();
        
        for (unsigned i = 0; i < styleSheet->m_importRules.size(); ++i) {
            StyleRuleImport* importRule = styleSheet->m_importRules[i].get();
            if (importRule->styleSheet()) {
                styleSheetQueue.append(importRule->styleSheet());
                addSubresourceURL(urls, importRule->styleSheet()->baseURL());
            }
        }
        for (unsigned i = 0; i < styleSheet->m_childRules.size(); ++i) {
            StyleRuleBase* rule = styleSheet->m_childRules[i].get();
            if (rule->isStyleRule())
                static_cast<StyleRule*>(rule)->properties()->addSubresourceStyleURLs(urls, this);
            else if (rule->isFontFaceRule())
                static_cast<StyleRuleFontFace*>(rule)->properties()->addSubresourceStyleURLs(urls, this);
        }
    }
}

StyleSheetInternal* StyleSheetInternal::parentStyleSheet() const
{
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : 0;
}

CSSStyleSheet::CSSStyleSheet(PassRefPtr<StyleSheetInternal> styleSheet, CSSImportRule* ownerRule)
    : m_internal(styleSheet)
    , m_isDisabled(false)
    , m_ownerRule(ownerRule)
{
}

CSSStyleSheet::~CSSStyleSheet()
{
    // For style rules outside the document, .parentStyleSheet can become null even if the style rule
    // is still observable from JavaScript. This matches the behavior of .parentNode for nodes, but
    // it's not ideal because it makes the CSSOM's behavior depend on the timing of garbage collection.
    for (unsigned i = 0; i < m_childRuleCSSOMWrappers.size(); ++i) {
        if (m_childRuleCSSOMWrappers[i])
            m_childRuleCSSOMWrappers[i]->setParentStyleSheet(0);
    }
}

void CSSStyleSheet::setDisabled(bool disabled)
{ 
    if (disabled == m_isDisabled)
        return;
    m_isDisabled = disabled; 
    m_internal->styleSheetChanged();
}

unsigned CSSStyleSheet::length() const
{
    return m_internal->ruleCount();
}

CSSRule* CSSStyleSheet::item(unsigned index)
{
    unsigned ruleCount = length();
    if (index >= ruleCount)
        return 0;

    if (m_childRuleCSSOMWrappers.isEmpty())
        m_childRuleCSSOMWrappers.grow(ruleCount);
    ASSERT(m_childRuleCSSOMWrappers.size() == ruleCount);
    
    RefPtr<CSSRule>& cssRule = m_childRuleCSSOMWrappers[index];
    if (!cssRule)
        cssRule = m_internal->createChildRuleCSSOMWrapper(index, this);
    return cssRule.get();
}

PassRefPtr<CSSRuleList> CSSStyleSheet::rules()
{
    KURL url = m_internal->finalURL();
    Document* document = m_internal->findDocument();
    if (!url.isEmpty() && document && !document->securityOrigin()->canRequest(url))
        return 0;
    // IE behavior.
    RefPtr<StaticCSSRuleList> nonCharsetRules = StaticCSSRuleList::create();
    unsigned ruleCount = length();
    for (unsigned i = 0; i < ruleCount; ++i) {
        CSSRule* rule = item(i);
        if (rule->isCharsetRule())
            continue;
        nonCharsetRules->rules().append(rule);
    }
    return nonCharsetRules.release();
}

unsigned CSSStyleSheet::insertRule(const String& ruleString, unsigned index, ExceptionCode& ec)
{
    ASSERT(m_childRuleCSSOMWrappers.isEmpty() || m_childRuleCSSOMWrappers.size() == m_internal->ruleCount());

    ec = 0;
    if (index > length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    CSSParser p(m_internal->parserContext());
    RefPtr<StyleRuleBase> rule = p.parseRule(m_internal.get(), ruleString);

    if (!rule) {
        ec = SYNTAX_ERR;
        return 0;
    }
    bool success = m_internal->wrapperInsertRule(rule, index);
    if (!success) {
        ec = HIERARCHY_REQUEST_ERR;
        return 0;
    }        
    if (!m_childRuleCSSOMWrappers.isEmpty())
        m_childRuleCSSOMWrappers.insert(index, RefPtr<CSSRule>());
    return index;
}

void CSSStyleSheet::deleteRule(unsigned index, ExceptionCode& ec)
{
    ASSERT(m_childRuleCSSOMWrappers.isEmpty() || m_childRuleCSSOMWrappers.size() == m_internal->ruleCount());

    ec = 0;
    if (index >= length()) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    m_internal->wrapperDeleteRule(index);

    if (!m_childRuleCSSOMWrappers.isEmpty()) {
        if (m_childRuleCSSOMWrappers[index])
            m_childRuleCSSOMWrappers[index]->setParentStyleSheet(0);
        m_childRuleCSSOMWrappers.remove(index);
    }
}

int CSSStyleSheet::addRule(const String& selector, const String& style, int index, ExceptionCode& ec)
{
    insertRule(selector + " { " + style + " }", index, ec);
    
    // As per Microsoft documentation, always return -1.
    return -1;
}

int CSSStyleSheet::addRule(const String& selector, const String& style, ExceptionCode& ec)
{
    return addRule(selector, style, length(), ec);
}


PassRefPtr<CSSRuleList> CSSStyleSheet::cssRules()
{
    KURL url = m_internal->finalURL();
    Document* document = m_internal->findDocument();
    if (!url.isEmpty() && document && !document->securityOrigin()->canRequest(url))
        return 0;
    if (!m_ruleListCSSOMWrapper)
        m_ruleListCSSOMWrapper = adoptPtr(new StyleSheetCSSRuleList(this));
    return m_ruleListCSSOMWrapper.get();
}

MediaList* CSSStyleSheet::media() const 
{ 
    if (!m_internal->mediaQueries())
        return 0;
    return m_internal->mediaQueries()->ensureMediaList(const_cast<CSSStyleSheet*>(this));
}

CSSStyleSheet* CSSStyleSheet::parentStyleSheet() const 
{ 
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : 0; 
}

void CSSStyleSheet::clearChildRuleCSSOMWrappers()
{
    m_childRuleCSSOMWrappers.clear();
}

}
