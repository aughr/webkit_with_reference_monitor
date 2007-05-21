/*
  Copyright (C) 2007 Trolltech ASA
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.
  
  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
  
  This class provides all functionality needed for loading images, style sheets and html
  pages from the web. It has a memory cache for these objects.
*/
#ifndef QWEBNETWORKINTERFACE_H
#define QWEBNETWORKINTERFACE_H

#include <qobject.h>
#include <qurl.h>
#include <qhttp.h>
#include <qbytearray.h>

class QWebNetworkJobPrivate;

class QWebNetworkJob
{
public:
    QUrl url() const;
    QByteArray postData() const;
    QHttpRequestHeader request() const;

    QHttpResponseHeader response() const;
    void setResponse(const QHttpResponseHeader &response);

    bool cancelled() const;

    void ref();
    bool deref();
    
    template <typename T>
    void setHandle(T *t);
    template <typename T>
    T *handle() const;

private:
    QWebNetworkJob();
    ~QWebNetworkJob();

    void setUserHandle(void *);
    void *userHandle() const;

    friend class QWebNetworkManager;
    QWebNetworkJobPrivate *d;
};

template <typename T>
void QWebNetworkJob::setHandle(T *t)
{
    setUserHandle(t);
}

template <typename T>
T *QWebNetworkJob::handle() const
{
    return static_cast<T *>(userHandle());
}

class QWebNetworkInterfacePrivate;

class QWebNetworkInterface : public QObject
{
    Q_OBJECT
public:
    QWebNetworkInterface();
    ~QWebNetworkInterface();

    static void setDefaultInterface(QWebNetworkInterface *defaultInterface);
    static QWebNetworkInterface *defaultInterface();

    virtual void addJob(QWebNetworkJob *job);
    virtual void cancelJob(QWebNetworkJob *job);
    
signals:
    void started(QWebNetworkJob*);
    void data(QWebNetworkJob*, const QByteArray &data);
    void finished(QWebNetworkJob*, int errorCode);

private:
    friend class QWebNetworkInterfacePrivate;
    QWebNetworkInterfacePrivate *d;
};

#endif
