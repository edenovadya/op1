#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"
#include <cstring>

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }


    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getChprompt() <<"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(strdup(cmd_line.c_str()));

    }
    return 0;
}