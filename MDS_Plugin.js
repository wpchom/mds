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

/* Utils ------------------------------------------------------------------- */
function toHex(val) {
    if ((val == undefined) || (val == null)) {
        return "N/A";
    }

    return ("0x" + val.toString(16).toUpperCase());
}

function resolveSymbol(addr) {
    if ((addr == undefined) || (addr == null) || (addr == 0)) {
        return "N/A";
    }

    var sym = Debug.getSymbol(addr);
    if ((sym != undefined) && (sym != "")) {
        return (sym + "(" + toHex(addr) + ")");
    }

    return toHex(addr);
}

/* Architecture --------------------------------------------------------------- */
function detectArchitecture() {
    var addr = Debug.evaluate("&Trap_Handler");
    if ((addr != undefined) && (addr != 0)) {
        return "riscv";
    }

    var addr = Debug.evaluate("&PendSV_Handler");
    if ((addr != undefined) && (addr != 0)) {
        return "arm";
    }

    return "unknown";
}

/**
 * Get RISC-V register state from saved stack frame.
 * StackFrame layout (riscv.c):
 *   [0]=mepc, [1]=ra, [2]=mstatus, [3]=fcsr,
 *   [4]=tp(x4), [5]=t0(x5), [6]=t1(x6), [7]=t2(x7),
 *   [8]=s0_fp(x8), [9]=s1(x9), [10]=a0(x10), [11]=a1(x11),
 *   [12]=a2(x12), [13]=a3(x13), [14]=a4(x14), [15]=a5(x15),
 *   [16]=a6(x16), [17]=a7(x17), [18]=s2(x18), [19]=s3(x19),
 *   [20]=s4(x20), [21]=s5(x21), [22]=s6(x22), [23]=s7(x23),
 *   [24]=s8(x24), [25]=s9(x25), [26]=s10(x26), [27]=s11(x27),
 *   [28]=t3(x28), [29]=t4(x29), [30]=t5(x30), [31]=t6(x31)
 */
function getregsRiscV(ThreadPtr) {
    var Regs = new Array(33); // x0-x31 + SP(32)

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

    return Regs;
}

/**
 * Get ARM Cortex-M register state from saved stack frame.
 * StackFrame layout (thumbv6m.c / thumbv7m.c):
 *   Software-saved: r4-r11 (plus exc_flag and s16-s31 if FPU)
 *   Exception-saved: r0-r3, r12, lr, pc, psr (plus s0-s15, fpscr if FPU)
 */
function getregsArm(ThreadPtr) {
    var Regs = new Array(17); // r0-r15 + psr(16)

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

/* Object ------------------------------------------------------------------ */
var OBJECT_MAX_ITER = 128; // safety limit against infinite loops
var MDS_Object = {
    DEVICE: { index: 1, label: "Device" },
    THREAD: { index: 2, label: "Thread" },
    WORKQUEUE: { index: 3, label: "WorkQueue" },
    WORKNODE: { index: 4, label: "WorkNode" },
    SEMAPHORE: { index: 5, label: "Semaphore" },
    MUTEX: { index: 6, label: "Mutex" },
    EVENT: { index: 7, label: "Event" },
    MSGQUEUE: { index: 8, label: "MsgQueue" },
    MEMPOOL: { index: 9, label: "MemPool" },
    MEMHEAP: { index: 10, label: "MemHeap" },
};

function iterateObjects(objIdx, callback) {
    var listExpr = "(MDS_DListNode_t *)(&g_objectList[" + objIdx + "].list)";

    var listHead = Debug.evaluate(listExpr);
    if ((listHead == undefined) || (listHead == 0)) {
        return;
    }

    var listNode = Debug.evaluate("((MDS_DListNode_t *)" + listHead + ")->next");
    for (var i = 0; i < OBJECT_MAX_ITER; i++) {
        if ((listNode == undefined) || (listNode == 0) || (listNode == listHead)) {
            break;
        }

        callback(listNode);

        listNode = Debug.evaluate("((MDS_DListNode_t *)" + listNode + ")->next");
    }
}

/* Thread ------------------------------------------------------------------ */
var THREAD_STATE_MASK = 0x0F;
var THREAD_STATE_INACTIVED = 0x00;
var THREAD_STATE_TERMINATED = 0x01;
var THREAD_STATE_READY = 0x02;
var THREAD_STATE_RUNNING = 0x03;
var THREAD_STATE_SUSPENDED = 0x04;
var THREAD_FLAG_YIELD = 0x80;

function getThreadState(state) {
    var result = "";

    if ((state & THREAD_FLAG_YIELD) != 0) {
        result = "Yield|";
    }

    var base = state & THREAD_STATE_MASK;
    switch (base) {
        case THREAD_STATE_INACTIVED:
            result += "Inactived";
            break;
        case THREAD_STATE_TERMINATED:
            result += "Terminated";
            break;
        case THREAD_STATE_READY:
            result += "Ready";
            break;
        case THREAD_STATE_RUNNING:
            result += "Running";
            break;
        case THREAD_STATE_SUSPENDED:
            result += "Blocking";
            break;
        default:
            result += "Unknown(0x" + base.toString(16) + ")";
            break;
    }

    return result;
}

function updateThread(nodeAddr) {
    var pfx = "(MDS_Thread_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var entry = Debug.evaluate("(void*)(" + pfx + ")->entry");
    var state = Debug.evaluate("(int)(" + pfx + ")->state");

    var stackPtr = Debug.evaluate("(" + pfx + ")->stackPoint");
    var stackBase = Debug.evaluate("(" + pfx + ")->stackBase");
    var stackSize = Debug.evaluate("(" + pfx + ")->stackSize");
    var stackLimit = stackBase + stackSize;
    var stackUsed = stackLimit - stackPtr;

    var currPrio = Debug.evaluate("(" + pfx + ")->currPrio.priority");
    var initTick = Debug.evaluate("(" + pfx + ")->initTick");
    var remainTick = Debug.evaluate("(" + pfx + ")->remainTick");

    var prioStr = "" + currPrio;
    var tickStr = "" + remainTick + "/" + initTick;

    var entrySym = resolveSymbol(entry);

    Threads.add(
        resolveSymbol(nodeAddr),        // Thread address
        name,                           // Name
        entrySym,                       // Entry
        getThreadState(state),          // State
        stackUsed + "/" + stackSize,    // Stack
        prioStr,                        // Priority
        tickStr,                        // Ticks
        stackPtr                        // Register pointer for getregs()
    );
}

function getWaitThreadList(listHeadExpr) {
    var result = "";

    var listHead = Debug.evaluate(listHeadExpr);
    if ((listHead == undefined) || (listHead == 0)) {
        return result;
    }

    var listNode = Debug.evaluate("((MDS_DListNode_t *)" + listHead + ")->next");

    for (var i = 0; i < OBJECT_MAX_ITER; i++) {
        if ((listNode == undefined) || (listNode == 0) || (listNode == listHead)) {
            break;
        }

        if (result != "") {
            result += ",";
        }
        result += toHex(listNode);

        listNode = Debug.evaluate("((MDS_DListNode_t *)" + listNode + ")->next");
    }

    return result;
}

/* Device ------------------------------------------------------------------ */
var DEV_FLAG_CLOSE = 0x00;
var DEV_FLAG_OPEN = 0x80;
var DEV_FLAG_MODULE = 0x04;
var DEV_FLAG_ADAPTR = 0x02;
var DEV_FLAG_PERIPH = 0x01;

function getDeviceType(flagMask) {
    var result = ((flagMask & DEV_FLAG_OPEN) != 0) ? "Opened" : "Closed";

    if ((flagMask & DEV_FLAG_MODULE) != 0) {
        result += "|Module";
    } else if ((flagMask & DEV_FLAG_ADAPTR) != 0) {
        result += "|Adaptr";
    } else if ((flagMask & DEV_FLAG_PERIPH) != 0) {
        result += "|Peripheral";
    } else {
        result += "|Unknown";
    }

    return result;
}

function updateDevice(nodeAddr) {
    var pfx = "(MDS_Device_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var flagMask = Debug.evaluate("(" + pfx + ")->flag.mask");

    var driverStr = "";
    var handleStr = "";
    var ownerStr = "";

    if ((flagMask & DEV_FLAG_MODULE) != 0) {
        var modPfx = "((MDS_DevModule_t *)" + nodeAddr + ")";
        var driverAddr = Debug.evaluate(modPfx + "->driver");
        var handleAddr = Debug.evaluate(modPfx + "->handle.ptr");
        driverStr = resolveSymbol(driverAddr);
        handleStr = resolveSymbol(handleAddr);
    } else if ((flagMask & DEV_FLAG_ADAPTR) != 0) {
        var adpPfx = "((MDS_DevAdaptr_t *)" + nodeAddr + ")";
        var driverAddr = Debug.evaluate(adpPfx + "->driver");
        var handleAddr = Debug.evaluate(adpPfx + "->handle.ptr");
        var ownerAddr = Debug.evaluate(adpPfx + "->owner");
        driverStr = resolveSymbol(driverAddr);
        handleStr = resolveSymbol(handleAddr);
        ownerStr = resolveSymbol(ownerAddr);
    } else if ((flagMask & DEV_FLAG_PERIPH) != 0) {
        var perPfx = "((MDS_DevPeriph_t *)" + nodeAddr + ")";
        var mountAddr = Debug.evaluate(perPfx + "->mount");
        ownerStr = resolveSymbol(mountAddr);
    }

    Threads.add2(MDS_Object.DEVICE.label,
        resolveSymbol(nodeAddr), name,
        getDeviceType(flagMask),
        driverStr, handleStr, ownerStr);
}

/* WorkQueue --------------------------------------------------------------- */
function updateWorkQueue(nodeAddr) {
    var pfx = "(MDS_WorkQueue_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var thread = Debug.evaluate("(" + pfx + ")->thread");

    Threads.add2(MDS_Object.WORKQUEUE.label,
        resolveSymbol(nodeAddr), name, resolveSymbol(thread));
}

/* WorkNode ---------------------------------------------------------------- */
function updateWorkNode(nodeAddr) {
    var pfx = "(MDS_WorkNode_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var queue = Debug.evaluate("(" + pfx + ")->queue");
    var entry = Debug.evaluate("(" + pfx + ")->entry");
    var arg = Debug.evaluate("(" + pfx + ")->arg.ptr");
    var tickout = Debug.evaluate("(" + pfx + ")->tickout");
    var tperiod = Debug.evaluate("(" + pfx + ")->tperiod");

    Threads.add2(MDS_Object.WORKNODE.label,
        resolveSymbol(nodeAddr), name,
        resolveSymbol(queue),
        resolveSymbol(entry),
        resolveSymbol(arg),
        "" + tickout,
        "" + tperiod);
}

/* WorkNode ---------------------------------------------------------------- */
function updateSemaphore(nodeAddr) {
    var pfx = "(MDS_Semaphore_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var value = Debug.evaluate("(" + pfx + ")->value");
    var max = Debug.evaluate("(" + pfx + ")->max");

    var waitList = getWaitThreadList("&((" + pfx + ")->queueWait.list)");

    Threads.add2(MDS_Object.SEMAPHORE.label,
        resolveSymbol(nodeAddr), name,
        "" + value, "" + max,
        waitList);
}

/* Mutex ------------------------------------------------------------------- */
function updateMutex(nodeAddr) {
    var pfx = "(MDS_Mutex_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var value = Debug.evaluate("(" + pfx + ")->value");
    var nest = Debug.evaluate("(" + pfx + ")->nest");
    var owner = Debug.evaluate("(" + pfx + ")->owner");

    var waitList = getWaitThreadList("&((" + pfx + ")->queueWait.list)");

    Threads.add2(MDS_Object.MUTEX.label,
        resolveSymbol(nodeAddr), name,
        "" + value, "" + nest,
        resolveSymbol(owner),
        waitList);
}

/* Event ------------------------------------------------------------------- */
function updateEvent(nodeAddr) {
    var pfx = "(MDS_Event_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var value = Debug.evaluate("(" + pfx + ")->value.mask");

    var waitList = getWaitThreadList("&((" + pfx + ")->queueWait.list)");

    Threads.add2(MDS_Object.EVENT.label,
        resolveSymbol(nodeAddr), name,
        resolveSymbol(value),
        waitList);
}

/* MsgQueue ---------------------------------------------------------------- */
function updateMsgQueue(nodeAddr) {
    var pfx = "(MDS_MsgQueue_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var queBuff = Debug.evaluate("(" + pfx + ")->queBuff");
    var msgSize = Debug.evaluate("(" + pfx + ")->msgSize");

    // Count free blocks via lfree linked list
    var freeCount = 0;
    var lfree = Debug.evaluate("(" + pfx + ")->lfree");
    for (var i = 0; (i < OBJECT_MAX_ITER) && (lfree != undefined) && (lfree != 0); i++) {
        freeCount++;
        lfree = Debug.evaluate("((MDS_MsgQueueHeader_t *)" + lfree + ")->next");
    }

    // Count used blocks via lhead linked list
    var usedCount = 0;
    var lhead = Debug.evaluate("(" + pfx + ")->lhead");
    for (var i = 0; (i < OBJECT_MAX_ITER) && (lhead != undefined) && (lhead != 0); i++) {
        usedCount++;
        lhead = Debug.evaluate("((MDS_MsgQueueHeader_t *)" + lhead + ")->next");
    }

    // Waiting threads on recv and send queues
    var recvList = getWaitThreadList("&((" + pfx + ")->queueRecv.list)");
    var sendList = getWaitThreadList("&((" + pfx + ")->queueSend.list)");

    Threads.add2(MDS_Object.MSGQUEUE.label,
        resolveSymbol(nodeAddr), name,
        resolveSymbol(queBuff),
        "" + msgSize,
        "" + freeCount + "/" + (freeCount + usedCount),
        recvList, sendList);
}

/* MemPool ----------------------------------------------------------------- */
function updateMemPool(nodeAddr) {
    var pfx = "(MDS_MemPool_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var memBuff = Debug.evaluate("(" + pfx + ")->memBuff");
    var blkSize = Debug.evaluate("(" + pfx + ")->blkSize");

    // Count free blocks via lfree singly-linked list.
    // union MDS_MemPoolHeader { .next; .memPool; }
    // The free list is a stack: lfree -> ... -> memBuff (sentinel).
    // Terminate when lfree == NULL or lfree == memBuff.
    var freeCount = 0;
    var lfree = Debug.evaluate("(" + pfx + ")->lfree");
    for (var i = 0; i < OBJECT_MAX_ITER; i++) {
        if ((lfree == undefined) || (lfree == 0) || (lfree == memBuff)) {
            break;
        }
        freeCount++;
        lfree = Debug.evaluate("((union MDS_MemPoolHeader *)" + lfree + ")->next");
    }

    var usedCount = 0;
    var lhead = Debug.evaluate("(" + pfx + ")->lhead");
    for (var i = 0; i < OBJECT_MAX_ITER; i++) {
        if ((lhead == undefined) || (lhead == 0)) {
            break;
        }
        usedCount++;
        lhead = Debug.evaluate("((union MDS_MemPoolHeader *)" + lhead + ")->next");
    }

    var waitList = getWaitThreadList("&((" + pfx + ")->queueWait.list)");

    Threads.add2(MDS_Object.MEMPOOL.label,
        resolveSymbol(nodeAddr), name,
        resolveSymbol(memBuff),
        "" + blkSize,
        "" + freeCount + "/" + (freeCount + usedCount),
        waitList);
}

/* MemHeap ----------------------------------------------------------------- */
function updateMemHeap(nodeAddr) {
    var pfx = "(MDS_MemHeap_t *)" + nodeAddr;

    var name = Debug.evaluate("(char*)(" + pfx + ")->object.name");
    var ops = Debug.evaluate("(uintptr_t)(" + pfx + ")->ops");

    var memsize = 0;
    var memused = 0;

    var llff = Debug.evaluate("(uintptr_t)(&G_MDS_MEMHEAP_OPS_LLFF)");
    if (ops == llff) {
        var limit = Debug.evaluate("(uintptr_t)(" + pfx + ")->limit");
        var nlimit = Debug.evaluate("(uintptr_t)(&((MemHeapLLFF_Node_t*)" + limit + ")->next) + sizeof(uintptr_t)");
        var nbegin = Debug.evaluate("(uintptr_t)(((MemHeapLLFF_Node_t*)" + limit + ")->next)");
        var nfree = Debug.evaluate("(uintptr_t)(((MemHeapLLFF_Node_t*)" + limit + ")->prev)");

        memsize = nlimit - nbegin;
        memused = memsize - (nlimit - nfree);
    }

    Threads.add2(MDS_Object.MEMHEAP.label,
        resolveSymbol(nodeAddr), name,
        "" + memused + "/" + memsize);
}

/*********************************************************************
*
*       API Functions
*
**********************************************************************
*/

var g_arch = "unknown";

/*********************************************************************
*
*       init
*
* Function description
*   Initializes the task window
*/

function init() {
    g_arch = detectArchitecture();

    Threads.clear();

    // Main thread table
    Threads.setColumns("Thread", "Name", "Entry", "State",
        "Stack", "Priority", "Ticks");
    Threads.setSortByNumber("Priority");
    Threads.setColor("State", "Ready", "Running", "Blocking");

    // Secondary tables for kernel objects
    if (Threads.setColumns2) {
        Threads.setColumns2(MDS_Object.DEVICE.label,
            "Device", "Name", "Type", "Driver", "Handle", "Owner/Mount");
        Threads.setColumns2(MDS_Object.WORKQUEUE.label,
            "WorkQueue", "Name", "Thread");
        Threads.setColumns2(MDS_Object.WORKNODE.label,
            "WorkNode", "Name", "Queue", "Entry", "Arg", "TickOut", "Period");
        Threads.setColumns2(MDS_Object.SEMAPHORE.label,
            "Semaphore", "Name", "Value", "Max", "WaitingThreads");
        Threads.setColumns2(MDS_Object.MUTEX.label,
            "Mutex", "Name", "Value", "Nest", "Owner", "WaitingThreads");
        Threads.setColumns2(MDS_Object.EVENT.label,
            "Event", "Name", "Value", "WaitingThreads");
        Threads.setColumns2(MDS_Object.MSGQUEUE.label,
            "MsgQueue", "Name", "QueBuff", "MsgSize", "Count", "Recvers", "Senders");
        Threads.setColumns2(MDS_Object.MEMPOOL.label,
            "MemPool", "Name", "MemBuff", "BlkSize", "Count", "WaitingThreads");
        Threads.setColumns2(MDS_Object.MEMHEAP.label,
            "MemHeap", "Name", "Size");
    }
}

/*********************************************************************
*
*       update
*
* Function description
*   Updates the task window
*/

function update() {
    Threads.clear();

    // Always enumerate threads
    iterateObjects(MDS_Object.THREAD.index, updateThread);

    // Secondary tables (only if shown)
    if (Threads.shown(MDS_Object.DEVICE.label)) {
        iterateObjects(MDS_Object.DEVICE.index, updateDevice);
    }
    if (Threads.shown(MDS_Object.WORKQUEUE.label)) {
        iterateObjects(MDS_Object.WORKQUEUE.index, updateWorkQueue);
    }
    if (Threads.shown(MDS_Object.WORKNODE.label)) {
        iterateObjects(MDS_Object.WORKNODE.index, updateWorkNode);
    }
    if (Threads.shown(MDS_Object.SEMAPHORE.label)) {
        iterateObjects(MDS_Object.SEMAPHORE.index, updateSemaphore);
    }
    if (Threads.shown(MDS_Object.MUTEX.label)) {
        iterateObjects(MDS_Object.MUTEX.index, updateMutex);
    }
    if (Threads.shown(MDS_Object.EVENT.label)) {
        iterateObjects(MDS_Object.EVENT.index, updateEvent);
    }
    if (Threads.shown(MDS_Object.MSGQUEUE.label)) {
        iterateObjects(MDS_Object.MSGQUEUE.index, updateMsgQueue);
    }
    if (Threads.shown(MDS_Object.MEMPOOL.label)) {
        iterateObjects(MDS_Object.MEMPOOL.index, updateMemPool);
    }
    if (Threads.shown(MDS_Object.MEMHEAP.label)) {
        iterateObjects(MDS_Object.MEMHEAP.index, updateMemHeap);
    }
}

/*********************************************************************
*
*       getregs
*
* Function description
*   Returns the register set of a task.
*   For ARM cores, this function is expected to return the values
*   of registers R0 to R15 and PSR.
*
* Parameters
*   hTask: integer number identifiying the task.
*   Identical to the last parameter supplied to method Threads.add.
*   For convenience, this should be the address of the TCB.
*
* Return Values
*   An array of unsigned integers containing the task's register values.
*   The array must be sorted according to the logical indexes of the regs.
*   The logical register indexing scheme is defined by the ELF-DWARF ABI.
*
**********************************************************************
*/
function getregs(ThreadPtr) {
    if (g_arch == "riscv") {
        return getregsRiscV(ThreadPtr);
    } else if (g_arch == "arm") {
        return getregsArm(ThreadPtr);
    }

    return [];
}

/*********************************************************************
*
*       getContextSwitchAddrs
*
*  Functions description
*    Returns an unsigned integer array containing the base addresses
*    of all functions that complete a task switch when executed.
*/
function getContextSwitchAddrs() {
    var addr = Debug.evaluate("&MDS_KernelSchedulerCheck");

    if (addr != undefined) {
        return [addr];
    }

    return [];
}

/*********************************************************************
*
*       getOSName()
*
*  Functions description:
*    Returns the name of the RTOS this script supplies support for
*/
function getOSName() {
    return "MDS";
}
