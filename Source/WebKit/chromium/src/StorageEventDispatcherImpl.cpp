/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StorageEventDispatcherImpl.h"

#include "DOMWindow.h"
#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "KURL.h"
#include "Page.h"
#include "PageGroup.h"
#include "SecurityOrigin.h"
#include "StorageEvent.h"

namespace WebCore {

StorageEventDispatcherImpl::StorageEventDispatcherImpl(const String& groupName)
    : m_pageGroup(PageGroup::pageGroup(groupName))
{
    ASSERT(m_pageGroup);
}

// FIXME: add a sourceStorageArea parameter to this
void StorageEventDispatcherImpl::dispatchStorageEvent(const String& key, const String& oldValue,
                                                      const String& newValue, SecurityOrigin* securityOrigin,
                                                      const KURL& url, StorageType storageType)
{
    // FIXME: Implement
    if (storageType == SessionStorage)
        return;

    // We need to copy all relevant frames from every page to a vector since sending the event to one frame might mutate the frame tree
    // of any given page in the group or mutate the page group itself.
    Vector<RefPtr<Frame> > frames;

    const HashSet<Page*>& pages = m_pageGroup->pages();
    HashSet<Page*>::const_iterator end = pages.end();
    for (HashSet<Page*>::const_iterator it = pages.begin(); it != end; ++it) {
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
            // FIXME: identify the srcFrame while in this loop too and exclude it from 'frames'.
            if (frame->document()->securityOrigin()->equal(securityOrigin))
                frames.append(frame);
        }
    }

    for (unsigned i = 0; i < frames.size(); ++i) {
        ExceptionCode ec = 0;
        Storage* storage = frames[i]->domWindow()->localStorage(ec);
        if (!ec)
            frames[i]->document()->dispatchWindowEvent(StorageEvent::create(eventNames().storageEvent, key, oldValue, newValue, url, storage));
    }
}

} // namespace WebCore
