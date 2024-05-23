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
#ifndef __MDS_BUTTON_H__
#define __MDS_BUTTON_H__

/* Include ----------------------------------------------------------------- */
#include "mds_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef uint8_t MDS_BUTTON_Level_t;

typedef enum MDS_BUTTON_Event {
    MDS_BUTTON_EVENT_NONE = 0,
    MDS_BUTTON_EVENT_CLICK = 1,
    MDS_BUTTON_EVENT_REPEAT,
    MDS_BUTTON_EVENT_RELEASE,
    MDS_BUTTON_EVENT_PRESS,
    MDS_BUTTON_EVENT_HOLD,
} MDS_BUTTON_Event_t;

typedef enum MDS_BUTTON_State {
    MDS_BUTTON_STATE_NONE,
    MDS_BUTTON_STATE_RELEASED,
    MDS_BUTTON_STATE_PRESSED,
    MDS_BUTTON_STATE_REPEAT,
    MDS_BUTTON_STATE_HOLD,
} MDS_BUTTON_State_t;

typedef struct MDS_BUTTON_Group {
    MDS_ListNode_t list;
} MDS_BUTTON_Group_t;

typedef struct MDS_BUTTON_Device MDS_BUTTON_Device_t;
typedef struct MDS_BUTTON_InitStruct {
    const MDS_Arg_t *periph;
    MDS_BUTTON_Level_t (*getLevel)(const MDS_Arg_t *periph);
    MDS_Tick_t clickTicks;
    MDS_Tick_t holdTicks;
    MDS_BUTTON_Level_t releasedLevel;
    uint8_t debounceOut;

    void (*callback)(const MDS_BUTTON_Device_t *button, MDS_BUTTON_Event_t event);
} MDS_BUTTON_InitStruct_t;

struct MDS_BUTTON_Device {
    MDS_ListNode_t node;

    MDS_BUTTON_InitStruct_t init;

    MDS_Tick_t tickCount;
    MDS_BUTTON_Level_t btnLevel;
    MDS_BUTTON_Level_t fakedLevel;
    int8_t isFaked;
    MDS_BUTTON_State_t state : 8;
    uint8_t debounceCnt;
    uint8_t repeatCnt;
};

/* Function ---------------------------------------------------------------- */
extern void MDS_BUTTON_GroupInit(MDS_BUTTON_Group_t *group);
extern MDS_Err_t MDS_BUTTON_DeviceInit(MDS_BUTTON_Device_t *button, const MDS_BUTTON_InitStruct_t *init);
extern void MDS_BUTTON_GroupInsertDevice(MDS_BUTTON_Group_t *group, MDS_BUTTON_Device_t *button);
extern void MDS_BUTTON_GroupRemoveDevice(MDS_BUTTON_Device_t *button);
extern void MDS_BUTTON_GroupPollCheck(MDS_BUTTON_Group_t *group, MDS_Tick_t interval);

extern bool MDS_BUTTON_IsPressed(const MDS_BUTTON_Device_t *button);
extern MDS_Tick_t MDS_BUTTON_GetTickCount(const MDS_BUTTON_Device_t *button);
extern uint8_t MDS_BUTTON_GetRepeatCount(const MDS_BUTTON_Device_t *button);

extern bool MDS_BUTTON_FakedState(const MDS_BUTTON_Device_t *button, MDS_BUTTON_Level_t *level);
extern void MDS_BUTTON_FakeButton(MDS_BUTTON_Device_t *button, bool faked, MDS_BUTTON_Level_t level);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_BUTTON_H__ */
