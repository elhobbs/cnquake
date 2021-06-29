#include <nds.h>
#include <string.h>	// For memcpy
#include "IPCFifo.h"

#define ASSERT(...)
#define ASSERT_TEXT1(...)


// TODO: Incorporate these vars into the main TransferRegion struct
typedef struct _IPC_fifoVars
{
	u32 arm9FifoSent;       // Incremented when sending a command to ARM7
	u32 arm9FifoProcessed;  // Incremented after processing a command from ARM7
	u32 arm7FifoSent;       // Incremented when sending a command to ARM9
	u32 arm7FifoProcessed;  // Incremented after processing a command from ARM9

} IPC_fifoVars;

static inline
IPC_fifoVars volatile * getIPC_fifo() {
	return (IPC_fifoVars volatile *)(getIPC() + 1);
}


char volatile * getIPC_buffer() {
	return (char volatile *)(getIPC_fifo() + 1);
}


// ----- Constants -----

#define FIFO_WAIT_TIME   100	// Arbitrary number of cycles to wait between polling other CPU's vars.
#define FIFO_TIMEOUT     100	// Arbitrary number of times to try polling if the other CPU has finished processing.


// ----- Global variables -----

IPCFifoHandler FifoRecvTable[FIFO_SUBSYSTEM_NUM];


// ----- Local function prototypes -----

static void spinWait(int cycles);


//{ ----- Global functions -----

// --------------------
// Call once at startup.
//
void IPCFifoInit()
{
	irqSet(IRQ_FIFO_NOT_EMPTY, IPCFifoIrqHandler);
	irqEnable(IRQ_FIFO_NOT_EMPTY);

	REG_IPC_FIFO_CR = IPC_FIFO_SEND_CLEAR | IPC_FIFO_RECV_IRQ | IPC_FIFO_ERROR | IPC_FIFO_ENABLE;
}

// --------------------
void IPCFifoIrqHandler()
{
	static u32 staticBuffer[FIFO_MAX_WORDS + 1];
	static u32 bufferPos = 0;

	while(!(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY))
	{
		u32 message;
		u32 wordCount;

		ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));

		if (bufferPos == 0)
		{
			staticBuffer[bufferPos++] = REG_IPC_FIFO_RX;
			ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));
		}

		message = staticBuffer[0];
		wordCount = (message & FIFO_WORD_COUNT_MASK) >> FIFO_WORD_COUNT_SHIFT;

		while(!(REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY) && bufferPos < wordCount + 1)
		{
			staticBuffer[bufferPos++] = REG_IPC_FIFO_RX;
			ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));
		}

		if (bufferPos >= wordCount + 1)
		{
			u32 localBuffer[FIFO_MAX_WORDS];
			u32 command;
			IPCFifoHandler fifoHandler;

			command = (message & FIFO_COMMAND_MASK) >> FIFO_COMMAND_SHIFT;
			fifoHandler = FifoRecvTable[(message & FIFO_SUBSYSTEM_MASK) >> FIFO_SUBSYSTEM_SHIFT];

			if (wordCount == 0)
			{
				localBuffer[0] = staticBuffer[0] & FIFO_DATA_MASK;
				wordCount = 1;
			}
			else
			{
				memcpy(localBuffer, staticBuffer + 1, wordCount * sizeof(u32));
			}

			bufferPos = 0;

			if (fifoHandler)
			{
				fifoHandler(command, localBuffer, wordCount);
			}
		}
	}

	#ifdef ARM9
		getIPC_fifo()->arm9FifoProcessed++;
	#else
		getIPC_fifo()->arm7FifoProcessed++;
	#endif
}

// --------------------
// Call to set handler functions for each subsystem.
//
void IPCFifoSetHandler(u32 subsystem, IPCFifoHandler handler)
{
	ASSERT(subsystem < FIFO_SUBSYSTEM_NUM);
	FifoRecvTable[subsystem] = handler;
}

// --------------------
// Send a single word over the FIFO, containing the command and 20 bits of optional embedded data.
//
// Returns false if the transfer fails.
//
bool IPCFifoSendWord(u32 subsystem, u32 command, u32 data)
{
	u32 message = IPCFifoSendWordAsync(subsystem, command, data);
	return IPCFifoWaitMessage(message);
}

// --------------------
// Send a command over the FIFO with multiple words of data.
//
// Returns false if the transfer fails.
//
bool IPCFifoSendMulti(u32 subsystem, u32 command, const u32 *data, u32 wordCount)
{
	u32 message = IPCFifoSendMultiAsync(subsystem, command, data, wordCount);
	return IPCFifoWaitMessage(message);
}

// --------------------
// Send a single word over the FIFO, containing the command and 20 bits of optional embedded data.
// Returns immediately after sending. Normally should be very fast, unless the FIFO was already completely full.
//
// Returns message tag that can be passed to IPCFifoCheckMessageDone, etc.
//
u32 IPCFifoSendWordAsync(u32 subsystem, u32 command, u32 data)
{
	u32 oldIME = REG_IME;

	// Check arguments
	ASSERT_TEXT1((subsystem & ~(FIFO_SUBSYSTEM_MASK >> FIFO_SUBSYSTEM_SHIFT)) == 0, "subsystem: 0x%x\n", subsystem);
	ASSERT_TEXT1((command & ~(FIFO_COMMAND_MASK >> FIFO_COMMAND_SHIFT)) == 0, "command: 0x%x\n", command);
	ASSERT_TEXT1((data & ~FIFO_DATA_MASK) == 0, "data: 0x%x\n", data);

	REG_IME = 0;

	ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));

	while(REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL)
	{
		REG_IME = oldIME;
		REG_IME = 1;
	}

	REG_IPC_FIFO_TX = FIFO_PACK_CMD(subsystem, command, 0, data);
	ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));

	REG_IME = oldIME;

#ifdef ARM9
	getIPC_fifo()->arm9FifoSent++;
	return getIPC_fifo()->arm9FifoSent;
#else
	getIPC_fifo()->arm7FifoSent++;
	return getIPC_fifo()->arm7FifoSent;
#endif
}

// --------------------
// Send a command over the FIFO with multiple words of data. Returns as soon as all words 
// have gone into the buffer. Not guaranteed to return quickly, as the FIFO could already 
// be full, in which case the other CPU has to process some commands first.
//
// Returns message tag that can be passed to IPCFifoCheckMessageDone, etc.
//
u32 IPCFifoSendMultiAsync(u32 subsystem, u32 command, const u32 *data, u32 wordCount)
{
	u32 wordsSent = 0;
	u32 oldIME = REG_IME;

	// Check arguments
	ASSERT_TEXT1((subsystem & ~(FIFO_SUBSYSTEM_MASK >> FIFO_SUBSYSTEM_SHIFT)) == 0, "subsystem: 0x%x\n", subsystem);
	ASSERT_TEXT1((command & ~(FIFO_COMMAND_MASK >> FIFO_COMMAND_SHIFT)) == 0, "command: 0x%x\n", command);
	ASSERT_TEXT1(wordCount < FIFO_MAX_WORDS, "maxWords: %d\n", wordCount);

	REG_IME = 0;

	ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));

	while(!(REG_IPC_FIFO_CR & IPC_FIFO_SEND_EMPTY))
	{
		REG_IME = oldIME;
		REG_IME = 1;
	}

	REG_IPC_FIFO_TX = FIFO_PACK_CMD(subsystem, command, wordCount, 0);
	ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));

	while(wordsSent < wordCount)
	{
		REG_IPC_FIFO_TX = data[wordsSent++];
		ASSERT(!(REG_IPC_FIFO_CR & IPC_FIFO_ERROR));
	}

	REG_IME = oldIME;

#ifdef ARM9
	getIPC_fifo()->arm9FifoSent++;
	return getIPC_fifo()->arm9FifoSent;
#else
	getIPC_fifo()->arm7FifoSent++;
	return getIPC_fifo()->arm7FifoSent;
#endif
}

// --------------------
// Check if the other CPU has finished processing a command that was sent.
// Message value is returned by IPCFifoSendAsync functions.
//
// Returns true if processing has completed.
//
bool IPCFifoCheckMessageDone(u32 message)
{
#ifdef ARM9
	u32 processed = getIPC_fifo()->arm7FifoProcessed;
#else
	u32 processed = getIPC_fifo()->arm9FifoProcessed;
#endif

	if (processed >= message)
	{
		return TRUE;
	}
	else if (message - processed > 0x80000000)	// Assume wraparound if difference is large
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// --------------------
// Wait until the other CPU has finished processing a command that was sent.
// Message value is returned by IPCFifoSendAsync functions.
//
// Returns false if processing takes too long.
//
bool IPCFifoWaitMessage(u32 message)
{
	u32 tries = 0;

	while(!IPCFifoCheckMessageDone(message))
	{
		// Wait an arbitrary amount of time for 
		// the other CPU to handle the message.
		spinWait(FIFO_WAIT_TIME);
		if (++tries >= FIFO_TIMEOUT)
			return FALSE;	// It's been too long, give up.
	}

	return TRUE;
}

//} ----- End global functions -----


//{ ----- Local functions -----

// --------------------
void spinWait(int cycles)
{
	while(cycles > 0)
	{
		// Theoretically the loop compiles to subs, bgt, which is 4 cycles per iteration
		cycles -= 4;
	}
}

//} ----- End local functions -----
