/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

#include "Notification.h"

#include "Document.h"
#include "ErrorEvent.h"
#include "EventNames.h"
#include "NotificationCenter.h"
#include "NotificationClient.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ThreadableLoader.h"
#include "WorkerContext.h"

namespace WebCore {

Notification::Notification()
    : ActiveDOMObject(0, this)
{
}

Notification::Notification(const KURL& url, ScriptExecutionContext* context, ExceptionCode& ec, PassRefPtr<NotificationCenter> provider)
    : ActiveDOMObject(context, this)
    , m_isHTML(true)
    , m_state(Idle)
    , m_notificationCenter(provider)
{
    if (m_notificationCenter->checkPermission() != NotificationClient::PermissionAllowed) {
        ec = SECURITY_ERR;
        return;
    }

    if (url.isEmpty() || !url.isValid()) {
        ec = SYNTAX_ERR;
        return;
    }

    m_notificationURL = url;
}

Notification::Notification(const String& title, const String& body, const String& iconURI, ScriptExecutionContext* context, ExceptionCode& ec, PassRefPtr<NotificationCenter> provider)
    : ActiveDOMObject(context, this)
    , m_isHTML(false)
    , m_title(title)
    , m_body(body)
    , m_state(Idle)
    , m_notificationCenter(provider)
{
    if (m_notificationCenter->checkPermission() != NotificationClient::PermissionAllowed) {
        ec = SECURITY_ERR;
        return;
    }

    m_icon = iconURI.isEmpty() ? KURL() : scriptExecutionContext()->completeURL(iconURI);
    if (!m_icon.isEmpty() && !m_icon.isValid()) {
        ec = SYNTAX_ERR;
        return;
    }
}

Notification::~Notification() 
{
    if (m_state == Loading) {
        ASSERT_NOT_REACHED();
        close();
    }
}

PassRefPtr<Notification> Notification::create(const KURL& url, ScriptExecutionContext* context, ExceptionCode& ec, PassRefPtr<NotificationCenter> provider) 
{ 
    RefPtr<Notification> notification(adoptRef(new Notification(url, context, ec, provider)));
    notification->suspendIfNeeded();
    return notification.release();
}

PassRefPtr<Notification> Notification::create(const String& title, const String& body, const String& iconURI, ScriptExecutionContext* context, ExceptionCode& ec, PassRefPtr<NotificationCenter> provider) 
{ 
    RefPtr<Notification> notification(adoptRef(new Notification(title, body, iconURI, context, ec, provider)));
    notification->suspendIfNeeded();
    return notification.release();
}

const AtomicString& Notification::interfaceName() const
{
    return eventNames().interfaceForNotification;
}

void Notification::show() 
{
#if PLATFORM(QT)
    if (iconURL().isEmpty()) {
        // Set the state before actually showing, because
        // handling of ondisplay may rely on that.
        if (m_state == Idle) {
            m_state = Showing;
            if (m_notificationCenter->client())
                m_notificationCenter->client()->show(this);
        }
    } else
        startLoading();
#elif PLATFORM(MAC)
    if (m_state == Idle && m_notificationCenter->client()) {
        m_notificationCenter->client()->show(this);
        m_state = Showing;
    }
#else
    // prevent double-showing
    if (m_state == Idle && m_notificationCenter->client() && m_notificationCenter->client()->show(this))
        m_state = Showing;
#endif
}

void Notification::close()
{
    switch (m_state) {
    case Idle:
        break;
    case Loading:
        m_state = Cancelled;
        stopLoading();
        break;
    case Showing:
        if (m_notificationCenter->client())
            m_notificationCenter->client()->cancel(this);
        break;
    case Cancelled:
        break;
    }
}

EventTargetData* Notification::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* Notification::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void Notification::contextDestroyed()
{
    ActiveDOMObject::contextDestroyed();
    if (m_notificationCenter->client())
        m_notificationCenter->client()->notificationObjectDestroyed(this);
}

void Notification::startLoading()
{
    if (m_state != Idle)
        return;
    setPendingActivity(this);
    m_state = Loading;
    ThreadableLoaderOptions options;
    options.sendLoadCallbacks = DoNotSendCallbacks;
    options.sniffContent = DoNotSniffContent;
    options.preflightPolicy = ConsiderPreflight;
    options.allowCredentials = AllowStoredCredentials;
    options.crossOriginRequestPolicy = AllowCrossOriginRequests;
    m_loader = ThreadableLoader::create(scriptExecutionContext(), this, ResourceRequest(iconURL()), options);
}

void Notification::stopLoading()
{
    m_iconData = 0;
    RefPtr<ThreadableLoader> protect(m_loader);
    m_loader->cancel();
}

void Notification::didReceiveResponse(unsigned long, const ResourceResponse& response)
{
    int status = response.httpStatusCode();
    if (status && (status < 200 || status > 299)) {
        stopLoading();
        return;
    }
    m_iconData = SharedBuffer::create();
}

void Notification::didReceiveData(const char* data, int dataLength)
{
    m_iconData->append(data, dataLength);
}

void Notification::didFinishLoading(unsigned long, double)
{
    finishLoading();
}

void Notification::didFail(const ResourceError&)
{
    finishLoading();
}

void Notification::didFailRedirectCheck()
{
    finishLoading();
}

void Notification::finishLoading()
{
    if (m_state == Loading) {
        if (m_notificationCenter->client() && m_notificationCenter->client()->show(this))
            m_state = Showing;
    }
    unsetPendingActivity(this);
}

void Notification::dispatchShowEvent()
{
    dispatchEvent(Event::create(eventNames().showEvent, false, false));
}

void Notification::dispatchClickEvent()
{
    dispatchEvent(Event::create(eventNames().clickEvent, false, false));
}

void Notification::dispatchCloseEvent()
{
    dispatchEvent(Event::create(eventNames().closeEvent, false, false));
}

void Notification::dispatchErrorEvent()
{
    dispatchEvent(ErrorEvent::create());
}

} // namespace WebCore

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
