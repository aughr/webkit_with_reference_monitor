/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef SecurityEvent_h
#define SecurityEvent_h

#include "DOMWindow.h"
#include "Event.h"

namespace WebCore {

class DOMWindow;

struct SecurityEventInit : public EventInit {
    SecurityEventInit();

    SecurityLabel securityLabel;
    String destination;
    RefPtr<DOMWindow> source;
};

class SecurityEvent : public Event {
public:
    static PassRefPtr<SecurityEvent> create()
    {
        return adoptRef(new SecurityEvent());
    }
    static PassRefPtr<SecurityEvent> create(const AtomicString& type, SecurityLabel label, const String& destination = String(), PassRefPtr<DOMWindow> source = 0)
    {
        return adoptRef(new SecurityEvent(type, label, destination, source));
    }
    static PassRefPtr<SecurityEvent> create(const AtomicString& type, const SecurityEventInit& initializer)
    {
        return adoptRef(new SecurityEvent(type, initializer));
    }
    virtual ~SecurityEvent();

    void initSecurityEvent(const AtomicString& type, SecurityLabel data, const String& destination, DOMWindow* source);

    const String& destination() const { return m_destination; }
    DOMWindow* source() const { return m_source.get(); }

    SecurityLabel securityLabel() const { return m_label; }
    
    virtual const AtomicString& interfaceName() const;

private:
    SecurityEvent();
    SecurityEvent(const AtomicString&, const SecurityEventInit&);
    SecurityEvent(const AtomicString& type, SecurityLabel label, const String& destination, PassRefPtr<DOMWindow> source);

    SecurityLabel m_label;
    String m_destination;
    RefPtr<DOMWindow> m_source;
};

} // namespace WebCore

#endif // SecurityEvent_h
