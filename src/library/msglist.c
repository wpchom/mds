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
        size_t cnt = MDS_MemBuffCopy((uint8_t *)buff + len, size, cur->buff, cur->len);
        if (cnt == 0) {
            break;
        }
        len += cnt;
        size -= cnt;
    }

    return (len);
}
