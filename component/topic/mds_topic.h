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
#ifndef __MDS_TOPIC_H__
#define __MDS_TOPIC_H__

/* Include ----------------------------------------------------------------- */
#include "mds_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_TOPIC_Pub {
    MDS_ListNode_t list;
} MDS_TOPIC_Pub_t;

typedef struct MDS_TOPIC_Sub {
    MDS_ListNode_t node;

    void (*callback)(const uint8_t *msg, size_t len);
} MDS_TOPIC_Sub_t;

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_TOPIC_PublisherInit(MDS_TOPIC_Pub_t *topicPub);
MDS_Err_t MDS_TOPIC_PublishMessage(MDS_TOPIC_Pub_t *topicPub, const void *msg, size_t msgLen);

MDS_Err_t MDS_TOPIC_SubscriberRegister(MDS_TOPIC_Sub_t *topicSub, MDS_TOPIC_Pub_t *topicPub);
MDS_Err_t MDS_TOPIC_SubscriberUnregister(MDS_TOPIC_Sub_t *topicSub);
void MDS_TOPIC_SubscriberCallback(MDS_TOPIC_Sub_t *topicSub, void (*onMessage)(const uint8_t *msg, size_t len));

#ifdef __cplusplus
}
#endif

#endif /* __MDS_TOPIC_H__ */
