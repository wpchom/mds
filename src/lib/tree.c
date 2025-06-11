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
    MDS_LIST_FOREACH_NEXT (iter, sibling, &(tree->child)) {
        cnt += MDS_TreeForeachNode(iter, func, arg);
    }

    return (cnt);
}
