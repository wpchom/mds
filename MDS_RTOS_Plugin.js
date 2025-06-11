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
/*
File    : MDS_RTOS_Plugin.js
Purpose : Script for thread aware debugging in Segger O-zone
*/

function getregs(ThreadPtr) {
    if (Debug.evaluate("g_mcause")) { // RISC-V
        var Regs = new Array(32);

        Regs[0] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->mepc");
        Regs[1] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->ra");
        Regs[3] = TargetInterface.getRes("gp");
        Regs[4] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->tp");
        Regs[5] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t0");
        Regs[6] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t1");
        Regs[7] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t2");
        Regs[8] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s0_fp");
        Regs[9] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s1");
        Regs[10] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a0");
        Regs[11] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a1");
        Regs[12] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a2");
        Regs[13] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a3");
        Regs[14] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a4");
        Regs[15] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a5");
        Regs[16] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a6");
        Regs[17] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->a7");
        Regs[18] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s2");
        Regs[19] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s3");
        Regs[20] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s4");
        Regs[21] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s5");
        Regs[22] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s6");
        Regs[23] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s7");
        Regs[24] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s8");
        Regs[25] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s9");
        Regs[26] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s10");
        Regs[27] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->s11");
        Regs[28] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t3");
        Regs[29] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t4");
        Regs[30] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t5");
        Regs[31] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->t6");


        Regs[2] = ThreadPtr + Debug.evaluate("sizeof(StackFrame)");

    } else { // ARM Thumb
        var Regs = new Array(17);

        Regs[0] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.r0");
        Regs[1] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.r1");
        Regs[2] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.r2");
        Regs[3] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.r3");
        Regs[12] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.r12");
        Regs[14] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.lr");
        Regs[15] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.pc");
        Regs[16] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->exception.psr");

        Regs[4] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r4");
        Regs[5] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r5");
        Regs[6] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r6");
        Regs[7] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r7");
        Regs[8] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r8");
        Regs[9] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r9");
        Regs[10] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r10");
        Regs[11] = Debug.evaluate("((StackFrame *)" + ThreadPtr + ")->r11");

        Regs[13] = ThreadPtr + Debug.evaluate("sizeof(StackFrame)");

        return Regs;
    }
}

var Devices = "Devices";
var Timers = "Timers";
var Semaphores = "Semaphores";
var Mutexes = "Mutexes";
var Events = "Events";
var MsgQueues = "MsgQueues";
var MemPools = "MemPools";
var MemHeaps = "MemHeaps";

function init() {
    Threads.clear();

    Threads.setColumns("Thread", "Name", "Entry", "State", "StackPoint", "StackBase", "StackLimit", "Priority", "Ticks");
    Threads.setSortByNumber("Priority");
    Threads.setColor("State", "Ready", "Running", "Blocking");

    if (Threads.setColumns2) {
        Threads.setColumns2(Devices, "Device", "Name", "Type", "Driver", "Handler", "Onwer/Mount");
        Threads.setColumns2(Timers, "Timer", "Name", "Type", "Entry", "Arg", "TickStart", "TickLimit");
        Threads.setColumns2(Semaphores, "Semaphore", "Name", "Value", "Max", "Thread");
        Threads.setColumns2(Mutexes, "Mutex", "Name", "Value", "Nest", "Onwer", "Thread");
        Threads.setColumns2(Events, "Event", "Name", "Value", "Thread");
        Threads.setColumns2(MsgQueues, "MsgQueue", "Name", "QueBuff", "MsgSize", "Free", "Recver", "Sender");
        Threads.setColumns2(MemPools, "MemPool", "Name", "MemBuff", "BlkSize", "Free", "Thread");
        Threads.setColumns2(MemHeaps, "MemHeap", "Name", "MemBuff", "BufSize", "Cur", "Max");
    }
}

function update() {
    Threads.clear();

    UpdateOjbects(3, UpdateTaskAdd);

    if (Threads.shown(Devices)) {
        UpdateOjbects(1, UpdateDeviceAdd);
    }
    if (Threads.shown(Timers)) {
        UpdateOjbects(2, UpdateTimerAdd);
    }
    if (Threads.shown(Semaphores)) {
        UpdateOjbects(4, UpdateSemaphoreAdd);
    }
    if (Threads.shown(Mutexes)) {
        UpdateOjbects(5, UpdateMutexAdd);
    }
    if (Threads.shown(Events)) {
        UpdateOjbects(6, UpdateEventAdd);
    }
    if (Threads.shown(MsgQueues)) {
        UpdateOjbects(7, UpdateMsgQueueAdd);
    }
    if (Threads.shown(MemPools)) {
        UpdateOjbects(8, UpdateMemPoolAdd);
    }
    if (Threads.shown(MemHeaps)) {
        UpdateOjbects(9, UpdateMemHeapAdd);
    }
}

function GetThreadState(ThreadState) {
    var state = ((ThreadState & 0x80) != 0) ? ("Yield|") : ("");
    if ((ThreadState & 0x0F) == 0x00) {
        state += "Inactived";
    } else if ((ThreadState & 0x0F) == 0x01) {
        state += "Ready";
    } else if ((ThreadState & 0x0F) == 0x02) {
        state += "Running";
    } else if ((ThreadState & 0x0F) == 0x03) {
        state += "Terminated";
    } else if ((ThreadState & 0x0F) == 0x04) {
        state += "Blocking";
    } else {
        state += "Unknown";
    }
    return state;
}

function UpdateTaskAdd(ThreadNode) {
    var ThreadName = Debug.evaluate("(char*)((MDS_Thread_t *)" + ThreadNode + ")->object.name");
    var ThreadEntry = Debug.getSymbol(Debug.evaluate("(uintptr_t)((MDS_Thread_t *)" + ThreadNode + ")->entry"));
    var ThreadState = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->state");

    var ThreadStackPtr = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->stackPoint");
    var ThreadStackSize = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->stackSize");
    var ThreadStackBase = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->stackBase");
    var ThreadStackLimit = ThreadStackBase + ThreadStackSize;

    var ThreadInitPrio = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->initPrio");
    var ThreadCurrPrio = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->currPrio");
    var ThreadPriority = (ThreadCurrPrio == ThreadInitPrio) ? (ThreadCurrPrio) : (ThreadInitPrio + "->" + ThreadCurrPrio);

    var ThreadInitTicks = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->initTick");
    var ThrheadRemainTicks = Debug.evaluate("((MDS_Thread_t *)" + ThreadNode + ")->remainTick");
    var ThreadTicks = ThrheadRemainTicks + "/" + ThreadInitTicks;

    Threads.add("0x" + ThreadNode.toString(16).toUpperCase(), ThreadName, ThreadEntry,
        GetThreadState(ThreadState),
        "0x" + ThreadStackPtr.toString(16).toUpperCase(),
        "0x" + ThreadStackBase.toString(16).toUpperCase() + "(0x" + (ThreadStackPtr - ThreadStackBase).toString(16).toUpperCase() + ")",
        "0x" + ThreadStackLimit.toString(16).toUpperCase() + "(0x" + (ThreadStackLimit - ThreadStackPtr).toString(16).toUpperCase() + ")",
        ThreadPriority, ThreadTicks,
        ThreadStackPtr);
}

function GetDeviceType(DeviceFlag) {
    var type = ((DeviceFlag & 0x80) != 0) ? ("Opened") : ("Closed");
    if ((DeviceFlag & 0x04) != 0) {
        type += "|Module";
    } else if ((DeviceFlag & 0x02) != 0) {
        type += "|Adaptr";
    } else if ((DeviceFlag & 0x01) != 0) {
        type += "|Peripheral";
    } else {
        type += "|Unknown";
    }
    return type;
}

function UpdateDeviceAdd(DeviceNode) {
    var DeviceName = Debug.evaluate("(char*)((MDS_Device_t *)" + DeviceNode + ")->object.name");
    var DeviceFlag = Debug.evaluate("((MDS_Device_t *)" + DeviceNode + ")->object.flags");

    var DeviceDriver = "";
    var DeviceHandler = "";

    if ((DeviceFlag & 0x04) != 0) {
        DeviceDriver = Debug.evaluate("((MDS_DevModule_t *)" + DeviceNode + ")->driver");
        DeviceHandler = Debug.evaluate("((MDS_DevModule_t *)" + DeviceNode + ")->handler");
    } else if ((DeviceFlag & 0x02) != 0) {
        DeviceDriver = Debug.evaluate("((MDS_DevAdaptr_t *)" + DeviceNode + ")->driver");
        DeviceHandler = Debug.evaluate("((MDS_DevAdaptr_t *)" + DeviceNode + ")->handler");
    }

    var DevDriverSym = Debug.getSymbol(DeviceDriver);
    if ((DevDriverSym != undefined) && (DevDriverSym != "")) {
        DeviceDriver = DevDrvSym + "(0x" + DeviceDriver.toString(16).toUpperCase() + ")";
    } else {
        DeviceDriver = "0x" + DeviceDriver.toString(16).toUpperCase();
    }
    var DevHandleSym = Debug.getSymbol(DeviceHandler);
    if ((DevHandleSym != undefined) && (DevHandleSym != "")) {
        DeviceHandler = DevHandleSym + "(0x" + DeviceHandler.toString(16).toUpperCase() + ")";
    } else {
        DeviceHandler = "0x" + DeviceHandler.toString(16).toUpperCase();
    }

    var DevMountOnwer = "";
    if ((DeviceFlag & 0x02) != 0) {
        DevMountOnwer = Debug.evaluate("((MDS_DevAdaptr_t *)" + DeviceNode + ")->onwer");
        DevMountOnwer = "0x" + DevMountOnwer.toString(16).toUpperCase();
    } else if ((DeviceFlag & 0x01) != 0) {
        DevMountOnwer = Debug.evaluate("((MDS_DevPeriph_t *)" + DeviceNode + ")->mount");
        DevMountOnwer = "0x" + DevMountOnwer.toString(16).toUpperCase();
    }

    Threads.add2(Devices, "0x" + DeviceNode.toString(16).toUpperCase(),
        DeviceName, GetDeviceType(DeviceFlag), DeviceDriver, DeviceHandler, DevMountOnwer);
}

function GetTimerType(TimerFlag) {
    var type = ((TimerFlag & 0x80) != 0) ? ("Actived") : ("Inactived");
    if ((TimerFlag & 0x08) != 0) {
        type += "|System";
    }
    if ((TimerFlag & 0x01) != 0) {
        type += "|Period";
    }
    return type;
}

function UpdateTimerAdd(TimerNode) {
    var TimerName = Debug.evaluate("(char*)((MDS_Timer_t *)" + TimerNode + ")->object.name");
    var TimerFlag = Debug.evaluate("((MDS_Timer_t *)" + TimerNode + ")->object.flags");
    var TimerEntry = Debug.getSymbol(Debug.evaluate("(uintptr_t)((MDS_Timer_t *)" + TimerNode + ")->entry"));
    var TimerArg = Debug.evaluate("((MDS_Timer_t *)" + TimerNode + ")->arg");
    var TimerArgSym = Debug.getSymbol(TimerArg);
    if ((TimerArgSym != undefined) && (TimerArgSym != "")) {
        TimerArg = TimerArgSym + "(0x" + TimerArg.toString(16).toUpperCase() + ")";
    } else {
        TimerArg = "0x" + TimerArg.toString(16).toUpperCase();
    }

    var TimerStartTick = Debug.evaluate("((MDS_Timer_t *)" + TimerNode + ")->tickstart");
    var TimerLimitTick = Debug.evaluate("((MDS_Timer_t *)" + TimerNode + ")->ticklimit");

    Threads.add2(Timers, "0x" + TimerNode.toString(16).toUpperCase(),
        TimerName, GetTimerType(TimerFlag), TimerEntry, TimerArg, TimerStartTick, TimerLimitTick);
}

function UpdateSemaphoreAdd(SemaphoreNode) {
    var SemaphoreName = Debug.evaluate("(char*)((MDS_Semaphore_t *)" + SemaphoreNode + ")->object.name");
    var SemaphoreValue = Debug.evaluate("((MDS_Semaphore_t *)" + SemaphoreNode + ")->value");
    var SemaphoreMax = Debug.evaluate("((MDS_Semaphore_t *)" + SemaphoreNode + ")->max");

    var SemaphoreList = "";
    var ThreadList = Debug.evaluate("&(((MDS_Semaphore_t *)" + SemaphoreNode + ")->list)");
    var ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (SemaphoreList != "") {
            SemaphoreList += ",";
        }
        SemaphoreList += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    Threads.add2(Semaphores, "0x" + SemaphoreNode.toString(16).toUpperCase(),
        SemaphoreName, SemaphoreValue, SemaphoreMax, SemaphoreList);
}

function UpdateMutexAdd(MutexNode) {
    var MutexName = Debug.evaluate("(char*)((MDS_Mutex_t *)" + MutexNode + ")->object.name");
    var MutexValue = Debug.evaluate("((MDS_Mutex_t *)" + MutexNode + ")->value");
    var MutexNest = Debug.evaluate("((MDS_Mutex_t *)" + MutexNode + ")->nest");
    var MutexOnwer = Debug.evaluate("((MDS_Mutex_t *)" + MutexNode + ")->owner");

    var MutexList = "";
    var ThreadList = Debug.evaluate("&(((MDS_Mutex_t *)" + SemaphoreNode + ")->list)");
    var ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (MutexList != "") {
            MutexList += ",";
        }
        MutexList += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    Threads.add2(Mutexes, "0x" + MutexNode.toString(16).toUpperCase(),
        MutexName, MutexValue, MutexNest, "0x" + MutexOnwer.toString(16).toUpperCase(), MutexList);
}

function UpdateEventAdd(EventNode) {
    var EventName = Debug.evaluate("(char*)((MDS_Event_t *)" + EventNode + ")->object.name");
    var EventValue = Debug.evaluate("((MDS_Event_t *)" + EventNode + ")->value");

    var EventList = "";
    var ThreadList = Debug.evaluate("&(((MDS_Mutex_t *)" + EventNode + ")->list)");
    var ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (EventList != "") {
            EventList += ",";
        }
        EventList += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    Threads.add2(Events, "0x" + EventNode.toString(16).toUpperCase(),
        EventName, EventValue, EventList);
}

function UpdateMsgQueueAdd(MsgQueueNode) {
    var MsgQueueName = Debug.evaluate("(char*)((MDS_MsgQueue_t *)" + MsgQueueNode + ")->object.name");
    var MsgQueueMemBuff = Debug.evaluate("(((MDS_MsgQueue_t *)" + MsgQueueNode + ")->queBuff)");
    var MsgQueueMsgSize = Debug.evaluate("((MDS_MsgQueue_t *)" + MsgQueueNode + ")->msgSize");

    var free = 0;
    var MsgQueueLfree = Debug.evaluate("(((MDS_MsgQueue_t *)" + MsgQueueNode + ")->lfree)");
    while (MsgQueueLfree != 0) {
        free += 1;
        MemPoolLfree = Debug.evaluate("(((MDS_MsgQueueHeader_t *)" + MsgQueueLfree + ")->next)");
    }

    var used = 0;
    var MsgQueueLhead = Debug.evaluate("(((MDS_MsgQueue_t *)" + MsgQueueNode + ")->lhead)");
    while (MsgQueueLhead != 0) {
        used += 1;
        MsgQueueLhead = Debug.evaluate("(((MDS_MsgQueueHeader_t *)" + MsgQueueLhead + ")->next)");
    }

    var MemPoolRecver = "";
    var MemPoolSender = "";

    var ThreadList = Debug.evaluate("&(((MDS_MemPool_t *)" + MsgQueueNode + ")->listRecv)");
    var ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (MemPoolRecver != "") {
            MemPoolRecver += ",";
        }
        MemPoolRecver += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    ThreadList = Debug.evaluate("&(((MDS_MemPool_t *)" + MsgQueueNode + ")->listSend)");
    ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (MemPoolSender != "") {
            MemPoolSender += ",";
        }
        MemPoolSender += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    Threads.add2(MsgQueues, "0x" + MsgQueueNode.toString(16).toUpperCase(),
        MsgQueueName, "0x" + MsgQueueMemBuff.toString(16).toUpperCase(), MsgQueueMsgSize,
        free + "/" + (free + used), MemPoolRecver, MemPoolSender);
}

function UpdateMemPoolAdd(MemPoolNode) {
    var MemPoolName = Debug.evaluate("(char*)((MDS_MemPool_t *)" + MemPoolNode + ")->object.name");
    var MemPoolMemBuff = Debug.evaluate("(((MDS_MemPool_t *)" + MemPoolNode + ")->memBuff)");
    var MemPoolBlkSize = Debug.evaluate("(((MDS_MemPool_t *)" + MemPoolNode + ")->blkSize)");

    var total = MemPoolMemBuff / MemPoolBlkSize;
    var free = 0;

    var MemPoolLfree = Debug.evaluate("(((MDS_MemPool_t *)" + MemPoolNode + ")->lfree)");
    while (MemPoolLfree != MemPoolNode) {
        free += 1;
        MemPoolLfree = Debug.evaluate("(((MDS_MemPoolHeader *)" + MemPoolLfree + ")->next)");
    }

    var MemPoolList = "";
    var ThreadList = Debug.evaluate("&(((MDS_MemPool_t *)" + MemPoolNode + ")->list)");
    var ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadList + ")->next)"));
    while (ThreadNext != ThreadList) {
        if (MemPoolList != "") {
            MemPoolList += ",";
        }
        MemPoolList += "0x" + ThreadNext.toString(16).toUpperCase();
        ThreadNext = Debug.evaluate(("(((MDS_DListNode_t *)" + ThreadNext + ")->next)"));
    }

    Threads.add2(MemPools, "0x" + MemPoolNode.toString(16).toUpperCase(),
        MemPoolName, "0x" + MemPoolMemBuff.toString(16).toUpperCase(), MemPoolBlkSize,
        free + "/" + total, MemPoolList);
}

function UpdateMemHeapAdd(MemHeapNode) {
    var MemHeapName = Debug.evaluate("(char*)((MDS_MemHeap_t *)" + MemHeapNode + ")->object.name");
    var MemHeapLimit = Debug.evaluate("((MDS_MemHeap_t *)" + MemHeapNode + ")->limit");
    var MemHeapMemBuff = Debug.evaluate("((MemHeapNode_t *)" + MemHeapLimit + ")->next");
    var MemHeapBufSize = MemHeapLimit - MemHeapMemBuff;

    var MemHeapCur = Debug.evaluate("((MDS_MemHeap_t *)" + MemHeapNode + ")->cur");
    var MemHeapMax = Debug.evaluate("((MDS_MemHeap_t *)" + MemHeapNode + ")->max");

    Threads.add2(MemHeaps, "0x" + MemHeapNode.toString(16).toUpperCase(),
        MemHeapName, "0x" + MemHeapMemBuff.toString(16).toUpperCase(), MemHeapBufSize,
        MemHeapCur, MemHeapMax);
}

function UpdateOjbects(obj, func) {
    var ObjectList = Debug.evaluate("(MDS_DListNode_t*)(&g_objectList[" + obj + "])");
    var ObjectNode = ObjectList;

    while (true) {
        ObjectNode = Debug.evaluate(("(((MDS_DListNode_t *)" + ObjectNode + ")->next)"));
        if (ObjectNode == ObjectList) {
            break;
        }

        func(ObjectNode);
    }

}

/**
 * TODO: TargetInterface.getReg() not a function object.
 */
function UpdateMainStack() {
    var StackPtr = undefined;
    if (Debug.evaluate("g_mcause")) { // RISC-V

    } else { // ARM Thumb
        // StackPtr = TargetInterface.getReg("msp");
    }

    if (StackPtr != undefined) {
        Threads.add("Main", "",
            "Main",
            "0x" + StackPtr.toString(16).toUpperCase(),
            "",
            "",
            ThreadPriority, ThreadTicks,
            undefined);
    }
}

function getContextSwitchAddrs() {
    var Addr = Debug.evaluate("&MDS_CoreSchedulerSwitch");

    TargetInterface.message(Addr);

    if (Addr != undefined) {
        return [Addr];
    } else {
        return [];
    }
}

function getOSName() {
    return "MDS_RTOS";
}
