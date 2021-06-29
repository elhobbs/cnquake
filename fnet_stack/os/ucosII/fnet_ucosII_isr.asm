
#include "fnet_comp_asm.h"

#if FNET_CFG_OS && FNET_CFG_OS_UCOSII && FNET_MCF

;*************************************************************************************************
;                                       PUBLIC DECLARATIONS
;*************************************************************************************************

	FNET_COMP_ASM_GLOBAL    FNET_COMP_ASM_PREFIX(fnet_os_isr)
	
;**************************************************************************************************
;                                     EXTERNAL DECLARATIONS
;**************************************************************************************************
	FNET_COMP_ASM_EXTERN    FNET_COMP_ASM_PREFIX(fnet_isr_handler)
	FNET_COMP_ASM_EXTERN    FNET_COMP_ASM_PREFIX(IntpStackTOS)
	FNET_COMP_ASM_EXTERN    FNET_COMP_ASM_PREFIX(OSIntNesting)
	FNET_COMP_ASM_EXTERN    FNET_COMP_ASM_PREFIX(OSTCBCur)


	FNET_COMP_ASM_CODE
	FNET_COMP_ASM_ALIGN 4
	
/************************************************************************
* NAME: fnet_os_isr
*
* DESCRIPTION: This handler is executed on every FNET interrupt 
*              (from ethernet and timer module).
*              Extracts vector number and calls fnet_isr_handler().
*************************************************************************/	
FNET_COMP_ASM_PREFIX(fnet_os_isr):

	STRLDSR.W	#0x2700                 ;Disable interrupts, but store SR on stack first

	lea      	(-60,a7),a7
	movem.l  	d0-d7/a0-a6,(a7) 
	MOVEA		A7,A0                   ;Save off original SP for later use
	
        move.l          (64,a7),d0                      ; Extract the irq number from 
        lsr.l           #8,d0                           ; The exception stack frame
        lsr.l           #8,d0     
        lsr.l           #2,d0
        and.l           #0xff,d0                        ;D0 holds arg for fnet_isr_handler

	MOVEQ.L		#0,D1				         ; OSIntNesting++
	MOVE.B		(FNET_COMP_ASM_PREFIX(OSIntNesting)),D1
	ADDQ.L		#1,D1
	MOVE.B		D1,(FNET_COMP_ASM_PREFIX(OSIntNesting))
	CMPI.L		#1, D1				         ; if (OSIntNesting == 1)
	BNE.B		FNET_COMP_ASM_PREFIX(fnet_isr_call)

;----------------------------------------
;	Nesting=1, then we've interrupted a task, not another interrupt
	MOVEA.L		(FNET_COMP_ASM_PREFIX(OSTCBCur)), A1    ; Save SP in OSTCBCur->OSTCBStkPtr
	MOVE.L		A7,(A1)
									
	;To use an interrupt-specific stack frame, must init SP to point to intp stack.
	;since this is only executed at top level of nested intps, TOS will always be at
	;beginning of stack space.
	MOVEA.L		(IntpStackTOS),A1
	LEA.L		(4,A1),A7			; Point to 1 beyond TOS as it will be pre-decremented when used
;----------------------------------------

FNET_COMP_ASM_PREFIX(fnet_isr_call):
	MOVE.W		(62,A0),D1			;Get saved status register back from original stack
	MOVE.W		D1,SR				;to re-enable higher priority interrupts
										
	JSR             (FNET_COMP_ASM_PREFIX(fnet_isr_handler))

	MOVE.W		#0x2700,SR			; Disable interrupts again, will be re-enabled by RTE
	MOVEQ.L		#0,D1				;if(OSIntNesting > 0) OSIntNesting--
	MOVE.B		(FNET_COMP_ASM_PREFIX(OSIntNesting)),D1
	BEQ             FNET_COMP_ASM_PREFIX(fnet_isr_exit)
	
	SUBQ		#1,D1
	MOVE.B		D1,(FNET_COMP_ASM_PREFIX(OSIntNesting))
	
	BNE             FNET_COMP_ASM_PREFIX(fnet_isr_exit)     ;if OSIntNesting now 0,			

	MOVEA.L		(FNET_COMP_ASM_PREFIX(OSTCBCur)),A1     ;Get Task SP back
	MOVEA.L		(A1),A7                                 ;into SP
	
fnet_isr_exit:
	MOVEM.L		(A7),D0-D7/A0-A6	; Restore processor registers from stack
	LEA             (64,A7),A7              ; offset of '64' is to discard SR saved on stack by STLDSR
	RTE                                     ; Return to task or nested ISR

#endif /*FNET_CFG_OS && FNET_CFG_OS_UCOSII && FNET_MCF*/

	FNET_COMP_ASM_END