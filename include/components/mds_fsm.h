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
#ifndef __MDS_FSM_H__
#define __MDS_FSM_H__

/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_FSM_TaskManager MDS_FSM_TaskManager_t;
typedef struct MDS_FSM_TaskNode MDS_FSM_TaskNode_t;

struct MDS_FSM_TaskNode {
    MDS_DListNode_t node;
    MDS_FSM_TaskManager_t *taskManager;
    void (*taskEntry)(MDS_FSM_TaskNode_t *);
    MDS_Tick_t ticknext;
    uint8_t priority;
    uint8_t taskStep;
    uint8_t waitEvent;
    uint8_t pollCount;
};

struct MDS_FSM_TaskManager {
    MDS_DListNode_t ready;
    MDS_DListNode_t block;
    MDS_FSM_TaskNode_t *curr;
};

/* Funtcion ---------------------------------------------------------------- */
MDS_Tick_t MDS_FSM_TaskManagerEntry(MDS_FSM_TaskManager_t *taskManager);
MDS_FSM_TaskNode_t *MDS_FSM_TaskManagerTaskCurr(const MDS_FSM_TaskManager_t *taskManager);
void MDS_FSM_TaskNodeSetupReady(MDS_FSM_TaskNode_t *taskNode, MDS_FSM_TaskManager_t *taskManager);
void MDS_FSM_TaskNodeWaitBlock(MDS_FSM_TaskNode_t *taskNode, MDS_Timeout_t timeout);
void MDS_FSM_TaskNodeExit(MDS_FSM_TaskNode_t *taskNode);
void MDS_FSM_TaskNodeNextStep(MDS_FSM_TaskNode_t *taskNode, uint8_t taskStep);
uint8_t MDS_FSM_TaskNodeGetStep(MDS_FSM_TaskNode_t *taskNode);
uint8_t MDS_FSM_TaskNodeEventPrepare(MDS_FSM_TaskNode_t *taskNode, uint8_t pollLimit);
uint8_t MDS_FSM_TaskNodeEventUntil(MDS_FSM_TaskNode_t *taskNode, MDS_Timeout_t timeout,
                                   uint8_t pollStep, uint8_t overStep);
uint8_t MDS_FSM_TaskNodeEventSet(MDS_FSM_TaskNode_t *taskNode, uint8_t event, bool force);

#define MDS_FSM_TASKNODE_INIT(name, entry, prio)                                                   \
    MDS_FSM_TaskNode_t *MDS_FSM_TaskNode_##name(void)                                              \
    {                                                                                              \
        static __attribute__((section("mds.fsm."))) MDS_FSM_TaskNode_t fsmTaskNode_##name = {      \
            .node.prev = &(fsmTaskNode_##name.node),                                               \
            .node.next = &(fsmTaskNode_##name.node),                                               \
            .taskEntry = entry,                                                                    \
            .ticknext = 0,                                                                         \
            .priority = prio,                                                                      \
            .taskStep = 0,                                                                         \
            .waitEvent = 0,                                                                        \
            .pollCount = 0,                                                                        \
        };                                                                                         \
                                                                                                   \
        return (&fsmTaskNode_##name);                                                              \
    }

#define MDS_FSM_TASKNODE_GET(name) MDS_FSM_TaskNode_##name()

#ifdef __cplusplus
}
#endif

#endif /* __MDS_FSM_H__ */
