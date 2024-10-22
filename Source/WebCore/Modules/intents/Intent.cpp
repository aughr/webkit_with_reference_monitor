/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Google, Inc. ("Google") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#include "config.h"
#include "Intent.h"

#if ENABLE(WEB_INTENTS)

#include "ExceptionCode.h"
#include "MessagePort.h"
#include "SerializedScriptValue.h"

namespace WebCore {

PassRefPtr<Intent> Intent::create(const String& action, const String& type, PassRefPtr<SerializedScriptValue> data, ExceptionCode& ec)
{
    if (action.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }
    if (type.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    return adoptRef(new Intent(action, type, data));
}

PassRefPtr<Intent> Intent::create(const String& action, const String& type, PassRefPtr<SerializedScriptValue> data, const MessagePortArray& ports, ExceptionCode& ec)
{
    if (action.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }
    if (type.isEmpty()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    OwnPtr<MessagePortChannelArray> channels = MessagePort::disentanglePorts(&ports, ec);

    return adoptRef(new Intent(action, type, data, channels.release()));
}

Intent::Intent(const String& action, const String& type, PassRefPtr<SerializedScriptValue> data)
    : m_action(action)
    , m_type(type)
{
    if (data)
        m_data = data;
    else
        m_data = SerializedScriptValue::nullValue();
}

Intent::Intent(const String& action, const String& type, PassRefPtr<SerializedScriptValue> data, PassOwnPtr<MessagePortChannelArray> ports)
    : m_action(action)
    , m_type(type)
    , m_ports(ports)
{
    if (data)
        m_data = data;
    else
        m_data = SerializedScriptValue::nullValue();
}

const String& Intent::action() const
{
    return m_action;
}

const String& Intent::type() const
{
    return m_type;
}

SerializedScriptValue* Intent::data() const
{
    return m_data.get();
}

MessagePortChannelArray* Intent::messagePorts() const
{
    return m_ports.get();
}

} // namespace WebCore

#endif // ENABLE(WEB_INTENTS)
