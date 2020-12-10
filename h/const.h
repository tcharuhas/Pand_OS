#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/


/* MAXPROC */
#define MAXPROC			  20

/* Hardware & software constants */
#define PAGESIZE		  4096			/* page size in bytes	*/
#define WORDLEN			  4				  /* word size in bytes	*/

/* General constants */
#define NUMDEVS		    49	/* NUMDEVS = (8 of each device * 5 device types) + 8 (because each term has 2 semaphores) + 1 (for the clock) */
#define OK				1
#define ON              1
#define OFF             0
#define DISABLE         0xFFFFFFFF
#define ERROR			-1
#define EMPTY			-1

	/* bit manipulation constants */
/* -------------------------------------- */

/* Status reg manipulation */
#define	ALLOFF			0x00000000
#define PBIT_USER_ON	0x00000008
#define PBIT_IE_ON		0x00000004
#define CBIT_IE_ON		0x00000001
#define FIELD_IM_ON		0x0000FF00
#define BIT_TE_ON		0x08000000
#define DISABLEINT		0xFFFFFFFE

/* Cause reg manipulation */
#define FETCH_EXCCODE	    0x0000007C
#define SHIFT_CAUSE		    2
#define	INTERRUPT_IO	    0
#define	INTERRUPT_TLB	    3
#define	INTERRUPT_SYSCALL   8
#define INTERRUPT_CHECK     0x00000200
#define INTERRUPT_LOCALT    0x00000200
#define INTERRUPT_TIMER     0x00000400
#define INTERRUPT_DISK      0x00000800
#define INTERRUPT_FLASH     0x00001000
#define INTERRUPT_PRINT     0x00004000
#define INTERRUPT_TERM      0x00008000
#define INTERRUPT_CHECKEND  0x00008000
#define SHIFT_INTER         1

/* -------------------------------------- */

/* Syscalls */
#define	CREATEPROC		1
#define KILLPROC		2		
#define PASSEREN		3
#define	VERHOGEN		4
#define IOWAIT			5
#define GETCPUTIME		6
#define CLOCKWAIT		7
#define GETSUPPORT		8
#define TERMINATE		9
#define GETTOD			10
#define WRITEPRINTER	11
#define WRITETERM		12
#define READTERM		13


/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR		0x10000000
#define RAMBASESIZE		0x10000004
#define TODLOADDR		0x1000001C
#define INTERVALTMR		0x10000020	
#define TIMESCALEADDR	0x10000024
#define PLTSLICE		5
#define CLOCK 			(NUMDEVS-1) /* the clock is the last semaphore on the list */
#define INTERVALTIME    100000
#define LOCALTIME  		5000

/* utility constants */
#define	TRUE			    1
#define	FALSE			    0
#define HIDDEN			  static
#define EOS				    '\0'

#define NULL 			    ((void *)0xFFFFFFFF)

/* device interrupts */
#define DISKINT			  3
#define FLASHINT 		  4
#define NETWINT 		  5
#define PRNTINT 		  6
#define TERMINT			  7
#define MAXDEVNUM		  7
#define DEVICE_CHECK    0x00000001

#define DISK0ADDR		  0x10000054
#define LINESCALE			  128
#define DEVSCALE		  16

#define DEVINTNUM		  5		  /* interrupt lines used by devices */
#define DEVPERINT		  8		  /* devices per interrupt line */
#define DEVREGLEN		  4		  /* device register field length in bytes, and regs per dev */	
#define DEVREGSIZE	  16 		/* device register size in bytes */
#define READYBIT        0x0F
#define SHIFT_DEV       2

/* device register field number for non-terminal devices */
#define STATUS			  0
#define COMMAND			  1
#define DATA0			    2
#define DATA1			    3

/* device register field number for terminal devices */
#define RECVSTATUS  	0
#define RECVCOMMAND 	1
#define TRANSTATUS  	2
#define TRANCOMMAND 	3

/* device common STATUS codes */
#define UNINSTALLED		0
#define READY			    1
#define BUSY			    3

/* device common COMMAND codes */
#define RESET			    0
#define ACK				    1

/* Memory related constants */
#define KSEG0           0x00000000
#define KSEG1           0x20000000
#define KSEG2           0x40000000
#define KUSEG           0x80000000
#define RAMSTART        0x20000000
#define BIOSDATAPAGE    0x0FFFF000
#define	PASSUPVECTOR	0x0FFFF900
#define STACK    		0x20001000

/* Exceptions related constants */
#define	PGFAULTEXCEPT	  0
#define GENERALEXCEPT	  1


/* Support constants */
#define UPROCMAX	8


/* Phase3 */
#define VPN_SHIFT	12
#define ASID_SHIFT	6
#define MAXPGNUM 	32
#define FLSH_POOL_STRT	(RAMSTART + (MAXPGNUM * PAGESIZE))
#define DSK_POOL_START	(FLSH_POOL_STRT + (DEVPERINT * PAGESIZE))
#define FRME_POOL_STRT	(DSK_POOL_START + (DEVPERINT * PAGESIZE))
#define TLBINVLOAD 	2
#define TLBINVSTORE 3
#define GETVPN		0x3FFFF000
#define GETASID		0x00000FC0
#define WRITEFLASH 	3
#define READFLASH 	2
#define V_BIT_ON	0x00000200
#define D_BIT_ON	0x00000400
#define SWAPSIZE 	(UPROCMAX*2)
#define SYS 		8
#define CHAR_TRANSMIT	2
#define CHAR_TRANSOK	5
#define BYTE_LENGTH		8
#define PRESENT_SHIFT	31
#define IE_BIT_OFF		0x1
#define V_BIT_OFF		0xFFFFFDFF
#define MAXSTRLENGTH	128
#define STATUSBIT		0xFF
#define ASCIINEWLINE     0x0A
#define UPROCADDR		0x800000B0
#define STACKADDR		0xC0000000
#define	UPROCSTRT		0x80000
#define PAGESTACK       0xBFFFF

/* operations */
#define	MIN(A,B)		((A) < (B) ? A : B)
#define MAX(A,B)		((A) < (B) ? B : A)
#define	ALIGNED(A)		(((unsigned)A & 0x3) == 0)

/* Macro to load the Interval Timer */
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

/* Macro to read the TOD clock */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define RAMTOP(T) ((T) = ((* ((int *) RAMBASEADDR)) + (* ((int *) RAMBASESIZE))))

#endif
