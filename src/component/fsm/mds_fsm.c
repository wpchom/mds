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
#include "mds_fsm.h"

/* Funtcion ---------------------------------------------------------------- */
MDS_Tick_t MDS_FSM_TaskManagerEntry(MDS_FSM_TaskManager_t *taskManager)
{
    MDS_ASSERT(taskManager != NULL);

    MDS_Tick_t tickdiff = MDS_CLOCK_TICK_FOREVER;

    while (!MDS_ListIsEmpty(&(taskManager->ready))) {
        MDS_FSM_TaskNode_t *taskNode = CONTAINER_OF(taskManager->ready.next, MDS_FSM_TaskNode_t, node);
        if (taskNode->taskEntry != NULL) {
            taskManager->curr = taskNode;
            taskNode->taskEntry(taskNode);
            taskManager->curr = NULL;
        }
    }

    if (!MDS_ListIsEmpty(&(taskManager->block))) {
        MDS_FSM_TaskNode_t *taskNode = CONTAINER_OF(taskManager->block.next, MDS_FSM_TaskNode_t, node);
        tickdiff = taskNode->ticknext - MDS_ClockGetTickCount();
    }

    return (tickdiff);
}

MDS_FSM_TaskNode_t *MDS_FSM_TaskManagerTaskCurr(const MDS_FSM_TaskManager_t *taskManager)
{
    MDS_ASSERT(taskManager != NULL);

    return (taskManager->curr);
}

void MDS_FSM_TaskNodeSetupReady(MDS_FSM_TaskNode_t *taskNode, MDS_FSM_TaskManager_t *taskManager)
{
    MDS_ASSERT(taskNode != NULL);

    if (taskManager == NULL) {
        return;
    }

    taskNode->taskManager = taskManager;
    if (MDS_ListIsEmpty(&(taskNode->taskManager->ready))) {
        MDS_ListInitNode(&(taskNode->taskManager->ready));
    }

    MDS_ListRemoveNode(&(taskNode->node));

    MDS_FSM_TaskNode_t *iterTask = NULL;
    MDS_LIST_FOREACH_NEXT (iterTask, node, &(taskNode->taskManager->ready)) {
        if (iterTask->priority > taskNode->priority) {
            MDS_ListInsertNodePrev(&(iterTask->node), &(taskNode->node));
            break;
        }
    }
}

void MDS_FSM_TaskNodeWaitBlock(MDS_FSM_TaskNode_t *taskNode, MDS_Timeout_t timeout)
{
    MDS_ASSERT(taskNode != NULL);

    if (taskNode->taskManager == NULL) {
        return;
    }

    if (MDS_ListIsEmpty(&(taskNode->taskManager->block))) {
        MDS_ListInitNode(&(taskNode->taskManager->block));
    }

    MDS_ListRemoveNode(&(taskNode->node));

    MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
    taskNode->ticknext = tickcurr + tickout;

    MDS_FSM_TaskNode_t *iterTask = NULL;
    MDS_LIST_FOREACH_NEXT (iterTask, node, &(taskNode->taskManager->block)) {
        if ((iterTask->ticknext - tickcurr) > (taskNode->ticknext - tickcurr)) {
            MDS_ListInsertNodePrev(&(iterTask->node), &(taskNode->node));
            break;
        }
    }
}

void MDS_FSM_TaskNodeExit(MDS_FSM_TaskNode_t *taskNode)
{
    MDS_ASSERT(taskNode != NULL);

    MDS_ListRemoveNode(&(taskNode->node));
}

void MDS_FSM_TaskNodeNextStep(MDS_FSM_TaskNode_t *taskNode, uint8_t taskStep)
{
    MDS_ASSERT(taskNode != NULL);

    taskNode->taskStep = taskStep;
}

uint8_t MDS_FSM_TaskNodeGetStep(MDS_FSM_TaskNode_t *taskNode)
{
    MDS_ASSERT(taskNode != NULL);

    return (taskNode->taskStep);
}

uint8_t MDS_FSM_TaskNodeEventPrepare(MDS_FSM_TaskNode_t *taskNode, uint8_t pollLimit)
{
    MDS_ASSERT(taskNode != NULL);

    uint8_t event = taskNode->waitEvent;
    taskNode->waitEvent = 0;
    taskNode->pollCount = pollLimit;

    return (event);
}

uint8_t MDS_FSM_TaskNodeEventUntil(MDS_FSM_TaskNode_t *taskNode, uint32_t tickout, uint8_t pollStep, uint8_t overStep)
{
    MDS_ASSERT(taskNode != NULL);

    uint8_t event = taskNode->waitEvent;
    if (event == 0) {
        if ((taskNode->pollCount > 0) && (tickout > 0)) {
            taskNode->pollCount -= 1;
            MDS_FSM_TaskNodeWaitBlock(taskNode, tickout);
            MDS_FSM_TaskNodeNextStep(taskNode, pollStep);
        } else {
            taskNode->pollCount = 0;
            MDS_FSM_TaskNodeNextStep(taskNode, overStep);
        }
    } else {
        taskNode->waitEvent = 0;
    }

    return (event);
}

uint8_t MDS_FSM_TaskNodeEventSet(MDS_FSM_TaskNode_t *taskNode, uint8_t event, bool force)
{
    MDS_ASSERT(taskNode != NULL);

    uint8_t lastEvent = taskNode->waitEvent;
    if ((taskNode->waitEvent == 0) || (force == true)) {
        taskNode->waitEvent = event;
        MDS_FSM_TaskNodeSetupReady(taskNode, taskNode->taskManager);
    }

    return (lastEvent);
}
