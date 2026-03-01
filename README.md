# STAGE 13 UPDATES
- Module
Parts of the eXpOS kernel that implements code for certain standard repetitive tasks like scheduling, managing resources, buffer etc. are implemented as separate subroutines called modules
since both caller and callee are in protected mode both uses the same kernel stack
NOTE that a module will always be invoked in kernel mode (either by any interrup/exception scheduler or by  another module)
When a CALL is made the IP of the next instruction is pushed to the top of the kernel stack 
While returning we return to the kernel mode itself so we use RET and IRET
Each module may have multiple functions which are going to be identified by function numbers - Passed as an argument R1
R0 - return value

- Boot module
os startup code - 1 page only so we require another module for all heavy OS initializations

OS Startup Code (page 1 only)
│
├── Create IDLE process
├── Set SP to IDLE kernel stack
├── Load Boot Module (Module 7)
├── Call Boot Module
│     ├── Initialize OS data structures
│     ├── Create INIT & other processes
│     ├── Load interrupt handlers
│     └── Load kernel modules
│
├── Return from Boot Module
└── Start IDLE process in user mode

why cant we run init first - NO cuz Because the kernel needs IDLE’s context before INIT can cause interrupts, system calls, or blocking.

### so  what if INIT runs first !!
Let’s walk through the exact failure case

Assume:

Boot module finished

Everything is loaded

INIT runs first

IDLE has never run

Now this happens:

Timeline

1️⃣ INIT starts executing in user mode
2️⃣ INIT makes a system call (even a simple one later stages)
3️⃣ INIT blocks
4️⃣ Kernel must schedule another process
5️⃣ Only choice = IDLE
6️⃣ Kernel tries to switch to IDLE

💥 But IDLE has never run → no valid context to restore

This failure has nothing to do with loading.
Everything was already loaded by the boot module.

Why Stage 12 “worked” then?

Because in Stage 12:

INIT cannot block
The only context switch happens via timer
Timer handler has special CREATED-process logic
IDLE’s first execution is carefully handled
That safety net is temporary and removed later.

so basically we are just using the kernel stack of our idle process to load the boot module cause it can only be invoked in kernel mode and then once loaded go back to os startup code to run the idle user code and considering the timer interrupt we run the init as well. not that the idle never ran before the module we just used its stack 

### why are we using idle first
Problem 1: INIT is not special enough

INIT:

Is a normal user program

-Mayblock
-May exit
-May use system calls
-Is not guaranteed to be always runnable

IDLE:

-Infinite loop
-Never blocks
-Never exits
-Always safe fallback

# STAGE 14

### Round Robin
- Preemptive scheduling algorithm
- fixed time slice
- a time quantum is the maximum cpu time a process can use at once


so basically we created a third process and explained how scheduler swictches the stacks

let a P1 be running and it faced a timer interrupt - switches to kernel mode (stack)- saves all the reg and return address to the kernel stack - calls scheduler -also pushed another return address to come back to the interrupt from the scheduler - saves SP,PTBR,PTLR of the current process in ProcessT - then scheduler choosed the P2 - loads new sp ptbr ptlr - now we have P2s kernel stack - note that P2s kernel stack will contain the return address from its previous timer interrupt(resume from where in timer interrupt i mean it will exactlty container the addrs of instruction after scheduler) - yeah now so return from scheduler to interrupt and then ireturn - other wise if the P2 was in CREATED state then we can directly use ireturn 

Why cant we directly ireturn from scheduler if P2 had some return address ?

ireturn is only correct when the top of the stack contains a USER-MODE context. here the top of stack contains the return add to the interrupt which is in KERNEL-MODE so NO


# STAGE 15

A process req many process like disk,terminal,inode --> therefor we need a Resource Manager (MODULE 0) . A process has to acquire the req resource by invoking the RM . If the resource is not available then it has to be blocked while some other process runs


TTS - contains STATUS AND PID (status indicted wheter terminal is available or not pid stores the pid of the process using the terminal )

Acquire module = function number 8 and Release module = function number 9 this function num is passedd as argument (R1) along with PID (R2) 

User Program
    ↓
write system call
    ↓
Terminal Write (Device Manager)
    ↓
Acquire Terminal (Resource Manager)
    ↓
Print word (R3)
    ↓
Release Terminal (Resource Manager)

save the register before invoking modules

### busy loop
the acquire terminal loop constantly invoke the scheduler if the resource is not free. Cause many processes are waiting at the time and whne the resource is available the scheduler assigns some P to get the resource while “the rest may run later and incorrectly assume the resource is free if no re-check is performed” creates issue . therefore we have a busy loop which consntly check if resource availble if not call scheduler else lock the the resource
w/o loop
if (terminal not available)
    wait once;
acquire terminal;   // no re-check

with loop
while (terminal is busy) 
    invoke scheduler;
lock terminal;
Without a busy wait, a process waits once and blindly proceeds; with a busy wait, a process repeatedly re-checks the resource condition and proceeds only when it is safe.

## how did it work last time when we just used print in INT 7 
## when we call multipush & multipop just becz we are in kernel mode and kernl stack it will automatically be using tht?



# stage 16

memory mapped io - Same address space is shared by RAM + I/O devices.
io mapped io (port mapped io) - I/O devices use a separate address space different from memory.

The driver for each device is assumed to be pre-loaded into the disk. modern system allows add/remove device drivers while the OS is running, without restarting the computer.

- when an interrupt occurs and sp + 1 is outside the page table 
First attempt failed due to bad stack → OS fixes the issue → CPU retries interrupt entry → ensures reliable interrupt handling.

### Exception handling
1. Illegal Memory Access - addrs generated is outside the logical address space ( also when write bit is not set in pagetable)
2. illegal instruction - like mov ip 2
3. Arithmetic exception - div / % by 0
4. page fault - logical address is within the range but valid bit is 0

Demand paging or lazy memory allocation 
Demand paging is a memory management technique in which pages are loaded into main memory only when they are actually required (on demand) rather than loading the entire program in advance.

EIP = logical IP that caused the exception
EPN = the page num thaht causd the page fault
EC = pageflt(0) , illegal instr(1) , illegal mem acc(2), arthmintc (3)
EMA = the illegal memory access addr

### IN
reading a word from console - machine proceed with the next instruction w/o wiating for console to cmplt reading . once finished stores word to port P0 and get a console o/p

Why READ is Privileged
Direct access to hardware (I/O devices)
READ may interact with disk, keyboard, console, etc.
Hardware must be controlled by OS.
If user programs access devices directly → conflicts, corruption.

1. Fd - a samll integer used by os to track which file ur prgm is reading/writing (many procces will be accessing the same file st the same time and fd identifies one open instance of a file in a process )
2. fp - from which position in the file the next read should happen
3. buffer - data is copied to buffer

### console vs terminal vs shell
The console is the basic input/output interface, the terminal is a software interface that provides access to the console, and the shell is a command interpreter program that runs inside the terminal to execute user commands.

IN instruction called -> current inst -> waiting and scheduler


# STAGE 17
A succesfull exec program kills the progrm that invoked it (not killing i mean a new code content will be pasted and thus the orginal code's code is lost and note that they have the same pid)

Process A: old program
  VPage 0 → PFrame 101 (code)
  VPage 1 → PFrame 102 (heap)
  VPage 2 → PFrame 103 (stack)

exec("new_program") is called
  ↓
Old pages freed
  ↓
Page table invalidated:
  VPage 0 → invalid
  VPage 1 → invalid
  VPage 2 → invalid
New program
  VPage 0 → PFrame 201 (new code)
  VPage 1 → PFrame 202 (new heap)
  VPage 2 → PFrame 203 (new stack)

Exec calls EXIT PROCESS from PROCESS_MANAGER(MOD_1) for deallocation
Exit process relearse the user are page - exec system call - kernel mode - therefore after exit process uses the new process's user area page - then allocate heap stack and data( depend on the num of block in the inode table loaded using LOADI) - getting new page using GET FREE PAGE from MEMORY MANAGER MOD

- Memory free list in page 57 of memory gives info about the number of processes each page shared

- exit process (fn no = 3)
dealloactes all pages - then dealloctes all pages in the page table ( free page table ) - free user area page - state = terminated;
- free page table (fn no = 2)
every valid entry in page table is made free by involing (RELEASE PAGE in mem mod) except the lib pages since its shared by all processes
- free user area page (fn no = 4)
usig release page - The mapping is removed, but the physical content is still sitting in RAM (for now). so return address and all is still there - this happens becuse release page is non blcoking meaning there is no context switch or the released page is not immediately used
- release page (fun num = 3 for mem manager mod)
decrements the number in the corrs page in mem free list, in the sst hold the num of available mem free - if any one wait for mem that process must be made ready on the availability of free mem pages
- get free page 
go thru the memory list to find a free page and if found increment the count
if no mem page availabe the process has to be made to WAITING STAGE scheduler called and when mem page available it should be made to READY



