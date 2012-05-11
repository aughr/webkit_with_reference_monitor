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

#include "config.h"
#include "SecurityEventTarget.h"

#include "DOMWindow.h"
#include "Event.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "InspectorInstrumentation.h"
#include "JSDOMWindow.h"
#include "JSEventListener.h"
#include "SecurityOrigin.h"

using namespace JSC;

namespace WebCore {

const AtomicString& SecurityEventTarget::interfaceName() const
{
    return nullAtom;
}

ScriptExecutionContext* SecurityEventTarget::scriptExecutionContext() const
{
    ASSERT(m_window);
    return m_window->scriptExecutionContext();
}

EventTargetData* SecurityEventTarget::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* SecurityEventTarget::ensureEventTargetData()
{
    return &m_eventTargetData;
}

bool SecurityEventTarget::fireEventListeners(Event* event, Event* concealedEvent)
{
    ASSERT(!eventDispatchForbidden());
    ASSERT(event && !event->type().isEmpty());
    ASSERT(concealedEvent && !concealedEvent->type().isEmpty());
    ASSERT(event->type() == concealedEvent->type());

    EventTargetData* d = eventTargetData();
    if (!d)
        return true;

    EventListenerVector* listenerVector = d->eventListenerMap.find(event->type());

    if (listenerVector)
        fireEventListeners(event, concealedEvent, d, *listenerVector);

    return !(event->defaultPrevented() || concealedEvent->defaultPrevented());
}

void SecurityEventTarget::fireEventListeners(Event* event, Event* concealedEvent, EventTargetData* d, EventListenerVector& entry)
{
    RefPtr<EventTarget> protect = this;

    // Fire all listeners registered for this event. Don't fire listeners removed
    // during event dispatch. Also, don't fire event listeners added during event
    // dispatch. Conveniently, all new event listeners will be added after 'end',
    // so iterating to 'end' naturally excludes new event listeners.

    size_t i = 0;
    size_t end = entry.size();
    d->firingEventIterators.append(FiringEventIterator(event->type(), i, end));
    for ( ; i < end; ++i) {
        RegisteredEventListener& registeredListener = entry[i];

        // If stopImmediatePropagation has been called, we just break out immediately, without
        // handling any more events on this target.
        if (event->immediatePropagationStopped() || concealedEvent->immediatePropagationStopped())
            break;

        ScriptExecutionContext* context = scriptExecutionContext();
        const JSEventListener* jsListener = JSEventListener::cast(registeredListener.listener.get());
        Event* eventToFire = event;
        if (jsListener) {
            JSDOMWindowBase* jsWindow = jsCast<JSDOMWindowBase*>(jsListener->wrapper());
            if (!context->securityOrigin()->canAccess(jsWindow->impl()->securityOrigin()))
                eventToFire = concealedEvent;
        }


        InspectorInstrumentationCookie cookie = InspectorInstrumentation::willHandleEvent(context, eventToFire);
        // To match Mozilla, the AT_TARGET phase fires both capturing and bubbling
        // event listeners, even though that violates some versions of the DOM spec.
        registeredListener.listener->handleEvent(context, eventToFire);
        InspectorInstrumentation::didHandleEvent(cookie);
    }
    d->firingEventIterators.removeLast();
}

void SecurityEventTarget::addListenerTypeIfNeeded(const AtomicString& eventType)
{
    if (eventType == eventNames().checkbeforeloadEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkcookiewriteEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkcopyEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkcutEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkpasteEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkstoragewriteEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkxhropenEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
    else if (eventType == eventNames().checkxhrsendEvent)
        addListenerType(CHECKBEFORELOAD_LISTENER);
}

} // namespace WebCore
