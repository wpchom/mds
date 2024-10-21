/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"

/* Define ------------------------------------------------------------------ */
#define OBJECT_FLAG_TYPEMASK  0x7FU
#define OBJECT_FLAG_CREATED   0x80U
#define OBJECT_LIST_INIT(obj) [obj] = {.prev = &(g_objectList[obj]), .next = &(g_objectList[obj])}

/* Variable ---------------------------------------------------------------- */
static MDS_ListNode_t g_objectList[] = {
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_DEVICE),
#ifndef MDS_TIMER_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_TIMER),
#endif
#ifndef MDS_THREAD_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_THREAD),
#endif
#ifndef MDS_SEMAPHORE_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_SEMAPHORE),
#endif
#ifndef MDS_MUTEX_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MUTEX),
#endif
#ifndef MDS_EVENT_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_EVENT),
#endif
#ifndef MDS_MSGQUEUE_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MSGQUEUE),
#endif
#ifndef MDS_MEMPOOL_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MEMPOOL),
#endif
#ifndef MDS_MEMHEAP_TYPE
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MEMHEAP),
#endif
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_ObjectInit(MDS_Object_t *object, MDS_ObjectType_t type, const char *name)
{
    MDS_ASSERT(object != NULL);
    MDS_ASSERT(type < ARRAY_SIZE(g_objectList));

    if (object->flags != MDS_OBJECT_TYPE_NONE) {
        return (MDS_EAGAIN);
    }

    MDS_ListInitNode(&(object->node));
    if (name != NULL) {
        MDS_MemBuffCcpy(object->name, name, '\0', sizeof(object->name));
    }
    object->flags = type;

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_ListInsertNodePrev(&(g_objectList[type]), &(object->node));
    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_ObjectDeInit(MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_ListRemoveNode(&(object->node));
    object->flags = MDS_OBJECT_TYPE_NONE;
    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Object_t *MDS_ObjectCreate(size_t typesz, MDS_ObjectType_t type, const char *name)
{
    MDS_Object_t *object = MDS_SysMemCalloc(1, typesz);

    if (object != NULL) {
        MDS_ObjectInit(object, type, name);
        object->flags |= OBJECT_FLAG_CREATED;
    }

    return (object);
}

MDS_Err_t MDS_ObjectDestory(MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);
    MDS_ASSERT(MDS_ObjectIsCreated(object));

    if (!MDS_ObjectIsCreated(object)) {
        return (MDS_EFAULT);
    }

    MDS_ObjectDeInit(object);
    MDS_SysMemFree(object);

    return (MDS_EOK);
}

MDS_Object_t *MDS_ObjectFind(const MDS_ObjectType_t type, const char *name)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    MDS_Object_t *iter = NULL;

    if ((name != NULL) && (name[0] != '\0')) {
        MDS_LIST_FOREACH_NEXT (iter, node, &(g_objectList[type])) {
            if (strncmp(iter->name, name, sizeof(iter->name)) == 0) {
                return (iter);
            }
        }
    }

    return (NULL);
}

MDS_Object_t *MDS_ObjectPrev(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    MDS_ObjectType_t type = MDS_ObjectGetType(object);
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    if (object->node.prev != &(g_objectList[type])) {
        return (CONTAINER_OF(object->node.prev, MDS_Object_t, node));
    } else {
        return (NULL);
    }
}

MDS_Object_t *MDS_ObjectNext(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    MDS_ObjectType_t type = MDS_ObjectGetType(object);
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    if (object->node.next != &(g_objectList[type])) {
        return (CONTAINER_OF(object->node.next, MDS_Object_t, node));
    } else {
        return (NULL);
    }
}

const MDS_ListNode_t *MDS_ObjectGetList(MDS_ObjectType_t type)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    return (&(g_objectList[type]));
}

size_t MDS_ObjectGetCount(MDS_ObjectType_t type)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    return (MDS_ListGetLength(&(g_objectList[type])));
}

const char *MDS_ObjectGetName(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    return (object->name);
}

MDS_ObjectType_t MDS_ObjectGetType(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    return (object->flags & OBJECT_FLAG_TYPEMASK);
}

bool MDS_ObjectIsCreated(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    return ((object->flags & OBJECT_FLAG_CREATED) != 0);
}
