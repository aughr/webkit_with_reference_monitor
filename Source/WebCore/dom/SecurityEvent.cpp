/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "SecurityEvent.h"

#include "DOMWindow.h"
#include "EventNames.h"

namespace WebCore {

SecurityEventInit::SecurityEventInit()
{
}

SecurityEvent::SecurityEvent()
: Event(AtomicString(), false, true)
{
}
    
SecurityEvent::SecurityEvent(const AtomicString& type, const SecurityEventInit& initializer)
    : Event(type, false, true)
    , m_label(initializer.securityLabel)
    , m_origin(initializer.origin)
    , m_destination(initializer.destination)
    , m_source(initializer.source)
{
}

SecurityEvent::SecurityEvent(const AtomicString& type, SecurityLabel label, const String& origin, const String& destination, PassRefPtr<DOMWindow> source)
    : Event(type, false, true)
    , m_label(label)
    , m_origin(origin)
    , m_destination(destination)
    , m_source(source)
{
}

SecurityEvent::~SecurityEvent()
{
}

    
void SecurityEvent::initSecurityEvent(const AtomicString& type, SecurityLabel label, const String& origin, const String& destination, DOMWindow* source)
{
    if (dispatched())
        return;

    initEvent(type, false, true);

    m_label = label;
    m_origin = origin;
    m_destination = destination;
    m_source = source;
}


const AtomicString& SecurityEvent::interfaceName() const
{
    return eventNames().interfaceForSecurityEvent;
}
} // namespace WebCore
