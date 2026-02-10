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
