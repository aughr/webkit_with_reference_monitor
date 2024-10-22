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

#include "config.h"
#include "SecurityLabelObject.h"

#include "JSGlobalObject.h"
#include "JSObject.h"
#include "SecurityLabelPrototype.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(SecurityLabelObject);

void SecurityLabelObject::destroy(JSCell* cell)
{
    SecurityLabelObject* thisObject = jsCast<SecurityLabelObject*>(cell);
    thisObject->SecurityLabelObject::~SecurityLabelObject();
}

const ClassInfo SecurityLabelObject::s_info = { "SecurityLabel", 0, 0, 0, CREATE_METHOD_TABLE(SecurityLabelObject) };

SecurityLabel SecurityLabelObject::securityLabelCell(const JSCell* cell)
{
    const SecurityLabelObject* obj = jsCast<const SecurityLabelObject*>(cell);
    return obj->securityLabel();
}

void SecurityLabelObject::mergeSecurityLabelCell(JSCell*, JSGlobalData&, SecurityLabel)
{
    return;
}

} // namespace JSC
