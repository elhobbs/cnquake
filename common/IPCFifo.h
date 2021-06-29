// ----- Change log -----
// 16-Mar-2006 - Initial implementation of FIFO transfer system.
// 15-Jun-2007 - Revamped to use a more flexible command format, and support multi-word transfers.
// ----------------------

#ifndef IPC_FIFO_H
#define IPC_FIFO_H

#include <nds.h>


// ----- Constants -----

// Fifo command format is bit 0..19 data, 20-23 word count, 24-28 command, 29-31 sybsystem
#define FIFO_SUBSYSTEM_SHIFT  29
#define FIFO_COMMAND_SHIFT    24
#define FIFO_WORD_COUNT_SHIFT 20
#define FIFO_SUBSYSTEM_MASK   0xe0000000
#define FIFO_COMMAND_MASK     0x1f000000
#define FIFO_WORD_COUNT_MASK  0x00f00000
#define FIFO_DATA_MASK        0x000fffff
#define FIFO_MAX_WORDS        15

#define FIFO_PACK_CMD(subsystem, command, wordCount, data) \
	(((data) & FIFO_DATA_MASK) | \
	(((wordCount) << FIFO_WORD_COUNT_SHIFT) & FIFO_WORD_COUNT_MASK) | \
	(((command)   << FIFO_COMMAND_SHIFT)    & FIFO_COMMAND_MASK)    | \
	(((subsystem) << FIFO_SUBSYSTEM_SHIFT)  & FIFO_SUBSYSTEM_MASK))

enum
{
	FIFO_SUBSYSTEM_SOUND,
	FIFO_SUBSYSTEM_MICROPHONE,
	FIFO_SUBSYSTEM_INPUT,
	FIFO_SUBSYSTEM_POWER,
	FIFO_SUBSYSTEM_RTC,
	FIFO_SUBSYSTEM_WIFI,
	FIFO_SUBSYSTEM_USER2,
	FIFO_SUBSYSTEM_USER3,

	FIFO_SUBSYSTEM_NUM,
};


// ----- Structures -----

typedef void (*IPCFifoHandler)(u32 command, const u32 *data, u32 wordCount);


// ----- Global functions -----

void IPCFifoInit();
void IPCFifoIrqHandler();
void IPCFifoSetHandler(u32 subsystem, IPCFifoHandler handler);
bool IPCFifoSendWord(u32 subsystem, u32 command, u32 data);
bool IPCFifoSendMulti(u32 subsystem, u32 command, const u32 *data, u32 wordCount);
u32 IPCFifoSendWordAsync(u32 subsystem, u32 command, u32 data);
u32 IPCFifoSendMultiAsync(u32 subsystem, u32 command, const u32 *data, u32 wordCount);
bool IPCFifoCheckMessageDone(u32 message);
bool IPCFifoWaitMessage(u32 message);

char volatile * getIPC_buffer();
// ----------

#endif	// IPC_FIFO_H
