/*
 * Copyright (C) 2004-2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
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

#import "config.h"
#import "DOMCSS.h"

#import "DOMCSSInternal.h"
#import "CSSCharsetRule.h"
#import "CSSFontFaceRule.h"
#import "CSSImportRule.h"
#import "CSSMediaRule.h"
#import "CSSPageRule.h"
#import "CSSRule.h"
#import "CSSRuleList.h"
#import "CSSStyleRule.h"
#import "CSSStyleSheet.h"
#import "CSSValueList.h"
#import "Counter.h"
#import "DOMInternal.h"
#import "DOMWindow.h"
#import "Document.h"
#import "FoundationExtras.h"
#import "HTMLLinkElement.h"
#import "HTMLStyleElement.h"
#import "MediaList.h"
#import "ProcessingInstruction.h"
#import "RectImpl.h"
#import "StyleSheet.h"
#import "StyleSheetList.h"

#import <objc/objc-class.h>

using namespace WebCore;

typedef DOMWindow AbstractView;

//------------------------------------------------------------------------------------------
// DOMStyleSheet

@implementation DOMStyleSheet (WebCoreInternal)

- (id)_initWithStyleSheet:(StyleSheet *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMStyleSheet *)_styleSheetWith:(StyleSheet *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    Class wrapperClass;
    if (impl->isCSSStyleSheet())
        wrapperClass = [DOMCSSStyleSheet class];
    else
        wrapperClass = [DOMStyleSheet class];
    return [[[wrapperClass alloc] _initWithStyleSheet:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMStyleSheetList

@implementation DOMStyleSheetList (WebCoreInternal)

- (id)_initWithStyleSheetList:(StyleSheetList *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMStyleSheetList *)_styleSheetListWith:(StyleSheetList *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithStyleSheetList:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMCSSStyleSheet

@implementation DOMCSSStyleSheet (WebCoreInternal)

+ (DOMCSSStyleSheet *)_CSSStyleSheetWith:(CSSStyleSheet *)impl
{
    return static_cast<DOMCSSStyleSheet*>([DOMStyleSheet _styleSheetWith:impl]);
}

@end


//------------------------------------------------------------------------------------------
// DOMMediaList

@implementation DOMMediaList (WebCoreInternal)

- (id)_initWithMediaList:(MediaList *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMMediaList *)_mediaListWith:(MediaList *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithMediaList:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMCSSRuleList

@implementation DOMCSSRuleList (WebCoreInternal)

- (id)_initWithRuleList:(CSSRuleList *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMCSSRuleList *)_ruleListWith:(CSSRuleList *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithRuleList:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMCSSRule

@implementation DOMCSSRule (WebCoreInternal)

- (id)_initWithRule:(CSSRule *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMCSSRule *)_ruleWith:(CSSRule *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];

    Class wrapperClass = nil;
    switch (impl->type()) {
        case DOM_UNKNOWN_RULE:
            wrapperClass = [DOMCSSRule class];
            break;
        case DOM_STYLE_RULE:
            wrapperClass = [DOMCSSStyleRule class];
            break;
        case DOM_CHARSET_RULE:
            wrapperClass = [DOMCSSCharsetRule class];
            break;
        case DOM_IMPORT_RULE:
            wrapperClass = [DOMCSSImportRule class];
            break;
        case DOM_MEDIA_RULE:
            wrapperClass = [DOMCSSMediaRule class];
            break;
        case DOM_FONT_FACE_RULE:
            wrapperClass = [DOMCSSFontFaceRule class];
            break;
        case DOM_PAGE_RULE:
            wrapperClass = [DOMCSSPageRule class];
            break;
    }
    return [[[wrapperClass alloc] _initWithRule:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMCSSStyleDeclaration

@implementation DOMCSSStyleDeclaration (DOMCSSStyleDeclarationExtensions)

- (NSString *)getPropertyShorthand:(NSString *)propertyName
{
    return [self _styleDeclaration]->getPropertyShorthand(propertyName);
}

- (BOOL)isPropertyImplicit:(NSString *)propertyName
{
    return [self _styleDeclaration]->isPropertyImplicit(propertyName);
}

@end

@implementation DOMCSSStyleDeclaration (WebCoreInternal)

- (id)_initWithStyleDeclaration:(CSSStyleDeclaration *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMCSSStyleDeclaration *)_styleDeclarationWith:(CSSStyleDeclaration *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithStyleDeclaration:impl] autorelease];
}

- (CSSStyleDeclaration *)_styleDeclaration
{
    return DOM_cast<CSSStyleDeclaration*>(_internal);
}

@end

//------------------------------------------------------------------------------------------
// DOMCSSValue

@implementation DOMCSSValue (WebCoreInternal)

- (id)_initWithValue:(CSSValue *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMCSSValue *)_valueWith:(CSSValue *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    Class wrapperClass = nil;
    switch (impl->cssValueType()) {
        case DOM_CSS_INHERIT:
            wrapperClass = [DOMCSSValue class];
            break;
        case DOM_CSS_PRIMITIVE_VALUE:
            wrapperClass = [DOMCSSPrimitiveValue class];
            break;
        case DOM_CSS_VALUE_LIST:
            wrapperClass = [DOMCSSValueList class];
            break;
        case DOM_CSS_CUSTOM:
            wrapperClass = [DOMCSSValue class];
            break;
    }
    return [[[wrapperClass alloc] _initWithValue:impl] autorelease];
}

@end


//------------------------------------------------------------------------------------------
// DOMCSSPrimitiveValue

@implementation DOMCSSPrimitiveValue (WebCoreInternal)

+ (DOMCSSPrimitiveValue *)_valueWith:(CSSValue *)impl
{
    return static_cast<DOMCSSPrimitiveValue*>([DOMCSSValue _valueWith:impl]);
}

@end


//------------------------------------------------------------------------------------------
// DOMRGBColor

@implementation DOMRGBColor (WebPrivate)

- (NSColor *)_color
{
    return [self color];
}

@end


//------------------------------------------------------------------------------------------
// DOMRect

@implementation DOMRect (WebCoreInternal)

- (id)_initWithRect:(RectImpl *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMRect *)_rectWith:(RectImpl *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithRect:impl] autorelease];
}

@end

//------------------------------------------------------------------------------------------
// DOMCounter

@implementation DOMCounter (WebCoreInternal)

- (id)_initWithCounter:(Counter *)impl
{
    [super _init];
    _internal = DOM_cast<DOMObjectInternal*>(impl);
    impl->ref();
    addDOMWrapper(self, impl);
    return self;
}

+ (DOMCounter *)_counterWith:(Counter *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithCounter:impl] autorelease];
}

@end

//------------------------------------------------------------------------------------------

@implementation DOMCSSStyleDeclaration (DOMCSS2Properties)

- (NSString *)azimuth
{
    return [self getPropertyValue:@"azimuth"];
}

- (void)setAzimuth:(NSString *)azimuth
{
    [self setProperty:@"azimuth" :azimuth :@""];
}

- (NSString *)background
{
    return [self getPropertyValue:@"background"];
}

- (void)setBackground:(NSString *)background
{
    [self setProperty:@"background" :background :@""];
}

- (NSString *)backgroundAttachment
{
    return [self getPropertyValue:@"background-attachment"];
}

- (void)setBackgroundAttachment:(NSString *)backgroundAttachment
{
    [self setProperty:@"background-attachment" :backgroundAttachment :@""];
}

- (NSString *)backgroundColor
{
    return [self getPropertyValue:@"background-color"];
}

- (void)setBackgroundColor:(NSString *)backgroundColor
{
    [self setProperty:@"background-color" :backgroundColor :@""];
}

- (NSString *)backgroundImage
{
    return [self getPropertyValue:@"background-image"];
}

- (void)setBackgroundImage:(NSString *)backgroundImage
{
    [self setProperty:@"background-image" :backgroundImage :@""];
}

- (NSString *)backgroundPosition
{
    return [self getPropertyValue:@"background-position"];
}

- (void)setBackgroundPosition:(NSString *)backgroundPosition
{
    [self setProperty:@"background-position" :backgroundPosition :@""];
}

- (NSString *)backgroundRepeat
{
    return [self getPropertyValue:@"background-repeat"];
}

- (void)setBackgroundRepeat:(NSString *)backgroundRepeat
{
    [self setProperty:@"background-repeat" :backgroundRepeat :@""];
}

- (NSString *)border
{
    return [self getPropertyValue:@"border"];
}

- (void)setBorder:(NSString *)border
{
    [self setProperty:@"border" :border :@""];
}

- (NSString *)borderCollapse
{
    return [self getPropertyValue:@"border-collapse"];
}

- (void)setBorderCollapse:(NSString *)borderCollapse
{
    [self setProperty:@"border-collapse" :borderCollapse :@""];
}

- (NSString *)borderColor
{
    return [self getPropertyValue:@"border-color"];
}

- (void)setBorderColor:(NSString *)borderColor
{
    [self setProperty:@"border-color" :borderColor :@""];
}

- (NSString *)borderSpacing
{
    return [self getPropertyValue:@"border-spacing"];
}

- (void)setBorderSpacing:(NSString *)borderSpacing
{
    [self setProperty:@"border-spacing" :borderSpacing :@""];
}

- (NSString *)borderStyle
{
    return [self getPropertyValue:@"border-style"];
}

- (void)setBorderStyle:(NSString *)borderStyle
{
    [self setProperty:@"border-style" :borderStyle :@""];
}

- (NSString *)borderTop
{
    return [self getPropertyValue:@"border-top"];
}

- (void)setBorderTop:(NSString *)borderTop
{
    [self setProperty:@"border-top" :borderTop :@""];
}

- (NSString *)borderRight
{
    return [self getPropertyValue:@"border-right"];
}

- (void)setBorderRight:(NSString *)borderRight
{
    [self setProperty:@"border-right" :borderRight :@""];
}

- (NSString *)borderBottom
{
    return [self getPropertyValue:@"border-bottom"];
}

- (void)setBorderBottom:(NSString *)borderBottom
{
    [self setProperty:@"border-bottom" :borderBottom :@""];
}

- (NSString *)borderLeft
{
    return [self getPropertyValue:@"border-left"];
}

- (void)setBorderLeft:(NSString *)borderLeft
{
    [self setProperty:@"border-left" :borderLeft :@""];
}

- (NSString *)borderTopColor
{
    return [self getPropertyValue:@"border-top-color"];
}

- (void)setBorderTopColor:(NSString *)borderTopColor
{
    [self setProperty:@"border-top-color" :borderTopColor :@""];
}

- (NSString *)borderRightColor
{
    return [self getPropertyValue:@"border-right-color"];
}

- (void)setBorderRightColor:(NSString *)borderRightColor
{
    [self setProperty:@"border-right-color" :borderRightColor :@""];
}

- (NSString *)borderBottomColor
{
    return [self getPropertyValue:@"border-bottom-color"];
}

- (void)setBorderBottomColor:(NSString *)borderBottomColor
{
    [self setProperty:@"border-bottom-color" :borderBottomColor :@""];
}

- (NSString *)borderLeftColor
{
    return [self getPropertyValue:@"border-left-color"];
}

- (void)setBorderLeftColor:(NSString *)borderLeftColor
{
    [self setProperty:@"border-left-color" :borderLeftColor :@""];
}

- (NSString *)borderTopStyle
{
    return [self getPropertyValue:@"border-top-style"];
}

- (void)setBorderTopStyle:(NSString *)borderTopStyle
{
    [self setProperty:@"border-top-style" :borderTopStyle :@""];
}

- (NSString *)borderRightStyle
{
    return [self getPropertyValue:@"border-right-style"];
}

- (void)setBorderRightStyle:(NSString *)borderRightStyle
{
    [self setProperty:@"border-right-style" :borderRightStyle :@""];
}

- (NSString *)borderBottomStyle
{
    return [self getPropertyValue:@"border-bottom-style"];
}

- (void)setBorderBottomStyle:(NSString *)borderBottomStyle
{
    [self setProperty:@"border-bottom-style" :borderBottomStyle :@""];
}

- (NSString *)borderLeftStyle
{
    return [self getPropertyValue:@"border-left-style"];
}

- (void)setBorderLeftStyle:(NSString *)borderLeftStyle
{
    [self setProperty:@"border-left-style" :borderLeftStyle :@""];
}

- (NSString *)borderTopWidth
{
    return [self getPropertyValue:@"border-top-width"];
}

- (void)setBorderTopWidth:(NSString *)borderTopWidth
{
    [self setProperty:@"border-top-width" :borderTopWidth :@""];
}

- (NSString *)borderRightWidth
{
    return [self getPropertyValue:@"border-right-width"];
}

- (void)setBorderRightWidth:(NSString *)borderRightWidth
{
    [self setProperty:@"border-right-width" :borderRightWidth :@""];
}

- (NSString *)borderBottomWidth
{
    return [self getPropertyValue:@"border-bottom-width"];
}

- (void)setBorderBottomWidth:(NSString *)borderBottomWidth
{
    [self setProperty:@"border-bottom-width" :borderBottomWidth :@""];
}

- (NSString *)borderLeftWidth
{
    return [self getPropertyValue:@"border-left-width"];
}

- (void)setBorderLeftWidth:(NSString *)borderLeftWidth
{
    [self setProperty:@"border-left-width" :borderLeftWidth :@""];
}

- (NSString *)borderWidth
{
    return [self getPropertyValue:@"border-width"];
}

- (void)setBorderWidth:(NSString *)borderWidth
{
    [self setProperty:@"border-width" :borderWidth :@""];
}

- (NSString *)bottom
{
    return [self getPropertyValue:@"bottom"];
}

- (void)setBottom:(NSString *)bottom
{
    [self setProperty:@"bottom" :bottom :@""];
}

- (NSString *)captionSide
{
    return [self getPropertyValue:@"caption-side"];
}

- (void)setCaptionSide:(NSString *)captionSide
{
    [self setProperty:@"caption-side" :captionSide :@""];
}

- (NSString *)clear
{
    return [self getPropertyValue:@"clear"];
}

- (void)setClear:(NSString *)clear
{
    [self setProperty:@"clear" :clear :@""];
}

- (NSString *)clip
{
    return [self getPropertyValue:@"clip"];
}

- (void)setClip:(NSString *)clip
{
    [self setProperty:@"clip" :clip :@""];
}

- (NSString *)color
{
    return [self getPropertyValue:@"color"];
}

- (void)setColor:(NSString *)color
{
    [self setProperty:@"color" :color :@""];
}

- (NSString *)content
{
    return [self getPropertyValue:@"content"];
}

- (void)setContent:(NSString *)content
{
    [self setProperty:@"content" :content :@""];
}

- (NSString *)counterIncrement
{
    return [self getPropertyValue:@"counter-increment"];
}

- (void)setCounterIncrement:(NSString *)counterIncrement
{
    [self setProperty:@"counter-increment" :counterIncrement :@""];
}

- (NSString *)counterReset
{
    return [self getPropertyValue:@"counter-reset"];
}

- (void)setCounterReset:(NSString *)counterReset
{
    [self setProperty:@"counter-reset" :counterReset :@""];
}

- (NSString *)cue
{
    return [self getPropertyValue:@"cue"];
}

- (void)setCue:(NSString *)cue
{
    [self setProperty:@"cue" :cue :@""];
}

- (NSString *)cueAfter
{
    return [self getPropertyValue:@"cue-after"];
}

- (void)setCueAfter:(NSString *)cueAfter
{
    [self setProperty:@"cue-after" :cueAfter :@""];
}

- (NSString *)cueBefore
{
    return [self getPropertyValue:@"cue-before"];
}

- (void)setCueBefore:(NSString *)cueBefore
{
    [self setProperty:@"cue-before" :cueBefore :@""];
}

- (NSString *)cursor
{
    return [self getPropertyValue:@"cursor"];
}

- (void)setCursor:(NSString *)cursor
{
    [self setProperty:@"cursor" :cursor :@""];
}

- (NSString *)direction
{
    return [self getPropertyValue:@"direction"];
}

- (void)setDirection:(NSString *)direction
{
    [self setProperty:@"direction" :direction :@""];
}

- (NSString *)display
{
    return [self getPropertyValue:@"display"];
}

- (void)setDisplay:(NSString *)display
{
    [self setProperty:@"display" :display :@""];
}

- (NSString *)elevation
{
    return [self getPropertyValue:@"elevation"];
}

- (void)setElevation:(NSString *)elevation
{
    [self setProperty:@"elevation" :elevation :@""];
}

- (NSString *)emptyCells
{
    return [self getPropertyValue:@"empty-cells"];
}

- (void)setEmptyCells:(NSString *)emptyCells
{
    [self setProperty:@"empty-cells" :emptyCells :@""];
}

- (NSString *)cssFloat
{
    return [self getPropertyValue:@"css-float"];
}

- (void)setCssFloat:(NSString *)cssFloat
{
    [self setProperty:@"css-float" :cssFloat :@""];
}

- (NSString *)font
{
    return [self getPropertyValue:@"font"];
}

- (void)setFont:(NSString *)font
{
    [self setProperty:@"font" :font :@""];
}

- (NSString *)fontFamily
{
    return [self getPropertyValue:@"font-family"];
}

- (void)setFontFamily:(NSString *)fontFamily
{
    [self setProperty:@"font-family" :fontFamily :@""];
}

- (NSString *)fontSize
{
    return [self getPropertyValue:@"font-size"];
}

- (void)setFontSize:(NSString *)fontSize
{
    [self setProperty:@"font-size" :fontSize :@""];
}

- (NSString *)fontSizeAdjust
{
    return [self getPropertyValue:@"font-size-adjust"];
}

- (void)setFontSizeAdjust:(NSString *)fontSizeAdjust
{
    [self setProperty:@"font-size-adjust" :fontSizeAdjust :@""];
}

- (NSString *)_fontSizeDelta
{
    return [self getPropertyValue:@"-webkit-font-size-delta"];
}

- (void)_setFontSizeDelta:(NSString *)fontSizeDelta
{
    [self setProperty:@"-webkit-font-size-delta" :fontSizeDelta :@""];
}

- (NSString *)fontStretch
{
    return [self getPropertyValue:@"font-stretch"];
}

- (void)setFontStretch:(NSString *)fontStretch
{
    [self setProperty:@"font-stretch" :fontStretch :@""];
}

- (NSString *)fontStyle
{
    return [self getPropertyValue:@"font-style"];
}

- (void)setFontStyle:(NSString *)fontStyle
{
    [self setProperty:@"font-style" :fontStyle :@""];
}

- (NSString *)fontVariant
{
    return [self getPropertyValue:@"font-variant"];
}

- (void)setFontVariant:(NSString *)fontVariant
{
    [self setProperty:@"font-variant" :fontVariant :@""];
}

- (NSString *)fontWeight
{
    return [self getPropertyValue:@"font-weight"];
}

- (void)setFontWeight:(NSString *)fontWeight
{
    [self setProperty:@"font-weight" :fontWeight :@""];
}

- (NSString *)height
{
    return [self getPropertyValue:@"height"];
}

- (void)setHeight:(NSString *)height
{
    [self setProperty:@"height" :height :@""];
}

- (NSString *)left
{
    return [self getPropertyValue:@"left"];
}

- (void)setLeft:(NSString *)left
{
    [self setProperty:@"left" :left :@""];
}

- (NSString *)letterSpacing
{
    return [self getPropertyValue:@"letter-spacing"];
}

- (void)setLetterSpacing:(NSString *)letterSpacing
{
    [self setProperty:@"letter-spacing" :letterSpacing :@""];
}

- (NSString *)lineHeight
{
    return [self getPropertyValue:@"line-height"];
}

- (void)setLineHeight:(NSString *)lineHeight
{
    [self setProperty:@"line-height" :lineHeight :@""];
}

- (NSString *)listStyle
{
    return [self getPropertyValue:@"list-style"];
}

- (void)setListStyle:(NSString *)listStyle
{
    [self setProperty:@"list-style" :listStyle :@""];
}

- (NSString *)listStyleImage
{
    return [self getPropertyValue:@"list-style-image"];
}

- (void)setListStyleImage:(NSString *)listStyleImage
{
    [self setProperty:@"list-style-image" :listStyleImage :@""];
}

- (NSString *)listStylePosition
{
    return [self getPropertyValue:@"list-style-position"];
}

- (void)setListStylePosition:(NSString *)listStylePosition
{
    [self setProperty:@"list-style-position" :listStylePosition :@""];
}

- (NSString *)listStyleType
{
    return [self getPropertyValue:@"list-style-type"];
}

- (void)setListStyleType:(NSString *)listStyleType
{
    [self setProperty:@"list-style-type" :listStyleType :@""];
}

- (NSString *)margin
{
    return [self getPropertyValue:@"margin"];
}

- (void)setMargin:(NSString *)margin
{
    [self setProperty:@"margin" :margin :@""];
}

- (NSString *)marginTop
{
    return [self getPropertyValue:@"margin-top"];
}

- (void)setMarginTop:(NSString *)marginTop
{
    [self setProperty:@"margin-top" :marginTop :@""];
}

- (NSString *)marginRight
{
    return [self getPropertyValue:@"margin-right"];
}

- (void)setMarginRight:(NSString *)marginRight
{
    [self setProperty:@"margin-right" :marginRight :@""];
}

- (NSString *)marginBottom
{
    return [self getPropertyValue:@"margin-bottom"];
}

- (void)setMarginBottom:(NSString *)marginBottom
{
    [self setProperty:@"margin-bottom" :marginBottom :@""];
}

- (NSString *)marginLeft
{
    return [self getPropertyValue:@"margin-left"];
}

- (void)setMarginLeft:(NSString *)marginLeft
{
    [self setProperty:@"margin-left" :marginLeft :@""];
}

- (NSString *)markerOffset
{
    return [self getPropertyValue:@"marker-offset"];
}

- (void)setMarkerOffset:(NSString *)markerOffset
{
    [self setProperty:@"marker-offset" :markerOffset :@""];
}

- (NSString *)marks
{
    return [self getPropertyValue:@"marks"];
}

- (void)setMarks:(NSString *)marks
{
    [self setProperty:@"marks" :marks :@""];
}

- (NSString *)maxHeight
{
    return [self getPropertyValue:@"max-height"];
}

- (void)setMaxHeight:(NSString *)maxHeight
{
    [self setProperty:@"max-height" :maxHeight :@""];
}

- (NSString *)maxWidth
{
    return [self getPropertyValue:@"max-width"];
}

- (void)setMaxWidth:(NSString *)maxWidth
{
    [self setProperty:@"max-width" :maxWidth :@""];
}

- (NSString *)minHeight
{
    return [self getPropertyValue:@"min-height"];
}

- (void)setMinHeight:(NSString *)minHeight
{
    [self setProperty:@"min-height" :minHeight :@""];
}

- (NSString *)minWidth
{
    return [self getPropertyValue:@"min-width"];
}

- (void)setMinWidth:(NSString *)minWidth
{
    [self setProperty:@"min-width" :minWidth :@""];
}

- (NSString *)orphans
{
    return [self getPropertyValue:@"orphans"];
}

- (void)setOrphans:(NSString *)orphans
{
    [self setProperty:@"orphans" :orphans :@""];
}

- (NSString *)outline
{
    return [self getPropertyValue:@"outline"];
}

- (void)setOutline:(NSString *)outline
{
    [self setProperty:@"outline" :outline :@""];
}

- (NSString *)outlineColor
{
    return [self getPropertyValue:@"outline-color"];
}

- (void)setOutlineColor:(NSString *)outlineColor
{
    [self setProperty:@"outline-color" :outlineColor :@""];
}

- (NSString *)outlineStyle
{
    return [self getPropertyValue:@"outline-style"];
}

- (void)setOutlineStyle:(NSString *)outlineStyle
{
    [self setProperty:@"outline-style" :outlineStyle :@""];
}

- (NSString *)outlineWidth
{
    return [self getPropertyValue:@"outline-width"];
}

- (void)setOutlineWidth:(NSString *)outlineWidth
{
    [self setProperty:@"outline-width" :outlineWidth :@""];
}

- (NSString *)overflow
{
    return [self getPropertyValue:@"overflow"];
}

- (void)setOverflow:(NSString *)overflow
{
    [self setProperty:@"overflow" :overflow :@""];
}

- (NSString *)padding
{
    return [self getPropertyValue:@"padding"];
}

- (void)setPadding:(NSString *)padding
{
    [self setProperty:@"padding" :padding :@""];
}

- (NSString *)paddingTop
{
    return [self getPropertyValue:@"padding-top"];
}

- (void)setPaddingTop:(NSString *)paddingTop
{
    [self setProperty:@"padding-top" :paddingTop :@""];
}

- (NSString *)paddingRight
{
    return [self getPropertyValue:@"padding-right"];
}

- (void)setPaddingRight:(NSString *)paddingRight
{
    [self setProperty:@"padding-right" :paddingRight :@""];
}

- (NSString *)paddingBottom
{
    return [self getPropertyValue:@"padding-bottom"];
}

- (void)setPaddingBottom:(NSString *)paddingBottom
{
    [self setProperty:@"padding-bottom" :paddingBottom :@""];
}

- (NSString *)paddingLeft
{
    return [self getPropertyValue:@"padding-left"];
}

- (void)setPaddingLeft:(NSString *)paddingLeft
{
    [self setProperty:@"padding-left" :paddingLeft :@""];
}

- (NSString *)page
{
    return [self getPropertyValue:@"page"];
}

- (void)setPage:(NSString *)page
{
    [self setProperty:@"page" :page :@""];
}

- (NSString *)pageBreakAfter
{
    return [self getPropertyValue:@"page-break-after"];
}

- (void)setPageBreakAfter:(NSString *)pageBreakAfter
{
    [self setProperty:@"page-break-after" :pageBreakAfter :@""];
}

- (NSString *)pageBreakBefore
{
    return [self getPropertyValue:@"page-break-before"];
}

- (void)setPageBreakBefore:(NSString *)pageBreakBefore
{
    [self setProperty:@"page-break-before" :pageBreakBefore :@""];
}

- (NSString *)pageBreakInside
{
    return [self getPropertyValue:@"page-break-inside"];
}

- (void)setPageBreakInside:(NSString *)pageBreakInside
{
    [self setProperty:@"page-break-inside" :pageBreakInside :@""];
}

- (NSString *)pause
{
    return [self getPropertyValue:@"pause"];
}

- (void)setPause:(NSString *)pause
{
    [self setProperty:@"pause" :pause :@""];
}

- (NSString *)pauseAfter
{
    return [self getPropertyValue:@"pause-after"];
}

- (void)setPauseAfter:(NSString *)pauseAfter
{
    [self setProperty:@"pause-after" :pauseAfter :@""];
}

- (NSString *)pauseBefore
{
    return [self getPropertyValue:@"pause-before"];
}

- (void)setPauseBefore:(NSString *)pauseBefore
{
    [self setProperty:@"pause-before" :pauseBefore :@""];
}

- (NSString *)pitch
{
    return [self getPropertyValue:@"pitch"];
}

- (void)setPitch:(NSString *)pitch
{
    [self setProperty:@"pitch" :pitch :@""];
}

- (NSString *)pitchRange
{
    return [self getPropertyValue:@"pitch-range"];
}

- (void)setPitchRange:(NSString *)pitchRange
{
    [self setProperty:@"pitch-range" :pitchRange :@""];
}

- (NSString *)playDuring
{
    return [self getPropertyValue:@"play-during"];
}

- (void)setPlayDuring:(NSString *)playDuring
{
    [self setProperty:@"play-during" :playDuring :@""];
}

- (NSString *)position
{
    return [self getPropertyValue:@"position"];
}

- (void)setPosition:(NSString *)position
{
    [self setProperty:@"position" :position :@""];
}

- (NSString *)quotes
{
    return [self getPropertyValue:@"quotes"];
}

- (void)setQuotes:(NSString *)quotes
{
    [self setProperty:@"quotes" :quotes :@""];
}

- (NSString *)richness
{
    return [self getPropertyValue:@"richness"];
}

- (void)setRichness:(NSString *)richness
{
    [self setProperty:@"richness" :richness :@""];
}

- (NSString *)right
{
    return [self getPropertyValue:@"right"];
}

- (void)setRight:(NSString *)right
{
    [self setProperty:@"right" :right :@""];
}

- (NSString *)size
{
    return [self getPropertyValue:@"size"];
}

- (void)setSize:(NSString *)size
{
    [self setProperty:@"size" :size :@""];
}

- (NSString *)speak
{
    return [self getPropertyValue:@"speak"];
}

- (void)setSpeak:(NSString *)speak
{
    [self setProperty:@"speak" :speak :@""];
}

- (NSString *)speakHeader
{
    return [self getPropertyValue:@"speak-header"];
}

- (void)setSpeakHeader:(NSString *)speakHeader
{
    [self setProperty:@"speak-header" :speakHeader :@""];
}

- (NSString *)speakNumeral
{
    return [self getPropertyValue:@"speak-numeral"];
}

- (void)setSpeakNumeral:(NSString *)speakNumeral
{
    [self setProperty:@"speak-numeral" :speakNumeral :@""];
}

- (NSString *)speakPunctuation
{
    return [self getPropertyValue:@"speak-punctuation"];
}

- (void)setSpeakPunctuation:(NSString *)speakPunctuation
{
    [self setProperty:@"speak-punctuation" :speakPunctuation :@""];
}

- (NSString *)speechRate
{
    return [self getPropertyValue:@"speech-rate"];
}

- (void)setSpeechRate:(NSString *)speechRate
{
    [self setProperty:@"speech-rate" :speechRate :@""];
}

- (NSString *)stress
{
    return [self getPropertyValue:@"stress"];
}

- (void)setStress:(NSString *)stress
{
    [self setProperty:@"stress" :stress :@""];
}

- (NSString *)tableLayout
{
    return [self getPropertyValue:@"table-layout"];
}

- (void)setTableLayout:(NSString *)tableLayout
{
    [self setProperty:@"table-layout" :tableLayout :@""];
}

- (NSString *)textAlign
{
    return [self getPropertyValue:@"text-align"];
}

- (void)setTextAlign:(NSString *)textAlign
{
    [self setProperty:@"text-align" :textAlign :@""];
}

- (NSString *)textDecoration
{
    return [self getPropertyValue:@"text-decoration"];
}

- (void)setTextDecoration:(NSString *)textDecoration
{
    [self setProperty:@"text-decoration" :textDecoration :@""];
}

- (NSString *)textIndent
{
    return [self getPropertyValue:@"text-indent"];
}

- (void)setTextIndent:(NSString *)textIndent
{
    [self setProperty:@"text-indent" :textIndent :@""];
}

- (NSString *)textShadow
{
    return [self getPropertyValue:@"text-shadow"];
}

- (void)setTextShadow:(NSString *)textShadow
{
    [self setProperty:@"text-shadow" :textShadow :@""];
}

- (NSString *)textTransform
{
    return [self getPropertyValue:@"text-transform"];
}

- (void)setTextTransform:(NSString *)textTransform
{
    [self setProperty:@"text-transform" :textTransform :@""];
}

- (NSString *)top
{
    return [self getPropertyValue:@"top"];
}

- (void)setTop:(NSString *)top
{
    [self setProperty:@"top" :top :@""];
}

- (NSString *)unicodeBidi
{
    return [self getPropertyValue:@"unicode-bidi"];
}

- (void)setUnicodeBidi:(NSString *)unicodeBidi
{
    [self setProperty:@"unicode-bidi" :unicodeBidi :@""];
}

- (NSString *)verticalAlign
{
    return [self getPropertyValue:@"vertical-align"];
}

- (void)setVerticalAlign:(NSString *)verticalAlign
{
    [self setProperty:@"vertical-align" :verticalAlign :@""];
}

- (NSString *)visibility
{
    return [self getPropertyValue:@"visibility"];
}

- (void)setVisibility:(NSString *)visibility
{
    [self setProperty:@"visibility" :visibility :@""];
}

- (NSString *)voiceFamily
{
    return [self getPropertyValue:@"voice-family"];
}

- (void)setVoiceFamily:(NSString *)voiceFamily
{
    [self setProperty:@"voice-family" :voiceFamily :@""];
}

- (NSString *)volume
{
    return [self getPropertyValue:@"volume"];
}

- (void)setVolume:(NSString *)volume
{
    [self setProperty:@"volume" :volume :@""];
}

- (NSString *)whiteSpace
{
    return [self getPropertyValue:@"white-space"];
}

- (void)setWhiteSpace:(NSString *)whiteSpace
{
    [self setProperty:@"white-space" :whiteSpace :@""];
}

- (NSString *)widows
{
    return [self getPropertyValue:@"widows"];
}

- (void)setWidows:(NSString *)widows
{
    [self setProperty:@"widows" :widows :@""];
}

- (NSString *)width
{
    return [self getPropertyValue:@"width"];
}

- (void)setWidth:(NSString *)width
{
    [self setProperty:@"width" :width :@""];
}

- (NSString *)wordSpacing
{
    return [self getPropertyValue:@"word-spacing"];
}

- (void)setWordSpacing:(NSString *)wordSpacing
{
    [self setProperty:@"word-spacing" :wordSpacing :@""];
}

- (NSString *)zIndex
{
    return [self getPropertyValue:@"z-index"];
}

- (void)setZIndex:(NSString *)zIndex
{
    [self setProperty:@"z-index" :zIndex :@""];
}

@end

//------------------------------------------------------------------------------------------

@implementation DOMDocument (DOMViewCSS)

- (DOMCSSStyleDeclaration *)getComputedStyle:(DOMElement *)elt :(NSString *)pseudoElt
{
    Element* element = [elt _element];
    AbstractView* dv = [self _document]->defaultView();
    String pseudoEltString(pseudoElt);
    
    if (!dv)
        return nil;
    
    return [DOMCSSStyleDeclaration _styleDeclarationWith:dv->getComputedStyle(element, pseudoEltString.impl()).get()];
}

- (DOMCSSRuleList *)getMatchedCSSRules:(DOMElement *)elt :(NSString *)pseudoElt
{
    AbstractView* dv = [self _document]->defaultView();

    if (!dv)
        return nil;
    
    // The parameter of "false" is handy for the DOM inspector and lets us see user agent and user rules.
    return [DOMCSSRuleList _ruleListWith:dv->getMatchedCSSRules([elt _element], String(pseudoElt).impl(), false).get()];
}

@end
