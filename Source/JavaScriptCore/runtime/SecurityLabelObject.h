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

#include "JSObject.h"
#include "wtf/SecurityLabel.h"

namespace JSC {
    
    class SecurityLabelObject : public JSNonFinalObject {
    protected:
        SecurityLabelObject(JSGlobalData&, Structure*);
        void finishCreation(JSGlobalData&);
        
    public:
        typedef JSNonFinalObject Base;
        
        static void destroy(JSCell*);
        
        static SecurityLabelObject* create(JSGlobalData& globalData, Structure* structure)
        {
            SecurityLabelObject* label = new (NotNull, allocateCell<SecurityLabelObject>(globalData.heap)) SecurityLabelObject(globalData, structure);
            label->finishCreation(globalData);
            return label;
        }

        static SecurityLabelObject* create(JSGlobalData& globalData, Structure* structure, SecurityLabel label)
        {
            SecurityLabelObject* labelObj = new (NotNull, allocateCell<SecurityLabelObject>(globalData.heap)) SecurityLabelObject(globalData, structure);
            labelObj->finishCreation(globalData);
            labelObj->m_label = label;
            return labelObj;
        }
        
        static const ClassInfo s_info;
        
        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }
        
        bool isNull() const;
        void add(const WTF::SecurityTag&);
        bool hasTag(const WTF::SecurityTag&) const;
        void merge(const SecurityLabelObject&);
        void merge(const SecurityLabel&);
        SecurityLabel securityLabel() { return m_label; }
    private:
        WTF::SecurityLabel m_label;
    };
    
    SecurityLabelObject* constructSecurityLabel(ExecState*, JSGlobalObject*);
    SecurityLabelObject* constructSecurityLabel(ExecState*, JSGlobalObject*, SecurityLabel);
    
    inline bool SecurityLabelObject::isNull() const {
        return m_label.isNull();
    }
    
    inline void SecurityLabelObject::add(const WTF::SecurityTag& tag) {
        m_label.add(tag);
    }
    
    inline bool SecurityLabelObject::hasTag(const WTF::SecurityTag& tag) const {
        return m_label.hasTag(tag);
    }
    
    inline void SecurityLabelObject::merge(const SecurityLabelObject& other) {
        m_label.merge(other.m_label);
    }
    
    inline void SecurityLabelObject::merge(const SecurityLabel& other) {
        m_label.merge(other);
    }
} // namespace JSC

#endif // SecurityLabelObject_h
