/*	CPU Module Header file
	Maxwell Crawford
	2-24-16
*/

#define MAX_PAGE	16
#define MAX_PCB	50

typedef enum {
	false, true
} BOOL;

typedef enum {
	running, ready, waiting, done //status types!
} STATUS;

typedef enum {
	iosvc, devint,
	pagefault, startsvc,
	termsvc, killsvc,
	waitsvc, sigsvc, timeint //interrupt types
} INT_TYPE;

typedef enum {
	read, write //I/O request action types
} IO_ACTION;

//External types:
typedef struct page_entry_node PAGE_ENTRY;
typedef struct page_tbl_node PAGE_TBL;
typedef struct event_node EVENT;
typedef struct iorb_node IORB;
typedef struct pcb_node PCB;
typedef struct int_vector_node INT_VECTOR;

//External data structures:
struct page_entry_node {
	int frame_id;
	BOOL valid;
	int *hook;
};

struct page_tbl_node {
	PCB *pcb;
	PAGE_ENTRY page_entry[MAX_PAGE];
	int *hook;
};

typedef struct pcb_node {
    int    pcb_id;         /* PCB id                                        */
    int    size;           /* process size in bytes; assigned by SIMCORE    */
    int    creation_time;  /* assigned by SIMCORE                           */
    int    last_dispatch;  /* last time the process was dispatched          */
    int    last_cpuburst;  /* length of the previous cpu burst              */
    int    accumulated_cpu;/* accumulated CPU time                          */
    PAGE_TBL *page_tbl;    /* page table associated with the PCB            */
    STATUS status;         /* status of process                             */
    EVENT  *event;         /* event upon which process may be suspended     */
    int    priority;       /* user-defined priority; used for scheduling    */
    PCB    *next;          /* next pcb in whatever queue                    */
    PCB    *prev;          /* previous pcb in whatever queue                */
    int    *hook;          /* can hook up anything here                     */
} PCB;

struct int_vector_node {
	INT_TYPE cause;
	PCB *pcb;
	int page_id;
	int dev_id;
	EVENT *event;
	IORB *iorb;
};


//External var's:
extern PAGE_TBL *PTBR;          /* page table base register */
extern INT_VECTOR Int_Vector;

extern int    Quantum;          /* global time quantum; contains the value
                                   entered at the beginning or changed
                                   at snapshot. Has no effect on timer
                                   interrupts, unless passed to set_timer() */

extern int    ______trace_switch;

//External routines:
extern prepage(/*PCB*/);
extern int start_cost(/*PCB*/);

extern set_timer(); //for quantum
extern int get_clock(); //handy for getting time of dispatch

//Internal routines:
void cpu_init();
void dispatch();
void insert_ready(PCB *);