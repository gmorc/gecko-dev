/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
* vim: set ts=8 sts=4 et sw=4 tw=99:
*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gc/Barrier.h"
#include "jsapi-tests/tests.h"

using namespace JS;
using namespace js;

struct AutoIgnoreRootingHazards {
    // Force a nontrivial destructor so the compiler sees the whole RAII scope
    static volatile int depth;
    AutoIgnoreRootingHazards() { depth++; }
    ~AutoIgnoreRootingHazards() { depth--; }
};
volatile int AutoIgnoreRootingHazards::depth = 0;

BEGIN_TEST(testGCStoreBufferRemoval)
{
    // Sanity check - objects start in the nursery and then become tenured.
    JS_GC(cx->runtime());
    JS::RootedObject obj(cx, NurseryObject());
    CHECK(js::gc::IsInsideNursery(obj.get()));
    JS_GC(cx->runtime());
    CHECK(!js::gc::IsInsideNursery(obj.get()));
    JS::RootedObject tenuredObject(cx, obj);

    // Hide the horrors herein from the static rooting analysis.
    AutoIgnoreRootingHazards ignore;

    // Test removal of store buffer entries added by HeapPtr<T>.
    {
        JSObject* badObject = reinterpret_cast<JSObject*>(1);
        JSObject* punnedPtr = nullptr;
        HeapPtrObject* relocPtr =
            reinterpret_cast<HeapPtrObject*>(&punnedPtr);
        new (relocPtr) HeapPtrObject;
        *relocPtr = NurseryObject();
        relocPtr->~HeapPtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (relocPtr) HeapPtrObject;
        *relocPtr = NurseryObject();
        *relocPtr = tenuredObject;
        relocPtr->~HeapPtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (relocPtr) HeapPtrObject;
        *relocPtr = NurseryObject();
        *relocPtr = nullptr;
        relocPtr->~HeapPtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());
    }

    // Test removal of store buffer entries added by HeapValue.
    {
        Value punnedValue;
        HeapValue* relocValue = reinterpret_cast<HeapValue*>(&punnedValue);
        new (relocValue) HeapValue;
        *relocValue = ObjectValue(*NurseryObject());
        relocValue->~HeapValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());

        new (relocValue) HeapValue;
        *relocValue = ObjectValue(*NurseryObject());
        *relocValue = ObjectValue(*tenuredObject);
        relocValue->~HeapValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());

        new (relocValue) HeapValue;
        *relocValue = ObjectValue(*NurseryObject());
        *relocValue = NullValue();
        relocValue->~HeapValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());
    }

    // Test removal of store buffer entries added by Heap<T>.
    {
        JSObject* badObject = reinterpret_cast<JSObject*>(1);
        JSObject* punnedPtr = nullptr;
        Heap<JSObject*>* heapPtr =
            reinterpret_cast<Heap<JSObject*>*>(&punnedPtr);
        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        *heapPtr = tenuredObject;
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        *heapPtr = nullptr;
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());
    }

    return true;
}

JSObject* NurseryObject()
{
    return JS_NewPlainObject(cx);
}
END_TEST(testGCStoreBufferRemoval)
