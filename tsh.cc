// 
// tsh - A tiny shell program with job control
// 
// Irvin Carbajal 104643719
//

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

//
// main - The shell's main routine 
//
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}
  
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//
void eval(char *cmdline) 
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
 
  //EVALUATES THE INPUT AND CHECKS IF ITS A BUILT IN COMMAND
    
  char *argv[MAXARGS];
  
  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //
    
  int bg = parseline(cmdline, argv); 
       
    pid_t pid;      //create a process id 
    sigset_t mask;  //creates signal set
    
    if(!builtin_cmd(argv)) 
    //fork and execs a child process if not a builtin_cmd. 
    //If it is, it continues to the builtin_cmd function.
    { 
        
        // blocking first
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        
        //check for valid fork
        if((pid = fork()) < 0){
            unix_error("forking error");
        }
        // checks if its a child process 
        else if(pid == 0) {
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            setpgid(0, 0); 
            //this sets the process group id for the fg processes
            //checks if command is there
            if(execvp(argv[0], argv) < 0)
            {
                printf("%s: Command not found\n", argv[0]);
                exit(1);
                //prints error message if command not found
                //if the command isnt found then it exits and doesnt return 
            }
        } 
        
        else //means its a parent  
        {
            if(!bg) //enters if the bg is false meaning its a fg process
            {
                addjob(jobs, pid, FG, cmdline);
                //adds job to fg struct
            }
            else //means its a bg process, so fg is false 
            {
                addjob(jobs, pid, BG, cmdline);
                //add job to bg struct
            }
            sigprocmask(SIG_UNBLOCK, &mask, NULL); //singal process
            
            //if bg/fg
            if (!bg){
                //wait for fg
                waitfg(pid);
                //pauses the parent process
                //awaits for child process and reaps it to avoid non functional process
            } 
            else 
            {
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
                //prints once job is added
            }
        }
    }
    

  return;
}



/////////////////////////////////////////////////////////////////////////////
//
// builtin_cmd - If the user has typed a built-in command then execute
// it immediately. The command name would be in argv[0] and
// is a C string. We've cast this to a C++ string type to simplify
// string comparisons; however, the do_bgfg routine will need 
// to use the argv array as well to look for a job number.
//
int builtin_cmd(char **argv) 
{
  
    if (!strcmp(argv[0], "quit")) // goes into an if statement unless its not a builtin command
    {
        exit(0);//quits
    }
    else if (!strcmp("&", argv[0])) //continues
    {
        return 1; 
    }
    else if (!strcmp("jobs", argv[0])) // calls jobs function
    {  
        listjobs(jobs);  
        return 1;  
    }  
    else if (!strcmp("bg", argv[0]) || !(strcmp("fg", argv[0]))) //calls tbe bgfg function
    {
        do_bgfg(argv);  
        return 1;  
    }  
    return 0;
    //if not a biultin command, shell creates child process
    //it executes the requested program inside the child
    

}

/////////////////////////////////////////////////////////////////////////////
//
// do_bgfg - Execute the builtin bg and fg commands
//
void do_bgfg(char **argv) 
{
    

  struct job_t *jobp=NULL;
    
  
  if (argv[1] == NULL) //checks if id exists 
  {
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
  if (isdigit(argv[1][0]))
  {
    pid_t pid = atoi(argv[1]);
    if (!(jobp = getjobpid(jobs, pid))) {
      printf("(%d): No such process\n", pid);
      return;
    }
  }
  else if (argv[1][0] == '%') {
    int jid = atoi(&argv[1][1]);
    if (!(jobp = getjobjid(jobs, jid))) {
      printf("%s: No such job\n", argv[1]);
      return;
    }
  }	    
  else {
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    return;
  }

  //
  // You need to complete rest. At this point,
  // the variable 'jobp' is the job pointer
  // for the job ID specified as an argument.
  //
  // Your actions will depend on the specified command
  // so we've converted argv[0] to a string (cmd) for
  // your benefit.
  //
    

    
   kill(-jobp -> pid, SIGCONT); //kills each time
    
    if(!strcmp("fg", argv[0])) 
    {
        //wait for fg
        jobp -> state = FG;
        //changes the state to fg
        waitfg(jobp -> pid);
        //makes fg wait
        //avoid more than one fg process running at once
    } 
    else{
        //print for bg
        printf("[%d] (%d) %s", jobp->jid, jobp->pid, jobp->cmdline);
        jobp->state = BG; //changes the state to bg
    } 
    

  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// waitfg - Block until process pid is no longer the foreground process
//
void waitfg(pid_t pid)
{
  while (pid == fgpid(jobs))
  {
      
    struct job_t* job;
    job = getjobpid(jobs,pid);
      
      
    if(pid == 0) //continues if pid is valid
    {
        return;
    }
    if(job != NULL)
    {
        while(pid==fgpid(jobs)) //sleeps until itd not a fg pid then continues
        {
            
        }
        
    }
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// Signal handlers
//


/////////////////////////////////////////////////////////////////////////////
//
// sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
//     a child job terminates (becomes a zombie), or stops because it
//     received a SIGSTOP or SIGTSTP signal. The handler reaps all
//     available zombie children, but doesn't wait for any other
//     currently running children to terminate.  
//
void sigchld_handler(int sig)
    //function that tells us there is a child 
{
    
     int status;  
    pid_t pid;  
    
    while ((pid = waitpid(fgpid(jobs), &status, WNOHANG|WUNTRACED)) > 0)
    //we use waitpid to wait until the child is done executing
    //stores a status value in the status int
    //pid stores the child process requested from caller
    //if pid is -1, this loop waits until the child process is over before reaping them 
    //WNHOANG function checks the child processes w/o suspending the caller
    //in WNHOANG function theres a process w/o status info available -> waitpid returns 0
    // which is why while loop contiues until it returns >0 before we delete/reap a child
    
    {  
        if (WIFSTOPPED(status))
        //changes state if stopped
        //WIFSTOPPED returns true is stopped.
        { 
            getjobpid(jobs, pid) -> state = ST; 
            int jid = pid2jid(pid);
            
            printf("Job [%d] (%d) Stopped by signal %d\n", jid, pid, WSTOPSIG(status));
        } 
        else if (WIFSIGNALED(status)) 
        //catches ctrl-c, delete is signaled
        {
            int jid = pid2jid(pid);  
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, WTERMSIG(status));
            deletejob(jobs, pid);
        }  
        else if (WIFEXITED(status))
        {  
            deletejob(jobs, pid);  
        }  
    }  
    
    
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
void sigint_handler(int sig) 
{
    pid_t pid = fgpid(jobs);  
    
    //checks for valid pid
    if (pid != 0) 
    {     
        kill(-pid, sig); //terminates
    }   
    return;   
    
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig) 
{
     pid_t pid = fgpid(jobs); 
    
    //check for valid pid
    if (pid != 0) { 
        kill(-pid, sig);  //terminates
    }  

    
  return;
}

/*********************
 * End signal handlers
 *********************/




