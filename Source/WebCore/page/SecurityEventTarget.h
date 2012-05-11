/*
 * Copyright (C) 2012 Andrew Bloomgarden. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SecurityEventTarget_h
#define SecurityEventTarget_h

#include "EventNames.h"
#include "EventTarget.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class DOMWindow;

    class SecurityEventTarget : public RefCounted<SecurityEventTarget>, public EventTarget {
    public:
        static PassRefPtr<SecurityEventTarget> create() { return adoptRef(new SecurityEventTarget()); }

        // EventTarget impl

        using RefCounted<SecurityEventTarget>::ref;
        using RefCounted<SecurityEventTarget>::deref;

        void setWindow(DOMWindow* window) { m_window = window; }

        virtual const AtomicString& interfaceName() const;
        virtual ScriptExecutionContext* scriptExecutionContext() const;

        bool fireEventListeners(Event* event, Event* concealedEvent);

        // keep track of what types of event listeners are registered, so we don't
        // dispatch events unnecessarily
        enum ListenerType {
            CHECKBEFORELOAD_LISTENER             = 0x01,
            CHECKCOOKIEWRITE_LISTENER            = 0x02,
            CHECKCOPY_LISTENER                   = 0x04,
            CHECKCUT_LISTENER                    = 0x08,
            CHECKPASTE_LISTENER                  = 0x10,
            CHECKSTORAGEWRITE_LISTENER           = 0x20,
            CHECKXHROPEN_LISTENER                = 0x40,
            CHECKXHRPASTE_LISTENER               = 0x80
        };

        bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
        void addListenerType(ListenerType listenerType) { m_listenerTypes = m_listenerTypes | listenerType; }
        void addListenerTypeIfNeeded(const AtomicString& eventType);

    private:
        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        void fireEventListeners(Event* event, Event* concealedEvent, EventTargetData* d, EventListenerVector& entry);

        EventTargetData m_eventTargetData;
        DOMWindow* m_window;

        unsigned short m_listenerTypes;
    };

} // namespace WebCore

#endif // SecurityEventTarget_h
