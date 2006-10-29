/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <Cocoa/Cocoa.h>

#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebHistoryItem.h>

@interface WebHistoryItem (WebPrivate)
- (void)_retainIconInDatabase:(BOOL)retain;
+ (void)_releaseAllPendingPageCaches;
- (BOOL)hasPageCache;
- (void)setHasPageCache:(BOOL)f;
- (NSMutableDictionary *)pageCache;

+ (WebHistoryItem *)entryWithURL:(NSURL *)URL;

- (id)initWithURL:(NSURL *)URL title:(NSString *)title;
- (id)initWithURL:(NSURL *)URL target:(NSString *)target parent:(NSString *)parent title:(NSString *)title;

- (NSDictionary *)dictionaryRepresentation;
- (id)initFromDictionaryRepresentation:(NSDictionary *)dict;

- (NSString *)parent;
- (NSURL *)URL;
- (NSString *)target;
- (NSPoint)scrollPoint;
- (NSArray *)documentState;
- (BOOL)isTargetItem;
- (NSData *)formData;
- (NSString *)formContentType;
- (NSString *)formReferrer;
- (NSString *)RSSFeedReferrer;
- (int)visitCount;
- (id)viewState;

- (void)_mergeAutoCompleteHints:(WebHistoryItem *)otherItem;

- (void)setURL:(NSURL *)URL;
- (void)setURLString:(NSString *)string;
- (void)setOriginalURLString:(NSString *)URL;
- (void)setTarget:(NSString *)target;
- (void)setParent:(NSString *)parent;
- (void)setTitle:(NSString *)title;
- (void)setScrollPoint:(NSPoint)p;
- (void)setDocumentState:(NSArray *)state;
- (void)setIsTargetItem:(BOOL)flag;
- (void)_setFormInfoFromRequest:(NSURLRequest *)request;
- (void)setRSSFeedReferrer:(NSString *)referrer;
- (void)setVisitCount:(int)count;
- (void)setViewState:(id)statePList;

- (NSArray *)children;
- (void)addChildItem:(WebHistoryItem *)item;
- (WebHistoryItem *)childItemWithName:(NSString *)name;
- (WebHistoryItem *)targetItem;

- (void)setAlwaysAttemptToUsePageCache:(BOOL)flag;
- (BOOL)alwaysAttemptToUsePageCache;

- (NSCalendarDate *)_lastVisitedDate;

// This should not be called directly for WebHistoryItems that are already included
// in WebHistory. Use -[WebHistory setLastVisitedTimeInterval:forItem:] instead.
- (void)_setLastVisitedTimeInterval:(NSTimeInterval)time;

// Transient properties may be of any ObjC type.  They are intended to be used to store state per back/forward list entry.
// The properties will not be persisted; when the history item is removed, the properties will be lost.
- (id)_transientPropertyForKey:(NSString *)key;
- (void)_setTransientProperty:(id)property forKey:(NSString *)key;

@end

@interface WebBackForwardList (WebPrivate)
- (void)_close;
- (BOOL)_usesPageCache;
- (void)_clearPageCache;
@end

