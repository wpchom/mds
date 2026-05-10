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
#ifndef CONFIG_MDS_BUTTON_CLICK_TICKS
#define CONFIG_MDS_BUTTON_CLICK_TICKS 150
#endif

#ifndef CONFIG_MDS_BUTTON_HOLD_TICKS
#define CONFIG_MDS_BUTTON_HOLD_TICKS 300
#endif

/* Function ---------------------------------------------------------------- */
static MDS_BUTTON_Mask_t MDS_BUTTON_GetLevel(MDS_BUTTON_Device_t *button)
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

static void BUTTON_CheckState(MDS_BUTTON_Device_t *button, MDS_BUTTON_Tick_t interval)
{
    static void (*const buttonState[])(MDS_BUTTON_Device_t *) = {
        [MDS_BUTTON_STATE_NONE] = BUTTON_StateNone,         //
        [MDS_BUTTON_STATE_RELEASED] = BUTTON_StateReleased, //
        [MDS_BUTTON_STATE_PRESSED] = BUTTON_StatePressed,   //
        [MDS_BUTTON_STATE_REPEAT] = BUTTON_StateRepeat,     //
        [MDS_BUTTON_STATE_HOLD] = BUTTON_StateHold,
    };

    MDS_BUTTON_Mask_t readLevel = MDS_BUTTON_GetLevel(button);

    if (readLevel == button->btnLevel) {
        button->debounceCnt = 0;
    } else if (++(button->debounceCnt) > button->init.debounceOut) {
        button->btnLevel = readLevel;
        button->debounceCnt = 0;
    }

    if (button->state > MDS_BUTTON_STATE_NONE) {
        button->tickCount += interval;
    }

    if (button->state <= (sizeof(buttonState) / sizeof(buttonState[0]))) {
        buttonState[button->state](button);
    }
}

void MDS_BUTTON_GroupInit(MDS_BUTTON_Group_t *group)
{
    group->next = group->prev = (void *)group;
}

void MDS_BUTTON_DeviceInit(MDS_BUTTON_Device_t *button, const MDS_BUTTON_InitStruct_t *init)
{
    if (init != NULL) {
        button->init = *init;
    }

    if (button->init.clickTicks == 0) {
        button->init.clickTicks = CONFIG_MDS_BUTTON_CLICK_TICKS;
    }
    if (button->init.holdTicks == 0) {
        button->init.holdTicks = CONFIG_MDS_BUTTON_HOLD_TICKS;
    }

    button->next = button->prev = button;

    button->tickCount = 0;
    button->btnLevel = button->init.releasedLevel;
    button->isFaked = false;
    button->state = MDS_BUTTON_STATE_NONE;
    button->debounceCnt = 0;
    button->repeatCnt = 0;
}

void MDS_BUTTON_GroupInsertDevice(MDS_BUTTON_Group_t *group, MDS_BUTTON_Device_t *button)
{
    button->next = group->next;
    button->prev = group->prev;
    group->prev->next = button;
    group->next->prev = button;
}

void MDS_BUTTON_GroupRemoveDevice(MDS_BUTTON_Device_t *button)
{
    button->prev->next = button->next;
    button->next->prev = button->prev;
    button->next = button->prev = button;
}

void MDS_BUTTON_GroupPollCheck(MDS_BUTTON_Group_t *group, MDS_BUTTON_Tick_t interval)
{
    for (MDS_BUTTON_Device_t *iter = group->next; iter != (void *)group; iter = iter->next) {
        BUTTON_CheckState(iter, interval);
    }
}

bool MDS_BUTTON_IsPressed(const MDS_BUTTON_Device_t *button)
{
    return (button->btnLevel != button->init.releasedLevel);
}

MDS_BUTTON_Tick_t MDS_BUTTON_GetTickCount(const MDS_BUTTON_Device_t *button)
{
    return (button->tickCount);
}

uint8_t MDS_BUTTON_GetRepeatCount(const MDS_BUTTON_Device_t *button)
{
    return (button->repeatCnt);
}

bool MDS_BUTTON_FakedState(const MDS_BUTTON_Device_t *button, MDS_BUTTON_Mask_t *level)
{
    if (level != NULL) {
        *level = button->fakedLevel;
    }

    return (button->isFaked);
}

void MDS_BUTTON_FakeButton(MDS_BUTTON_Device_t *button, bool faked, MDS_BUTTON_Mask_t level)
{
    button->isFaked = (int8_t)faked;
    button->fakedLevel = level;
}
