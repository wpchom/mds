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
#include "mds_utils.h"

/* SkipList ---------------------------------------------------------------- */
void MDS_SkipListInitNode(MDS_DListNode_t node[], size_t size)
{
    if ((node == NULL) || (size == 0)) {
        return;
    }

    for (size_t level = 0; level < size; level++) {
        MDS_DListInitNode(&(node[level]));
    }
}

void MDS_SkipListRemoveNode(MDS_DListNode_t node[], size_t size)
{
    if ((node == NULL) || (size == 0)) {
        return;
    }

    for (size_t level = 0; level < size; level++) {
        MDS_DListRemoveNode(&(node[level]));
    }
}

MDS_DListNode_t *MDS_SkipListSearchNode(MDS_DListNode_t *prev[], MDS_DListNode_t list[],
                                        size_t size, int (*cmp)(const MDS_DListNode_t *, MDS_Arg_t),
                                        MDS_Arg_t arg)
{
    if ((prev == NULL) || (list == NULL) || (size == 0) || (cmp == NULL)) {
        return (NULL);
    }

    MDS_DListNode_t *skip = list;

    for (size_t level = 0; level < size; level++) {
        for (; skip->next != &(list[level]); skip = skip->next) {
            const MDS_DListNode_t *node = skip->next - level;
            if (cmp(node, arg) > 0) {
                break;
            }
        }
        if (prev != NULL) {
            prev[level] = skip;
        }
        if (level != (size - 1)) {
            skip = skip + 1;
        }
    }

    skip = skip - size + 1;

    return ((skip != &(list[0])) ? (skip) : (NULL));
}

size_t MDS_SkipListInsertNode(MDS_DListNode_t *prev[], MDS_DListNode_t node[], size_t size,
                              size_t rand, size_t shift)
{
    if ((prev == NULL) || (node == NULL) || (size == 0)) {
        return (0);
    }

    size_t level = 1;

    do {
        MDS_DListInsertNodeNext(prev[size - level], &(node[size - level]));
        rand >>= shift;
    } while ((++level <= size) && !(rand & ((1UL << shift) - 1U)));

    return (level - 1);
}

/* Tree -------------------------------------------------------------------- */
void MDS_TreeInitNode(MDS_TreeNode_t *node)
{
    if (node != NULL) {
        node->parent = NULL;
        MDS_DListInitNode(&(node->child));
        MDS_DListInitNode(&(node->sibling));
    }
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
                           void (*func)(const MDS_TreeNode_t *, MDS_Arg_t), MDS_Arg_t arg)
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
