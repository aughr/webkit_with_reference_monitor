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

#ifndef SecurityTagObject_h
#define SecurityTagObject_h

#include "JSObject.h"

namespace JSC {
    
    class SecurityTagObject : public JSNonFinalObject {
    protected:
        SecurityTagObject(JSGlobalData&, Structure*, WTF::SecurityTag=WTF::SecurityTag());
        void finishCreation(JSGlobalData&);
        
    public:
        typedef JSNonFinalObject Base;
        
        static SecurityTagObject* create(JSGlobalData& globalData, Structure* structure, WTF::SecurityTag tag=WTF::SecurityTag())
        {
            SecurityTagObject* tagObject = new (NotNull, allocateCell<SecurityTagObject>(globalData.heap)) SecurityTagObject(globalData, structure, tag);
            tagObject->finishCreation(globalData);
            return tagObject;
        }
        
        static JS_EXPORTDATA const ClassInfo s_info;
        
        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::freezeTransition(globalData, Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info));
        }
        
        WTF::SecurityTag tag() const { return m_tag; };
        SecurityLabel labelForTag() const { return m_labelForTag; };
    private:
        WTF::SecurityTag m_tag;
        SecurityLabel m_labelForTag;
    };
    
    JS_EXPORT_PRIVATE SecurityTagObject* constructSecurityTag(ExecState*, JSGlobalObject*);
    JS_EXPORT_PRIVATE SecurityTagObject* constructSecurityTag(ExecState*, JSGlobalObject*, WTF::SecurityTag);
    
    SecurityTagObject* asSecurityTagObject(JSValue);
    
    inline SecurityTagObject* asSecurityTagObject(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&SecurityTagObject::s_info));
        return static_cast<SecurityTagObject*>(asObject(value));
    }
    
} // namespace JSC

#endif // SecurityTagObject_h
