/* Code sample: using ptrace for simple tracing of a child process.
**
** Note: this was originally developed for a 32-bit x86 Linux system; some
** changes may be required to port to x86-64.
**
** Eli Bendersky (http://eli.thegreenplace.net)
** This code is in the public domain.
*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>


pid_t run_target(const char* programname, char** argv)
{
	pid_t pid;
	
	pid = fork();
	
    if (pid > 0) {
		return pid;
		
    } else if (pid == 0) {
		/* Allow tracing of this process */
		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			perror("ptrace");
			exit(1);
		}
		/* Replace this process's image with the given program */
		execv(programname, argv);
		
	} else {
		// fork error
		perror("fork");
        exit(1);
    }
}

void run_breakpoint_debugger(pid_t child_pid, unsigned long address)
{
    int wait_status;
    struct user_regs_struct regs;
    int count = 1;
    /* Wait for child to stop on its first instruction */
    wait(&wait_status);

    /* Look at the word at the address we're interested in */
    long addr = address;
    unsigned long func_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, NULL); //original opcode where breakpoint is going to be set
    unsigned long data_trap = (func_data & 0xFFFFFFFFFFFFFF00) | 0xCC;//change opcode to have breakpoint
    unsigned long return_from_func_address, return_from_func_data, return_from_data_trap;//
    ptrace(PTRACE_POKETEXT, child_pid, (void *) addr, (void *) data_trap);
    ptrace(PTRACE_CONT, child_pid, 0, 0); //continue to the function
    wait(&wait_status);
    while(!WIFEXITED(wait_status))
    {
        /* Write the trap instruction 'int 3' into the address */
        /* Let the child run to the breakpoint and wait for it to reach it */
        ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)func_data);
        ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
        //take rsp, then peekdata in offset of 0 from rsp
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);
        return_from_func_address = ptrace(PTRACE_PEEKDATA, child_pid, (void*)regs.rsp, NULL);
//put breakpoint in the wanted location - return address from the function.
        return_from_func_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)return_from_func_address, NULL);

        return_from_data_trap = (return_from_func_data & 0xFFFFFFFFFFFFFF00) | 0xCC;
        ptrace(PTRACE_POKETEXT, child_pid, (void *) return_from_func_address, (void *) return_from_data_trap);
        /* The child can continue running now */
        ptrace(PTRACE_CONT, child_pid, 0, 0); //continue to the function
        //wait until return from the function
        wait(&wait_status);
        ptrace(PTRACE_POKETEXT, child_pid, (void *) return_from_func_address, (void *) return_from_func_data);
        /* See where the child is now */
        ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
        //take rsp, then peekdata in offset of 0 from rsp
        printf("PRF:: run #%d returned with %d\n", count, (int)regs.rax);
        regs.rip -= 1;
        ptrace(PTRACE_SETREGS, child_pid, 0, &regs);
        count++;
        ptrace(PTRACE_POKETEXT, child_pid, (void *) addr, (void *) data_trap);
        ptrace(PTRACE_CONT, child_pid, 0, 0); //continue to the function
        wait(&wait_status);
    }

}

//void run_syscall_debugger(pid_t child_pid)
//{
//    int wait_status;
//
//    /* Wait for child to stop on its first instruction */
//    wait(&wait_status);
//
//	struct user_regs_struct regs;
//	/* Enter next system call */
//	ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
//	wait(&wait_status);
//
//	ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
//	regs.rdx = 5;
//	ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
//
//	/* Run system call and stop on exit */
//	ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
//	wait(&wait_status);
//
//	ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
//	printf("DBG: the syscall returned: %d\n", regs.rax);
//
//	/* The child can continue running now */
//    ptrace(PTRACE_CONT, child_pid, 0, 0);
//    wait(&wait_status);
//    if (WIFEXITED(wait_status)) {
//        printf("DBG: Child exited\n");
//    } else {
//        printf("DBG: Unexpected signal\n");
//    }
//}

//void run_regs_override_debugger(pid_t child_pid)
//{
//    int wait_status;
//
//    /* Wait for child to stop on its first instruction */
//    wait(&wait_status);
//    while (WIFSTOPPED(wait_status)) {
//        struct user_regs_struct regs;
//
//        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
//		regs.rdx = 5;
//		ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
//
//        /* Make the child execute another instruction */
//        if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) < 0) {
//            perror("ptrace");
//            return;
//        }
//
//        /* Wait for child to stop on its next instruction */
//        wait(&wait_status);
//    }
//}

//void run_instruction_debugger(pid_t child_pid)
//{
//    int wait_status;
//    int icounter = 0;
//
//    /* Wait for child to stop on its first instruction */
//    wait(&wait_status);
//    while (WIFSTOPPED(wait_status)) {
//        icounter++;
//        struct user_regs_struct regs;
//
//        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
//        unsigned long instr = ptrace(PTRACE_PEEKTEXT, child_pid, regs.rip, NULL);
//
//        printf("DBG: icounter = %u.  RIP = 0x%x.  instr = 0x%08x\n",
//                    icounter, regs.rip, instr);
//
//        /* Make the child execute another instruction */
//        if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) < 0) {
//            perror("ptrace");
//            return;
//        }
//
//        /* Wait for child to stop on its next instruction */
//        wait(&wait_status);
//    }
//}

//void run_counter_debugger(pid_t child_pid)
//{
//    int wait_status;
//    int icounter = 0;
//
//    /* Wait for child to stop on its first instruction */
//    wait(&wait_status);
//    while (WIFSTOPPED(wait_status)) {
//        icounter++;
//
//        /* Make the child execute another instruction */
//        if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) < 0) {
//            perror("ptrace");
//            return;
//        }
//
//        /* Wait for child to stop on its next instruction */
//        wait(&wait_status);
//    }
//
//    printf("DBG: the child executed %d instructions\n", icounter);
//}

void deb(char** argv, unsigned long address) // in argv we dont send whole argv , just relevant arguments (from index 2 and so on)
{
    pid_t child_pid;
    char* exefile_name = argv[0];
    child_pid = run_target(exefile_name, argv);
    run_breakpoint_debugger(child_pid, address);
}

