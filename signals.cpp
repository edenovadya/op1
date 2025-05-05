#include <iostream>
#include <signal.h>
#include "signals.h"
#include <syscall.h>
#include <unistd.h>
#include <bits/syscall.h>

#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    std::cout<<"smash: got ctrl-C"<<endl;
    pid_t fgPid = SmallShell::getInstance().get_current_pid_fg();
    if (fgPid != pid_t(-1)) {
        if (syscall(SYS_kill, fgPid, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        std::cout<<"smash: process " << fgPid << " was killed" << std::endl;
        SmallShell::getInstance().set_current_pid_fg(-1);
    }
    // TODO: Add your implementation
}
