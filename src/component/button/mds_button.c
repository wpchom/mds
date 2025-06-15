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
#include "mds_button.h"
#include "mds_log.h"

/*
 * Button [State] (Action) {Event}
 *
 *    ┌──────────── {click/repeat} <─────────────────── (>clickTicks) <────────────┐
 *    │                                                                            │
 *    │                                                                            │
 * [{None}] ───────> ({presse}) ───────> [Pressed] ──────> ({release}) ──────> [Released]
 *    │                                       │                                  │    │
 *    │                                 (>holdTicks)                             │    │
 *    │                                       │                                  │    │
 *    ├──── (release) <────── [{Hold}] <──────┘     ┌────> ({release}) ──────────┘    │
 *    │                                             │                             ({pressed})
 *    │                                             │                                 │
 *    └──── (>holdTicks) <───────────────────── [Repeat] <────────────────────────────┘
 *
 */

/* Define ------------------------------------------------------------------ */
#ifndef MDS_BUTTON_CLICK_TICKS
#define MDS_BUTTON_CLICK_TICKS 150
#endif

#ifndef MDS_BUTTON_HOLD_TICKS
#define MDS_BUTTON_HOLD_TICKS 300
#endif

/* Function ---------------------------------------------------------------- */
static MDS_Mask_t MDS_BUTTON_GetLevel(MDS_BUTTON_Device_t *button)
{
    if (button->isFaked != 0) {
        return (button->fakedLevel);
    } else {
        return (button->init.getLevel(button->init.periph));
    }
}

static void BUTTON_Callback(MDS_BUTTON_Device_t *button, MDS_BUTTON_Event_t event)
{
    if (button->init.callback != NULL) {
        (button->init.callback)(button, event);
    }
}

static void BUTTON_StateNone(MDS_BUTTON_Device_t *button)
{
    if (button->btnLevel != button->init.releasedLevel) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_PRESS);
        button->tickCount = 0;
        button->repeatCnt = 0;
        button->state = MDS_BUTTON_STATE_PRESSED;
    } else {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_NONE);
    }
}

static void BUTTON_StateReleased(MDS_BUTTON_Device_t *button)
{
    if (button->btnLevel != button->init.releasedLevel) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_PRESS);
        button->tickCount = 0;
        button->repeatCnt += 1;
        button->state = MDS_BUTTON_STATE_REPEAT;
    } else if (button->tickCount > button->init.clickTicks) {
        if (button->repeatCnt == 1) {
            BUTTON_Callback(button, MDS_BUTTON_EVENT_CLICK);
        } else if (button->repeatCnt > 1) {
            BUTTON_Callback(button, MDS_BUTTON_EVENT_REPEAT);
        }
        button->state = MDS_BUTTON_STATE_NONE;
    }
}

static void BUTTON_StatePressed(MDS_BUTTON_Device_t *button)
{
    if (button->btnLevel == button->init.releasedLevel) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_RELEASE);
        button->tickCount = 0;
        button->repeatCnt = 1;
        button->state = MDS_BUTTON_STATE_RELEASED;
    } else if (button->tickCount > button->init.holdTicks) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_HOLD);
        button->state = MDS_BUTTON_STATE_HOLD;
    }
}

static void BUTTON_StateRepeat(MDS_BUTTON_Device_t *button)
{
    if (button->btnLevel == button->init.releasedLevel) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_RELEASE);
        if (button->tickCount <= button->init.clickTicks) {
            button->tickCount = 0;
            button->state = MDS_BUTTON_STATE_RELEASED;
        } else {
            button->state = MDS_BUTTON_STATE_NONE;
        }
    } else if (button->repeatCnt == 0) {
        button->state = MDS_BUTTON_STATE_NONE;
        BUTTON_StateNone(button);
    }
}

static void BUTTON_StateHold(MDS_BUTTON_Device_t *button)
{
    if (button->btnLevel != button->init.releasedLevel) {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_HOLD);
    } else {
        BUTTON_Callback(button, MDS_BUTTON_EVENT_RELEASE);
        button->tickCount = 0;
        button->state = MDS_BUTTON_STATE_NONE;
    }
}

static void BUTTON_CheckState(MDS_BUTTON_Device_t *button, MDS_Tick_t interval)
{
    static void (*const buttonState[])(MDS_BUTTON_Device_t *) = {
        [MDS_BUTTON_STATE_NONE] = BUTTON_StateNone,          //
        [MDS_BUTTON_STATE_RELEASED] = BUTTON_StateReleased,  //
        [MDS_BUTTON_STATE_PRESSED] = BUTTON_StatePressed,    //
        [MDS_BUTTON_STATE_REPEAT] = BUTTON_StateRepeat,      //
        [MDS_BUTTON_STATE_HOLD] = BUTTON_StateHold,
    };

    MDS_Mask_t readLevel = MDS_BUTTON_GetLevel(button);

    MDS_ASSERT(button->state < ARRAY_SIZE(buttonState));

    if (readLevel == button->btnLevel) {
        button->debounceCnt = 0;
    } else if (++(button->debounceCnt) > button->init.debounceOut) {
        button->btnLevel = readLevel;
        button->debounceCnt = 0;
    }

    if (button->state > MDS_BUTTON_STATE_NONE) {
        button->tickCount += interval;
    }

    if (button->state <= ARRAY_SIZE(buttonState)) {
        buttonState[button->state](button);
    }
}

void MDS_BUTTON_GroupInit(MDS_BUTTON_Group_t *group)
{
    MDS_ASSERT(group != NULL);

    MDS_ListInitNode(&(group->list));
}

MDS_Err_t MDS_BUTTON_DeviceInit(MDS_BUTTON_Device_t *button, const MDS_BUTTON_InitStruct_t *init)
{
    MDS_ASSERT(button != NULL);

    if (init != NULL) {
        button->init = *init;
    }

    if (button->init.getLevel == NULL) {
        return (MDS_EINVAL);
    }

    if (button->init.clickTicks == 0) {
        button->init.clickTicks = MDS_BUTTON_CLICK_TICKS;
    }
    if (button->init.holdTicks == 0) {
        button->init.holdTicks = MDS_BUTTON_HOLD_TICKS;
    }

    MDS_ListInitNode(&(button->node));

    button->tickCount = 0;
    button->btnLevel = button->init.releasedLevel;
    button->isFaked = false;
    button->state = MDS_BUTTON_STATE_NONE;
    button->debounceCnt = 0;
    button->repeatCnt = 0;

    return (MDS_EOK);
}

void MDS_BUTTON_GroupInsertDevice(MDS_BUTTON_Group_t *group, MDS_BUTTON_Device_t *button)
{
    MDS_ASSERT(group != NULL);
    MDS_ASSERT(button != NULL);

    MDS_ListInsertNodePrev(&(group->list), &(button->node));
}

void MDS_BUTTON_GroupRemoveDevice(MDS_BUTTON_Device_t *button)
{
    MDS_ASSERT(button != NULL);

    MDS_ListRemoveNode(&(button->node));
}

void MDS_BUTTON_GroupPollCheck(MDS_BUTTON_Group_t *group, MDS_Tick_t interval)
{
    MDS_ASSERT(group != NULL);

    MDS_BUTTON_Device_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, node, &(group->list)) {
        BUTTON_CheckState(iter, interval);
    }
}

bool MDS_BUTTON_IsPressed(const MDS_BUTTON_Device_t *button)
{
    MDS_ASSERT(button != NULL);

    return (button->btnLevel != button->init.releasedLevel);
}

MDS_Tick_t MDS_BUTTON_GetTickCount(const MDS_BUTTON_Device_t *button)
{
    MDS_ASSERT(button != NULL);

    return (button->tickCount);
}

uint8_t MDS_BUTTON_GetRepeatCount(const MDS_BUTTON_Device_t *button)
{
    MDS_ASSERT(button != NULL);

    return (button->repeatCnt);
}

bool MDS_BUTTON_FakedState(const MDS_BUTTON_Device_t *button, MDS_Mask_t *level)
{
    MDS_ASSERT(button != NULL);

    if (level != NULL) {
        *level = button->fakedLevel;
    }

    return (button->isFaked);
}

void MDS_BUTTON_FakeButton(MDS_BUTTON_Device_t *button, bool faked, MDS_Mask_t level)
{
    MDS_ASSERT(button != NULL);

    button->isFaked = faked;
    button->fakedLevel = level;
}
