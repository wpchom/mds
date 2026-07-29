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
#include "mds/topic/topic.h"

/* Function ---------------------------------------------------------------- */
void MDS_TOPIC_PublisherInit(MDS_TOPIC_Pub_t *topicPub)
{
    MDS_ASSERT(topicPub != NULL);

    MDS_DListInitNode(&(topicPub->list));
}

MDS_Err_t MDS_TOPIC_PublishMessage(MDS_TOPIC_Pub_t *topicPub, const void *msg, size_t len)
{
    MDS_ASSERT(topicPub != NULL);
    MDS_ASSERT(msg != NULL);

    MDS_TOPIC_Sub_t *iter = NULL;

    MDS_DLIST_CONTAINER_FOREACH_NEXT (iter, node, &(topicPub->list)) {
        if (iter->callback != NULL) {
            iter->callback(msg, len);
        }
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_TOPIC_SubscriberRegister(MDS_TOPIC_Sub_t *topicSub, MDS_TOPIC_Pub_t *topicPub)
{
    MDS_ASSERT(topicSub != NULL);
    MDS_ASSERT(topicPub != NULL);

    if (!MDS_DListIsEmpty(&(topicSub->node))) {
        return (MDS_EEXIST);
    }

    MDS_DListInsertNodePrev(&(topicPub->list), &(topicSub->node));

    return (MDS_EOK);
}

MDS_Err_t MDS_TOPIC_SubscriberUnregister(MDS_TOPIC_Sub_t *topicSub)
{
    MDS_ASSERT(topicSub != NULL);

    MDS_DListRemoveNode(&(topicSub->node));

    return (MDS_EOK);
}

void MDS_TOPIC_SubscriberCallback(MDS_TOPIC_Sub_t *topicSub,
                                  void (*callback)(const uint8_t *msg, size_t len))
{
    MDS_ASSERT(topicSub != NULL);

    topicSub->callback = callback;
}
