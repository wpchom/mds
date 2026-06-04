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
#include "mds/sys.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Variable ---------------------------------------------------------------- */
#define OBJECT_LIST_INIT(obj) [obj] = {.list = MDS_DLIST_INIT(g_objectList[obj].list)}
static MDS_ObjectInfo_t g_objectList[] = {
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_DEVICE),    //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_THREAD),    //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_WORKQUEUE), //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_WORKNODE),  //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_SEMAPHORE), //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MUTEX),     //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_EVENT),     //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MSGQUEUE),  //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MEMPOOL),   //
    OBJECT_LIST_INIT(MDS_OBJECT_TYPE_MEMHEAP),   //
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_ObjectInit(MDS_Object_t *object, MDS_ObjectType_t type, const char *name)
{
    MDS_ASSERT(object != NULL);
    MDS_ASSERT(type < ARRAY_SIZE(g_objectList));

    if (object->type != MDS_OBJECT_TYPE_NONE) {
        MDS_LOG_W("[object] init a already object:%p type:%u", object, object->type);
        return (MDS_EAGAIN);
    }

    MDS_DListInitNode(&(object->node));
    object->type = type;
    strlcpy(object->name, name, sizeof(object->name));

    MDS_Lock_t lock = MDS_CriticalLock(&(g_objectList[type].spinlock));
    MDS_DListInsertNodePrev(&(g_objectList[type].list), &(object->node));
    MDS_CriticalRestore(&(g_objectList[type].spinlock), lock);

    MDS_LOG_D("[object] init a object:%p type:%u created:%u", object, object->type,
              object->created);

    return (MDS_EOK);
}

MDS_Err_t MDS_ObjectDeInit(MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    if (MDS_ObjectIsCreated(object)) {
        MDS_LOG_W("[object] deinit a not static object:%p type:%u", object, object->type);
        return (MDS_EPERM);
    }

    MDS_LOG_D("[object] deinit a object:%p type:%u created:%u", object, object->type,
              object->created);

    MDS_Lock_t lock = MDS_CriticalLock(&(g_objectList[object->type].spinlock));
    MDS_DListRemoveNode(&(object->node));
    object->type = MDS_OBJECT_TYPE_NONE;
    MDS_CriticalRestore(&(g_objectList[object->type].spinlock), lock);

    return (MDS_EOK);
}

MDS_Object_t *MDS_ObjectCreate(size_t typesz, MDS_ObjectType_t type, const char *name)
{
    MDS_Object_t *object = MDS_SysMemCalloc(1, typesz);

    if (object != NULL) {
        object->created = true;
        MDS_ObjectInit(object, type, name);
    }

    return (object);
}

MDS_Err_t MDS_ObjectDestroy(MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);
    MDS_ASSERT(MDS_ObjectIsCreated(object));

    if (!MDS_ObjectIsCreated(object)) {
        MDS_LOG_W("[object] destroy a not dynamic object:%p type:%u", object, object->type);
        return (MDS_EPERM);
    }

    MDS_ObjectDeInit(object);
    MDS_SysMemFree(object);

    return (MDS_EOK);
}

MDS_Object_t *MDS_ObjectFind(const MDS_ObjectType_t type, const char *name)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));
    MDS_ASSERT(strlen(name) < sizeof(((MDS_Object_t *)0)->name));

    if ((name == NULL) || (name[0] == '\0')) {
        return (NULL);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(g_objectList[type].spinlock));

    MDS_Object_t *find = NULL;
    MDS_Object_t *iter = NULL;
    MDS_DLIST_CONTAINER_FOREACH_PREV (iter, node, &(g_objectList[type].list)) {
        if (strncmp(iter->name, name, sizeof(find->name)) == 0) {
            find = iter;
            break;
        }
    }

    MDS_CriticalRestore(&(g_objectList[type].spinlock), lock);

    return (find);
}

MDS_ObjectInfo_t *MDS_ObjectGetInfo(MDS_ObjectType_t type)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    return (&(g_objectList[type]));
}

size_t MDS_ObjectGetCount(MDS_ObjectType_t type)
{
    MDS_ASSERT((type != MDS_OBJECT_TYPE_NONE) && (type < ARRAY_SIZE(g_objectList)));

    return (MDS_DListGetCount(&(g_objectList[type].list)));
}

MDS_String_t MDS_ObjectGetName(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    MDS_String_t str = {
        .str = object->name,
        .len = strnlen(object->name, CONFIG_MDS_OBJECT_NAME_SIZE),
    };

    return (str);
}

MDS_ObjectType_t MDS_ObjectGetType(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    return (object->type);
}

bool MDS_ObjectIsCreated(const MDS_Object_t *object)
{
    MDS_ASSERT(object != NULL);

    return (object->created);
}
