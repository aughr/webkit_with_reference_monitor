/*
 *  Copyright (C) 2012 Andrew Bloomgarden. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SecurityLabelObject_h
#define SecurityLabelObject_h

#include "JSCell.h"
#include "Structure.h"
#include "wtf/SecurityLabel.h"

namespace JSC {

    class Structure;

    class SecurityLabelObject : public JSCell {
    protected:
        SecurityLabelObject(JSGlobalData& globalData)
        : JSCell(globalData, globalData.securityLabelStructure.get())
        {
        }

        void finishCreation(JSGlobalData& globalData)
        {
            Base::finishCreation(globalData);
            ASSERT(inherits(&s_info));
        }
        
    public:
        typedef JSCell Base;
        
        static void destroy(JSCell*);

        static SecurityLabelObject* create(JSGlobalData&, SecurityLabel);
        
        static JS_EXPORTDATA const ClassInfo s_info;
        
        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(SecurityLabelType, OverridesGetOwnPropertySlot), &s_info);
        }
        
        SecurityLabel securityLabel() const { return m_label; }
        
        JS_EXPORT_PRIVATE static SecurityLabel securityLabelCell(const JSCell*);
        JS_EXPORT_PRIVATE static void mergeSecurityLabelCell(JSCell*, JSGlobalData&, SecurityLabel);
    private:
        WTF::SecurityLabel m_label;
    };
    
    inline SecurityLabel JSCell::internalSecurityLabel() const {
        if (m_label)
            return m_label->securityLabel();
        else {
            return SecurityLabel();
        }
    }

} // namespace JSC

#include "SecurityLabelCache.h"

#endif // SecurityLabelObject_h
