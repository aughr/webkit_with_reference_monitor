// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * This file is part of the KDE libraries
 * Copyright (C) 2005 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA 
 *
 */

#include "config.h"
#include "JSLock.h"

#include "collector.h"
#include "ExecState.h"

#if USE(MULTIPLE_THREADS)
#include <pthread.h>
#endif

namespace KJS {

#if USE(MULTIPLE_THREADS)

// Acquire this mutex before accessing lock-related data.
static pthread_mutex_t JSMutex = PTHREAD_MUTEX_INITIALIZER;

// Thread-specific key that tells whether a thread holds the JSMutex, and how many times it was taken recursively.
pthread_key_t JSLockCount;

static void createJSLockCount()
{
    pthread_key_create(&JSLockCount, 0);
}

pthread_once_t createJSLockCountOnce = PTHREAD_ONCE_INIT;

// Lock nesting count.
ssize_t JSLock::lockCount()
{
    pthread_once(&createJSLockCountOnce, createJSLockCount);

    return reinterpret_cast<ssize_t>(pthread_getspecific(JSLockCount));
}

static void setLockCount(ssize_t count)
{
    ASSERT(count >= 0);
    pthread_setspecific(JSLockCount, reinterpret_cast<void*>(count));
}

JSLock::JSLock(ExecState* exec)
    : m_lockingForReal(exec->globalData().isSharedInstance)
{
    lock(m_lockingForReal);
    if (m_lockingForReal)
        registerThread();
}

void JSLock::lock(bool lockForReal)
{
#ifdef NDEBUG
    // For per-thread contexts, locking is a debug-only feature.
    if (!lockForReal)
        return;
#endif

    pthread_once(&createJSLockCountOnce, createJSLockCount);

    ssize_t currentLockCount = lockCount();
    if (!currentLockCount && lockForReal) {
        int result;
        result = pthread_mutex_lock(&JSMutex);
        ASSERT(!result);
    }
    setLockCount(currentLockCount + 1);
}

void JSLock::unlock(bool lockForReal)
{
    ASSERT(lockCount());

#ifdef NDEBUG
    // For per-thread contexts, locking is a debug-only feature.
    if (!lockForReal)
        return;
#endif

    ssize_t newLockCount = lockCount() - 1;
    setLockCount(newLockCount);
    if (!newLockCount && lockForReal) {
        int result;
        result = pthread_mutex_unlock(&JSMutex);
        ASSERT(!result);
    }
}

void JSLock::lock(ExecState* exec)
{
    lock(exec->globalData().isSharedInstance);
}

void JSLock::unlock(ExecState* exec)
{
    unlock(exec->globalData().isSharedInstance);
}

bool JSLock::currentThreadIsHoldingLock()
{
    pthread_once(&createJSLockCountOnce, createJSLockCount);
    return !!pthread_getspecific(JSLockCount);
}

void JSLock::registerThread()
{
    Heap::registerThread();
}

JSLock::DropAllLocks::DropAllLocks(ExecState* exec)
    : m_lockingForReal(exec->globalData().isSharedInstance)
{
    pthread_once(&createJSLockCountOnce, createJSLockCount);

    m_lockCount = JSLock::lockCount();
    for (ssize_t i = 0; i < m_lockCount; i++)
        JSLock::unlock(m_lockingForReal);
}

JSLock::DropAllLocks::DropAllLocks(bool lockingForReal)
    : m_lockingForReal(lockingForReal)
{
    pthread_once(&createJSLockCountOnce, createJSLockCount);

    // It is necessary to drop even "unreal" locks, because having a non-zero lock count
    // will prevent a real lock from being taken.

    m_lockCount = JSLock::lockCount();
    for (ssize_t i = 0; i < m_lockCount; i++)
        JSLock::unlock(m_lockingForReal);
}

JSLock::DropAllLocks::~DropAllLocks()
{
    for (ssize_t i = 0; i < m_lockCount; i++)
        JSLock::lock(m_lockingForReal);
}

#else

JSLock::JSLock(ExecState* exec)
    : m_lockingForReal(false)
{
}

// If threading support is off, set the lock count to a constant value of 1 so assertions
// that the lock is held don't fail
ssize_t JSLock::lockCount()
{
    return 1;
}

bool JSLock::currentThreadIsHoldingLock()
{
    return true;
}

void JSLock::lock(bool)
{
}

void JSLock::unlock(bool)
{
}

void JSLock::lock(ExecState*)
{
}

void JSLock::unlock(ExecState*)
{
}

void JSLock::registerThread()
{
}

JSLock::DropAllLocks::DropAllLocks(ExecState*)
{
}

JSLock::DropAllLocks::DropAllLocks(bool)
{
}

JSLock::DropAllLocks::~DropAllLocks()
{
}

#endif // USE(MULTIPLE_THREADS)

}
