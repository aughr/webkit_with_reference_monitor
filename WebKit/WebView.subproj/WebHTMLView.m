/*
    WebHTMLView.m
    Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebHTMLView.h>

#import <WebKit/DOM.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/WebArchive.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebClipView.h>
#import <WebKit/WebDataProtocol.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDocumentInternal.h>
#import <WebKit/WebDOMOperationsPrivate.h>
#import <WebKit/WebEditingDelegate.h>
#import <WebKit/WebException.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebFrameViewInternal.h>
#import <WebKit/WebHTMLViewInternal.h>
#import <WebKit/WebHTMLRepresentationPrivate.h>
#import <WebKit/WebImageRenderer.h>
#import <WebKit/WebImageRendererFactory.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebNetscapePluginEmbeddedView.h>
#import <WebKit/WebNSEventExtras.h>
#import <WebKit/WebNSImageExtras.h>
#import <WebKit/WebNSObjectExtras.h>
#import <WebKit/WebNSPasteboardExtras.h>
#import <WebKit/WebNSPrintOperationExtras.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKit/WebNSViewExtras.h>
#import <WebKit/WebPluginController.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebResourcePrivate.h>
#import <WebKit/WebStringTruncator.h>
#import <WebKit/WebTextRenderer.h>
#import <WebKit/WebTextRendererFactory.h>
#import <WebKit/WebUIDelegatePrivate.h>
#import <WebKit/WebUnicode.h>
#import <WebKit/WebViewInternal.h>
#import <WebKit/WebViewPrivate.h>

#import <AppKit/NSAccessibility.h>
#import <AppKit/NSGraphicsContextPrivate.h>
#import <AppKit/NSResponder_Private.h>

#import <Foundation/NSFileManager_NSURLExtras.h>
#import <Foundation/NSURL_NSURLExtras.h>
#import <Foundation/NSURLFileTypeMappings.h>

#import <CoreGraphics/CGContextGState.h>

// Included to help work around this bug:
// <rdar://problem/3630640>: "Calling interpretKeyEvents: in a custom text view can fail to process keys right after app startup"
#import <AppKit/NSKeyBindingManager.h>

// Kill ring calls. Would be better to use NSKillRing.h, but that's not available in SPI.
void _NSInitializeKillRing(void);
void _NSAppendToKillRing(NSString *);
void _NSPrependToKillRing(NSString *);
NSString *_NSYankFromKillRing(void);
NSString *_NSYankPreviousFromKillRing(void);
void _NSNewKillRingSequence(void);
void _NSSetKillRingToYankedState(void);
void _NSResetKillRingOperationFlag(void);

@interface NSView (AppKitSecretsIKnowAbout)
- (void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)rect isVisibleRect:(BOOL)isVisibleRect rectIsVisibleRectForView:(NSView *)visibleView topView:(BOOL)topView;
- (void)_recursiveDisplayAllDirtyWithLockFocus:(BOOL)needsLockFocus visRect:(NSRect)visRect;
- (NSRect)_dirtyRect;
- (void)_setDrawsOwnDescendants:(BOOL)drawsOwnDescendants;
@end

@interface NSApplication (AppKitSecretsIKnowAbout)
- (void)speakString:(NSString *)string;
@end

@interface NSWindow (AppKitSecretsIKnowAbout)
- (id)_newFirstResponderAfterResigning;
@end

@interface NSAttributedString (AppKitSecretsIKnowAbout)
- (id)_initWithDOMRange:(DOMRange *)domRange;
- (DOMDocumentFragment *)_documentFromRange:(NSRange)range document:(DOMDocument *)document documentAttributes:(NSDictionary *)dict subresources:(NSArray **)subresources;
@end

@interface NSSpellChecker (CurrentlyPrivateForTextView)
- (void)learnWord:(NSString *)word;
@end

// By imaging to a width a little wider than the available pixels,
// thin pages will be scaled down a little, matching the way they
// print in IE and Camino. This lets them use fewer sheets than they
// would otherwise, which is presumably why other browsers do this.
// Wide pages will be scaled down more than this.
#define PrintingMinimumShrinkFactor     1.25

// This number determines how small we are willing to reduce the page content
// in order to accommodate the widest line. If the page would have to be
// reduced smaller to make the widest line fit, we just clip instead (this
// behavior matches MacIE and Mozilla, at least)
#define PrintingMaximumShrinkFactor     2.0

#define AUTOSCROLL_INTERVAL             0.1

#define DRAG_LABEL_BORDER_X             4.0
#define DRAG_LABEL_BORDER_Y             2.0
#define DRAG_LABEL_RADIUS               5.0
#define DRAG_LABEL_BORDER_Y_OFFSET              2.0

#define MIN_DRAG_LABEL_WIDTH_BEFORE_CLIP        120.0
#define MAX_DRAG_LABEL_WIDTH                    320.0

#define DRAG_LINK_LABEL_FONT_SIZE   11.0
#define DRAG_LINK_URL_FONT_SIZE   10.0

#ifndef OMIT_TIGER_FEATURES
#define USE_APPKIT_FOR_ATTRIBUTED_STRINGS
#endif

// Any non-zero value will do, but using something recognizable might help us debug some day.
#define TRACKING_RECT_TAG 0xBADFACE

// FIXME: This constant is copied from AppKit's _NXSmartPaste constant.
#define WebSmartPastePboardType @"NeXT smart paste pasteboard type"

static BOOL forceRealHitTest = NO;

@interface WebHTMLView (WebTextSizing) <_web_WebDocumentTextSizing>
@end

@interface WebHTMLView (WebHTMLViewFileInternal)
- (BOOL)_imageExistsAtPaths:(NSArray *)paths;
- (DOMDocumentFragment *)_documentFragmentFromPasteboard:(NSPasteboard *)pasteboard allowPlainText:(BOOL)allowPlainText;
- (void)_pasteWithPasteboard:(NSPasteboard *)pasteboard allowPlainText:(BOOL)allowPlainText;
- (BOOL)_shouldInsertFragment:(DOMDocumentFragment *)fragment replacingDOMRange:(DOMRange *)range givenAction:(WebViewInsertAction)action;
- (BOOL)_shouldReplaceSelectionWithText:(NSString *)text givenAction:(WebViewInsertAction)action;
- (float)_calculatePrintHeight;
- (void)_updateTextSizeMultiplier;
- (DOMRange *)_selectedRange;
- (BOOL)_shouldDeleteRange:(DOMRange *)range;
- (void)_deleteRange:(DOMRange *)range 
           preflight:(BOOL)preflight 
            killRing:(BOOL)killRing 
             prepend:(BOOL)prepend 
       smartDeleteOK:(BOOL)smartDeleteOK;
- (void)_deleteSelection;
- (BOOL)_canSmartReplaceWithPasteboard:(NSPasteboard *)pasteboard;
@end

@interface WebHTMLView (WebForwardDeclaration) // FIXME: Put this in a normal category and stop doing the forward declaration trick.
- (void)_setPrinting:(BOOL)printing minimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustViewSize:(BOOL)adjustViewSize;
@end

@interface WebHTMLView (WebNSTextInputSupport) <NSTextInput>
- (void)_updateSelectionForInputManager;
- (void)_insertText:(NSString *)text selectInsertedText:(BOOL)selectText;
@end

@interface NSView (WebHTMLViewFileInternal)
- (void)_web_setPrintingModeRecursive;
- (void)_web_clearPrintingModeRecursive;
- (void)_web_layoutIfNeededRecursive:(NSRect)rect testDirtyRect:(bool)testDirtyRect;
@end

@interface NSMutableDictionary (WebHTMLViewFileInternal)
- (void)_web_setObjectIfNotNil:(id)object forKey:(id)key;
@end

// Handles the complete: text command
@interface WebTextCompleteController : NSObject
{
@private
    WebHTMLView *_view;
    NSWindow *_popupWindow;
    NSTableView *_tableView;
    NSArray *_completions;
    NSString *_originalString;
    int prefixLength;
}
- (id)initWithHTMLView:(WebHTMLView *)view;
- (void)doCompletion;
- (void)endRevertingChange:(BOOL)revertChange moveLeft:(BOOL)goLeft;
- (BOOL)filterKeyDown:(NSEvent *)event;
- (void)_reflectSelection;
@end

@implementation WebHTMLViewPrivate

- (void)dealloc
{
    ASSERT(autoscrollTimer == nil);
    ASSERT(autoscrollTriggerEvent == nil);
    
    [mouseDownEvent release];
    [draggingImageURL release];
    [pluginController release];
    [toolTip release];
    [compController release];

    [super dealloc];
}

@end

@implementation WebHTMLView (WebHTMLViewFileInternal)

- (BOOL)_imageExistsAtPaths:(NSArray *)paths
{
    NSURLFileTypeMappings *mappings = [NSURLFileTypeMappings sharedMappings];
    NSArray *imageMIMETypes = [[WebImageRendererFactory sharedFactory] supportedMIMETypes];
    NSEnumerator *enumerator = [paths objectEnumerator];
    NSString *path;
    
    while ((path = [enumerator nextObject]) != nil) {
        NSString *MIMEType = [mappings MIMETypeForExtension:[path pathExtension]];
        if ([imageMIMETypes containsObject:MIMEType]) {
            return YES;
        }
    }
    
    return NO;
}

- (DOMDocumentFragment *)_documentFragmentWithPaths:(NSArray *)paths
{
    DOMDocumentFragment *fragment = [[[self _bridge] DOMDocument] createDocumentFragment];
    NSURLFileTypeMappings *mappings = [NSURLFileTypeMappings sharedMappings];
    NSArray *imageMIMETypes = [[WebImageRendererFactory sharedFactory] supportedMIMETypes];
    NSEnumerator *enumerator = [paths objectEnumerator];
    WebDataSource *dataSource = [self _dataSource];
    NSString *path;
    
    while ((path = [enumerator nextObject]) != nil) {
        NSString *MIMEType = [mappings MIMETypeForExtension:[path pathExtension]];
        if ([imageMIMETypes containsObject:MIMEType]) {
            WebResource *resource = [[WebResource alloc] initWithData:[NSData dataWithContentsOfFile:path]
                                                                  URL:[NSURL fileURLWithPath:path]
                                                             MIMEType:MIMEType 
                                                     textEncodingName:nil
                                                            frameName:nil];
            if (resource) {
                [fragment appendChild:[dataSource _imageElementWithImageResource:resource]];
                [resource release];
            }
        }
    }
    
    return [fragment firstChild] != nil ? fragment : nil;
}

- (DOMDocumentFragment *)_documentFragmentFromPasteboard:(NSPasteboard *)pasteboard allowPlainText:(BOOL)allowPlainText
{
    NSArray *types = [pasteboard types];

    if ([types containsObject:WebArchivePboardType]) {
        WebArchive *archive = [[WebArchive alloc] initWithData:[pasteboard dataForType:WebArchivePboardType]];
        if (archive) {
            DOMDocumentFragment *fragment = [[self _dataSource] _documentFragmentWithArchive:archive];
            [archive release];
            if (fragment) {
                return fragment;
            }
        }
    }
    
    if ([types containsObject:NSFilenamesPboardType]) {
        DOMDocumentFragment *fragment = [self _documentFragmentWithPaths:[pasteboard propertyListForType:NSFilenamesPboardType]];
        if (fragment != nil) {
            return fragment;
        }
    }
    
    NSURL *URL;
    
    if ([types containsObject:NSHTMLPboardType]) {
        NSString *HTMLString = [pasteboard stringForType:NSHTMLPboardType];
        // This is a hack to make Microsoft's HTML pasteboard data work. See 3778785.
        if ([HTMLString hasPrefix:@"Version:"]) {
            NSRange range = [HTMLString rangeOfString:@"<html" options:NSCaseInsensitiveSearch];
            if (range.location != NSNotFound) {
                HTMLString = [HTMLString substringFromIndex:range.location];
            }
        }
        if ([HTMLString length] != 0) {
            return [[self _bridge] documentFragmentWithMarkupString:HTMLString baseURLString:nil];
        }
    }
    
    if ([types containsObject:NSTIFFPboardType]) {
        WebResource *resource = [[WebResource alloc] initWithData:[pasteboard dataForType:NSTIFFPboardType]
                                                              URL:[NSURL _web_uniqueWebDataURLWithRelativeString:@"/image.tiff"]
                                                         MIMEType:@"image/tiff" 
                                                 textEncodingName:nil
                                                        frameName:nil];
        DOMDocumentFragment *fragment = [[self _dataSource] _documentFragmentWithImageResource:resource];
        [resource release];
        return fragment;
    }
    
    if ([types containsObject:NSPICTPboardType]) {
        WebResource *resource = [[WebResource alloc] initWithData:[pasteboard dataForType:NSPICTPboardType]
                                                              URL:[NSURL _web_uniqueWebDataURLWithRelativeString:@"/image.pict"]
                                                         MIMEType:@"image/pict" 
                                                 textEncodingName:nil
                                                        frameName:nil];
        DOMDocumentFragment *fragment = [[self _dataSource] _documentFragmentWithImageResource:resource];
        [resource release];
        return fragment;
    }
    
#ifdef USE_APPKIT_FOR_ATTRIBUTED_STRINGS
    NSAttributedString *string = nil;
    if ([types containsObject:NSRTFDPboardType]) {
        string = [[NSAttributedString alloc] initWithRTFD:[pasteboard dataForType:NSRTFDPboardType] documentAttributes:NULL];
    }
    if (string == nil && [types containsObject:NSRTFPboardType]) {
        string = [[NSAttributedString alloc] initWithRTF:[pasteboard dataForType:NSRTFPboardType] documentAttributes:NULL];
    }
    if (string != nil) {
        NSArray *elements = [[NSArray alloc] initWithObjects:@"style", nil];
        NSDictionary *documentAttributes = [[NSDictionary alloc] initWithObjectsAndKeys:elements, NSExcludedElementsDocumentAttribute, nil];
        [elements release];
        NSArray *subresources;
        DOMDocumentFragment *fragment = [string _documentFromRange:NSMakeRange(0, [string length]) 
                                                          document:[[self _bridge] DOMDocument] 
                                                documentAttributes:documentAttributes
                                                      subresources:&subresources];
        [documentAttributes release];
        [string release];
        if (fragment) {
            if ([subresources count] != 0) {
                [[self _dataSource] _addSubresources:subresources];
            }
            return fragment;
        }
    }
#endif
    
    if ((URL = [NSURL URLFromPasteboard:pasteboard])) {
        NSString *URLString = [URL _web_userVisibleString];
        if ([URLString length] > 0) {
            return [[self _bridge] documentFragmentWithText:URLString];
        }
    }
    
    if (allowPlainText && [types containsObject:NSStringPboardType]) {
        return [[self _bridge] documentFragmentWithText:[pasteboard stringForType:NSStringPboardType]];
    }
    
    return nil;
}

- (void)_pasteWithPasteboard:(NSPasteboard *)pasteboard allowPlainText:(BOOL)allowPlainText
{
    DOMDocumentFragment *fragment = [self _documentFragmentFromPasteboard:pasteboard allowPlainText:allowPlainText];
    WebBridge *bridge = [self _bridge];
    if (fragment && [self _shouldInsertFragment:fragment replacingDOMRange:[self _selectedRange] givenAction:WebViewInsertActionPasted]) {
        [bridge replaceSelectionWithFragment:fragment selectReplacement:NO smartReplace:[self _canSmartReplaceWithPasteboard:pasteboard]];
    }
}

- (BOOL)_shouldInsertFragment:(DOMDocumentFragment *)fragment replacingDOMRange:(DOMRange *)range givenAction:(WebViewInsertAction)action
{
    WebView *webView = [self _webView];
    DOMNode *child = [fragment firstChild];
    if ([fragment lastChild] == child && [child isKindOfClass:[DOMCharacterData class]]) {
        return [[webView _editingDelegateForwarder] webView:webView shouldInsertText:[(DOMCharacterData *)child data] replacingDOMRange:range givenAction:action];
    } else {
        return [[webView _editingDelegateForwarder] webView:webView shouldInsertNode:fragment replacingDOMRange:range givenAction:action];
    }
}

- (BOOL)_shouldReplaceSelectionWithText:(NSString *)text givenAction:(WebViewInsertAction)action
{
    WebView *webView = [self _webView];
    DOMRange *selectedRange = [self _selectedRange];
    return [[webView _editingDelegateForwarder] webView:webView shouldInsertText:text replacingDOMRange:selectedRange givenAction:action];
}

// Calculate the vertical size of the view that fits on a single page
- (float)_calculatePrintHeight
{
    // Obtain the print info object for the current operation
    NSPrintInfo *pi = [[NSPrintOperation currentOperation] printInfo];
    
    // Calculate the page height in points
    NSSize paperSize = [pi paperSize];
    return paperSize.height - [pi topMargin] - [pi bottomMargin];
}

- (void)_updateTextSizeMultiplier
{
    [[self _bridge] setTextSizeMultiplier:[[self _webView] textSizeMultiplier]];    
}

- (DOMRange *)_selectedRange
{
    return [[self _bridge] selectedDOMRange];
}

- (BOOL)_shouldDeleteRange:(DOMRange *)range
{
    if (range == nil || [range collapsed])
        return NO;
    WebView *webView = [self _webView];
    return [[webView _editingDelegateForwarder] webView:webView shouldDeleteDOMRange:range];
}

- (void)_deleteRange:(DOMRange *)range 
           preflight:(BOOL)preflight 
            killRing:(BOOL)killRing 
             prepend:(BOOL)prepend 
       smartDeleteOK:(BOOL)smartDeleteOK 
{
    if (![self _shouldDeleteRange:range]) {
        return;
    }
    WebBridge *bridge = [self _bridge];
    if (killRing && _private->startNewKillRingSequence) {
        _NSNewKillRingSequence();
    }
    [bridge setSelectedDOMRange:range affinity:NSSelectionAffinityUpstream];
    if (killRing) {
        if (prepend) {
            _NSPrependToKillRing([bridge selectedString]);
        } else {
            _NSAppendToKillRing([bridge selectedString]);
        }
        _private->startNewKillRingSequence = NO;
    }
    BOOL smartDelete = smartDeleteOK ? [self _canSmartCopyOrDelete] : NO;
    [bridge deleteSelectionWithSmartDelete:smartDelete];
}

- (void)_deleteSelection
{
    [self _deleteRange:[self _selectedRange]
             preflight:YES 
              killRing:YES 
               prepend:NO
         smartDeleteOK:YES];
}

- (BOOL)_canSmartReplaceWithPasteboard:(NSPasteboard *)pasteboard
{
    return [[self _webView] smartInsertDeleteEnabled] && [[pasteboard types] containsObject:WebSmartPastePboardType];
}

@end

@implementation WebHTMLView (WebPrivate)

- (void)_reset
{
    [WebImageRenderer stopAnimationsInView:self];
}

+ (void)_postFlagsChangedEvent:(NSEvent *)flagsChangedEvent
{
    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
        location:[[flagsChangedEvent window] convertScreenToBase:[NSEvent mouseLocation]]
        modifierFlags:[flagsChangedEvent modifierFlags]
        timestamp:[flagsChangedEvent timestamp]
        windowNumber:[flagsChangedEvent windowNumber]
        context:[flagsChangedEvent context]
        eventNumber:0 clickCount:0 pressure:0];

    // Pretend it's a mouse move.
    [[NSNotificationCenter defaultCenter]
        postNotificationName:NSMouseMovedNotification object:self
        userInfo:[NSDictionary dictionaryWithObject:fakeEvent forKey:@"NSEvent"]];
}

- (void)_updateMouseoverWithFakeEvent
{
    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
        location:[[self window] convertScreenToBase:[NSEvent mouseLocation]]
        modifierFlags:[[NSApp currentEvent] modifierFlags]
        timestamp:[NSDate timeIntervalSinceReferenceDate]
        windowNumber:[[self window] windowNumber]
        context:[[NSApp currentEvent] context]
        eventNumber:0 clickCount:0 pressure:0];
    
    [self _updateMouseoverWithEvent:fakeEvent];
}

- (void)_frameOrBoundsChanged
{
    if (!NSEqualSizes(_private->lastLayoutSize, [(NSClipView *)[self superview] documentVisibleRect].size)) {
        [self setNeedsLayout:YES];
        [self setNeedsDisplay:YES];
        [_private->compController endRevertingChange:NO moveLeft:NO];
    }

    NSPoint origin = [[self superview] bounds].origin;
    if (!NSEqualPoints(_private->lastScrollPosition, origin)) {
        [[self _bridge] sendScrollEvent];
        [_private->compController endRevertingChange:NO moveLeft:NO];
    }
    _private->lastScrollPosition = origin;

    SEL selector = @selector(_updateMouseoverWithFakeEvent);
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
    [self performSelector:selector withObject:nil afterDelay:0];
}

- (void)_setAsideSubviews
{
    ASSERT(!_private->subviewsSetAside);
    ASSERT(_private->savedSubviews == nil);
    _private->savedSubviews = _subviews;
    _subviews = nil;
    _private->subviewsSetAside = YES;
 }
 
 - (void)_restoreSubviews
 {
    ASSERT(_private->subviewsSetAside);
    ASSERT(_subviews == nil);
    _subviews = _private->savedSubviews;
    _private->savedSubviews = nil;
    _private->subviewsSetAside = NO;
}

// Don't let AppKit even draw subviews. We take care of that.
- (void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)rect isVisibleRect:(BOOL)isVisibleRect rectIsVisibleRectForView:(NSView *)visibleView topView:(BOOL)topView
{
    // This helps when we print as part of a larger print process.
    // If the WebHTMLView itself is what we're printing, then we will never have to do this.
    BOOL wasInPrintingMode = _private->printing;
    BOOL isPrinting = ![NSGraphicsContext currentContextDrawingToScreen];
    if (wasInPrintingMode != isPrinting) {
        if (isPrinting) {
            [self _web_setPrintingModeRecursive];
        } else {
            [self _web_clearPrintingModeRecursive];
        }
    }

    [self _web_layoutIfNeededRecursive: rect testDirtyRect:YES];

    [self _setAsideSubviews];
    [super _recursiveDisplayRectIfNeededIgnoringOpacity:rect isVisibleRect:isVisibleRect
        rectIsVisibleRectForView:visibleView topView:topView];
    [self _restoreSubviews];

    if (wasInPrintingMode != isPrinting) {
        if (wasInPrintingMode) {
            [self _web_setPrintingModeRecursive];
        } else {
            [self _web_clearPrintingModeRecursive];
        }
    }
}

// Don't let AppKit even draw subviews. We take care of that.
- (void)_recursiveDisplayAllDirtyWithLockFocus:(BOOL)needsLockFocus visRect:(NSRect)visRect
{
    BOOL needToSetAsideSubviews = !_private->subviewsSetAside;

    BOOL wasInPrintingMode = _private->printing;
    BOOL isPrinting = ![NSGraphicsContext currentContextDrawingToScreen];

    if (needToSetAsideSubviews) {
        // This helps when we print as part of a larger print process.
        // If the WebHTMLView itself is what we're printing, then we will never have to do this.
        if (wasInPrintingMode != isPrinting) {
            if (isPrinting) {
                [self _web_setPrintingModeRecursive];
            } else {
                [self _web_clearPrintingModeRecursive];
            }
        }

        [self _web_layoutIfNeededRecursive: visRect testDirtyRect:NO];

        [self _setAsideSubviews];
    }
    
    [super _recursiveDisplayAllDirtyWithLockFocus:needsLockFocus visRect:visRect];
    
    if (needToSetAsideSubviews) {
        if (wasInPrintingMode != isPrinting) {
            if (wasInPrintingMode) {
                [self _web_setPrintingModeRecursive];
            } else {
                [self _web_clearPrintingModeRecursive];
            }
        }

        [self _restoreSubviews];
    }
}

- (BOOL)_insideAnotherHTMLView
{
    NSView *view = self;
    while ((view = [view superview])) {
        if ([view isKindOfClass:[WebHTMLView class]]) {
            return YES;
        }
    }
    return NO;
}

- (void)scrollPoint:(NSPoint)point
{
    // Since we can't subclass NSTextView to do what we want, we have to second guess it here.
    // If we get called during the handling of a key down event, we assume the call came from
    // NSTextView, and ignore it and use our own code to decide how to page up and page down
    // We are smarter about how far to scroll, and we have "superview scrolling" logic.
    NSEvent *event = [[self window] currentEvent];
    if ([event type] == NSKeyDown) {
        const unichar pageUp = NSPageUpFunctionKey;
        if ([[event characters] rangeOfString:[NSString stringWithCharacters:&pageUp length:1]].length == 1) {
            [self tryToPerform:@selector(scrollPageUp:) with:nil];
            return;
        }
        const unichar pageDown = NSPageDownFunctionKey;
        if ([[event characters] rangeOfString:[NSString stringWithCharacters:&pageDown length:1]].length == 1) {
            [self tryToPerform:@selector(scrollPageDown:) with:nil];
            return;
        }
    }
    
    [super scrollPoint:point];
}

- (NSView *)hitTest:(NSPoint)point
{
    // WebHTMLView objects handle all left mouse clicks for objects inside them.
    // That does not include left mouse clicks with the control key held down.
    BOOL captureHitsOnSubviews;
    if (forceRealHitTest) {
        captureHitsOnSubviews = NO;
    } else {
        NSEvent *event = [[self window] currentEvent];
        captureHitsOnSubviews = [event type] == NSLeftMouseDown && ([event modifierFlags] & NSControlKeyMask) == 0;
    }
    if (!captureHitsOnSubviews) {
        return [super hitTest:point];
    }
    if ([[self superview] mouse:point inRect:[self frame]]) {
        return self;
    }
    return nil;
}

static WebHTMLView *lastHitView = nil;

- (void)_clearLastHitViewIfSelf
{
    if (lastHitView == self) {
        lastHitView = nil;
    }
}

- (NSTrackingRectTag)addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside
{
    ASSERT(_private->trackingRectOwner == nil);
    _private->trackingRectOwner = owner;
    _private->trackingRectUserData = data;
    return TRACKING_RECT_TAG;
}

- (NSTrackingRectTag)_addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside useTrackingNum:(int)tag
{
    ASSERT(tag == TRACKING_RECT_TAG);
    return [self addTrackingRect:rect owner:owner userData:data assumeInside:assumeInside];
}

- (void)removeTrackingRect:(NSTrackingRectTag)tag
{
    ASSERT(tag == TRACKING_RECT_TAG);
    if (_private != nil) {
        _private->trackingRectOwner = nil;
    }
}

- (void)_sendToolTipMouseExited
{
    // Nothing matters except window, trackingNumber, and userData.
    NSEvent *fakeEvent = [NSEvent enterExitEventWithType:NSMouseExited
        location:NSMakePoint(0, 0)
        modifierFlags:0
        timestamp:0
        windowNumber:[[self window] windowNumber]
        context:NULL
        eventNumber:0
        trackingNumber:TRACKING_RECT_TAG
        userData:_private->trackingRectUserData];
    [_private->trackingRectOwner mouseExited:fakeEvent];
}

- (void)_sendToolTipMouseEntered
{
    // Nothing matters except window, trackingNumber, and userData.
    NSEvent *fakeEvent = [NSEvent enterExitEventWithType:NSMouseEntered
        location:NSMakePoint(0, 0)
        modifierFlags:0
        timestamp:0
        windowNumber:[[self window] windowNumber]
        context:NULL
        eventNumber:0
        trackingNumber:TRACKING_RECT_TAG
        userData:_private->trackingRectUserData];
    [_private->trackingRectOwner mouseEntered:fakeEvent];
}

- (void)_setToolTip:(NSString *)string
{
    NSString *toolTip = [string length] == 0 ? nil : string;
    NSString *oldToolTip = _private->toolTip;
    if ((toolTip == nil || oldToolTip == nil) ? toolTip == oldToolTip : [toolTip isEqualToString:oldToolTip]) {
        return;
    }
    if (oldToolTip) {
        [self _sendToolTipMouseExited];
        [oldToolTip release];
    }
    _private->toolTip = [toolTip copy];
    if (toolTip) {
        [self removeAllToolTips];
        NSRect wideOpenRect = NSMakeRect(-100000, -100000, 200000, 200000);
        [self addToolTipRect:wideOpenRect owner:self userData:NULL];
        [self _sendToolTipMouseEntered];
    }
}

- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data
{
    return [[_private->toolTip copy] autorelease];
}

- (void)_updateMouseoverWithEvent:(NSEvent *)event
{
    WebHTMLView *view = nil;
    if ([event window] == [self window]) {
        forceRealHitTest = YES;
        NSView *hitView = [[[self window] contentView] hitTest:[event locationInWindow]];
        forceRealHitTest = NO;
        while (hitView) {
            if ([hitView isKindOfClass:[WebHTMLView class]]) {
                view = (WebHTMLView *)hitView;
                break;
            }
            hitView = [hitView superview];
        }
    }

    if (lastHitView != view && lastHitView != nil) {
        // If we are moving out of a view (or frame), let's pretend the mouse moved
        // all the way out of that view. But we have to account for scrolling, because
        // khtml doesn't understand our clipping.
        NSRect visibleRect = [[[[lastHitView _frame] frameView] _scrollView] documentVisibleRect];
        float yScroll = visibleRect.origin.y;
        float xScroll = visibleRect.origin.x;

        event = [NSEvent mouseEventWithType:NSMouseMoved
                         location:NSMakePoint(-1 - xScroll, -1 - yScroll )
                         modifierFlags:[[NSApp currentEvent] modifierFlags]
                         timestamp:[NSDate timeIntervalSinceReferenceDate]
                         windowNumber:[[self window] windowNumber]
                         context:[[NSApp currentEvent] context]
                         eventNumber:0 clickCount:0 pressure:0];
        [[lastHitView _bridge] mouseMoved:event];
    }

    lastHitView = view;
    
    NSDictionary *element;
    if (view == nil) {
        element = nil;
    } else {
        [[view _bridge] mouseMoved:event];

        NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
        element = [view elementAtPoint:point];
    }

    // Have the web view send a message to the delegate so it can do status bar display.
    [[self _webView] _mouseDidMoveOverElement:element modifierFlags:[event modifierFlags]];

    // Set a tool tip; it won't show up right away but will if the user pauses.
    [self _setToolTip:[element objectForKey:WebCoreElementTitleKey]];
}

+ (NSArray *)_insertablePasteboardTypes
{
    static NSArray *types = nil;
    if (!types) {
        types = [[NSArray alloc] initWithObjects:WebArchivePboardType, NSHTMLPboardType,
            NSFilenamesPboardType, NSTIFFPboardType, NSPICTPboardType, NSURLPboardType, 
            NSRTFDPboardType, NSRTFPboardType, NSStringPboardType, nil];
    }
    return types;
}

+ (NSArray *)_selectionPasteboardTypes
{
    // FIXME: We should put data for NSHTMLPboardType on the pasteboard but Microsoft Excel doesn't like our format of HTML (3640423).
    return [NSArray arrayWithObjects:WebArchivePboardType, NSRTFPboardType, NSRTFDPboardType, NSStringPboardType, nil];
}

- (WebArchive *)_selectedArchive
{
    NSArray *nodes;
#if !LOG_DISABLED        
    double start = CFAbsoluteTimeGetCurrent();
#endif
    NSString *markupString = [[self _bridge] markupStringFromRange:[self _selectedRange] nodes:&nodes];
#if !LOG_DISABLED
    double duration = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "copying markup took %f seconds.", duration);
#endif
    
    return [[self _dataSource] _archiveWithMarkupString:markupString nodes:nodes];
}

- (void)_writeSelectionToPasteboard:(NSPasteboard *)pasteboard
{
    ASSERT([self _hasSelection]);
    NSArray *types = [self pasteboardTypesForSelection];
    [pasteboard declareTypes:types owner:nil];
    [self writeSelectionWithPasteboardTypes:types toPasteboard:pasteboard];
}

- (NSImage *)_dragImageForLinkElement:(NSDictionary *)element
{
    NSURL *linkURL = [element objectForKey: WebElementLinkURLKey];

    BOOL drawURLString = YES;
    BOOL clipURLString = NO, clipLabelString = NO;
    
    NSString *label = [element objectForKey: WebElementLinkLabelKey];
    NSString *urlString = [linkURL _web_userVisibleString];
    
    if (!label) {
        drawURLString = NO;
        label = urlString;
    }
    
    NSFont *labelFont = [[NSFontManager sharedFontManager] convertFont:[NSFont systemFontOfSize:DRAG_LINK_LABEL_FONT_SIZE]
                                                   toHaveTrait:NSBoldFontMask];
    NSFont *urlFont = [NSFont systemFontOfSize: DRAG_LINK_URL_FONT_SIZE];
    NSSize labelSize;
    labelSize.width = [label _web_widthWithFont: labelFont];
    labelSize.height = [labelFont ascender] - [labelFont descender];
    if (labelSize.width > MAX_DRAG_LABEL_WIDTH){
        labelSize.width = MAX_DRAG_LABEL_WIDTH;
        clipLabelString = YES;
    }
    
    NSSize imageSize, urlStringSize;
    imageSize.width = labelSize.width + DRAG_LABEL_BORDER_X * 2;
    imageSize.height = labelSize.height + DRAG_LABEL_BORDER_Y * 2;
    if (drawURLString) {
        urlStringSize.width = [urlString _web_widthWithFont: urlFont];
        urlStringSize.height = [urlFont ascender] - [urlFont descender];
        imageSize.height += urlStringSize.height;
        if (urlStringSize.width > MAX_DRAG_LABEL_WIDTH) {
            imageSize.width = MAX(MAX_DRAG_LABEL_WIDTH + DRAG_LABEL_BORDER_X * 2, MIN_DRAG_LABEL_WIDTH_BEFORE_CLIP);
            clipURLString = YES;
        } else {
            imageSize.width = MAX(labelSize.width + DRAG_LABEL_BORDER_X * 2, urlStringSize.width + DRAG_LABEL_BORDER_X * 2);
        }
    }
    NSImage *dragImage = [[[NSImage alloc] initWithSize: imageSize] autorelease];
    [dragImage lockFocus];
    
    [[NSColor colorWithCalibratedRed: 0.7 green: 0.7 blue: 0.7 alpha: 0.8] set];
    
    // Drag a rectangle with rounded corners/
    NSBezierPath *path = [NSBezierPath bezierPath];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0,0, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0,imageSize.height - DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS * 2, imageSize.height - DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS * 2,0, DRAG_LABEL_RADIUS * 2, DRAG_LABEL_RADIUS * 2)];
    
    [path appendBezierPathWithRect: NSMakeRect(DRAG_LABEL_RADIUS, 0, imageSize.width - DRAG_LABEL_RADIUS * 2, imageSize.height)];
    [path appendBezierPathWithRect: NSMakeRect(0, DRAG_LABEL_RADIUS, DRAG_LABEL_RADIUS + 10, imageSize.height - 2 * DRAG_LABEL_RADIUS)];
    [path appendBezierPathWithRect: NSMakeRect(imageSize.width - DRAG_LABEL_RADIUS - 20,DRAG_LABEL_RADIUS, DRAG_LABEL_RADIUS + 20, imageSize.height - 2 * DRAG_LABEL_RADIUS)];
    [path fill];
        
    NSColor *topColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.75];
    NSColor *bottomColor = [NSColor colorWithCalibratedWhite:1.0 alpha:0.5];
    if (drawURLString) {
        if (clipURLString)
            urlString = [WebStringTruncator centerTruncateString: urlString toWidth:imageSize.width - (DRAG_LABEL_BORDER_X * 2) withFont:urlFont];

        [urlString _web_drawDoubledAtPoint:NSMakePoint(DRAG_LABEL_BORDER_X, DRAG_LABEL_BORDER_Y - [urlFont descender]) 
             withTopColor:topColor bottomColor:bottomColor font:urlFont];
    }

    if (clipLabelString)
        label = [WebStringTruncator rightTruncateString: label toWidth:imageSize.width - (DRAG_LABEL_BORDER_X * 2) withFont:labelFont];
    [label _web_drawDoubledAtPoint:NSMakePoint (DRAG_LABEL_BORDER_X, imageSize.height - DRAG_LABEL_BORDER_Y_OFFSET - [labelFont pointSize])
             withTopColor:topColor bottomColor:bottomColor font:labelFont];
    
    [dragImage unlockFocus];
    
    return dragImage;
}

- (BOOL)_startDraggingImage:(NSImage *)wcDragImage at:(NSPoint)wcDragLoc operation:(NSDragOperation)op event:(NSEvent *)mouseDraggedEvent sourceIsDHTML:(BOOL)srcIsDHTML DHTMLWroteData:(BOOL)dhtmlWroteData
{
    NSPoint mouseDownPoint = [self convertPoint:[_private->mouseDownEvent locationInWindow] fromView:nil];
    NSDictionary *element = [self elementAtPoint:mouseDownPoint];

    NSURL *linkURL = [element objectForKey:WebElementLinkURLKey];
    NSURL *imageURL = [element objectForKey:WebElementImageURLKey];
    BOOL isSelected = [[element objectForKey:WebElementIsSelectedKey] boolValue];

    [_private->draggingImageURL release];
    _private->draggingImageURL = nil;

    NSPoint mouseDraggedPoint = [self convertPoint:[mouseDraggedEvent locationInWindow] fromView:nil];
    _private->webCoreDragOp = op;     // will be DragNone if WebCore doesn't care
    NSImage *dragImage = nil;
    NSPoint dragLoc;

    // We allow WebCore to override the drag image, even if its a link, image or text we're dragging.
    // This is in the spirit of the IE API, which allows overriding of pasteboard data and DragOp.
    // We could verify that ActionDHTML is allowed, although WebCore does claim to respect the action.
    if (wcDragImage) {
        dragImage = wcDragImage;
        // wcDragLoc is the cursor position relative to the lower-left corner of the image.
        // We add in the Y dimension because we are a flipped view, so adding moves the image down.
        if (linkURL) {
            // see HACK below
            dragLoc = NSMakePoint(mouseDraggedPoint.x - wcDragLoc.x, mouseDraggedPoint.y + wcDragLoc.y);
        } else {
            dragLoc = NSMakePoint(mouseDownPoint.x - wcDragLoc.x, mouseDownPoint.y + wcDragLoc.y);
        }
        _private->dragOffset = wcDragLoc;
    }
    
    WebView *webView = [self _webView];
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
    WebImageRenderer *image = [element objectForKey:WebElementImageKey];
    BOOL startedDrag = YES;  // optimism - we almost always manage to start the drag

    // note per kwebster, the offset arg below is always ignored in positioning the image
    if (imageURL != nil && image != nil && (_private->dragSourceActionMask & WebDragSourceActionImage)) {
        id source = self;
        if (!dhtmlWroteData) {
            // Select the image when it is dragged. This allows the image to be moved via MoveSelectionCommandImpl and this matches NSTextView's behavior.
            DOMHTMLElement *imageElement = [element objectForKey:WebElementDOMNodeKey];
            ASSERT(imageElement != nil);
            [webView setSelectedDOMRange:[[[self _bridge] DOMDocument] _createRangeWithNode:imageElement] affinity:NSSelectionAffinityUpstream];
            _private->draggingImageURL = [imageURL retain];
            source = [pasteboard _web_declareAndWriteDragImage:image
                                                           URL:linkURL ? linkURL : imageURL
                                                         title:[element objectForKey:WebElementImageAltStringKey]
                                                       archive:[imageElement webArchive]
                                                        source:self];
        }
        [[webView _UIDelegateForwarder] webView:webView willPerformDragSourceAction:WebDragSourceActionImage fromPoint:mouseDownPoint withPasteboard:pasteboard];
        if (dragImage == nil) {
            [self _web_dragImage:[element objectForKey:WebElementImageKey]
                            rect:[[element objectForKey:WebElementImageRectKey] rectValue]
                           event:_private->mouseDownEvent
                      pasteboard:pasteboard
                          source:source
                          offset:&_private->dragOffset];
        } else {
            [self dragImage:dragImage
                         at:dragLoc
                     offset:NSZeroSize
                      event:_private->mouseDownEvent
                 pasteboard:pasteboard
                     source:source
                  slideBack:YES];
        }
    } else if (linkURL && (_private->dragSourceActionMask & WebDragSourceActionLink)) {
        if (!dhtmlWroteData) {
            NSArray *types = [NSPasteboard _web_writableTypesForURL];
            [pasteboard declareTypes:types owner:self];
            [pasteboard _web_writeURL:linkURL andTitle:[element objectForKey:WebElementLinkLabelKey] types:types];            
        }
        [[webView _UIDelegateForwarder] webView:webView willPerformDragSourceAction:WebDragSourceActionLink fromPoint:mouseDownPoint withPasteboard:pasteboard];
        if (dragImage == nil) {
            dragImage = [self _dragImageForLinkElement:element];
            NSSize offset = NSMakeSize([dragImage size].width / 2, -DRAG_LABEL_BORDER_Y);
            dragLoc = NSMakePoint(mouseDraggedPoint.x - offset.width, mouseDraggedPoint.y - offset.height);
            _private->dragOffset.x = offset.width;
            _private->dragOffset.y = -offset.height;        // inverted because we are flipped
        }
        // HACK:  We should pass the mouseDown event instead of the mouseDragged!  This hack gets rid of
        // a flash of the image at the mouseDown location when the drag starts.
        [self dragImage:dragImage
                     at:dragLoc
                 offset:NSZeroSize
                  event:mouseDraggedEvent
             pasteboard:pasteboard
                 source:self
              slideBack:YES];
    } else if (isSelected && (_private->dragSourceActionMask & WebDragSourceActionSelection)) {
        if (!dhtmlWroteData) {
            [self _writeSelectionToPasteboard:pasteboard];
        }
        [[webView _UIDelegateForwarder] webView:webView willPerformDragSourceAction:WebDragSourceActionSelection fromPoint:mouseDownPoint withPasteboard:pasteboard];
        if (dragImage == nil) {
            dragImage = [[self _bridge] selectionImage];
            [dragImage _web_dissolveToFraction:WebDragImageAlpha];
            NSRect visibleSelectionRect = [[self _bridge] visibleSelectionRect];
            dragLoc = NSMakePoint(NSMinX(visibleSelectionRect), NSMaxY(visibleSelectionRect));
            _private->dragOffset.x = mouseDownPoint.x - dragLoc.x;
            _private->dragOffset.y = dragLoc.y - mouseDownPoint.y;        // inverted because we are flipped
        }
        [self dragImage:dragImage
                     at:dragLoc
                 offset:NSZeroSize
                  event:_private->mouseDownEvent
             pasteboard:pasteboard
                 source:self
              slideBack:YES];
    } else if (srcIsDHTML) {
        ASSERT(_private->dragSourceActionMask & WebDragSourceActionDHTML);
        [[webView _UIDelegateForwarder] webView:webView willPerformDragSourceAction:WebDragSourceActionDHTML fromPoint:mouseDownPoint withPasteboard:pasteboard];
        if (dragImage == nil) {
            // WebCore should have given us an image, but we'll make one up
            NSString *imagePath = [[NSBundle bundleForClass:[self class]] pathForResource:@"missing_image" ofType:@"tiff"];
            dragImage = [[[NSImage alloc] initWithContentsOfFile:imagePath] autorelease];
            NSSize imageSize = [dragImage size];
            dragLoc = NSMakePoint(mouseDownPoint.x - imageSize.width/2, mouseDownPoint.y + imageSize.height/2);
            _private->dragOffset.x = imageSize.width/2;
            _private->dragOffset.y = imageSize.height/2;        // inverted because we are flipped
        }
        [self dragImage:dragImage
                     at:dragLoc
                 offset:NSZeroSize
                  event:_private->mouseDownEvent
             pasteboard:pasteboard
                 source:self
              slideBack:YES];
    } else {
        // Only way I know if to get here is if the original element clicked on in the mousedown is no longer
        // under the mousedown point, so linkURL, imageURL and isSelected are all false/nil.
        startedDrag = NO;
    }
    return startedDrag;
}

- (void)_handleAutoscrollForMouseDragged:(NSEvent *)event
{
    [self autoscroll:event];
    [self _startAutoscrollTimer:event];
}

- (BOOL)_mayStartDragAtEventLocation:(NSPoint)location
{
    NSPoint mouseDownPoint = [self convertPoint:location fromView:nil];
    NSDictionary *mouseDownElement = [self elementAtPoint:mouseDownPoint];

    if ([mouseDownElement objectForKey: WebElementImageKey] != nil &&
        [mouseDownElement objectForKey: WebElementImageURLKey] != nil && 
        [[WebPreferences standardPreferences] loadsImagesAutomatically] && 
        (_private->dragSourceActionMask & WebDragSourceActionImage)) {
        return YES;
    }
    
    if ([mouseDownElement objectForKey:WebElementLinkURLKey] != nil && 
        (_private->dragSourceActionMask & WebDragSourceActionLink)) {
        return YES;
    }
    
    if ([[mouseDownElement objectForKey:WebElementIsSelectedKey] boolValue] &&
        (_private->dragSourceActionMask & WebDragSourceActionSelection)) {
        return YES;
    }
    
    return NO;
}

- (WebPluginController *)_pluginController
{
    return _private->pluginController;
}

- (void)_web_setPrintingModeRecursive
{
    [self _setPrinting:YES minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:NO];
    [super _web_setPrintingModeRecursive];
}

- (void)_web_clearPrintingModeRecursive
{
    [self _setPrinting:NO minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:NO];
    [super _web_clearPrintingModeRecursive];
}

- (void)_web_layoutIfNeededRecursive:(NSRect)displayRect testDirtyRect:(bool)testDirtyRect
{
    ASSERT(!_private->subviewsSetAside);
    displayRect = NSIntersectionRect(displayRect, [self bounds]);

    if (!testDirtyRect || [self needsDisplay]) {
        if (testDirtyRect) {
            NSRect dirtyRect = [self _dirtyRect];
            displayRect = NSIntersectionRect(displayRect, dirtyRect);
        }
        if (!NSIsEmptyRect(displayRect)) {
            if ([[self _bridge] needsLayout])
                _private->needsLayout = YES;
            if (_private->needsToApplyStyles || _private->needsLayout)
                [self layout];
        }
    }

    [super _web_layoutIfNeededRecursive: displayRect testDirtyRect: NO];
}

- (NSRect)_selectionRect
{
    return [[self _bridge] selectionRect];
}

- (void)_startAutoscrollTimer: (NSEvent *)triggerEvent
{
    if (_private->autoscrollTimer == nil) {
        _private->autoscrollTimer = [[NSTimer scheduledTimerWithTimeInterval:AUTOSCROLL_INTERVAL
            target:self selector:@selector(_autoscroll) userInfo:nil repeats:YES] retain];
        _private->autoscrollTriggerEvent = [triggerEvent retain];
    }
}

- (void)_stopAutoscrollTimer
{
    NSTimer *timer = _private->autoscrollTimer;
    _private->autoscrollTimer = nil;
    [_private->autoscrollTriggerEvent release];
    _private->autoscrollTriggerEvent = nil;
    [timer invalidate];
    [timer release];
}

- (void)_autoscroll
{
    int isStillDown;
    
    // Guarantee that the autoscroll timer is invalidated, even if we don't receive
    // a mouse up event.
    PSstilldown([_private->autoscrollTriggerEvent eventNumber], &isStillDown);
    if (!isStillDown){
        [self _stopAutoscrollTimer];
        return;
    }

    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSLeftMouseDragged
        location:[[self window] convertScreenToBase:[NSEvent mouseLocation]]
        modifierFlags:[[NSApp currentEvent] modifierFlags]
        timestamp:[NSDate timeIntervalSinceReferenceDate]
        windowNumber:[[self window] windowNumber]
        context:[[NSApp currentEvent] context]
        eventNumber:0 clickCount:0 pressure:0];
    [self mouseDragged:fakeEvent];
}

- (BOOL)_canCopy
{
    // Copying can be done regardless of whether you can edit.
    return [self _hasSelection];
}

- (BOOL)_canCut
{
    return [self _hasSelection] && [self _isEditable];
}

- (BOOL)_canDelete
{
    return [self _hasSelection] && [self _isEditable];
}

- (BOOL)_canPaste
{
    return [self _hasSelectionOrInsertionPoint] && [self _isEditable];
}

- (BOOL)_canEdit
{
    return [self _hasSelectionOrInsertionPoint] && [self _isEditable];
}

- (BOOL)_hasSelection
{
    return [[self _bridge] selectionState] == WebSelectionStateRange;
}

- (BOOL)_hasSelectionOrInsertionPoint
{
    return [[self _bridge] selectionState] != WebSelectionStateNone;
}

- (BOOL)_isEditable
{
    return [[self _webView] isEditable] || [[self _bridge] isSelectionEditable];
}

- (BOOL)_isSelectionMisspelled
{
    NSString *selectedString = [self selectedString];
    unsigned length = [selectedString length];
    if (length == 0) {
        return NO;
    }
    NSRange range = [[NSSpellChecker sharedSpellChecker] checkSpellingOfString:selectedString
                                                                    startingAt:0
                                                                      language:@""
                                                                          wrap:NO
                                                        inSpellDocumentWithTag:[[self _webView] spellCheckerDocumentTag]
                                                                     wordCount:NULL];
    return range.length == length;
}

- (NSArray *)_guessesForMisspelledSelection
{
    ASSERT([[self selectedString] length] != 0);
    return [[NSSpellChecker sharedSpellChecker] guessesForWord:[self selectedString]];
}

- (void)_changeSpellingFromMenu:(id)sender
{
    ASSERT([[self selectedString] length] != 0);
    [[self _bridge] replaceSelectionWithText:[sender title] selectReplacement:YES smartReplace:NO];
}

- (void)_ignoreSpellingFromMenu:(id)sender
{
    ASSERT([[self selectedString] length] != 0);
    [[NSSpellChecker sharedSpellChecker] ignoreWord:[self selectedString] inSpellDocumentWithTag:[[self _webView] spellCheckerDocumentTag]];
}

- (void)_learnSpellingFromMenu:(id)sender
{
    ASSERT([[self selectedString] length] != 0);
    [[NSSpellChecker sharedSpellChecker] learnWord:[self selectedString]];
}

#if APPKIT_CODE_FOR_REFERENCE

- (void)_openLinkFromMenu:(id)sender
{
    NSTextStorage *text = _getTextStorage(self);
    NSRange charRange = [self selectedRange];
    if (charRange.location != NSNotFound && charRange.length > 0) {
        id link = [text attribute:NSLinkAttributeName atIndex:charRange.location effectiveRange:NULL];
        if (link) {
            [self clickedOnLink:link atIndex:charRange.location];
        } else {
            NSString *string = [[text string] substringWithRange:charRange];
            link = [NSURL URLWithString:string];
            if (link) [[NSWorkspace sharedWorkspace] openURL:link];
        }
    }
}

#endif

@end

@implementation NSView (WebHTMLViewFileInternal)

- (void)_web_setPrintingModeRecursive
{
    [_subviews makeObjectsPerformSelector:@selector(_web_setPrintingModeRecursive)];
}

- (void)_web_clearPrintingModeRecursive
{
    [_subviews makeObjectsPerformSelector:@selector(_web_clearPrintingModeRecursive)];
}

- (void)_web_layoutIfNeededRecursive: (NSRect)rect testDirtyRect:(bool)testDirtyRect
{
    unsigned index, count;
    for (index = 0, count = [_subviews count]; index < count; index++) {
        NSView *subview = [_subviews objectAtIndex:index];
        NSRect dirtiedSubviewRect = [subview convertRect: rect fromView: self];
        [subview _web_layoutIfNeededRecursive: dirtiedSubviewRect testDirtyRect:testDirtyRect];
    }
}

@end

@implementation NSMutableDictionary (WebHTMLViewFileInternal)

- (void)_web_setObjectIfNotNil:(id)object forKey:(id)key
{
    if (object == nil) {
        [self removeObjectForKey:key];
    } else {
        [self setObject:object forKey:key];
    }
}

@end

// The following is a workaround for
// <rdar://problem/3429631> window stops getting mouse moved events after first tooltip appears
// The trick is to define a category on NSToolTipPanel that implements setAcceptsMouseMovedEvents:.
// Since the category will be searched before the real class, we'll prevent the flag from being
// set on the tool tip panel.

@interface NSToolTipPanel : NSPanel
@end

@interface NSToolTipPanel (WebHTMLViewFileInternal)
@end

@implementation NSToolTipPanel (WebHTMLViewFileInternal)

- (void)setAcceptsMouseMovedEvents:(BOOL)flag
{
    // Do nothing, preventing the tool tip panel from trying to accept mouse-moved events.
}

@end


@interface WebHTMLView (TextSizing) <_web_WebDocumentTextSizing>
@end

@interface NSArray (WebHTMLView)
- (void)_web_makePluginViewsPerformSelector:(SEL)selector withObject:(id)object;
@end

@implementation WebHTMLView

+ (void)initialize
{
    WebKitInitializeUnicode();
    [NSApp registerServicesMenuSendTypes:[[self class] _selectionPasteboardTypes] returnTypes:nil];
    _NSInitializeKillRing();
}

- (id)initWithFrame:(NSRect)frame
{
    [super initWithFrame:frame];
    
    // Make all drawing go through us instead of subviews.
    if (NSAppKitVersionNumber >= 711) {
        [self _setDrawsOwnDescendants:YES];
    }
    
    _private = [[WebHTMLViewPrivate alloc] init];

    _private->pluginController = [[WebPluginController alloc] initWithDocumentView:self];
    _private->needsLayout = YES;

    return self;
}

- (void)dealloc
{
    [self _clearLastHitViewIfSelf];
    [self _reset];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_private->pluginController destroyAllPlugins];
    [_private release];
    _private = nil;
    [super dealloc];
}

- (void)finalize
{
    [self _clearLastHitViewIfSelf];
    [self _reset];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_private->pluginController destroyAllPlugins];
    _private = nil;
    [super finalize];
}

- (IBAction)takeFindStringFromSelection:(id)sender
{
    if (![self _hasSelection]) {
        NSBeep();
        return;
    }

    [NSPasteboard _web_setFindPasteboardString:[self selectedString] withOwner:self];
}

- (NSArray *)pasteboardTypesForSelection
{
    if ([self _canSmartCopyOrDelete]) {
        NSMutableArray *types = [[[[self class] _selectionPasteboardTypes] mutableCopy] autorelease];
        [types addObject:WebSmartPastePboardType];
        return types;
    } else {
        return [[self class] _selectionPasteboardTypes];
    }
}

// This method is copied from NSTextView
- (NSAttributedString *)_stripAttachmentCharactersFromAttributedString:(NSAttributedString *)originalAttributedString
{
    NSRange attachmentRange;
    NSString *originalString = [originalAttributedString string];
    static NSString *attachmentCharString = nil;
    
    if (!attachmentCharString) {
        unichar chars[2];
        if (!attachmentCharString) {
            chars[0] = NSAttachmentCharacter;
            chars[1] = 0;
            attachmentCharString = [[NSString alloc] initWithCharacters:chars length:1];
        }
    }
    
    attachmentRange = [originalString rangeOfString:attachmentCharString];
    if (attachmentRange.location != NSNotFound && attachmentRange.length > 0) {
        NSMutableAttributedString *newAttributedString = [[originalAttributedString mutableCopyWithZone:NULL] autorelease];
        
        while (attachmentRange.location != NSNotFound && attachmentRange.length > 0) {
            [newAttributedString replaceCharactersInRange:attachmentRange withString:@""];
            attachmentRange = [[newAttributedString string] rangeOfString:attachmentCharString];
        }
        return newAttributedString;
    } else {
        return originalAttributedString;
    }
}

- (void)writeSelectionWithPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard
{
    // Put HTML on the pasteboard.
    if ([types containsObject:WebArchivePboardType]) {
        WebArchive *archive = [self _selectedArchive];
        [pasteboard setData:[archive data] forType:WebArchivePboardType];
    }
    
    // Put the attributed string on the pasteboard (RTF/RTFD format).
    NSAttributedString *attributedString = nil;
    if ([types containsObject:NSRTFDPboardType]) {
        attributedString = [self selectedAttributedString];
        if ([attributedString containsAttachments]) {
            NSData *RTFDData = [attributedString RTFDFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
            [pasteboard setData:RTFDData forType:NSRTFDPboardType];
        }
    }        
    if ([types containsObject:NSRTFPboardType]) {
        if (attributedString == nil) {
            attributedString = [self selectedAttributedString];
        }
        if ([attributedString containsAttachments]) {
            attributedString = [self _stripAttachmentCharactersFromAttributedString:attributedString];
        }
        NSData *RTFData = [attributedString RTFFromRange:NSMakeRange(0, [attributedString length]) documentAttributes:nil];
        [pasteboard setData:RTFData forType:NSRTFPboardType];
    }
        
    // Put plain string on the pasteboard.
    if ([types containsObject:NSStringPboardType]) {
        // Map &nbsp; to a plain old space because this is better for source code, other browsers do it,
        // and because HTML forces you to do this any time you want two spaces in a row.
        NSMutableString *s = [[self selectedString] mutableCopy];
        const unichar NonBreakingSpaceCharacter = 0xA0;
        NSString *NonBreakingSpaceString = [NSString stringWithCharacters:&NonBreakingSpaceCharacter length:1];
        [s replaceOccurrencesOfString:NonBreakingSpaceString withString:@" " options:0 range:NSMakeRange(0, [s length])];
        [pasteboard setString:s forType:NSStringPboardType];
        [s release];
    }
    
    if ([self _canSmartCopyOrDelete] && [types containsObject:WebSmartPastePboardType]) {
        [pasteboard setData:nil forType:WebSmartPastePboardType];
    }
}

- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pasteboard types:(NSArray *)types
{
    [self _writeSelectionToPasteboard:pasteboard];
    return YES;
}

- (void)selectAll:(id)sender
{
    [self selectAll];
}

- (void)jumpToSelection: sender
{
    [[self _bridge] jumpToSelection];
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item 
{
    SEL action = [item action];
    WebBridge *bridge = [self _bridge];

    if (action == @selector(cut:)) {
        return [bridge mayDHTMLCut] || [self _canDelete];
    } else if (action == @selector(copy:)) {
        return [bridge mayDHTMLCopy] || [self _canCopy];
    } else if (action == @selector(delete:)) {
        return [self _canDelete];
    } else if (action == @selector(paste:)) {
        return [bridge mayDHTMLPaste] || [self _canPaste];
    } else if (action == @selector(takeFindStringFromSelection:)) {
        return [self _hasSelection];
    } else if (action == @selector(jumpToSelection:)) {
        return [self _hasSelection];
    } else if (action == @selector(checkSpelling:)
               || action == @selector(showGuessPanel:)
               || action == @selector(changeSpelling:)
               || action == @selector(ignoreSpelling:)) {
        return [[self _bridge] isSelectionEditable];
    }

    return YES;
}

- (id)validRequestorForSendType:(NSString *)sendType returnType:(NSString *)returnType
{
    if (sendType && ([[[self class] _selectionPasteboardTypes] containsObject:sendType]) && [self _hasSelection]){
        return self;
    }

    return [super validRequestorForSendType:sendType returnType:returnType];
}

- (BOOL)acceptsFirstResponder
{
    // Don't accept first responder when we first click on this view.
    // We have to pass the event down through WebCore first to be sure we don't hit a subview.
    // Do accept first responder at any other time, for example from keyboard events,
    // or from calls back from WebCore once we begin mouse-down event handling.
    NSEvent *event = [NSApp currentEvent];
    if ([event type] == NSLeftMouseDown && event != _private->mouseDownEvent && 
        NSPointInRect([event locationInWindow], [self convertRect:[self visibleRect] toView:nil])) {
        return NO;
    }
    return YES;
}

- (BOOL)maintainsInactiveSelection
{
    // This method helps to determing whether the view should maintain
    // an inactive selection when the view is not first responder.
    // Traditionally, these views have not maintained such selections,
    // clearing them when the view was not first responder. However,
    // to fix bugs like this one:
    // <rdar://problem/3672088>: "Editable WebViews should maintain a selection even 
    //                            when they're not firstResponder"
    // it was decided to add a switch to act more like an NSTextView.
    // For now, however, the view only acts in this way when the
    // web view is set to be editable. This will maintain traditional
    // behavior for WebKit clients dating back to before this change,
    // and will likely be a decent switch for the long term, since
    // clients to ste the web view to be editable probably want it
    // to act like a "regular" Cocoa view in terms of its selection
    // behavior.
    if (![[self _webView] isEditable])
        return NO;
        
    id nextResponder = [[self window] _newFirstResponderAfterResigning];
    return !nextResponder || ![nextResponder isKindOfClass:[NSView class]] || ![nextResponder isDescendantOf:[self _webView]];
}

- (void)addMouseMovedObserver
{
    // Always add a mouse move observer if the DB requested, or if we're the key window.
    if (([[self window] isKeyWindow] && ![self _insideAnotherHTMLView]) ||
        [[self _webView] _dashboardBehavior:WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows]){
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(mouseMovedNotification:)
            name:NSMouseMovedNotification object:nil];
        [self _frameOrBoundsChanged];
    }
}

- (void)removeMouseMovedObserver
{
    // Don't remove the observer if we're running the DB
    if ([[self _webView] _dashboardBehavior:WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows])
        return;
        
    [[self _webView] _mouseDidMoveOverElement:nil modifierFlags:0];
    [[NSNotificationCenter defaultCenter] removeObserver:self
        name:NSMouseMovedNotification object:nil];
}

- (void)updateFocusDisplay
{
    // This method does the job of updating the view based on the view's firstResponder-ness and
    // the window key-ness of the window containing this view. This involves three kinds of 
    // drawing updates right now, all handled in WebCore in response to the call over the bridge. 
    // 
    // The three display attributes are as follows:
    // 
    // 1. The background color used to draw behind selected content (active | inactive color)
    // 2. Caret blinking (blinks | does not blink)
    // 3. The drawing of a focus ring around links in web pages.

    BOOL flag = !_private->resigningFirstResponder && [[self window] isKeyWindow] && [self _web_firstResponderCausesFocusDisplay];
    [[self _bridge] setDisplaysWithFocusAttributes:flag];
}

- (void)addSuperviewObservers
{
    // We watch the bounds of our superview, so that we can do a layout when the size
    // of the superview changes. This is different from other scrollable things that don't
    // need this kind of thing because their layout doesn't change.
    
    // We need to pay attention to both height and width because our "layout" has to change
    // to extend the background the full height of the space and because some elements have
    // sizes that are based on the total size of the view.
    
    NSView *superview = [self superview];
    if (superview && [self window]) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_frameOrBoundsChanged) 
            name:NSViewFrameDidChangeNotification object:superview];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_frameOrBoundsChanged) 
            name:NSViewBoundsDidChangeNotification object:superview];
    }
}

- (void)removeSuperviewObservers
{
    NSView *superview = [self superview];
    if (superview && [self window]) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSViewFrameDidChangeNotification object:superview];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSViewBoundsDidChangeNotification object:superview];
    }
}

- (void)addWindowObservers
{
    NSWindow *window = [self window];
    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeKey:)
            name:NSWindowDidBecomeKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResignKey:)
            name:NSWindowDidResignKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:)
            name:NSWindowWillCloseNotification object:window];
    }
}

- (void)removeWindowObservers
{
    NSWindow *window = [self window];
    if (window) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidBecomeKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowDidResignKeyNotification object:window];
        [[NSNotificationCenter defaultCenter] removeObserver:self
            name:NSWindowWillCloseNotification object:window];
    }
}

- (void)viewWillMoveToSuperview:(NSView *)newSuperview
{
    [self removeSuperviewObservers];
}

- (void)viewDidMoveToSuperview
{
    // Do this here in case the text size multiplier changed when a non-HTML
    // view was installed.
    if ([self superview] != nil) {
        [self _updateTextSizeMultiplier];
        [self addSuperviewObservers];
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    // Don't do anything if we aren't initialized.  This happens
    // when decoding a WebView.  When WebViews are decoded their subviews
    // are created by initWithCoder: and so won't be normally
    // initialized.  The stub views are discarded by WebView.
    if (_private){
        // FIXME: Some of these calls may not work because this view may be already removed from it's superview.
        [self removeMouseMovedObserver];
        [self removeWindowObservers];
        [self removeSuperviewObservers];
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(_updateMouseoverWithFakeEvent) object:nil];
    
        [[self _pluginController] stopAllPlugins];
    }
}

- (void)viewDidMoveToWindow
{
    // Don't do anything if we aren't initialized.  This happens
    // when decoding a WebView.  When WebViews are decoded their subviews
    // are created by initWithCoder: and so won't be normally
    // initialized.  The stub views are discarded by WebView.
    if (_private) {
        [self _stopAutoscrollTimer];
        if ([self window]) {
            _private->lastScrollPosition = [[self superview] bounds].origin;
            [self addWindowObservers];
            [self addSuperviewObservers];
            [self addMouseMovedObserver];
            // Schedule this update, rather than making the call right now.
            // The reason is that placing the caret in the just-installed view requires
            // the HTML/XML document to be available on the WebCore side, but it is not
            // at the time this code is running. However, it will be there on the next
            // crank of the run loop. Doing this helps to make a blinking caret appear 
            // in a new, empty window "automatic".
            [self performSelector:@selector(updateFocusDisplay) withObject:nil afterDelay:0];
    
            [[self _pluginController] startAllPlugins];
    
            _private->lastScrollPosition = NSZeroPoint;
            
            _private->inWindow = YES;
        } else {
            // Reset when we are moved out of a window after being moved into one.
            // Without this check, we reset ourselves before we even start.
            // This is only needed because viewDidMoveToWindow is called even when
            // the window is not changing (bug in AppKit).
            if (_private->inWindow) {
                [self _reset];
                _private->inWindow = NO;
            }
        }
    }
}

- (void)viewWillMoveToHostWindow:(NSWindow *)hostWindow
{
    [[self subviews] _web_makePluginViewsPerformSelector:@selector(viewWillMoveToHostWindow:) withObject:hostWindow];
}

- (void)viewDidMoveToHostWindow
{
    [[self subviews] _web_makePluginViewsPerformSelector:@selector(viewDidMoveToHostWindow) withObject:nil];
}


- (void)addSubview:(NSView *)view
{
    [super addSubview:view];

    if ([[view class] respondsToSelector:@selector(plugInViewWithArguments:)] || [view respondsToSelector:@selector(pluginInitialize)] || [view respondsToSelector:@selector(webPlugInInitialize)]) {
        [[self _pluginController] addPlugin:view];
    }
}

- (void)reapplyStyles
{
    if (!_private->needsToApplyStyles) {
        return;
    }
    
#ifdef _KWQ_TIMING        
    double start = CFAbsoluteTimeGetCurrent();
#endif

    [[self _bridge] reapplyStylesForDeviceType:
        _private->printing ? WebCoreDevicePrinter : WebCoreDeviceScreen];
    
#ifdef _KWQ_TIMING        
    double thisTime = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "%s apply style seconds = %f", [self URL], thisTime);
#endif

    _private->needsToApplyStyles = NO;
}

// Do a layout, but set up a new fixed width for the purposes of doing printing layout.
// minPageWidth==0 implies a non-printing layout
- (void)layoutToMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)adjustViewSize
{
    [self reapplyStyles];
    
    // Ensure that we will receive mouse move events.  Is this the best place to put this?
    [[self window] setAcceptsMouseMovedEvents: YES];
    [[self window] _setShouldPostEventNotifications: YES];

    if (!_private->needsLayout) {
        return;
    }

#ifdef _KWQ_TIMING        
    double start = CFAbsoluteTimeGetCurrent();
#endif

    LOG(View, "%@ doing layout", self);

    if (minPageWidth > 0.0) {
        [[self _bridge] forceLayoutWithMinimumPageWidth:minPageWidth maximumPageWidth:maxPageWidth adjustingViewSize:adjustViewSize];
    } else {
        [[self _bridge] forceLayoutAdjustingViewSize:adjustViewSize];
    }
    _private->needsLayout = NO;
    
    if (!_private->printing) {
        // get size of the containing dynamic scrollview, so
        // appearance and disappearance of scrollbars will not show up
        // as a size change
        NSSize newLayoutFrameSize = [[[self superview] superview] frame].size;
        if (_private->laidOutAtLeastOnce && !NSEqualSizes(_private->lastLayoutFrameSize, newLayoutFrameSize)) {
            [[self _bridge] sendResizeEvent];
        }
        _private->laidOutAtLeastOnce = YES;
        _private->lastLayoutSize = [(NSClipView *)[self superview] documentVisibleRect].size;
        _private->lastLayoutFrameSize = newLayoutFrameSize;
    }

#ifdef _KWQ_TIMING        
    double thisTime = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "%s layout seconds = %f", [self URL], thisTime);
#endif
}

- (void)layout
{
    [self layoutToMinimumPageWidth:0.0 maximumPageWidth:0.0 adjustingViewSize:NO];
}

- (NSMenu *)menuForEvent:(NSEvent *)event
{
    [_private->compController endRevertingChange:NO moveLeft:NO];

    if ([[self _bridge] sendContextMenuEvent:event]) {
        return nil;
    }
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSDictionary *element = [self elementAtPoint:point];
    return [[self _webView] _menuForElement:element];
}

// Search from the end of the currently selected location, or from the beginning of the
// document if nothing is selected.
- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag;
{
    return [[self _bridge] searchFor:string direction:forward caseSensitive:caseFlag wrap:wrapFlag];
}

- (DOMRange *)_documentRange
{
    return [[[self _bridge] DOMDocument] _documentRange];
}

- (NSString *)string
{
    return [[self _bridge] stringForRange:[self _documentRange]];
}

- (NSAttributedString *)_attributeStringFromDOMRange:(DOMRange *)range
{
    NSAttributedString *attributedString = nil;
#ifdef USE_APPKIT_FOR_ATTRIBUTED_STRINGS
#if !LOG_DISABLED        
    double start = CFAbsoluteTimeGetCurrent();
#endif    
    attributedString = [[[NSAttributedString alloc] _initWithDOMRange:range] autorelease];
#if !LOG_DISABLED
    double duration = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "creating attributed string from selection took %f seconds.", duration);
#endif
#endif
    return attributedString;
}

- (NSAttributedString *)attributedString
{
    WebBridge *bridge = [self _bridge];
    DOMDocument *document = [bridge DOMDocument];
    NSAttributedString *attributedString = [self _attributeStringFromDOMRange:[document _documentRange]];
    if (attributedString == nil) {
        attributedString = [bridge attributedStringFrom:document startOffset:0 to:nil endOffset:0];
    }
    return attributedString;
}

- (NSString *)selectedString
{
    return [[self _bridge] selectedString];
}

- (NSAttributedString *)selectedAttributedString
{
    WebBridge *bridge = [self _bridge];
    NSAttributedString *attributedString = [self _attributeStringFromDOMRange:[self _selectedRange]];
    if (attributedString == nil) {
        attributedString = [bridge selectedAttributedString];
    }
    return attributedString;
}

- (void)selectAll
{
    [[self _bridge] selectAll];
}

// Remove the selection.
- (void)deselectAll
{
    [[self _bridge] deselectAll];
}

- (void)deselectText
{
    [[self _bridge] deselectText];
}

- (BOOL)isOpaque
{
    return [[self _webView] drawsBackground];
}

- (void)setNeedsDisplay:(BOOL)flag
{
    LOG(View, "%@ flag = %d", self, (int)flag);
    [super setNeedsDisplay: flag];
}

- (void)setNeedsLayout: (BOOL)flag
{
    LOG(View, "%@ flag = %d", self, (int)flag);
    _private->needsLayout = flag;
}


- (void)setNeedsToApplyStyles: (BOOL)flag
{
    LOG(View, "%@ flag = %d", self, (int)flag);
    _private->needsToApplyStyles = flag;
}

- (void)drawRect:(NSRect)rect
{
    LOG(View, "%@ drawing", self);
    
    BOOL subviewsWereSetAside = _private->subviewsSetAside;
    if (subviewsWereSetAside) {
        [self _restoreSubviews];
    }

#ifdef _KWQ_TIMING
    double start = CFAbsoluteTimeGetCurrent();
#endif

    [NSGraphicsContext saveGraphicsState];
    NSRectClip(rect);
        
    ASSERT([[self superview] isKindOfClass:[WebClipView class]]);

    [(WebClipView *)[self superview] setAdditionalClip:rect];

    NS_DURING {
        WebTextRendererFactory *textRendererFactoryIfCoalescing = nil;
        if ([WebTextRenderer shouldBufferTextDrawing] && [NSView focusView]) {
            textRendererFactoryIfCoalescing = [WebTextRendererFactory sharedFactory];
            [textRendererFactoryIfCoalescing startCoalesceTextDrawing];
        }

        if (![[self _webView] drawsBackground]) {
            [[NSColor clearColor] set];
            NSRectFill (rect);
        }
        
        //double start = CFAbsoluteTimeGetCurrent();
        [[self _bridge] drawRect:rect];
        //LOG(Timing, "draw time %e", CFAbsoluteTimeGetCurrent() - start);

        if (textRendererFactoryIfCoalescing != nil) {
            [textRendererFactoryIfCoalescing endCoalesceTextDrawing];
        }

        [(WebClipView *)[self superview] resetAdditionalClip];

        [NSGraphicsContext restoreGraphicsState];
    } NS_HANDLER {
        [(WebClipView *)[self superview] resetAdditionalClip];
        [NSGraphicsContext restoreGraphicsState];
        ERROR("Exception caught while drawing: %@", localException);
        [localException raise];
    } NS_ENDHANDLER

#ifdef DEBUG_LAYOUT
    NSRect vframe = [self frame];
    [[NSColor blackColor] set];
    NSBezierPath *path;
    path = [NSBezierPath bezierPath];
    [path setLineWidth:(float)0.1];
    [path moveToPoint:NSMakePoint(0, 0)];
    [path lineToPoint:NSMakePoint(vframe.size.width, vframe.size.height)];
    [path closePath];
    [path stroke];
    path = [NSBezierPath bezierPath];
    [path setLineWidth:(float)0.1];
    [path moveToPoint:NSMakePoint(0, vframe.size.height)];
    [path lineToPoint:NSMakePoint(vframe.size.width, 0)];
    [path closePath];
    [path stroke];
#endif

#ifdef _KWQ_TIMING
    double thisTime = CFAbsoluteTimeGetCurrent() - start;
    LOG(Timing, "%s draw seconds = %f", widget->part()->baseURL().URL().latin1(), thisTime);
#endif

    if (subviewsWereSetAside) {
        [self _setAsideSubviews];
    }
}

// Turn off the additional clip while computing our visibleRect.
- (NSRect)visibleRect
{
    if (!([[self superview] isKindOfClass:[WebClipView class]]))
        return [super visibleRect];
        
    WebClipView *clipView = (WebClipView *)[self superview];

    BOOL hasAdditionalClip = [clipView hasAdditionalClip];
    if (!hasAdditionalClip) {
        return [super visibleRect];
    }
    
    NSRect additionalClip = [clipView additionalClip];
    [clipView resetAdditionalClip];
    NSRect visibleRect = [super visibleRect];
    [clipView setAdditionalClip:additionalClip];
    return visibleRect;
}

- (BOOL)isFlipped 
{
    return YES;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    ASSERT([notification object] == [self window]);
    [self addMouseMovedObserver];
    [self updateFocusDisplay];
}

- (void)windowDidResignKey: (NSNotification *)notification
{
    ASSERT([notification object] == [self window]);
    [_private->compController endRevertingChange:NO moveLeft:NO];
    [self removeMouseMovedObserver];
    [self updateFocusDisplay];
}

- (void)windowWillClose:(NSNotification *)notification
{
    [_private->compController endRevertingChange:NO moveLeft:NO];
    [[self _pluginController] destroyAllPlugins];
}

- (BOOL)_isSelectionEvent:(NSEvent *)event
{
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    return [[[self elementAtPoint:point] objectForKey:WebElementIsSelectedKey] boolValue];
}

- (void)_setMouseDownEvent:(NSEvent *)event
{
    ASSERT([event type] == NSLeftMouseDown || [event type] == NSRightMouseDown || [event type] == NSOtherMouseDown);
    [event retain];
    [_private->mouseDownEvent release];
    _private->mouseDownEvent = event;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    [self _setMouseDownEvent:event];
    
    // We hack AK's hitTest method to catch all events at the topmost WebHTMLView.  However, for
    // the purposes of this method we want to really query the deepest view, so we forward to it.
    forceRealHitTest = YES;
    NSView *hitView = [[[self window] contentView] hitTest:[event locationInWindow]];
    forceRealHitTest = NO;
    
    if ([hitView isKindOfClass:[self class]]) {
        WebHTMLView *hitHTMLView = (WebHTMLView *)hitView;
        [[hitHTMLView _bridge] setActivationEventNumber:[event eventNumber]];
        return [self _isSelectionEvent:event] ? [[hitHTMLView _bridge] eventMayStartDrag:event] : NO;
    } else {
        return [hitView acceptsFirstMouse:event];
    }
}

- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)event
{
    [self _setMouseDownEvent:event];

    // We hack AK's hitTest method to catch all events at the topmost WebHTMLView.  However, for
    // the purposes of this method we want to really query the deepest view, so we forward to it.
    forceRealHitTest = YES;
    NSView *hitView = [[[self window] contentView] hitTest:[event locationInWindow]];
    forceRealHitTest = NO;
    
    if ([hitView isKindOfClass:[self class]]) {
        WebHTMLView *hitHTMLView = (WebHTMLView *)hitView;
        return [self _isSelectionEvent:event] ? [[hitHTMLView _bridge] eventMayStartDrag:event] : NO;
    } else {
        return [hitView shouldDelayWindowOrderingForEvent:event];
    }
}

- (void)mouseDown:(NSEvent *)event
{   
    // Record the mouse down position so we can determine drag hysteresis.
    [self _setMouseDownEvent:event];

    // TEXTINPUT: if there is marked text and the current input
    // manager wants to handle mouse events, we need to make sure to
    // pass it to them. If not, then we need to notify the input
    // manager when the marked text is abandoned (user clicks outside
    // the marked area)

    [_private->compController endRevertingChange:NO moveLeft:NO];

    // If the web page handles the context menu event and menuForEvent: returns nil, we'll get control click events here.
    // We don't want to pass them along to KHTML a second time.
    if ([event modifierFlags] & NSControlKeyMask) {
        return;
    }
    
    _private->ignoringMouseDraggedEvents = NO;
    
    // Don't do any mouseover while the mouse is down.
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(_updateMouseoverWithFakeEvent) object:nil];

    // Let KHTML get a chance to deal with the event. This will call back to us
    // to start the autoscroll timer if appropriate.
    [[self _bridge] mouseDown:event];
}

- (void)dragImage:(NSImage *)dragImage
               at:(NSPoint)at
           offset:(NSSize)offset
            event:(NSEvent *)event
       pasteboard:(NSPasteboard *)pasteboard
           source:(id)source
        slideBack:(BOOL)slideBack
{   
    [self _stopAutoscrollTimer];
    
    _private->initiatedDrag = YES;
    [[self _webView] _setInitiatedDrag:YES];
    
    // Retain this view during the drag because it may be released before the drag ends.
    [self retain];

    [super dragImage:dragImage at:at offset:offset event:event pasteboard:pasteboard source:source slideBack:slideBack];
}

- (void)mouseDragged:(NSEvent *)event
{
    // TEXTINPUT: if there is marked text and the current input
    // manager wants to handle mouse events, we need to make sure to
    // pass it to them.

    if (!_private->ignoringMouseDraggedEvents) {
        [[self _bridge] mouseDragged:event];
    }
}

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
    if (_private->webCoreDragOp == NSDragOperationNone) {
        return (NSDragOperationGeneric | NSDragOperationCopy);
    } else {
        return _private->webCoreDragOp;
    }
}

- (void)draggedImage:(NSImage *)image movedTo:(NSPoint)screenLoc
{
    NSPoint windowImageLoc = [[self window] convertScreenToBase:screenLoc];
    NSPoint windowMouseLoc = NSMakePoint(windowImageLoc.x + _private->dragOffset.x, windowImageLoc.y + _private->dragOffset.y);
    [[self _bridge] dragSourceMovedTo:windowMouseLoc];
}

- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation
{
    NSPoint windowImageLoc = [[self window] convertScreenToBase:aPoint];
    NSPoint windowMouseLoc = NSMakePoint(windowImageLoc.x + _private->dragOffset.x, windowImageLoc.y + _private->dragOffset.y);
    [[self _bridge] dragSourceEndedAt:windowMouseLoc operation:operation];

    _private->initiatedDrag = NO;
    [[self _webView] _setInitiatedDrag:NO];
    
    // Prevent queued mouseDragged events from coming after the drag and fake mouseUp event.
    _private->ignoringMouseDraggedEvents = YES;
    
    // Once the dragging machinery kicks in, we no longer get mouse drags or the up event.
    // khtml expects to get balanced down/up's, so we must fake up a mouseup.
    NSEvent *fakeEvent = [NSEvent mouseEventWithType:NSLeftMouseUp
                                            location:windowMouseLoc
                                       modifierFlags:[[NSApp currentEvent] modifierFlags]
                                           timestamp:[NSDate timeIntervalSinceReferenceDate]
                                        windowNumber:[[self window] windowNumber]
                                             context:[[NSApp currentEvent] context]
                                         eventNumber:0 clickCount:0 pressure:0];
    [self mouseUp:fakeEvent]; // This will also update the mouseover state.
    
    // Balance the previous retain from when the drag started.
    [self release];
}

- (NSArray *)namesOfPromisedFilesDroppedAtDestination:(NSURL *)dropDestination
{
    ASSERT(_private->draggingImageURL);
    
    NSFileWrapper *wrapper = [[self _dataSource] _fileWrapperForURL:_private->draggingImageURL];
    ASSERT(wrapper);    
    
    // FIXME: Report an error if we fail to create a file.
    NSString *path = [[dropDestination path] stringByAppendingPathComponent:[wrapper preferredFilename]];
    path = [[NSFileManager defaultManager] _web_pathWithUniqueFilenameForPath:path];
    if (![wrapper writeToFile:path atomically:NO updateFilenames:YES]) {
        ERROR("Failed to create image file via -[NSFileWrapper writeToFile:atomically:updateFilenames:]");
    }
    
    return [NSArray arrayWithObject:[path lastPathComponent]];
}

- (BOOL)_canProcessDragWithDraggingInfo:(id <NSDraggingInfo>)draggingInfo
{
    NSPasteboard *pasteboard = [draggingInfo draggingPasteboard];
    NSMutableSet *types = [NSMutableSet setWithArray:[pasteboard types]];
    [types intersectSet:[NSSet setWithArray:[WebHTMLView _insertablePasteboardTypes]]];
    if ([types count] == 0) {
        return NO;
    } else if ([types count] == 1 && 
               [types containsObject:NSFilenamesPboardType] && 
               ![self _imageExistsAtPaths:[pasteboard propertyListForType:NSFilenamesPboardType]]) {
        return NO;
    }
    
    NSPoint point = [self convertPoint:[draggingInfo draggingLocation] fromView:nil];
    NSDictionary *element = [self elementAtPoint:point];
    if ([[self _webView] isEditable] || [[element objectForKey:WebElementDOMNodeKey] isContentEditable]) {
        if (_private->initiatedDrag && [[element objectForKey:WebElementIsSelectedKey] boolValue]) {
            // Can't drag onto the selection being dragged.
            return NO;
        }
        return YES;
    }
    
    return NO;
}

- (BOOL)_isMoveDrag
{
    return _private->initiatedDrag && 
    ([self _isEditable] && [self _hasSelection]) &&
    ([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask) == 0;
}

- (NSDragOperation)draggingUpdatedWithDraggingInfo:(id <NSDraggingInfo>)draggingInfo actionMask:(unsigned int)actionMask
{
    NSDragOperation operation = NSDragOperationNone;
    
    if (actionMask & WebDragDestinationActionDHTML) {
        operation = [[self _bridge] dragOperationForDraggingInfo:draggingInfo];
    }
    _private->webCoreHandlingDrag = (operation != NSDragOperationNone);
    
    if ((actionMask & WebDragDestinationActionEdit) &&
        !_private->webCoreHandlingDrag
        && [self _canProcessDragWithDraggingInfo:draggingInfo]) {
        WebView *webView = [self _webView];
        [webView moveDragCaretToPoint:[webView convertPoint:[draggingInfo draggingLocation] fromView:nil]];
        operation = [self _isMoveDrag] ? NSDragOperationMove : NSDragOperationCopy;
    } else {
        [[self _webView] removeDragCaret];
    }
    
    return operation;
}

- (void)draggingCancelledWithDraggingInfo:(id <NSDraggingInfo>)draggingInfo
{
    [[self _bridge] dragExitedWithDraggingInfo:draggingInfo];
    [[self _webView] removeDragCaret];
}

- (BOOL)concludeDragForDraggingInfo:(id <NSDraggingInfo>)draggingInfo actionMask:(unsigned int)actionMask
{
    WebView *webView = [self _webView];
    WebBridge *bridge = [self _bridge];
    if (_private->webCoreHandlingDrag) {
        ASSERT(actionMask & WebDragDestinationActionDHTML);
        [[webView _UIDelegateForwarder] webView:webView willPerformDragDestinationAction:WebDragDestinationActionDHTML forDraggingInfo:draggingInfo];
        [bridge concludeDragForDraggingInfo:draggingInfo];
        return YES;
    } else if (actionMask & WebDragDestinationActionEdit) {
        BOOL didInsert = NO;
        if ([self _canProcessDragWithDraggingInfo:draggingInfo]) {
            NSPasteboard *pasteboard = [draggingInfo draggingPasteboard];
            DOMDocumentFragment *fragment = [self _documentFragmentFromPasteboard:pasteboard allowPlainText:YES];
            if (fragment && [self _shouldInsertFragment:fragment replacingDOMRange:[bridge dragCaretDOMRange] givenAction:WebViewInsertActionDropped]) {
                [[webView _UIDelegateForwarder] webView:webView willPerformDragDestinationAction:WebDragDestinationActionEdit forDraggingInfo:draggingInfo];
                if ([self _isMoveDrag]) {
                    BOOL smartMove = [[self _bridge] selectionGranularity] == WebSelectByWord && [self _canSmartReplaceWithPasteboard:pasteboard];
                    [bridge moveSelectionToDragCaret:fragment smartMove:smartMove];
                } else {
                    [bridge setSelectionToDragCaret];
                    [bridge replaceSelectionWithFragment:fragment selectReplacement:YES smartReplace:[self _canSmartReplaceWithPasteboard:pasteboard]];
                }
                didInsert = YES;
            }
        }
        [webView removeDragCaret];
        return didInsert;
    }
    return NO;
}

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    NSDictionary *elementInfoWC = [[self _bridge] elementAtPoint:point];
    NSMutableDictionary *elementInfo = [elementInfoWC mutableCopy];
    
    // Convert URL strings to NSURLs
    [elementInfo _web_setObjectIfNotNil:[NSURL _web_URLWithDataAsString:[elementInfoWC objectForKey:WebElementLinkURLKey]] forKey:WebElementLinkURLKey];
    [elementInfo _web_setObjectIfNotNil:[NSURL _web_URLWithDataAsString:[elementInfoWC objectForKey:WebElementImageURLKey]] forKey:WebElementImageURLKey];
    
    WebFrameView *webFrameView = [self _web_parentWebFrameView];
    ASSERT(webFrameView);
    WebFrame *webFrame = [webFrameView webFrame];
    
    if (webFrame) {
        NSString *frameName = [elementInfoWC objectForKey:WebElementLinkTargetFrameKey];
        if ([frameName length] == 0) {
            [elementInfo setObject:webFrame forKey:WebElementLinkTargetFrameKey];
        } else {
            WebFrame *wf = [webFrame findFrameNamed:frameName];
            if (wf != nil)
                [elementInfo setObject:wf forKey:WebElementLinkTargetFrameKey];
            else
                [elementInfo removeObjectForKey:WebElementLinkTargetFrameKey];
        }
        
        [elementInfo setObject:webFrame forKey:WebElementFrameKey];
    }
    
    return [elementInfo autorelease];
}

- (void)mouseUp:(NSEvent *)event
{
    // TEXTINPUT: if there is marked text and the current input
    // manager wants to handle mouse events, we need to make sure to
    // pass it to them.

    [self _stopAutoscrollTimer];
    [[self _bridge] mouseUp:event];
    [self _updateMouseoverWithFakeEvent];
}

- (void)mouseMovedNotification:(NSNotification *)notification
{
    [self _updateMouseoverWithEvent:[[notification userInfo] objectForKey:@"NSEvent"]];
}

- (BOOL)supportsTextEncoding
{
    return YES;
}

- (NSView *)nextValidKeyView
{
    NSView *view = nil;
    if (![self isHiddenOrHasHiddenAncestor]) {
        view = [[self _bridge] nextKeyViewInsideWebFrameViews];
    }
    if (view == nil) {
        view = [super nextValidKeyView];
    }
    return view;
}

- (NSView *)previousValidKeyView
{
    NSView *view = nil;
    if (![self isHiddenOrHasHiddenAncestor]) {
        view = [[self _bridge] previousKeyViewInsideWebFrameViews];
    }
    if (view == nil) {
        view = [super previousValidKeyView];
    }
    return view;
}

- (BOOL)becomeFirstResponder
{
    NSView *view = nil;
    if (![[self _webView] _isPerformingProgrammaticFocus]) {
        switch ([[self window] keyViewSelectionDirection]) {
        case NSDirectSelection:
            break;
        case NSSelectingNext:
            view = [[self _bridge] nextKeyViewInsideWebFrameViews];
            break;
        case NSSelectingPrevious:
            view = [[self _bridge] previousKeyViewInsideWebFrameViews];
            break;
        }
    }
    if (view) {
        [[self window] makeFirstResponder:view];
    }
    [self updateFocusDisplay];
    _private->startNewKillRingSequence = YES;
    return YES;
}

// This approach could be relaxed when dealing with 3228554.
// Some alteration to the selection behavior was done to deal with 3672088.
- (BOOL)resignFirstResponder
{
    BOOL resign = [super resignFirstResponder];
    if (resign) {
        [_private->compController endRevertingChange:NO moveLeft:NO];
        _private->resigningFirstResponder = YES;
        if (![self maintainsInactiveSelection]) { 
            if ([[self _webView] _isPerformingProgrammaticFocus]) {
                [self deselectText];
            }
            else {
                [self deselectAll];
            }
        }
        [self updateFocusDisplay];
        _private->resigningFirstResponder = NO;
    }
    return resign;
}

//------------------------------------------------------------------------------------
// WebDocumentView protocol
//------------------------------------------------------------------------------------
- (void)setDataSource:(WebDataSource *)dataSource 
{
}

- (void)dataSourceUpdated:(WebDataSource *)dataSource
{
}

// Does setNeedsDisplay:NO as a side effect when printing is ending.
// pageWidth != 0 implies we will relayout to a new width
- (void)_setPrinting:(BOOL)printing minimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustViewSize:(BOOL)adjustViewSize
{
    WebFrame *frame = [self _frame];
    NSArray *subframes = [frame childFrames];
    unsigned n = [subframes count];
    unsigned i;
    for (i = 0; i != n; ++i) {
        WebFrame *subframe = [subframes objectAtIndex:i];
        WebFrameView *frameView = [subframe frameView];
        if ([[subframe dataSource] _isDocumentHTML]) {
            [(WebHTMLView *)[frameView documentView] _setPrinting:printing minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:adjustViewSize];
        }
    }

    if (printing != _private->printing) {
        [_private->pageRects release];
        _private->pageRects = nil;
        _private->printing = printing;
        [self setNeedsToApplyStyles:YES];
        [self setNeedsLayout:YES];
        [self layoutToMinimumPageWidth:minPageWidth maximumPageWidth:maxPageWidth adjustingViewSize:adjustViewSize];
        if (printing) {
            [[self _webView] _adjustPrintingMarginsForHeaderAndFooter];
        } else {
            // Can't do this when starting printing or nested printing won't work, see 3491427.
            [self setNeedsDisplay:NO];
        }
    }
}

// This is needed for the case where the webview is embedded in the view that's being printed.
// It shouldn't be called when the webview is being printed directly.
- (void)adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit
{
    // This helps when we print as part of a larger print process.
    // If the WebHTMLView itself is what we're printing, then we will never have to do this.
    BOOL wasInPrintingMode = _private->printing;
    if (!wasInPrintingMode) {
        [self _setPrinting:YES minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:NO];
    }
    
    [[self _bridge] adjustPageHeightNew:newBottom top:oldTop bottom:oldBottom limit:bottomLimit];
    
    if (!wasInPrintingMode) {
        [self _setPrinting:NO minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:NO];
    }
}

- (float)_availablePaperWidthForPrintOperation:(NSPrintOperation *)printOperation
{
    NSPrintInfo *printInfo = [printOperation printInfo];
    return [printInfo paperSize].width - [printInfo leftMargin] - [printInfo rightMargin];
}

- (float)_scaleFactorForPrintOperation:(NSPrintOperation *)printOperation
{
    float viewWidth = NSWidth([self bounds]);
    if (viewWidth < 1) {
        ERROR("%@ has no width when printing", self);
        return 1.0;
    }

    float userScaleFactor = [printOperation _web_pageSetupScaleFactor];
    float maxShrinkToFitScaleFactor = 1/PrintingMaximumShrinkFactor;
    float shrinkToFitScaleFactor = [self _availablePaperWidthForPrintOperation:printOperation]/viewWidth;
    return userScaleFactor * MAX(maxShrinkToFitScaleFactor, shrinkToFitScaleFactor);
}

// FIXME 3491344: This is a secret AppKit-internal method that we need to override in order
// to get our shrink-to-fit to work with a custom pagination scheme. We can do this better
// if AppKit makes it SPI/API.
- (float)_provideTotalScaleFactorForPrintOperation:(NSPrintOperation *)printOperation 
{
    return [self _scaleFactorForPrintOperation:printOperation];
}

- (void)setPageWidthForPrinting:(float)pageWidth
{
    [self _setPrinting:NO minimumPageWidth:0. maximumPageWidth:0. adjustViewSize:NO];
    [self _setPrinting:YES minimumPageWidth:pageWidth maximumPageWidth:pageWidth adjustViewSize:YES];
}


// Return the number of pages available for printing
- (BOOL)knowsPageRange:(NSRangePointer)range {
    // Must do this explicit display here, because otherwise the view might redisplay while the print
    // sheet was up, using printer fonts (and looking different).
    [self displayIfNeeded];
    [[self window] setAutodisplay:NO];
    
    // If we are a frameset just print with the layout we have onscreen, otherwise relayout
    // according to the paper size
    float minLayoutWidth = 0.0;
    float maxLayoutWidth = 0.0;
    if (![[self _bridge] isFrameSet]) {
        float paperWidth = [self _availablePaperWidthForPrintOperation:[NSPrintOperation currentOperation]];
        minLayoutWidth = paperWidth*PrintingMinimumShrinkFactor;
        maxLayoutWidth = paperWidth*PrintingMaximumShrinkFactor;
    }
    [self _setPrinting:YES minimumPageWidth:minLayoutWidth maximumPageWidth:maxLayoutWidth adjustViewSize:YES]; // will relayout
    
    // There is a theoretical chance that someone could do some drawing between here and endDocument,
    // if something caused setNeedsDisplay after this point. If so, it's not a big tragedy, because
    // you'd simply see the printer fonts on screen. As of this writing, this does not happen with Safari.

    range->location = 1;
    NSPrintOperation *printOperation = [NSPrintOperation currentOperation];
    float totalScaleFactor = [self _scaleFactorForPrintOperation:printOperation];
    float userScaleFactor = [printOperation _web_pageSetupScaleFactor];
    [_private->pageRects release];
    NSArray *newPageRects = [[self _bridge] computePageRectsWithPrintWidthScaleFactor:userScaleFactor
                                                                          printHeight:[self _calculatePrintHeight]/totalScaleFactor];
    // AppKit gets all messed up if you give it a zero-length page count (see 3576334), so if we
    // hit that case we'll pass along a degenerate 1 pixel square to print. This will print
    // a blank page (with correct-looking header and footer if that option is on), which matches
    // the behavior of IE and Camino at least.
    if ([newPageRects count] == 0) {
        newPageRects = [NSArray arrayWithObject:[NSValue valueWithRect: NSMakeRect(0, 0, 1, 1)]];
    }
    _private->pageRects = [newPageRects retain];
    
    range->length = [_private->pageRects count];
    
    return YES;
}

// Return the drawing rectangle for a particular page number
- (NSRect)rectForPage:(int)page {
    return [[_private->pageRects objectAtIndex: (page-1)] rectValue];
}

- (void)drawPageBorderWithSize:(NSSize)borderSize
{
    ASSERT(NSEqualSizes(borderSize, [[[NSPrintOperation currentOperation] printInfo] paperSize]));    
    [[self _webView] _drawHeaderAndFooter];
}

- (void)endDocument
{
    [super endDocument];
    // Note sadly at this point [NSGraphicsContext currentContextDrawingToScreen] is still NO 
    [self _setPrinting:NO minimumPageWidth:0.0 maximumPageWidth:0.0 adjustViewSize:YES];
    [[self window] setAutodisplay:YES];
}

- (BOOL)_interceptEditingKeyEvent:(NSEvent *)event
{   
    // Work around this bug:
    // <rdar://problem/3630640>: "Calling interpretKeyEvents: in a custom text view can fail to process keys right after app startup"
    [NSKeyBindingManager sharedKeyBindingManager];
    
    // Use the isEditable state to determine whether or not to process tab key events.
    // The idea here is that isEditable will be NO when this WebView is being used
    // in a browser, and we desire the behavior where tab moves to the next element
    // in tab order. If isEditable is YES, it is likely that the WebView is being
    // embedded as the whole view, as in Mail, and tabs should input tabs as expected
    // in a text editor.
    if (![[self _webView] isEditable] && [event _web_isTabKeyEvent]) 
        return NO;
    
    // Now process the key normally
    [self interpretKeyEvents:[NSArray arrayWithObject:event]];
    return YES;
}

- (void)keyDown:(NSEvent *)event
{
    BOOL callSuper = NO;

    _private->keyDownEvent = event;

    WebBridge *bridge = [self _bridge];
    if ([bridge interceptKeyEvent:event toView:self]) {
        // WebCore processed a key event, bail on any outstanding complete: UI
        [_private->compController endRevertingChange:YES moveLeft:NO];
    } else if (_private->compController && [_private->compController filterKeyDown:event]) {
        // Consumed by complete: popup window
    } else {
        // We're going to process a key event, bail on any outstanding complete: UI
        [_private->compController endRevertingChange:YES moveLeft:NO];
        if ([self _canEdit] && [self _interceptEditingKeyEvent:event]) {
            // Consumed by key bindings manager.
        } else {
            callSuper = YES;
        }
    }
    if (callSuper) {
        [super keyDown:event];
    } else {
        [NSCursor setHiddenUntilMouseMoves:YES];
    }

    _private->keyDownEvent = nil;
}

- (void)keyUp:(NSEvent *)event
{
    if (![[self _bridge] interceptKeyEvent:event toView:self]) {
        [super keyUp:event];
    }
}

- (id)accessibilityAttributeValue:(NSString*)attributeName
{
    if ([attributeName isEqualToString: NSAccessibilityChildrenAttribute]) {
        id accTree = [[self _bridge] accessibilityTree];
        if (accTree)
            return [NSArray arrayWithObject: accTree];
        return nil;
    }
    return [super accessibilityAttributeValue:attributeName];
}

- (id)accessibilityHitTest:(NSPoint)point
{
    id accTree = [[self _bridge] accessibilityTree];
    if (accTree) {
        NSPoint windowCoord = [[self window] convertScreenToBase: point];
        return [accTree accessibilityHitTest: [self convertPoint:windowCoord fromView:nil]];
    }
    else
        return self;
}

- (void)centerSelectionInVisibleArea:(id)sender
{
    [[self _bridge] centerSelectionInVisibleArea];
}

- (void)_alterCurrentSelection:(WebSelectionAlteration)alteration direction:(WebSelectionDirection)direction granularity:(WebSelectionGranularity)granularity
{
    WebBridge *bridge = [self _bridge];
    DOMRange *proposedRange = [bridge rangeByAlteringCurrentSelection:alteration direction:direction granularity:granularity];
    WebView *webView = [self _webView];
    if ([[webView _editingDelegateForwarder] webView:webView shouldChangeSelectedDOMRange:[self _selectedRange] toDOMRange:proposedRange affinity:[bridge selectionAffinity] stillSelecting:NO]) {
        [bridge alterCurrentSelection:alteration direction:direction granularity:granularity];
    }
}

- (void)_alterCurrentSelection:(WebSelectionAlteration)alteration verticalDistance:(float)verticalDistance
{
    WebBridge *bridge = [self _bridge];
    DOMRange *proposedRange = [bridge rangeByAlteringCurrentSelection:alteration verticalDistance:verticalDistance];
    WebView *webView = [self _webView];
    if ([[webView _editingDelegateForwarder] webView:webView shouldChangeSelectedDOMRange:[self _selectedRange] toDOMRange:proposedRange affinity:[bridge selectionAffinity] stillSelecting:NO]) {
        [bridge alterCurrentSelection:alteration verticalDistance:verticalDistance];
    }
}

- (void)moveBackward:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectByCharacter];
}

- (void)moveBackwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectByCharacter];
}

- (void)moveDown:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectByLine];
}

- (void)moveDownAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectByLine];
}

- (void)moveForward:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectByCharacter];
}

- (void)moveForwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectByCharacter];
}

- (void)moveLeft:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectLeft granularity:WebSelectByCharacter];
}

- (void)moveLeftAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectLeft granularity:WebSelectByCharacter];
}

- (void)moveRight:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectRight granularity:WebSelectByCharacter];
}

- (void)moveRightAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectRight granularity:WebSelectByCharacter];
}

- (void)moveToBeginningOfDocument:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectToDocumentBoundary];
}

- (void)moveToBeginningOfDocumentAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectToDocumentBoundary];
}

- (void)moveToBeginningOfLine:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectToLineBoundary];
}

- (void)moveToBeginningOfLineAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectToLineBoundary];
}

- (void)moveToBeginningOfParagraph:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectToParagraphBoundary];
}

- (void)moveToBeginningOfParagraphAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectToParagraphBoundary];
}

- (void)moveToEndOfDocument:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectToDocumentBoundary];
}

- (void)moveToEndOfDocumentAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectToDocumentBoundary];
}

- (void)moveToEndOfLine:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectToLineBoundary];
}

- (void)moveToEndOfLineAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectToLineBoundary];
}

- (void)moveToEndOfParagraph:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectToParagraphBoundary];
}

- (void)moveToEndOfParagraphAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectToParagraphBoundary];
}

- (void)moveParagraphBackwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectByParagraph];
}

- (void)moveParagraphForwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectByParagraph];
}

- (void)moveUp:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectByLine];
}

- (void)moveUpAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectByLine];
}

- (void)moveWordBackward:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectBackward granularity:WebSelectByWord];
}

- (void)moveWordBackwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectBackward granularity:WebSelectByWord];
}

- (void)moveWordForward:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectForward granularity:WebSelectByWord];
}

- (void)moveWordForwardAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectForward granularity:WebSelectByWord];
}

- (void)moveWordLeft:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectLeft granularity:WebSelectByWord];
}

- (void)moveWordLeftAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectLeft granularity:WebSelectByWord];
}

- (void)moveWordRight:(id)sender
{
    [self _alterCurrentSelection:WebSelectByMoving direction:WebSelectRight granularity:WebSelectByWord];
}

- (void)moveWordRightAndModifySelection:(id)sender
{
    [self _alterCurrentSelection:WebSelectByExtending direction:WebSelectRight granularity:WebSelectByWord];
}

- (void)pageUp:(id)sender
{
    WebFrameView *frameView = [self _web_parentWebFrameView];
    if (frameView == nil)
        return;
    [self _alterCurrentSelection:WebSelectByMoving verticalDistance:-[frameView _verticalPageScrollDistance]];
}

- (void)pageDown:(id)sender
{
    WebFrameView *frameView = [self _web_parentWebFrameView];
    if (frameView == nil)
        return;
    [self _alterCurrentSelection:WebSelectByMoving verticalDistance:[frameView _verticalPageScrollDistance]];
}

- (void)pageUpAndModifySelection:(id)sender
{
    WebFrameView *frameView = [self _web_parentWebFrameView];
    if (frameView == nil)
        return;
    [self _alterCurrentSelection:WebSelectByExtending verticalDistance:-[frameView _verticalPageScrollDistance]];
}

- (void)pageDownAndModifySelection:(id)sender
{
    WebFrameView *frameView = [self _web_parentWebFrameView];
    if (frameView == nil)
        return;
    [self _alterCurrentSelection:WebSelectByExtending verticalDistance:[frameView _verticalPageScrollDistance]];
}

- (void)_expandSelectionToGranularity:(WebSelectionGranularity)granularity
{
    WebBridge *bridge = [self _bridge];
    DOMRange *range = [bridge rangeByExpandingSelectionWithGranularity:granularity];
    if (range && ![range collapsed]) {
        WebView *webView = [self _webView];
        if ([[webView _editingDelegateForwarder] webView:webView shouldChangeSelectedDOMRange:[self _selectedRange] toDOMRange:range affinity:[bridge selectionAffinity] stillSelecting:NO]) {
            [bridge setSelectedDOMRange:range affinity:[bridge selectionAffinity]];
        }
    }
}

- (void)selectParagraph:(id)sender
{
    [self _expandSelectionToGranularity:WebSelectByParagraph];
}

- (void)selectLine:(id)sender
{
    [self _expandSelectionToGranularity:WebSelectByLine];
}

- (void)selectWord:(id)sender
{
    [self _expandSelectionToGranularity:WebSelectByWord];
}

- (void)copy:(id)sender
{
    if ([[self _bridge] tryDHTMLCopy]) {
        return;     // DHTML did the whole operation
    }
    if (![self _canCopy]) {
        NSBeep();
        return;
    }
    [self _writeSelectionToPasteboard:[NSPasteboard generalPasteboard]];
}

- (void)delete:(id)sender
{
    if (![self _canDelete]) {
        NSBeep();
        return;
    }
    [self _deleteSelection];
}

- (void)cut:(id)sender
{
    WebBridge *bridge = [self _bridge];
    if ([bridge tryDHTMLCut]) {
        return;     // DHTML did the whole operation
    }
    if (![self _canCut]) {
        NSBeep();
        return;
    }
    DOMRange *range = [self _selectedRange];
    if ([self _shouldDeleteRange:range]) {
        [self _writeSelectionToPasteboard:[NSPasteboard generalPasteboard]];
        [bridge deleteSelectionWithSmartDelete:[self _canSmartCopyOrDelete]];
   }
}

- (void)paste:(id)sender
{
    if ([[self _bridge] tryDHTMLPaste]) {
        return;     // DHTML did the whole operation
    }
    if (![self _canPaste]) {
        return;
    }
    [self _pasteWithPasteboard:[NSPasteboard generalPasteboard] allowPlainText:YES];
}

- (NSDictionary *)_selectionFontAttributes
{
    NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
    NSFont *font = [[self _bridge] fontForSelection:NULL];
    if (font != nil)
        [dictionary setObject:font forKey:NSFontAttributeName];
    return dictionary;
}

- (NSData *)_selectionFontAttributesAsRTF
{
    NSAttributedString *string = [[NSAttributedString alloc] initWithString:@"x" attributes:[self _selectionFontAttributes]];
    NSData *data = [string RTFFromRange:NSMakeRange(0, [string length]) documentAttributes:nil];
    [string release];
    return data;
}

- (NSDictionary *)_fontAttributesFromFontPasteboard
{
    NSPasteboard *fontPasteboard = [NSPasteboard pasteboardWithName:NSFontPboard];
    if (fontPasteboard == nil)
        return nil;
    NSData *data = [fontPasteboard dataForType:NSFontPboardType];
    if (data == nil || [data length] == 0)
        return nil;
    // NSTextView does something more efficient by parsing the attributes only, but that's not available in API.
    NSAttributedString *string = [[[NSAttributedString alloc] initWithRTF:data documentAttributes:NULL] autorelease];
    if (string == nil || [string length] == 0)
        return nil;
    return [string fontAttributesInRange:NSMakeRange(0, 1)];
}

- (DOMCSSStyleDeclaration *)_emptyStyle
{
    return [[[self _bridge] DOMDocument] createCSSStyleDeclaration];
}

- (NSString *)_colorAsString:(NSColor *)color
{
    NSColor *rgbColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
    // FIXME: If color is non-nil and rgbColor is nil, that means we got some kind
    // of fancy color that can't be converted to RGB. Changing that to "transparent"
    // might not be great, but it's probably OK.
    if (rgbColor == nil)
        return @"transparent";
    float r = [rgbColor redComponent];
    float g = [rgbColor greenComponent];
    float b = [rgbColor blueComponent];
    float a = [rgbColor alphaComponent];
    if (a == 0)
        return @"transparent";
    if (r == 0 && g == 0 && b == 0 && a == 1)
        return @"black";
    if (r == 1 && g == 1 && b == 1 && a == 1)
        return @"white";
    // FIXME: Lots more named colors. Maybe we could use the table in WebCore?
    if (a == 1)
        return [NSString stringWithFormat:@"rgb(%.0f,%.0f,%.0f)", r * 255, g * 255, b * 255];
    return [NSString stringWithFormat:@"rgba(%.0f,%.0f,%.0f,%f)", r * 255, g * 255, b * 255, a];
}

- (NSString *)_shadowAsString:(NSShadow *)shadow
{
    if (shadow == nil)
        return @"none";
    NSSize offset = [shadow shadowOffset];
    float blurRadius = [shadow shadowBlurRadius];
    if (offset.width == 0 && offset.height == 0 && blurRadius == 0)
        return @"none";
    NSColor *color = [shadow shadowColor];
    if (color == nil)
        return @"none";
    // FIXME: Handle non-integral values here?
    if (blurRadius == 0)
        return [NSString stringWithFormat:@"%@ %.0fpx %.0fpx", [self _colorAsString:color], offset.width, offset.height];
    return [NSString stringWithFormat:@"%@ %.0fpx %.0fpx %.0fpx", [self _colorAsString:color], offset.width, offset.height, blurRadius];
}

- (DOMCSSStyleDeclaration *)_styleFromFontAttributes:(NSDictionary *)dictionary
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];

    NSColor *color = [dictionary objectForKey:NSBackgroundColorAttributeName];
    if (color != nil) {
        [style setBackgroundColor:[self _colorAsString:color]];
    }

    NSFont *font = [dictionary objectForKey:NSFontAttributeName];
    if (font != nil) {
        NSFontManager *fm = [NSFontManager sharedFontManager];
        [style setFontFamily:[font familyName]];
        [style setFontSize:[NSString stringWithFormat:@"%0.fpx", [font pointSize]]];
        if ([fm weightOfFont:font] >= 9) {
            [style setFontWeight:@"bold"];
        } else {
            [style setFontWeight:@"normal"];
        }
        if (([fm traitsOfFont:font] & NSItalicFontMask) != 0) {
            [style setFontStyle:@"italic"];
        } else {
            [style setFontStyle:@"normal"];
        }
    }

    color = [dictionary objectForKey:NSForegroundColorAttributeName];
    if (color != nil) {
        [style setColor:[self _colorAsString:color]];
    }

    NSShadow *shadow = [dictionary objectForKey:NSShadowAttributeName];
    if (shadow) {
        [style setTextShadow:[self _shadowAsString:shadow]];
    }

    // FIXME: NSStrikethroughStyleAttributeName
    // FIXME: NSSuperscriptAttributeName
    // FIXME: NSUnderlineStyleAttributeName

    return style;
}

- (void)_applyStyleToSelection:(DOMCSSStyleDeclaration *)style
{
    if (style == nil || [style length] == 0 || ![self _canEdit])
        return;
    WebView *webView = [self _webView];
    WebBridge *bridge = [self _bridge];
    if ([[webView _editingDelegateForwarder] webView:webView shouldApplyStyle:style toElementsInDOMRange:[self _selectedRange]]) {
        [bridge applyStyle:style];
    }
}

- (void)_toggleBold
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setFontWeight:@"bold"];
    if ([[self _bridge] selectionStartHasStyle:style])
        [style setFontWeight:@"normal"];
    [self _applyStyleToSelection:style];
}

- (void)_toggleItalic
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setFontStyle:@"italic"];
    if ([[self _bridge] selectionStartHasStyle:style])
        [style setFontStyle:@"normal"];
    [self _applyStyleToSelection:style];
}

- (BOOL)_handleStyleKeyEquivalent:(NSEvent *)event
{
    if (![[WebPreferences standardPreferences] respectStandardStyleKeyEquivalents]) {
        return NO;
    }
    
    if (![self _canEdit])
        return NO;
    
    NSString *string = [event charactersIgnoringModifiers];
    if ([string isEqualToString:@"b"]) {
        [self _toggleBold];
        return YES;
    }
    if ([string isEqualToString:@"i"]) {
        [self _toggleItalic];
        return YES;
    }
    
    return NO;
}

- (BOOL)performKeyEquivalent:(NSEvent *)event
{
    if ([self _handleStyleKeyEquivalent:event]) {
        return YES;
    }
    
    // Pass command-key combos through WebCore if there is a key binding available for
    // this event. This lets web pages have a crack at intercepting command-modified keypresses.
    if ([self _web_firstResponderIsSelfOrDescendantView] && [[self _bridge] interceptKeyEvent:event toView:self]) {
        return YES;
    }
    return [super performKeyEquivalent:event];
}

- (void)copyFont:(id)sender
{
    // Put RTF with font attributes on the pasteboard.
    // Maybe later we should add a pasteboard type that contains CSS text for "native" copy and paste font.
    NSPasteboard *fontPasteboard = [NSPasteboard pasteboardWithName:NSFontPboard];
    [fontPasteboard declareTypes:[NSArray arrayWithObject:NSFontPboardType] owner:nil];
    [fontPasteboard setData:[self _selectionFontAttributesAsRTF] forType:NSFontPboardType];
}

- (void)pasteFont:(id)sender
{
    // Read RTF with font attributes from the pasteboard.
    // Maybe later we should add a pasteboard type that contains CSS text for "native" copy and paste font.
    [self _applyStyleToSelection:[self _styleFromFontAttributes:[self _fontAttributesFromFontPasteboard]]];
}

- (void)pasteAsPlainText:(id)sender
{
    if (![self _canEdit])
        return;
        
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSString *text = [pasteboard stringForType:NSStringPboardType];
    WebBridge *bridge = [self _bridge];
    if ([self _shouldReplaceSelectionWithText:text givenAction:WebViewInsertActionPasted]) {
        [bridge replaceSelectionWithText:text selectReplacement:NO smartReplace:[self _canSmartReplaceWithPasteboard:pasteboard]];
    }
}

- (void)pasteAsRichText:(id)sender
{
    // Since rich text always beats plain text when both are on the pasteboard, it's not
    // clear how this is different from plain old paste.
    [self _pasteWithPasteboard:[NSPasteboard generalPasteboard] allowPlainText:NO];
}

- (NSFont *)_originalFontA
{
    return [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:0 weight:5 size:10];
}

- (NSFont *)_originalFontB
{
    return [[NSFontManager sharedFontManager] fontWithFamily:@"Times" traits:(NSBoldFontMask | NSItalicFontMask) weight:10 size:12];
}

- (void)_addToStyle:(DOMCSSStyleDeclaration *)style fontA:(NSFont *)a fontB:(NSFont *)b
{
    if (a == nil || b == nil)
        return;

    NSFontManager *fm = [NSFontManager sharedFontManager];

    NSFont *oa = [self _originalFontA];

    NSString *fa = [a familyName];
    NSString *fb = [b familyName];
    if ([fa isEqualToString:fb]) {
        [style setFontFamily:fa];
    }

    int sa = [a pointSize];
    int sb = [b pointSize];
    int soa = [oa pointSize];
    if (sa == sb) {
        [style setFontSize:[NSString stringWithFormat:@"%dpx", sa]];
    } else if (sa < soa) {
        // FIXME: set up a style to tell WebCore to make the font in the selection 1 pixel smaller
    } else if (sa > soa) {
        // FIXME: set up a style to tell WebCore to make the font in the selection 1 pixel larger
    }

    int wa = [fm weightOfFont:a];
    int wb = [fm weightOfFont:b];
    if (wa == wb) {
        if (wa >= 9) {
            [style setFontWeight:@"bold"];
        } else {
            [style setFontWeight:@"normal"];
        }
    }

    BOOL ia = ([fm traitsOfFont:a] & NSItalicFontMask) != 0;
    BOOL ib = ([fm traitsOfFont:b] & NSItalicFontMask) != 0;
    if (ia == ib) {
        if (ia) {
            [style setFontStyle:@"italic"];
        } else {
            [style setFontStyle:@"normal"];
        }
    }
}

- (DOMCSSStyleDeclaration *)_styleFromFontManagerOperation
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];

    NSFontManager *fm = [NSFontManager sharedFontManager];

    NSFont *oa = [self _originalFontA];
    NSFont *ob = [self _originalFontB];    
    [self _addToStyle:style fontA:[fm convertFont:oa] fontB:[fm convertFont:ob]];

    return style;
}

- (void)changeFont:(id)sender
{
    [self _applyStyleToSelection:[self _styleFromFontManagerOperation]];
}

- (DOMCSSStyleDeclaration *)_styleForAttributeChange:(id)sender
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];

    NSShadow *shadow = [[NSShadow alloc] init];
    [shadow setShadowOffset:NSMakeSize(1, 1)];

    NSDictionary *oa = [NSDictionary dictionaryWithObjectsAndKeys:
        [self _originalFontA], NSFontAttributeName,
        nil];
    NSDictionary *ob = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSColor blackColor], NSBackgroundColorAttributeName,
        [self _originalFontB], NSFontAttributeName,
        [NSColor whiteColor], NSForegroundColorAttributeName,
        shadow, NSShadowAttributeName,
        [NSNumber numberWithInt:NSUnderlineStyleSingle], NSStrikethroughStyleAttributeName,
        [NSNumber numberWithInt:1], NSSuperscriptAttributeName,
        [NSNumber numberWithInt:NSUnderlineStyleSingle], NSUnderlineStyleAttributeName,
        nil];

    [shadow release];

#if 0

NSObliquenessAttributeName        /* float; skew to be applied to glyphs, default 0: no skew */
    // font-style, but that is just an on-off switch

NSExpansionAttributeName          /* float; log of expansion factor to be applied to glyphs, default 0: no expansion */
    // font-stretch?

NSKernAttributeName               /* float, amount to modify default kerning, if 0, kerning off */
    // letter-spacing? probably not good enough

NSUnderlineColorAttributeName     /* NSColor, default nil: same as foreground color */
NSStrikethroughColorAttributeName /* NSColor, default nil: same as foreground color */
    // text-decoration-color?

NSLigatureAttributeName           /* int, default 1: default ligatures, 0: no ligatures, 2: all ligatures */
NSBaselineOffsetAttributeName     /* float, in points; offset from baseline, default 0 */
NSStrokeWidthAttributeName        /* float, in percent of font point size, default 0: no stroke; positive for stroke alone, negative for stroke and fill (a typical value for outlined text would be 3.0) */
NSStrokeColorAttributeName        /* NSColor, default nil: same as foreground color */
    // need extensions?

#endif
    
    NSDictionary *a = [sender convertAttributes:oa];
    NSDictionary *b = [sender convertAttributes:ob];

    NSColor *ca = [a objectForKey:NSBackgroundColorAttributeName];
    NSColor *cb = [b objectForKey:NSBackgroundColorAttributeName];
    if (ca == cb) {
        [style setBackgroundColor:[self _colorAsString:ca]];
    }

    [self _addToStyle:style fontA:[a objectForKey:NSFontAttributeName] fontB:[b objectForKey:NSFontAttributeName]];

    ca = [a objectForKey:NSForegroundColorAttributeName];
    cb = [b objectForKey:NSForegroundColorAttributeName];
    if (ca == cb) {
        [style setColor:[self _colorAsString:ca]];
    }

    NSShadow *sha = [a objectForKey:NSShadowAttributeName];
    if (sha) {
        [style setTextShadow:[self _shadowAsString:sha]];
    } else if ([b objectForKey:NSShadowAttributeName] == nil) {
        [style setTextShadow:@"none"];
    }

    int sa = [[a objectForKey:NSStrikethroughStyleAttributeName] intValue];
    int sb = [[b objectForKey:NSStrikethroughStyleAttributeName] intValue];
    if (sa == sb) {
        if (sa == NSUnderlineStyleNone)
            [style setTextDecoration:@"none"]; // we really mean "no line-through" rather than "none"
        else
            [style setTextDecoration:@"line-through"]; // we really mean "add line-through" rather than "line-through"
    }

    sa = [[a objectForKey:NSSuperscriptAttributeName] intValue];
    sb = [[b objectForKey:NSSuperscriptAttributeName] intValue];
    if (sa == sb) {
        if (sa > 0)
            [style setVerticalAlign:@"super"];
        else if (sa < 0)
            [style setVerticalAlign:@"sub"];
        else
            [style setVerticalAlign:@"baseline"];
    }

    int ua = [[a objectForKey:NSUnderlineStyleAttributeName] intValue];
    int ub = [[b objectForKey:NSUnderlineStyleAttributeName] intValue];
    if (ua == ub) {
        if (ua == NSUnderlineStyleNone)
            [style setTextDecoration:@"none"]; // we really mean "no underline" rather than "none"
        else
            [style setTextDecoration:@"underline"]; // we really mean "add underline" rather than "underline"
    }

    return style;
}

- (void)changeAttributes:(id)sender
{
    [self _applyStyleToSelection:[self _styleForAttributeChange:sender]];
}

- (DOMCSSStyleDeclaration *)_styleFromColorPanelWithSelector:(SEL)selector
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];

    ASSERT([style respondsToSelector:selector]);
    [style performSelector:selector withObject:[self _colorAsString:[[NSColorPanel sharedColorPanel] color]]];
    
    return style;
}

- (void)_changeCSSColorUsingSelector:(SEL)selector inRange:(DOMRange *)range
{
    DOMCSSStyleDeclaration *style = [self _styleFromColorPanelWithSelector:selector];
    WebView *webView = [self _webView];
    if ([[webView _editingDelegateForwarder] webView:webView shouldApplyStyle:style toElementsInDOMRange:range]) {
        [[self _bridge] applyStyle:style];
    }
}

- (void)changeDocumentBackgroundColor:(id)sender
{
    // Mimicking NSTextView, this method sets the background color for the
    // entire document. There is no NSTextView API for setting the background
    // color on the selected range only. Note that this method is currently
    // never called from the UI (see comment in changeColor:).
    // FIXME: this actually has no effect when called, probably due to 3654850. _documentRange seems
    // to do the right thing because it works in startSpeaking:, and I know setBackgroundColor: does the
    // right thing because I tested it with [self _selectedRange].
    // FIXME: This won't actually apply the style to the entire range here, because it ends up calling
    // [bridge applyStyle:], which operates on the current selection. To make this work right, we'll
    // need to save off the selection, temporarily set it to the entire range, make the change, then
    // restore the old selection.
    [self _changeCSSColorUsingSelector:@selector(setBackgroundColor:) inRange:[self _documentRange]];
}

- (void)changeColor:(id)sender
{
    // FIXME: in NSTextView, this method calls changeDocumentBackgroundColor: when a
    // private call has earlier been made by [NSFontFontEffectsBox changeColor:], see 3674493. 
    // AppKit will have to be revised to allow this to work with anything that isn't an 
    // NSTextView. However, this might not be required for Tiger, since the background-color 
    // changing box in the font panel doesn't work in Mail (3674481), though it does in TextEdit.
    [self _applyStyleToSelection:[self _styleFromColorPanelWithSelector:@selector(setColor:)]];
}

- (void)_alignSelectionUsingCSSValue:(NSString *)CSSAlignmentValue
{
    if (![self _canEdit])
        return;
        
    // FIXME 3675191: This doesn't work yet. Maybe it's blocked by 3654850, or maybe something other than
    // just applyStyle: needs to be called for block-level attributes like this.
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setTextAlign:CSSAlignmentValue];
    [self _applyStyleToSelection:style];
}

- (void)alignCenter:(id)sender
{
    [self _alignSelectionUsingCSSValue:@"center"];
}

- (void)alignJustified:(id)sender
{
    [self _alignSelectionUsingCSSValue:@"justify"];
}

- (void)alignLeft:(id)sender
{
    [self _alignSelectionUsingCSSValue:@"left"];
}

- (void)alignRight:(id)sender
{
    [self _alignSelectionUsingCSSValue:@"right"];
}

- (void)insertTab:(id)sender
{
    [self insertText:@"\t"];
}

- (void)insertBacktab:(id)sender
{
    // Doing nothing matches normal NSTextView behavior. If we ever use WebView for a field-editor-type purpose
    // we might add code here.
}

- (void)insertNewline:(id)sender
{
    if (![self _canEdit])
        return;
        
    // Perhaps we should make this delegate call sensitive to the real DOM operation we actually do.
    WebBridge *bridge = [self _bridge];
    if ([self _shouldReplaceSelectionWithText:@"\n" givenAction:WebViewInsertActionTyped]) {
        [bridge insertNewline];
    }
}

- (void)insertParagraphSeparator:(id)sender
{
    if (![self _canEdit])
        return;

    // FIXME: Should this do something different? Do we have the equivalent of a paragraph separator?
    [self insertNewline:sender];
}

- (void)_changeWordCaseWithSelector:(SEL)selector
{
    if (![self _canEdit])
        return;

    WebBridge *bridge = [self _bridge];
    [self selectWord:nil];
    NSString *word = [[bridge selectedString] performSelector:selector];
    // FIXME: Does this need a different action context other than "typed"?
    if ([self _shouldReplaceSelectionWithText:word givenAction:WebViewInsertActionTyped]) {
        [bridge replaceSelectionWithText:word selectReplacement:NO smartReplace:NO];
    }
}

- (void)uppercaseWord:(id)sender
{
    [self _changeWordCaseWithSelector:@selector(uppercaseString)];
}

- (void)lowercaseWord:(id)sender
{
    [self _changeWordCaseWithSelector:@selector(lowercaseString)];
}

- (void)capitalizeWord:(id)sender
{
    [self _changeWordCaseWithSelector:@selector(capitalizedString)];
}

- (BOOL)_deleteWithDirection:(WebSelectionDirection)direction granularity:(WebSelectionGranularity)granularity killRing:(BOOL)killRing
{
    // Delete the selection, if there is one.
    // If not, make a selection using the passed-in direction and granularity.
    if (![self _canEdit])
        return NO;
        
    DOMRange *range;
    BOOL prepend = NO;
    BOOL smartDeleteOK = NO;
    if ([self _hasSelection]) {
        range = [self _selectedRange];
        smartDeleteOK = YES;
    } else {
        WebBridge *bridge = [self _bridge];
        range = [bridge rangeByAlteringCurrentSelection:WebSelectByExtending direction:direction granularity:granularity];
        if (range == nil || [range collapsed])
            return NO;
        switch (direction) {
            case WebSelectForward:
            case WebSelectRight:
                break;
            case WebSelectBackward:
            case WebSelectLeft:
                prepend = YES;
                break;
        }
    }
    [self _deleteRange:range preflight:YES killRing:killRing prepend:prepend smartDeleteOK:smartDeleteOK];
    return YES;
}

- (void)deleteForward:(id)sender
{
    [self _deleteWithDirection:WebSelectForward granularity:WebSelectByCharacter killRing:NO];
}

- (void)deleteBackward:(id)sender
{
    if (![self _isEditable])
        return;
    if ([self _hasSelection]) {
        [self _deleteSelection];
    } else {
        // FIXME: We are not calling the delegate here. Why can't we just call _deleteRange here?
        [[self _bridge] deleteKeyPressed];
    }
}

- (void)deleteBackwardByDecomposingPreviousCharacter:(id)sender
{
    ERROR("unimplemented, doing deleteBackward instead");
    [self deleteBackward:sender];
}

- (void)deleteWordForward:(id)sender
{
    [self _deleteWithDirection:WebSelectForward granularity:WebSelectByWord killRing:YES];
}

- (void)deleteWordBackward:(id)sender
{
    [self _deleteWithDirection:WebSelectBackward granularity:WebSelectByWord killRing:YES];
}

- (void)deleteToBeginningOfLine:(id)sender
{
    [self _deleteWithDirection:WebSelectBackward granularity:WebSelectToLineBoundary killRing:YES];
}

- (void)deleteToEndOfLine:(id)sender
{
    // FIXME: To match NSTextView, this command should delete the newline at the end of
    // a paragraph if you are at the end of a paragraph (like deleteToEndOfParagraph does below).
    [self _deleteWithDirection:WebSelectForward granularity:WebSelectToLineBoundary killRing:YES];
}

- (void)deleteToBeginningOfParagraph:(id)sender
{
    [self _deleteWithDirection:WebSelectBackward granularity:WebSelectToParagraphBoundary killRing:YES];
}

- (void)deleteToEndOfParagraph:(id)sender
{
    // Despite the name of the method, this should delete the newline if the caret is at the end of a paragraph.
    // If deletion to the end of the paragraph fails, we delete one character forward, which will delete the newline.
    if (![self _deleteWithDirection:WebSelectForward granularity:WebSelectToParagraphBoundary killRing:YES])
        [self _deleteWithDirection:WebSelectForward granularity:WebSelectByCharacter killRing:YES];
}

- (void)complete:(id)sender
{
    if (![self _canEdit])
        return;

    if (!_private->compController) {
        _private->compController = [[WebTextCompleteController alloc] initWithHTMLView:self];
    }
    [_private->compController doCompletion];
}

- (void)checkSpelling:(id)sender
{
    // WebCore does everything but update the spelling panel
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (!checker) {
        ERROR("No NSSpellChecker");
        return;
    }
    NSString *badWord = [[self _bridge] advanceToNextMisspelling];
    if (badWord) {
        [checker updateSpellingPanelWithMisspelledWord:badWord];
    }
}

- (void)showGuessPanel:(id)sender
{
    // WebCore does everything but update the spelling panel
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (!checker) {
        ERROR("No NSSpellChecker");
        return;
    }
    NSString *badWord = [[self _bridge] advanceToNextMisspellingStartingJustBeforeSelection];
    if (badWord) {
        [checker updateSpellingPanelWithMisspelledWord:badWord];
    }
    [[checker spellingPanel] orderFront:sender];
}

- (void)_changeSpellingToWord:(NSString *)newWord
{
    if (![self _canEdit])
        return;

    // Don't correct to empty string.  (AppKit checked this, we might as well too.)
    if (![NSSpellChecker sharedSpellChecker]) {
        ERROR("No NSSpellChecker");
        return;
    }
    
    if ([newWord isEqualToString:@""]) {
        return;
    }

    if ([self _shouldReplaceSelectionWithText:newWord givenAction:WebViewInsertActionPasted]) {
        [[self _bridge] replaceSelectionWithText:newWord selectReplacement:YES smartReplace:NO];
    }
}

- (void)changeSpelling:(id)sender
{
    [self _changeSpellingToWord:[[sender selectedCell] stringValue]];
}

- (void)ignoreSpelling:(id)sender
{
    if (![self _canEdit])
        return;
    
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (!checker) {
        ERROR("No NSSpellChecker");
        return;
    }
    
    NSString *stringToIgnore = [sender stringValue];
    unsigned int length = [stringToIgnore length];
    if (stringToIgnore && length > 0) {
        [checker ignoreWord:stringToIgnore inSpellDocumentWithTag:[[self _webView] spellCheckerDocumentTag]];
        // FIXME: Need to clear misspelling marker if the currently selected word is the one we are to ignore?
    }
}

- (void)performFindPanelAction:(id)sender
{
    // Implementing this will probably require copying all of NSFindPanel.h and .m.
    // We need *almost* the same thing as AppKit, but not quite.
    ERROR("unimplemented");
}

- (void)startSpeaking:(id)sender
{
    WebBridge *bridge = [self _bridge];
    DOMRange *range = [self _selectedRange];
    if (!range || [range collapsed]) {
        range = [self _documentRange];
    }
    [NSApp speakString:[bridge stringForRange:range]];
}

- (void)stopSpeaking:(id)sender
{
    [NSApp stopSpeaking:sender];
}

- (void)insertNewlineIgnoringFieldEditor:(id)sender
{
    [self insertNewline:sender];
}

- (void)insertTabIgnoringFieldEditor:(id)sender
{
    [self insertTab:sender];
}

- (void)subscript:(id)sender
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setVerticalAlign:@"sub"];
    [self _applyStyleToSelection:style];
}

- (void)superscript:(id)sender
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setVerticalAlign:@"super"];
    [self _applyStyleToSelection:style];
}

- (void)unscript:(id)sender
{
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setVerticalAlign:@"baseline"];
    [self _applyStyleToSelection:style];
}

- (void)underline:(id)sender
{
    // Despite the name, this method is actually supposed to toggle underline.
    // FIXME: This currently clears overline, line-through, and blink as an unwanted side effect.
    DOMCSSStyleDeclaration *style = [self _emptyStyle];
    [style setTextDecoration:@"underline"];
    if ([[self _bridge] selectionStartHasStyle:style])
        [style setTextDecoration:@"none"];
    [self _applyStyleToSelection:style];
}

- (void)yank:(id)sender
{
    if (![self _canEdit])
        return;
        
    [self insertText:_NSYankFromKillRing()];
    _NSSetKillRingToYankedState();
}

- (void)yankAndSelect:(id)sender
{
    if (![self _canEdit])
        return;

    [self _insertText:_NSYankPreviousFromKillRing() selectInsertedText:YES];
    _NSSetKillRingToYankedState();
}

- (void)setMark:(id)sender
{
    [[self _bridge] setMarkDOMRange:[self _selectedRange]];
}

static DOMRange *unionDOMRanges(DOMRange *a, DOMRange *b)
{
    ASSERT(a);
    ASSERT(b);
    DOMRange *s = [a compareBoundaryPoints:DOM_START_TO_START :b] <= 0 ? a : b;
    DOMRange *e = [a compareBoundaryPoints:DOM_END_TO_END :b] <= 0 ? b : a;
    DOMRange *r = [[[a startContainer] ownerDocument] createRange];
    [r setStart:[s startContainer] :[s startOffset]];
    [r setEnd:[e endContainer] :[e endOffset]];
    return r;
}

- (void)deleteToMark:(id)sender
{
    if (![self _canEdit])
        return;

    DOMRange *mark = [[self _bridge] markDOMRange];
    if (mark == nil) {
        [self delete:sender];
    } else {
        DOMRange *selection = [self _selectedRange];
        DOMRange *r;
        @try {
            r = unionDOMRanges(mark, selection);
        } @catch (NSException *exception) {
            r = selection;
        }
        [self _deleteRange:r preflight:YES killRing:YES prepend:YES smartDeleteOK:NO];
    }
    [self setMark:sender];
}

- (void)selectToMark:(id)sender
{
    WebBridge *bridge = [self _bridge];
    DOMRange *mark = [bridge markDOMRange];
    if (mark == nil) {
        NSBeep();
        return;
    }
    DOMRange *selection = [self _selectedRange];
    @try {
        [bridge setSelectedDOMRange:unionDOMRanges(mark, selection) affinity:NSSelectionAffinityUpstream];
    } @catch (NSException *exception) {
        NSBeep();
    }
}

- (void)swapWithMark:(id)sender
{
    if (![self _canEdit])
        return;

    WebBridge *bridge = [self _bridge];
    DOMRange *mark = [bridge markDOMRange];
    if (mark == nil) {
        NSBeep();
        return;
    }
    DOMRange *selection = [self _selectedRange];
    @try {
        [bridge setSelectedDOMRange:mark affinity:NSSelectionAffinityUpstream];
    } @catch (NSException *exception) {
        NSBeep();
        return;
    }
    [bridge setMarkDOMRange:selection];
}

- (void)transpose:(id)sender
{
    if (![self _canEdit])
        return;

    WebBridge *bridge = [self _bridge];
    DOMRange *r = [bridge rangeOfCharactersAroundCaret];
    if (!r) {
        return;
    }
    NSString *characters = [bridge stringForRange:r];
    if ([characters length] != 2) {
        return;
    }
    NSString *transposed = [[characters substringFromIndex:1] stringByAppendingString:[characters substringToIndex:1]];
    WebView *webView = [self _webView];
    if (![[webView _editingDelegateForwarder] webView:webView shouldChangeSelectedDOMRange:[self _selectedRange] toDOMRange:r affinity:NSSelectionAffinityUpstream stillSelecting:NO]) {
        return;
    }
    [bridge setSelectedDOMRange:r affinity:NSSelectionAffinityUpstream];
    if ([self _shouldReplaceSelectionWithText:transposed givenAction:WebViewInsertActionTyped]) {
        [bridge replaceSelectionWithText:transposed selectReplacement:NO smartReplace:NO];
    }
}

#if 0

// CSS does not have a way to specify an outline font, which may make this difficult to implement.
// Maybe a special case of text-shadow?
- (void)outline:(id)sender;

// This is part of table support, which may be in NSTextView for Tiger.
// It's probably simple to do the equivalent thing for WebKit.
- (void)insertTable:(id)sender;

// === key binding methods that NSTextView has that don't have standard key bindings

// These could be important.
- (void)toggleBaseWritingDirection:(id)sender;
- (void)toggleTraditionalCharacterShape:(id)sender;
- (void)changeBaseWritingDirection:(id)sender;

// I'm not sure what the equivalents of these in the web world are; we use <br> as a paragraph break.
- (void)insertLineBreak:(id)sender;
- (void)insertLineSeparator:(id)sender;
- (void)insertPageBreak:(id)sender;

// === methods not present in NSTextView

// These methods are not implemented in NSTextView yet, so perhaps there's no rush.
- (void)changeCaseOfLetter:(id)sender;
- (void)indent:(id)sender;
- (void)transposeWords:(id)sender;

#endif

// Super-hack alert.
// Workaround for bug 3789278.

// Returns a selector only if called while:
//   1) first responder is self
//   2) handling a key down event
//   3) not yet inside keyDown: method
//   4) key is an arrow key
// The selector is the one that gets sent by -[NSWindow _processKeyboardUIKey] for this key.
- (SEL)_arrowKeyDownEventSelectorIfPreprocessing
{
    NSWindow *w = [self window];
    if ([w firstResponder] != self)
        return NULL;
    NSEvent *e = [w currentEvent];
    if ([e type] != NSKeyDown)
        return NULL;
    if (e == _private->keyDownEvent)
        return NULL;
    NSString *s = [e charactersIgnoringModifiers];
    if ([s length] == 0)
        return NULL;
    switch ([s characterAtIndex:0]) {
        case NSDownArrowFunctionKey:
            return @selector(moveDown:);
        case NSLeftArrowFunctionKey:
            return @selector(moveLeft:);
        case NSRightArrowFunctionKey:
            return @selector(moveRight:);
        case NSUpArrowFunctionKey:
            return @selector(moveUp:);
        default:
            return NULL;
    }
}

// Returns NO instead of YES if called on the selector that the
// _arrowKeyDownEventSelectorIfPreprocessing method returns.
// This should only happen inside -[NSWindow _processKeyboardUIKey],
// and together with the change below should cause that method
// to return NO rather than handling the key.
// Also set a 1-shot flag for the nextResponder check below.
- (BOOL)respondsToSelector:(SEL)selector
{
    if (![super respondsToSelector:selector])
        return NO;
    SEL arrowKeySelector = [self _arrowKeyDownEventSelectorIfPreprocessing];
    if (selector != arrowKeySelector)
        return YES;
    _private->nextResponderDisabledOnce = YES;
    return NO;
}

// Returns nil instead of the next responder if called when the
// one-shot flag is set, and _arrowKeyDownEventSelectorIfPreprocessing
// returns something other than NULL. This should only happen inside
// -[NSWindow _processKeyboardUIKey] and together with the change above
// should cause that method to return NO rather than handling the key.
- (NSResponder *)nextResponder
{
    BOOL disabled = _private->nextResponderDisabledOnce;
    _private->nextResponderDisabledOnce = NO;
    if (disabled && [self _arrowKeyDownEventSelectorIfPreprocessing] != NULL) {
        return nil;
    }
    return [super nextResponder];
}

@end

@implementation WebHTMLView (WebTextSizing)

- (void)_web_textSizeMultiplierChanged
{
    [self _updateTextSizeMultiplier];
}

@end

@implementation NSArray (WebHTMLView)

- (void)_web_makePluginViewsPerformSelector:(SEL)selector withObject:(id)object
{
    NSEnumerator *enumerator = [self objectEnumerator];
    WebNetscapePluginEmbeddedView *view;
    while ((view = [enumerator nextObject]) != nil) {
        if ([view isKindOfClass:[WebNetscapePluginEmbeddedView class]]) {
            [view performSelector:selector withObject:object];
        }
    }
}

@end

@implementation WebHTMLView (WebInternal)

- (void)_selectionChanged
{
    [self _updateSelectionForInputManager];
    [self _updateFontPanel];
    _private->startNewKillRingSequence = YES;
}

- (void)_updateFontPanel
{
    // FIXME: NSTextView bails out if becoming or resigning first responder, for which it has ivar flags. Not
    // sure if we need to do something similar.
    
    if (![self _canEdit])
        return;
    
    NSWindow *window = [self window];
    // FIXME: is this first-responder check correct? What happens if a subframe is editable and is first responder?
    if ([NSApp keyWindow] != window || [window firstResponder] != self) {
        return;
    }
    
    BOOL multiple = NO;
    NSFont *font = [[self _bridge] fontForSelection:&multiple];

    // FIXME: for now, return a bogus font that distinguishes the empty selection from the non-empty
    // selection. We should be able to remove this once the rest of this code works properly.
    if (font == nil) {
        if (![self _hasSelection]) {
            font = [NSFont toolTipsFontOfSize:17];
        } else {
            font = [NSFont menuFontOfSize:23];
        }
    }
    ASSERT(font != nil);

    NSFontManager *fm = [NSFontManager sharedFontManager];
    [fm setSelectedFont:font isMultiple:multiple];

    // FIXME: we don't keep track of selected attributes, or set them on the font panel. This
    // appears to have no effect on the UI. E.g., underlined text in Mail or TextEdit is
    // not reflected in the font panel. Maybe someday this will change.
}

- (unsigned int)_delegateDragSourceActionMask
{
    ASSERT(_private->mouseDownEvent != nil);
    WebView *webView = [self _webView];
    NSPoint point = [webView convertPoint:[_private->mouseDownEvent locationInWindow] fromView:nil];
    _private->dragSourceActionMask = [[webView _UIDelegateForwarder] webView:webView dragSourceActionMaskForPoint:point];
    return _private->dragSourceActionMask;
}

- (BOOL)_canSmartCopyOrDelete
{
    return [[self _webView] smartInsertDeleteEnabled] && [[self _bridge] selectionGranularity] == WebSelectByWord;
}

@end

@implementation WebHTMLView (WebNSTextInputSupport)

- (NSArray *)validAttributesForMarkedText
{
    // FIXME: TEXTINPUT: validAttributesForMarkedText not yet implemented
    return [NSArray array];
}

- (unsigned int)characterIndexForPoint:(NSPoint)thePoint
{
    ERROR("TEXTINPUT: characterIndexForPoint: not yet implemented");
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)theRange
{
    ERROR("TEXTINPUT: firstRectForCharacterRange: not yet implemented");
    return NSMakeRect(0,0,0,0);
}

- (NSRange)selectedRange
{
    ERROR("TEXTINPUT: selectedRange not yet implemented");
    return NSMakeRange(0,0);
}

- (NSRange)markedRange
{
    if (![self hasMarkedText]) {
	return NSMakeRange(NSNotFound,0);
    }

    DOMRange *markedTextDOMRange = [[self _bridge] markedTextDOMRange];

    unsigned rangeLocation = [markedTextDOMRange startOffset];
    unsigned rangeLength = [markedTextDOMRange endOffset] - rangeLocation;

    return NSMakeRange(rangeLocation, rangeLength);
}

- (NSAttributedString *)attributedSubstringFromRange:(NSRange)theRange
{
    ERROR("TEXTINPUT: attributedSubstringFromRange: not yet implemented");
    return nil;
}

- (long)conversationIdentifier
{
    return (long)self;
}

- (BOOL)hasMarkedText
{
    return [[self _bridge] markedTextDOMRange] != nil;
}

- (void)unmarkText
{
    [[self _bridge] setMarkedTextDOMRange:nil];
}

- (void)_selectMarkedText
{
    if ([self hasMarkedText]) {
	WebBridge *bridge = [self _bridge];
	DOMRange *markedTextRange = [bridge markedTextDOMRange];
	[bridge setSelectedDOMRange:markedTextRange affinity:NSSelectionAffinityUpstream];
    }
}

- (void)_selectRangeInMarkedText:(NSRange)range
{
    ASSERT([self hasMarkedText]);

    WebBridge *bridge = [self _bridge];
    DOMRange *selectedRange = [[bridge DOMDocument] createRange];
    DOMRange *markedTextRange = [bridge markedTextDOMRange];
    
    ASSERT([markedTextRange startContainer] == [markedTextRange endContainer]);

    unsigned selectionStart = [markedTextRange startOffset] + range.location;
    unsigned selectionEnd = selectionStart + range.length;

    [selectedRange setStart:[markedTextRange startContainer] :selectionStart];
    [selectedRange setEnd:[markedTextRange startContainer] :selectionEnd];

    [bridge setSelectedDOMRange:selectedRange affinity:NSSelectionAffinityUpstream];
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)newSelRange
{
    WebBridge *bridge = [self _bridge];

    if (![self _isEditable])
	return;

    _private->ignoreMarkedTextSelectionChange = YES;

    // if we had marked text already, we need to make sure to replace
    // that, instead of the selection/caret
    [self _selectMarkedText];

    NSString *text;
    if ([string isKindOfClass:[NSAttributedString class]]) {
	ERROR("TEXTINPUT: requested set marked text with attributed string");
	text = [string string];
    } else {
	text = string;
    }

    [bridge replaceSelectionWithText:text selectReplacement:YES smartReplace:NO];
    [bridge setMarkedTextDOMRange:[self _selectedRange]];
    [self _selectRangeInMarkedText:newSelRange];

    _private->ignoreMarkedTextSelectionChange = NO;
}

- (void)doCommandBySelector:(SEL)aSelector
{
    WebView *webView = [self _webView];
    // FIXME 3810158: need to enable this code when Mail returns NO for this delegate method
    if (![[webView _editingDelegateForwarder] webView:webView doCommandBySelector:aSelector] || YES) {
//  if (![[webView _editingDelegateForwarder] webView:webView doCommandBySelector:aSelector]) {
        [super doCommandBySelector:aSelector];
    }
}

- (void)_discardMarkedText
{
    if (![self hasMarkedText])
	return;

    _private->ignoreMarkedTextSelectionChange = YES;

    [self _selectMarkedText];
    [[NSInputManager currentInputManager] markedTextAbandoned:self];
    [self unmarkText];
    // FIXME: Should we be calling the delegate here?
    [[self _bridge] deleteSelectionWithSmartDelete:NO];

    _private->ignoreMarkedTextSelectionChange = NO;
}

- (void)_insertText:(NSString *)text selectInsertedText:(BOOL)selectText
{
    if (text == nil || (![self _isEditable] && ![self hasMarkedText])) {
        return;
    }

    if (![self _shouldReplaceSelectionWithText:text givenAction:WebViewInsertActionTyped]) {
	[self _discardMarkedText];
	return;
    }

    _private->ignoreMarkedTextSelectionChange = YES;

    // If we had marked text, we replace that, instead of the selection/caret.
    [self _selectMarkedText];

    [[self _bridge] insertText:text selectInsertedText:selectText];

    _private->ignoreMarkedTextSelectionChange = NO;

    // Inserting unmarks any marked text.
    [self unmarkText];
}

- (void)insertText:(id)string
{
    NSString *text;
    if ([string isKindOfClass:[NSAttributedString class]]) {
	ERROR("TEXTINPUT: requested insert of attributed string");
	text = [string string];
    } else {
	text = string;
    }
    [self _insertText:text selectInsertedText:NO];
}

- (BOOL)_selectionIsInsideMarkedText
{
    WebBridge *bridge = [self _bridge];
    DOMRange *selection = [self _selectedRange];
    DOMRange *markedTextRange = [bridge markedTextDOMRange];

    ASSERT([markedTextRange startContainer] == [markedTextRange endContainer]);

    if ([selection startContainer] != [markedTextRange startContainer]) 
	return NO;

    if ([selection endContainer] != [markedTextRange startContainer])
	return NO;

    if ([selection startOffset] < [markedTextRange startOffset])
	return NO;

    if ([selection endOffset] > [markedTextRange endOffset])
	return NO;

    return YES;
}

- (void)_updateSelectionForInputManager
{
    if (![self hasMarkedText] || _private->ignoreMarkedTextSelectionChange)
	return;

    if ([self _selectionIsInsideMarkedText]) {
	DOMRange *selection = [self _selectedRange];
	DOMRange *markedTextDOMRange = [[self _bridge] markedTextDOMRange];

	unsigned markedSelectionStart = [selection startOffset] - [markedTextDOMRange startOffset];
	unsigned markedSelectionLength = [selection endOffset] - [selection startOffset];
	NSRange newSelectionRange = NSMakeRange(markedSelectionStart, markedSelectionLength);
	
	[[NSInputManager currentInputManager] markedTextSelectionChanged:newSelectionRange client:self];
    } else {
	[[NSInputManager currentInputManager] markedTextAbandoned:self];
	[self unmarkText];
    }
}

@end

/*
    This class runs the show for handing the complete: NSTextView operation.  It counts on its HTML view
    to call endRevertingChange: whenever the current completion needs to be aborted.
 
    The class is in one of two modes:  PopupWindow showing, or not.  It is shown when a completion yields
    more than one match.  If a completion yields one or zero matches, it is not shown, and **there is no
    state carried across to the next completion**.
 */
@implementation WebTextCompleteController

- (id)initWithHTMLView:(WebHTMLView *)view
{
    [super init];
    _view = view;
    return self;
}

- (void)dealloc
{
    [_popupWindow release];
    [_completions release];
    [_originalString release];
    [super dealloc];
}

- (void)_insertMatch:(NSString *)match
{
    // FIXME: 3769654 - We should preserve case of string being inserted, even in prefix (but then also be
    // able to revert that).  Mimic NSText.
    WebBridge *bridge = [_view _bridge];
    NSString *newText = [match substringFromIndex:prefixLength];
    [bridge replaceSelectionWithText:newText selectReplacement:YES smartReplace:NO];
}

// mostly lifted from NSTextView_KeyBinding.m
- (void)_buildUI
{
    NSRect scrollFrame = NSMakeRect(0, 0, 100, 100);
    NSRect tableFrame = NSZeroRect;    
    tableFrame.size = [NSScrollView contentSizeForFrameSize:scrollFrame.size hasHorizontalScroller:NO hasVerticalScroller:YES borderType:NSNoBorder];
    // Added cast to work around problem with multiple Foundation initWithIdentifier: methods with different parameter types.
    NSTableColumn *column = [(NSTableColumn *)[NSTableColumn alloc] initWithIdentifier:[NSNumber numberWithInt:0]];
    [column setWidth:tableFrame.size.width];
    [column setEditable:NO];
    
    _tableView = [[NSTableView alloc] initWithFrame:tableFrame];
    [_tableView setAutoresizingMask:NSViewWidthSizable];
    [_tableView addTableColumn:column];
    [column release];
    [_tableView setDrawsGrid:NO];
    [_tableView setCornerView:nil];
    [_tableView setHeaderView:nil];
    [_tableView setAutoresizesAllColumnsToFit:YES];
    [_tableView setDelegate:self];
    [_tableView setDataSource:self];
    [_tableView setTarget:self];
    [_tableView setDoubleAction:@selector(tableAction:)];
    
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:scrollFrame];
    [scrollView setBorderType:NSNoBorder];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [scrollView setDocumentView:_tableView];
    [_tableView release];
    
    _popupWindow = [[NSWindow alloc] initWithContentRect:scrollFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    [_popupWindow setAlphaValue:0.88];
    [_popupWindow setContentView:scrollView];
    [scrollView release];
    [_popupWindow setHasShadow:YES];
    [_popupWindow setOneShot:YES];
    //[_popupWindow _setForceActiveControls:YES];   // AK secret - no known problem from leaving this out
    [_popupWindow setReleasedWhenClosed:NO];
}

// mostly lifted from NSTextView_KeyBinding.m
- (void)_placePopupWindow:(NSPoint)topLeft
{
    int numberToShow = [_completions count];
    if (numberToShow > 20) {
        numberToShow = 20;
    }

    NSRect windowFrame;
    NSPoint wordStart = topLeft;
    windowFrame.origin = [[_view window] convertBaseToScreen:[_view convertPoint:wordStart toView:nil]];
    windowFrame.size.height = numberToShow * [_tableView rowHeight] + (numberToShow + 1) * [_tableView intercellSpacing].height;
    windowFrame.origin.y -= windowFrame.size.height;
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:12.0], NSFontAttributeName, nil];
    float maxWidth = 0.0;
    int maxIndex = -1;
    int i;
    for (i = 0; i < numberToShow; i++) {
        float width = ceil([[_completions objectAtIndex:i] sizeWithAttributes:attributes].width);
        if (width > maxWidth) {
            maxWidth = width;
            maxIndex = i;
        }
    }
    windowFrame.size.width = 100;
    if (maxIndex >= 0) {
        maxWidth = ceil([NSScrollView frameSizeForContentSize:NSMakeSize(maxWidth, 100) hasHorizontalScroller:NO hasVerticalScroller:YES borderType:NSNoBorder].width);
        maxWidth = ceil([NSWindow frameRectForContentRect:NSMakeRect(0, 0, maxWidth, 100) styleMask:NSBorderlessWindowMask].size.width);
        maxWidth += 5.0;
        windowFrame.size.width = MAX(maxWidth, windowFrame.size.width);
        maxWidth = MIN(400.0, windowFrame.size.width);
    }
    [_popupWindow setFrame:windowFrame display:NO];
    
    [_tableView reloadData];
    [_tableView selectRow:0 byExtendingSelection:NO];
    [_tableView scrollRowToVisible:0];
    [self _reflectSelection];
    [_popupWindow setLevel:NSPopUpMenuWindowLevel];
    [_popupWindow orderFront:nil];    
    [[_view window] addChildWindow:_popupWindow ordered:NSWindowAbove];
}

- (void)doCompletion
{
    if (!_popupWindow) {
        NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
        if (!checker) {
            ERROR("No NSSpellChecker");
            return;
        }

        // Get preceeding word stem
        WebBridge *bridge = [_view _bridge];
        DOMRange *selection = [bridge selectedDOMRange];
        DOMRange *wholeWord = [bridge rangeByExpandingSelectionWithGranularity:WebSelectByWord];
        DOMRange *prefix = [wholeWord cloneRange];
        [prefix setEnd:[selection startContainer] :[selection startOffset]];

        // Reject some NOP cases
        if ([prefix collapsed]) {
            NSBeep();
            return;
        }
        NSString *prefixStr = [bridge stringForRange:prefix];
        NSString *trimmedPrefix = [prefixStr stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        if ([trimmedPrefix length] == 0) {
            NSBeep();
            return;
        }
        prefixLength = [prefixStr length];

        // Lookup matches
        [_completions release];
        _completions = [checker completionsForPartialWordRange:NSMakeRange(0, [prefixStr length]) inString:prefixStr language:nil inSpellDocumentWithTag:[[_view _webView] spellCheckerDocumentTag]];
        [_completions retain];
    
        if (!_completions || [_completions count] == 0) {
            NSBeep();
        } else if ([_completions count] == 1) {
            [self _insertMatch:[_completions objectAtIndex:0]];
        } else {
            ASSERT(!_originalString);       // this should only be set IFF we have a popup window
            _originalString = [[bridge stringForRange:selection] retain];
            [self _buildUI];
            NSRect wordRect = [bridge caretRectAtNode:[wholeWord startContainer] offset:[wholeWord startOffset]];
            // +1 to be under the word, not the caret
            // FIXME - 3769652 - Wrong positioning for right to left languages.  We should line up the upper
            // right corner with the caret instead of upper left, and the +1 would be a -1.
            NSPoint wordLowerLeft = { NSMinX(wordRect)+1, NSMaxY(wordRect) };
            [self _placePopupWindow:wordLowerLeft];
        }
    } else {
        [self endRevertingChange:YES moveLeft:NO];
    }
}

- (void)endRevertingChange:(BOOL)revertChange moveLeft:(BOOL)goLeft
{
    if (_popupWindow) {
        // tear down UI
        [[_view window] removeChildWindow:_popupWindow];
        [_popupWindow orderOut:self];
        // Must autorelease because event tracking code may be on the stack touching UI
        [_popupWindow autorelease];
        _popupWindow = nil;

        if (revertChange) {
            WebBridge *bridge = [_view _bridge];
            [bridge replaceSelectionWithText:_originalString selectReplacement:YES smartReplace:NO];
        } else if (goLeft) {
            [_view moveBackward:nil];
        } else {
            [_view moveForward:nil];
        }
        [_originalString release];
        _originalString = nil;
    }
    // else there is no state to abort if the window was not up
}

// WebHTMLView gives us a crack at key events it sees.  Return whether we consumed the event.
// The features for the various keys mimic NSTextView.
- (BOOL)filterKeyDown:(NSEvent *)event
{
    if (_popupWindow) {
        NSString *string = [event charactersIgnoringModifiers];
        unichar c = [string characterAtIndex:0];
        if (c == NSUpArrowFunctionKey) {
            int selectedRow = [_tableView selectedRow];
            if (0 < selectedRow) {
                [_tableView selectRow:selectedRow-1 byExtendingSelection:NO];
                [_tableView scrollRowToVisible:selectedRow-1];
            }
            return YES;
        } else if (c == NSDownArrowFunctionKey) {
            int selectedRow = [_tableView selectedRow];
            if (selectedRow < (int)[_completions count]-1) {
                [_tableView selectRow:selectedRow+1 byExtendingSelection:NO];
                [_tableView scrollRowToVisible:selectedRow+1];
            }
            return YES;
        } else if (c == NSRightArrowFunctionKey || c == '\n' || c == '\r' || c == '\t') {
            [self endRevertingChange:NO moveLeft:NO];
            return YES;
        } else if (c == NSLeftArrowFunctionKey) {
            [self endRevertingChange:NO moveLeft:YES];
            return YES;
        } else if (c == 0x1b || c == NSF5FunctionKey) {
            [self endRevertingChange:YES moveLeft:NO];
            return YES;
        } else if (c == ' ' || ispunct(c)) {
            [self endRevertingChange:NO moveLeft:NO];
            return NO;  // let the char get inserted
        }
    }
    return NO;
}

- (void)_reflectSelection
{
    int selectedRow = [_tableView selectedRow];
    ASSERT(selectedRow >= 0 && selectedRow < (int)[_completions count]);
    [self _insertMatch:[_completions objectAtIndex:selectedRow]];
}

- (void)tableAction:(id)sender
{
    [self _reflectSelection];
    [self endRevertingChange:NO moveLeft:NO];
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
    return [_completions count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(int)row
{
    return [_completions objectAtIndex:row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [self _reflectSelection];
}

@end
