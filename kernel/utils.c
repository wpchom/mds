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
#include "mds_def.h"
#include "mds_log.h"

/* Skip List --------------------------------------------------------------- */
void MDS_SkipListInitNode(MDS_DListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);

    for (size_t level = 0; level < size; level++) {
        MDS_DListInitNode(&(node[level]));
    }
}

void MDS_SkipListRemoveNode(MDS_DListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);

    for (size_t level = 0; level < size; level++) {
        MDS_DListRemoveNode(&(node[level]));
    }
}

bool MDS_SkipListIsEmpty(MDS_DListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);

    if (size > 0) {
        return (MDS_DListIsEmpty(&(node[0])));
    } else {
        return (true);
    }
}

MDS_DListNode_t *MDS_SkipListSearchNode(MDS_DListNode_t *last[], MDS_DListNode_t list[],
                                        size_t size, const void *value,
                                        int (*compare)(const MDS_DListNode_t *node,
                                                       const void *value))
{
    MDS_ASSERT(last != NULL);
    MDS_ASSERT(list != NULL);
    MDS_ASSERT(size > 0);
    MDS_ASSERT(compare != NULL);

    MDS_DListNode_t *skip = list;

    for (size_t level = 0; level < size; level++) {
        for (; skip->next != &(list[level]); skip = skip->next) {
            const MDS_DListNode_t *node = skip->next - level;
            if (compare(node, value) > 0) {
                break;
            }
        }
        if (last != NULL) {
            last[level] = skip;
        }
        if (level != (size - 1)) {
            skip = skip + 1;
        }
    }

    skip = skip - size + 1;

    return ((skip != &(list[0])) ? (skip) : (NULL));
}

size_t MDS_SkipListInsertNode(MDS_DListNode_t *last[], MDS_DListNode_t node[], size_t size,
                              size_t rand, size_t shift)
{
    MDS_ASSERT(last != NULL);
    MDS_ASSERT(node != NULL);
    MDS_ASSERT(size > 0);

    size_t level = 1;

    do {
        MDS_DListInsertNodeNext(last[size - level], &(node[size - level]));
        rand >>= shift;
    } while ((++level <= size) && !(rand & ((1UL << shift) - 1U)));

    return (level - 1);
}

/* Tree -------------------------------------------------------------------- */
void MDS_TreeInitNode(MDS_TreeNode_t *node)
{
    MDS_ASSERT(node != NULL);

    node->parent = NULL;
    MDS_DListInitNode(&(node->child));
    MDS_DListInitNode(&(node->sibling));
}

MDS_TreeNode_t *MDS_TreeInsertNode(MDS_TreeNode_t *parent, MDS_TreeNode_t *node)
{
    MDS_TreeNode_t *tree = parent;

    if (tree == NULL) {
        tree = node;
    } else if (node != NULL) {
        MDS_DListInsertNodeNext(&(tree->child), &(node->sibling));
        node->parent = tree;
    }

    return (tree);
}

MDS_TreeNode_t *MDS_TreeRemoveNode(MDS_TreeNode_t *node)
{
    if ((node == NULL) || (!MDS_DListIsEmpty(&(node->child)))) {
        return (NULL);
    }

    MDS_DListRemoveNode(&(node->sibling));
    node->parent = NULL;

    return (node);
}

size_t MDS_TreeForeachNode(const MDS_TreeNode_t *tree,
                           void (*func)(const MDS_TreeNode_t *, MDS_Arg_t *), MDS_Arg_t *arg)
{
    if (tree == NULL) {
        return (0);
    }

    if (func != NULL) {
        func(tree, arg);
    }

    size_t cnt = 1;

    MDS_TreeNode_t *iter = NULL;
    MDS_DLIST_CONTAINER_FOREACH_NEXT (iter, sibling, &(tree->child)) {
        cnt += MDS_TreeForeachNode(iter, func, arg);
    }

    return (cnt);
}

/* Message ----------------------------------------------------------------- */

size_t MDS_MsgListGetLength(const MDS_MsgList_t *msg)
{
    size_t len = 0;
    const MDS_MsgList_t *cur = msg;

    while (cur != NULL) {
        len += cur->len;
        cur = cur->next;
    }

    return (len);
}

size_t MDS_MsgListCopyBuff(void *buff, size_t size, const MDS_MsgList_t *msg)
{
    MDS_ASSERT(buff != NULL);

    size_t len = 0;

    for (const MDS_MsgList_t *cur = msg; cur != NULL; cur = cur->next) {
        size_t cnt = (cur->len <= size) ? (cur->len) : (size);
        if (cnt == 0) {
            break;
        }
        memcpy((uint8_t *)buff + len, cur->buff, cnt);
        len += cnt;
        size -= cnt;
    }

    return (len);
}
