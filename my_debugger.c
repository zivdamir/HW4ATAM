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

void run_breakpoint_debugger(pid_t child_pid,long *address, int dyn_flag)
{
    int wait_status;
    int counter = 1;
    long addr = *address;
    wait(&wait_status);
    if(dyn_flag == 1)
        *address = ptrace(PTRACE_PEEKTEXT,child_pid,(void *)*address,NULL);
    struct user_regs_struct regs;

    // Insert breakpoint in the beginnig of function
    unsigned long orig_opcode = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)*address, NULL);
    unsigned long trap_opcode = (orig_opcode & 0xffffffffffffff00) | 0xcc;
    ptrace(PTRACE_POKETEXT,child_pid,(void *) *address,(void *)trap_opcode);
    ptrace(PTRACE_CONT,child_pid,NULL,NULL);
    wait(&wait_status);

    while(WIFSTOPPED(wait_status))
    {
        // Restore rip
        ptrace(PTRACE_GETREGS,child_pid,0,&regs);
        regs.rip -=1;
        ptrace(PTRACE_SETREGS,child_pid,0,&regs);
        unsigned long after_call_adress = ptrace(PTRACE_PEEKTEXT,child_pid,(void *)(regs.rsp),NULL);
        ptrace(PTRACE_POKETEXT,child_pid,(void *)*address,(void *)orig_opcode);

        // Insert breakpoint in the return address
        orig_opcode = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)after_call_adress, NULL);
        trap_opcode = (orig_opcode & 0xffffffffffffff00) | 0xcc;
        ptrace(PTRACE_POKETEXT,child_pid,(void *) after_call_adress,(void *)trap_opcode);
        //Child continue running
        ptrace(PTRACE_CONT,child_pid,NULL,NULL);
        wait(&wait_status);

        // Restore rip
        ptrace(PTRACE_GETREGS,child_pid,0,&regs);
        regs.rip -=1;
        ptrace(PTRACE_SETREGS,child_pid,0,&regs);
        //Print return value from function and inc counter
        printf("PRF:: run #%d returned with %d\n",counter,(int)regs.rax);
        counter ++;
        ptrace(PTRACE_POKETEXT,child_pid,(void *)after_call_adress,(void *)orig_opcode);

        // Insert breakpoint in the function
        if(dyn_flag == 1)
            *address = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)addr, NULL);
        orig_opcode = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)*address, NULL);
        trap_opcode = (orig_opcode & 0xffffffffffffff00) | 0xcc;
        ptrace(PTRACE_POKETEXT,child_pid,(void *) *address,(void *)trap_opcode);
        //Child continue running
        ptrace(PTRACE_CONT,child_pid,NULL,NULL);
        wait(&wait_status);
    }

    // Restore function orig_opcode
    ptrace(PTRACE_POKETEXT,child_pid,(void *)address,(void *)orig_opcode);

}

void deb(char** argv, unsigned long address, int dyn_flag) // in argv we dont send whole argv , just relevant arguments (from index 2 and so on)
{
    pid_t child_pid;
    char* exefile_name = argv[0];
    child_pid = run_target(exefile_name, argv);
    run_breakpoint_debugger(child_pid,& address, dyn_flag);
}
