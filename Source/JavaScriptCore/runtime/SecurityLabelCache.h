/*
 * Copyright (C) 2012 Andrew Bloomgarden. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SecurityLabelCache_h
#define SecurityLabelCache_h

#include <wtf/text/StringImpl.h>
#include "SecurityLabelObject.h"
#include "WeakGCMap.h"

namespace JSC {

class SecurityLabelCache : public WeakGCMap<StringImpl*, SecurityLabelObject>
{
};

inline SecurityLabelObject* SecurityLabelObject::create(JSGlobalData& globalData, SecurityLabel label)
{
    SecurityLabelObject* labelObj = new (NotNull, allocateCell<SecurityLabelObject>(globalData.heap)) SecurityLabelObject(globalData);
    labelObj->finishCreation(globalData);
    labelObj->m_label = label;
    globalData.securityLabelCache->add(globalData, label.descriptor(), labelObj);
    return labelObj;
}

inline SecurityLabelObject* lookupOrConstructSecurityLabel(JSGlobalData& globalData, SecurityLabel label) {
    SecurityLabelObject* object = globalData.securityLabelCache->get(label.descriptor());
    if (object)
        return object;
    else
        return SecurityLabelObject::create(globalData, label);
}

} // namespace JSC

#endif
