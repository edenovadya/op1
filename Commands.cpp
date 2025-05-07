#include "Commands.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <complex>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <set>
#include <regex>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/route.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <pwd.h>
#include <sys/syscall.h>
#include <stdint.h>
extern char **environ;

#include <cstdint>

struct linux_dirent64 {
    uint64_t        d_ino;
    int64_t         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

using namespace std;
#define BUFFER_SIZE 4096
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}
void symbols_cleanup(const std::string &s,string* output) {
    output[0] = s;

    size_t pos_redirection = s.find('>');
    size_t pos_redirection2 = s.find('>',pos_redirection + 1);
    size_t pos_pipe = s.find('|');
    size_t pos_pipe_end = s.find('&',pos_pipe + 1);


    if(s.find('>') != std::string::npos){
        output[0] = s.substr(0,pos_redirection);
        output[1] = s.substr(pos_redirection2 ==
        pos_redirection + 1?pos_redirection + 2:pos_redirection + 1);
    }
    if(s.find('|') != std::string::npos){
        output[0] = s.substr(0,pos_pipe);
        output[1] = s.substr(pos_pipe_end == pos_pipe + 1?pos_pipe + 2:pos_pipe + 1);
    }
    return;
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(SmallShell::getInstance().alias_preparse_Cmd(cmd_line)));
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}


bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

bool _isComplexCommand(const char* cmd_line) {
    std::string str(cmd_line);
    return str.find('*') != std::string::npos || str.find('?') != std::string::npos;
}

bool _isBackgroundComamndForString(const std::string& cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void _removeBackgroundSignForString(std::string& cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

std::string SmallShell::alias_preparse_Cmd(const char *cmd_line) const{
    string trimmed = _trim(string(cmd_line));
    istringstream iss(trimmed);
    string firstWord;
    iss >> firstWord;

    string firstWord_cleanup[2];
    symbols_cleanup(firstWord,firstWord_cleanup);
    firstWord = firstWord_cleanup[0];

    string restOfLine = firstWord_cleanup[1];
    getline(iss, restOfLine);
    restOfLine = _trim(restOfLine);

    if(restOfLine.empty()){
        _removeBackgroundSignForString(firstWord);
    }

    if (find_alias(firstWord)) {
        firstWord = get_alias(firstWord);
    }

    string newCommandLine = firstWord + " " + restOfLine;
    return newCommandLine;
}


bool isNumber(const std::string &s) {
    if (s.empty()) {
        return false;
    }
    for (int i = 0; i < s.length(); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}

bool containsArrow(const std::string &s) {

    if (s.empty()) {
        return false;
    }
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == '>' && s[i + 1] == '>') {
            return true;
        }
    }
    return false;
}

bool containsTwoArrow(const std::string &s) {
    if (s.empty()) {
        return false;
    }
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != '>' && s[i+1] == '>' && s[i + 2] != '>') {
            return false;
        }
    }
    return true;
}

bool isNumberWithDash(const std::string &s) {
    if (s.empty()) {
        return false;
    }
    for (int i = 1; i < s.length(); i++) {
        if (!isdigit(s[i]) && s[0] == '-') {
            return false;
        }
    }
    return true;
}


bool contains(const std::string &s1,const std::string &s2){
    return s1.find(s2) != std::string::npos;
}


Command::Command(const char *cmd_line):cmd_line(cmd_line) {}


SmallShell::SmallShell() : chprompt("smash"), last_dir(""), current_pid_fg(-1) {
    char cwd[COMMAND_MAX_LENGTH];
    if (syscall(SYS_getcwd, cwd, COMMAND_MAX_LENGTH) == -1) {
        std::perror("smash error: getcwd failed");
    } else {
        current_dir = std::string(cwd);
    }
}

Command *SmallShell::CommandByFirstWord(const char *cmd_line){
    string cmd_s = alias_preparse_Cmd(_trim(string(cmd_line)).c_str());
    _removeBackgroundSignForString(cmd_s);
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new Chprompt(cmd_line);
        std::cout <<"CommandByFirstWord"<< cmd_line << std::endl;

    }else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);

    }else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);

    }else if(firstWord.compare("cd") == 0){
        return new ChangeDirCommand(cmd_line, &last_dir);

    }else if(firstWord.compare("jobs") == 0){
        return new JobsCommand(cmd_line,&jobs);

    }else if(firstWord.compare("fg") == 0){
        return new ForegroundCommand(cmd_line,&jobs);

    }else if(firstWord.compare("quit") == 0){
        return new QuitCommand(cmd_line,&jobs);

    }else if(firstWord.compare("kill") == 0){
        return new KillCommand(cmd_line,&jobs);

    }else if(firstWord.compare("alias") == 0){
        return new AliasCommand(cmd_line);

    }else if(firstWord.compare("unalias") == 0){
        return new UnAliasCommand(cmd_line);

    }else if(firstWord.compare("unsetenv") == 0){
        return new UnSetEnvCommand(cmd_line);

    }else if(firstWord.compare("watchproc") == 0){
        return new WatchProcCommand(cmd_line);

    }else if(firstWord.compare("du") == 0){
        return new DiskUsageCommand(cmd_line);

    }else if(firstWord.compare("whoami") == 0){
        return new WhoAmICommand(cmd_line);

    }else if(firstWord.compare("netinfo") == 0){
    return new NetInfo(cmd_line);

    }else if(!string(cmd_line).empty()) {
        return new ExternalCommand(cmd_line);
    }else{
        return nullptr;
    }

}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));

    size_t first_gt = cmd_s.find('>');
    size_t second_gt = cmd_s.find('>', first_gt + 1);
    bool is_valid_redirection = (first_gt != string::npos && second_gt == string::npos)
                                || (second_gt == first_gt + 1 && cmd_s.find('>', second_gt + 1) == string::npos);


    if(is_valid_redirection){
        string* command = new string(_trim(cmd_s.substr(0,first_gt)));
        return new RedirectionCommand(cmd_line,CommandByFirstWord(command->c_str()));
    }else if(contains(cmd_s,"|")){
        int pos = cmd_s.find("|");
        bool isCerr = contains(cmd_s,"|&");

        string temp = cmd_s.substr(0,pos);
        _removeBackgroundSignForString(temp);
        string* first_command = new string(_trim(temp));

        temp = cmd_s.substr(isCerr?pos+2:pos+1);
        _removeBackgroundSignForString(temp);
        string* second_command = new string(_trim(temp));

        Command* first = CommandByFirstWord(first_command->c_str());
        Command* second = CommandByFirstWord(second_command->c_str());

        return new PipeCommand(cmd_line,first,second, isCerr);

    }else{
        return CommandByFirstWord(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
     if(string(cmd_line).empty()){
         return;
     }
     Command* cmd = CreateCommand(cmd_line);
     cmd->execute();
     //delete cmd;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

//todo: chprompt
Chprompt::Chprompt(const char *cmd_line) : BuiltInCommand(cmd_line) {
}

void SmallShell::setChprompt(std::string newChprompt) {
    chprompt = newChprompt;
}

std::string SmallShell::getChprompt() const {
    return chprompt;
}

void Chprompt::execute() {
    char* args[20];
    string line = _trim(string(cmd_line));

    _removeBackgroundSignForString(line);

   _parseCommandLine(line.c_str(),args);

    if (args[1] == nullptr){
        SmallShell::getInstance().setChprompt();
    }else{
        SmallShell::getInstance().setChprompt(args[1]);
    }
}


//todo: showpid command
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute()  {
    pid_t pid = syscall(SYS_getpid);
    if (pid < 0) {
        perror("smash error: getpid failed");
    }
    std::cout << "smash pid is " << pid << std::endl;
}

//todo: GetCurrDirCommand
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute()  {
    char* cwd = new char[COMMAND_MAX_LENGTH];

    if (syscall(SYS_getcwd, cwd, COMMAND_MAX_LENGTH) == -1) {
        std::perror("smash error: getcwd failed");
        delete[] cwd;
        return;
    }

    std::cout << cwd << std::endl;
    delete[] cwd;
}

//todo:ChangeDirCommand
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, std::string* plastPwd) : BuiltInCommand(cmd_line) , plastPwd(plastPwd) {}
void ChangeDirCommand::execute() {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);

    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    if (num_of_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    if (num_of_args == 1) {
        char cwd[COMMAND_MAX_LENGTH];
        if (syscall(SYS_getcwd, cwd, sizeof(cwd)) == -1) {
            perror("smash error: getcwd failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }
        //*plastPwd = std::string(cwd);
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    // num_of_args == 2
    if (strcmp(args[1], "-") == 0) {
        if (plastPwd->empty()) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        char curr_dir[COMMAND_MAX_LENGTH];
        if (syscall(SYS_getcwd, curr_dir, sizeof(curr_dir)) == -1) {
            perror("smash error: getcwd failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        if (syscall(SYS_chdir, plastPwd->c_str()) == -1) {
            perror("smash error: chdir failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        *plastPwd = std::string(curr_dir);
    } else {
        char curr_dir[COMMAND_MAX_LENGTH];
        if (syscall(SYS_getcwd, curr_dir, sizeof(curr_dir)) == -1) {
            perror("smash error: getcwd failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        if (syscall(SYS_chdir, args[1]) == -1) {
            perror("smash error: chdir failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        *plastPwd = std::string(curr_dir);
    }

    for (int i = 0; i < num_of_args; i++) {
        free(args[i]);
    }
}


//todo: Job Command
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs) {}

void JobsCommand::execute(){
    jobs->printJobsList();
}

//todo: fg command
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
        : BuiltInCommand(cmd_line),jobs(jobs) {}

void ForegroundCommand::execute() {
    string line = string(cmd_line);
    _removeBackgroundSignForString(line);
    char* args[20];
    int arg_num = _parseCommandLine(line.c_str(),args);
    JobsList::JobEntry* job;

    if(arg_num == 1 && jobs->isJobsListEmpty()) {
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        return;
    }else if (arg_num == 1){
        job = jobs->getLastJob(jobs->get_max_job_id());
    }

    if(arg_num >= 2) {
        if(arg_num > 2 || !isNumber(args[1])){
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
        char* end;
        long val = strtol(args[1], &end, 10);

        job = jobs->getJobById(val);
        if(jobs->getJobById(val) == nullptr){
            std::cerr << "smash error: fg: job-id " <<val <<" does not exist" << std::endl;
            return;
        }
    }
    pid_t pid = job->getPid();
    SmallShell::getInstance().set_current_pid_fg(pid);
    if(waitpid(pid,nullptr,0) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    SmallShell::getInstance().set_current_pid_fg(-1);
    jobs->removeJobById(job->getJobId());
}

//todo:Quit Command
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line) , jobs(jobs) {}
void QuitCommand::execute()  {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);
    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    if (num_of_args >= 2 && strcmp(args[1], "kill") == 0) {
        jobs->KillForQuitCommand();
        for(int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        syscall(SYS_exit, 0);
    }

    if (num_of_args >= 1) {
        for(int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        syscall(SYS_exit, 0);
    }
}

void JobsList::KillForQuitCommand() {
    this->removeFinishedJobs();
    std::cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << std::endl;
    for (auto job : jobs) {
        std::cout << job.getPid() << ": " << job.getCmd()->getCmdLine() << std::endl;
        if (syscall(SYS_kill, job.getPid(), SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    if (jobs.size() == 0) {
        max_job_id = 0;
    } else {
        max_job_id = findNewMaxJobId();
    }
}

int JobsList::findNewMaxJobId() const {
    int max = 0;
    for (auto job : jobs) {
        if (max<job.getJobId()) {
            max = job.getJobId();
        }
    }
    return max;
}

//todo: KillCommand
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) , jobs(jobs) {}
void KillCommand::execute() {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);
    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    if (num_of_args != 3) {
        std::cout << "smash error: kill: invalid arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    if (!isNumber(args[2]) || !isNumberWithDash(args[1])) {
        std::cout << "smash error: kill: invalid arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    //checkinf for valid numbers
    int job_id = stoi(args[2]);
    int signal_number = stoi(args[1]);

    //check if job id exists
    JobsList::JobEntry *job = jobs->getJobById(job_id);
    if (job == NULL) {
        std::cout << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    cout << "signal number " << signal_number << " was sent to pid " << job->getPid() << endl;
    // if (signal_number > -1 || signal_number < -31 || job_id < 0) {
    //     std::cout << "smash error: kill: invalid arguments" << std::endl;
    //     for (int i = 0; i < num_of_args; i++) {
    //         free(args[i]);
    //     }
    //     return;
    // }
        if (syscall(SYS_kill, job->getPid(), abs(signal_number)) == -1) {
                    perror("smash error: kill failed");
                    for (int i = 0; i < num_of_args; i++) {
                        free(args[i]);
                    }
                    return;
                }

            if (abs(signal_number) == SIGKILL) {
                jobs->removeJobById(job_id);
            }

            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

//todo: alias command
AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void AliasCommand::execute() {
    string line = _trim(SmallShell::getInstance()
            .alias_preparse_Cmd(cmd_line));
    _removeBackgroundSignForString(line);
    char *args[20];
    int arg_num = _parseCommandLine(line.c_str(), args);
    std::string alias;
    std::string alias_value;

    size_t eq_pos = line.find('=');
    size_t alias_pos = line.find("alias");
    size_t wrap_pos = line.find('\'');
    size_t wrap_pos2 = line.find('\'',wrap_pos + 1);


    alias = _trim(line.substr(alias_pos+5,eq_pos-(alias_pos+5)));
    alias_value = _trim(line.substr(wrap_pos+1,wrap_pos2-(wrap_pos+1)));

    //check if reserved or exiting
    if(SmallShell::getInstance().find_alias(alias)|| SmallShell::getInstance
            ().isBuiltInCommand(alias.c_str()) || (access(alias.c_str(), X_OK)
            == 0 && arg_num != 1)){
        std::cerr << "smash error: alias: " <<alias<< " already exists or is a reserved command" << std::endl;
        return;
    }
    regex pattern(R"(^alias [a-zA-Z0-9_]+='[^']*'$)");
    if (!regex_search(_trim(line), pattern) && arg_num != 1) {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }
    if(arg_num == 1){
        SmallShell::getInstance().print_alias();
    }else{
        SmallShell::getInstance().set_alias(alias,alias_value);
    }
}

bool SmallShell::find_alias(std::string alias) const{
    for (const auto& a : this->alias) {
        if (a.first == alias){
            return true;
        }
    }
    return false;
}
std::string SmallShell::get_alias(std::string& alias) const{
    for (auto a : this->alias) {
        if (a.first == alias){
            return a.second;
        }
    }
    return "";
}

void SmallShell::set_alias(std::string& alias,std::string& command) {
    this->alias.emplace_back(alias,command);
}

//todo: unalias command
UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void UnAliasCommand::execute() {
    string line = string(cmd_line);
    _removeBackgroundSignForString(line);
    line = _trim(line);
    char *args[20];
    int arg_num = _parseCommandLine(line.c_str(), args);
    if (arg_num < 2){
        std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    }
    for (int i = 1; i < arg_num; ++i) {
        if(SmallShell::getInstance().find_alias(args[i])){
            SmallShell::getInstance().remove_alias(args[i]);
        }else{
            std::cerr << "smash error: unalias: " << args[i] << " alias does not exist"
            << std::endl;
            return;
        }
    }
}

void SmallShell::remove_alias(std::string alias) {
    for (auto it = this->alias.begin(); it != this->alias.end(); ++it) {
        if (it->first == alias){
            this->alias.erase(it);
            return;
        }
    }
}

//todo: UnSetEnvCommand
UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void UnSetEnvCommand::execute()  {
    std::string cmd_string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);

    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    if (num_of_args <= 1) {
        std::cerr << "smash error: unsetenv: not enough arguments" << std::endl;
        for (int i = 0; i < num_of_args; ++i) free(args[i]);
        return;
    }

    for (int i = 1; i < num_of_args; ++i) {
        const char* var_name = args[i];
        bool found = false;

        int fd = open("/proc/self/environ", O_RDONLY);
        if (fd == -1) {
            perror("smash error: cannot open /proc/self/environ");
            for (int j = 0; j < num_of_args; ++j) free(args[j]);
            return;
        }

        constexpr size_t BUF_SIZE = 8192;
        char buffer[BUF_SIZE];
        ssize_t bytes_read = read(fd, buffer, BUF_SIZE);
        close(fd);

        if (bytes_read <= 0) {
            perror("smash error: failed to read /proc/self/environ");
            for (int j = 0; j < num_of_args; ++j) free(args[j]);
            return;
        }

        size_t pos = 0;
        while (pos < (size_t)bytes_read) {
            const char* entry = &buffer[pos];
            size_t len = strlen(entry);
            if (strncmp(entry, var_name, strlen(var_name)) == 0 && entry[strlen(var_name)] == '=') {
                found = true;
                break;
            }
            pos += len + 1;
        }

        if (!found) {
            std::cerr << "smash error: unsetenv: " << var_name << " does not exist" << std::endl;
            continue;
        }

        std::vector<char*> new_env;
        for (int j = 0; environ[j] != nullptr; ++j) {
            if (strncmp(environ[j], var_name, strlen(var_name)) == 0 &&
                environ[j][strlen(var_name)] == '=') {
                continue; // skip the variable to delete
            }
            new_env.push_back(environ[j]);
        }
        new_env.push_back(nullptr);

        for (size_t j = 0; j < new_env.size(); ++j) {
            environ[j] = new_env[j];
        }
    }

    for (int i = 0; i < num_of_args; ++i) {
        free(args[i]);
    }

}

//todo:WatchProcCommand
WatchProcCommand::WatchProcCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

bool is_pid(int pid) {
    std::string procPath = "/proc/" + std::to_string(pid);
    return (access(procPath.c_str(), F_OK) == 0);
}

double get_memory_usage_ffor_command(int pid) {
    std::string statusPath = "/proc/" + std::to_string(pid) + "/status";
    int fd = open(statusPath.c_str(), O_RDONLY);
    if (fd == -1) return 0.0;

    char buffer[BUFFER_SIZE];
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (n <= 0) return 0.0;

    buffer[n] = '\0';
    std::istringstream iss(buffer);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream lineStream(line.substr(6));
            double kb;
            lineStream >> kb;
            return kb / 1024.0;
        }
    }

    return 0.0;
}

long long getCPUtime() {
    std::ifstream file("/proc/stat");
    if (!file) return 0;

    std::string line;
    std::getline(file, line);
    if (line.rfind("cpu ", 0) != 0) return 0;

    std::istringstream iss(line.substr(4));
    long long sum = 0, val;
    for (int i = 0; i < 8 && iss >> val; ++i) {
        sum += val;
    }
    return sum;
}

long long get_process_time(pid_t pid) {
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(statPath);
    if (!file) return 0;

    std::string token;
    long long utime = 0, stime = 0;
    for (int i = 1; file >> token; ++i) {
        if (i == 14) utime = std::stoll(token);
        else if (i == 15) {
            stime = std::stoll(token);
            break;
        }
    }
    return utime + stime;
}

double getCPUusage(int pid) {
    long long total_before = getCPUtime();
    long long proc_before = get_process_time(pid);
    sleep(1);
    long long total_after = getCPUtime();
    long long proc_after = get_process_time(pid);

    long long total_delta = total_after - total_before;
    long long proc_delta = proc_after - proc_before;

    return (total_delta > 0) ? (100.0 * proc_delta / total_delta) : 0.0;
}

int Command::how_many_words_in_line() const {
    std::istringstream iss(cmd_line);
    return std::distance(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>());
}

std::string Command::get_word_in_place(int index) const {
    std::istringstream iss(cmd_line);
    std::string word;
    int i = 0;
    while (iss >> word) {
        if (i++ == index) return word;
    }
    throw std::out_of_range("Index out of range in get_word_in_place");
}

void WatchProcCommand::execute() {
    try {
        if (how_many_words_in_line() != 2) {
            std::cerr << "smash error: watchproc: invalid arguments" << std::endl;
            return;
        }

        std::string pidStr = get_word_in_place(1);
        if (!isNumber(pidStr)) {
            std::cerr << "smash error: watchproc: invalid arguments" << std::endl;
            return;
        }

        int pid = std::stoi(pidStr);
        if (!is_pid(pid)) {
            std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
            return;
        }

        double memMB = get_memory_usage_ffor_command(pid);
        double cpuPercent = getCPUusage(pid);

        std::cout << std::fixed << std::setprecision(1)
                  << "PID: " << pid
                  << " | CPU Usage: " << cpuPercent << "%"
                  << " | Memory Usage: " << memMB << " MB"
                  << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "smash error: watchproc: invalid arguments" << std::endl;
    }
}



// void WatchProcCommand::execute() {
//     char* args[COMMAND_MAX_ARGS];
//     string line = _trim(cmd_line);
//     _removeBackgroundSignForString(line);
//     int num_of_args = _parseCommandLine(line.c_str(), args);
//     if (num_of_args != 2 || !isNumber(args[1])) {
//         write(STDERR_FILENO, "smash error: watchproc: invalid arguments\n", 42);
//         for (int i = 0; i < num_of_args; i++) free(args[i]);
//         return;
//     }
//
//     pid_t pid = atoi(args[1]);
//     char buf[512];
//
//     if (syscall(SYS_kill, pid, 0) == -1) {
//         if (errno == ESRCH) {
//             int len = snprintf(buf, sizeof(buf), "smash error: watchproc: pid %d does not exist\n", pid);
//             write(STDERR_FILENO, buf, len);
//         } else {
//             perror("smash error: kill failed");
//         }
//         for (int i = 0; i < num_of_args; i++) free(args[i]);
//         return;
//     }
//
//     long clk_ticks = sysconf(_SC_CLK_TCK);
//     if (clk_ticks == -1) {
//         perror("smash error: sysconf failed");
//         for (int i = 0; i < num_of_args; i++) free(args[i]);
//         return;
//     }
//
//     // Get initial process and system times
//     long t0_proc_time, t0_total_time;
//     if (!getProcessCPUTime(pid, &t0_proc_time) || !getTotalCPUTime(&t0_total_time)) {
//         for (int i = 0; i < num_of_args; i++) free(args[i]);
//         return;
//     }
//
//     sleep(1); // delta time window
//
//     // Get updated process and system times
//     long t1_proc_time, t1_total_time;
//     if (!getProcessCPUTime(pid, &t1_proc_time) || !getTotalCPUTime(&t1_total_time)) {
//         for (int i = 0; i < num_of_args; i++) free(args[i]);
//         return;
//     }
//
//     double delta_proc = t1_proc_time - t0_proc_time;
//     double delta_total = t1_total_time - t0_total_time;
//     double cpu_usage = (delta_proc / delta_total) * 100.0;
//
//     double memory_mb = getMemoryUsageMB(pid);
//
//     int len = snprintf(buf, sizeof(buf),
//         "PID: %d | CPU Usage: %.1f%% | Memory Usage: %.1f MB\n",
//         pid, cpu_usage, memory_mb);
//     write(STDOUT_FILENO, buf, len);
//
//     for (int i = 0; i < num_of_args; i++) free(args[i]);
// }
//
// bool WatchProcCommand::getProcessCPUTime(pid_t pid, long* total_time) {
//     char path[64];
//     snprintf(path, sizeof(path), "/proc/%d/stat", pid);
//     int fd = syscall(SYS_open, path, O_RDONLY);
//     if (fd == -1) return false;
//
//     char buffer[1024];
//     ssize_t bytes = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);
//     syscall(SYS_close, fd);
//     if (bytes <= 0) return false;
//     buffer[bytes] = '\0';
//
//     // parse fields 14 and 15 (utime and stime)
//     char* token = buffer;
//     for (int i = 0; i < 13; ++i) token = strchr(token, ' ') + 1;
//     long utime = atol(token);
//     token = strchr(token, ' ') + 1;
//     long stime = atol(token);
//
//     *total_time = utime + stime;
//     return true;
// }
//
// bool WatchProcCommand::getTotalCPUTime(long* total_time) {
//     int fd = syscall(SYS_open, "/proc/stat", O_RDONLY);
//     if (fd == -1) return false;
//
//     char buffer[1024];
//     ssize_t bytes = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);
//     syscall(SYS_close, fd);
//     if (bytes <= 0) return false;
//     buffer[bytes] = '\0';
//
//     if (strncmp(buffer, "cpu ", 4) != 0) return false;
//
//     long user, nice, system, idle, iowait, irq, softirq, steal;
//     sscanf(buffer + 4, "%ld %ld %ld %ld %ld %ld %ld %ld",
//         &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
//     *total_time = user + nice + system + idle + iowait + irq + softirq + steal;
//     return true;
// }
//
// double WatchProcCommand::getMemoryUsageMB(pid_t pid) {
//     char path[64], buffer[1024];
//     snprintf(path, sizeof(path), "/proc/%d/status", pid);
//     int fd = syscall(SYS_open, path, O_RDONLY);
//     if (fd == -1) return 0;
//
//     ssize_t bytes = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);
//     syscall(SYS_close, fd);
//     if (bytes <= 0) return 0;
//     buffer[bytes] = '\0';
//
//     char* line = buffer;
//     while (line && *line) {
//         if (strncmp(line, "VmRSS:", 6) == 0) {
//             long kb;
//             sscanf(line + 6, "%ld", &kb);
//             return kb / 1024.0;
//         }
//         line = strchr(line, '\n');
//         if (line) line++;
//     }
//     return 0;
// }



//todo: external command
JobsList& SmallShell::getJobs() {
    return jobs;
}

ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line){}
void ExternalCommand:: execute() {
    std::string cmd_string = SmallShell::getInstance().alias_preparse_Cmd(cmd_line);
    cmd_string = _trim(cmd_string);
    background = _isBackgroundComamnd(cmd_string.c_str());
    complex = _isComplexCommand(cmd_string.c_str());
    _removeBackgroundSignForString(cmd_string);
    bool reserved = SmallShell::getInstance().isBuiltInCommand(cmd_string.c_str());

    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    if (reserved) {
        SmallShell::getInstance().executeCommand(cmd_string.c_str());
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (complex && background) {
        if (pid == 0) { // child
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            char* const bashArgs[] = {(char*)"/bin/bash", (char*)"-c", (char*)cmd_string.c_str(), nullptr};
            execv("/bin/bash", bashArgs);
            perror("smash error: execv failed");
            syscall(SYS_exit, 0);
        } else {
            SmallShell::getInstance().getJobs().addJob(this, pid);
        }
    }

    else if (background && !complex) {
        if (pid == 0) { // child
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            execvp(args[0], args);
            perror("smash error: execvp failed");
            syscall(SYS_exit, 0);
        } else {
            SmallShell::getInstance().getJobs().addJob(this, pid);
        }
    }

    else if (!complex && !background) {
        if (pid == 0) { // child
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            execvp(args[0], args);
            perror("smash error: execvp failed");
            syscall(SYS_exit, 0);
        } else {
            SmallShell::getInstance().set_current_pid_fg(pid);
            if(waitpid(pid, nullptr, WUNTRACED)==-1) {
                perror("smash error: waitpid failed");
                return;
            }
            SmallShell::getInstance().set_current_pid_fg(-1);

        }
    }

    else if (complex && !background) {
        std::cout<<cmd_string<<std::endl;
        if (pid == 0) { // child
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            char* const bashArgs[] = {(char*)"/bin/bash", (char*)"-c", (char*)cmd_string.c_str(), nullptr};
            execv("/bin/bash", bashArgs);
            perror("smash error: execv failed");
            syscall(SYS_exit, 0);
        } else {
            SmallShell::getInstance().set_current_pid_fg(pid);
            if(waitpid(pid, nullptr, WUNTRACED)==-1) {
                perror("smash error: waitpid failed");
                return;
            }
            SmallShell::getInstance().set_current_pid_fg(-1);

        }
    }
}


bool SmallShell::isBuiltInCommand(const char *cmd_line) const{
    string cmd_to_check = _trim(string(cmd_line));
    string firstWord = cmd_to_check.substr(0, cmd_to_check.find_first_of(" \n"));
    static const set<string> built_in_commands = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "quit",
        "kill", "alias", "unalias", "unsetenv", "watchproc"};
    return built_in_commands.find(firstWord) != built_in_commands.end();
}



//todo: Job
void JobsList::printJobsList() {
    removeFinishedJobs();
    for (JobEntry job : jobs) {
        std::cout << "[" << job.getJobId() << "] " << job.getCmd()->getCmdLine() << std::endl;
    }
}

// void JobsList::printJobsList() {
//     for (const JobEntry &job : jobs) {
//         std::cout << "Job ID: " << job.getJobId() << ", is stopped: " << (job.isJobStopped() ? "true" : "false") << std::endl;
//
//     }
// }

void JobsList::setMaxJobId(int max) {
    max_job_id = max;
}

long JobsList::JobEntry::getJobId() const{
    return job_id;
}

bool JobsList::JobEntry::isJobStopped() const {
    return isStopped;
}

Command* JobsList::JobEntry::getCmd() const {
    return cmd;
}

const char* Command::getCmdLine() const {
    return cmd_line;
}

pid_t JobsList::JobEntry::getPid() const {
    return this->pid;
}

void JobsList::removeFinishedJobs() {
    int new_max_job_id = 0;
    int job_id = 0;
    for (std::list<JobEntry>::iterator it = jobs.begin(); it != jobs.end();) {
        int wait_ret = waitpid(it->getPid(), nullptr, WNOHANG);
        if (wait_ret < 0) {
            perror("smash error: waitpid failed");
            return;
        }

        if (wait_ret > 0) {
            it = jobs.erase(it);
        } else {
            job_id = it->getJobId();
            new_max_job_id = std::max(new_max_job_id, job_id);
            ++it;
        }
    }
    max_job_id = new_max_job_id;
}


int JobsList::get_max_job_id() const{
    return max_job_id;
};
void JobsList::addJob(Command *cmd, pid_t pid){
    jobs.push_back(JobEntry(++max_job_id,cmd,pid));
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(auto& job : jobs){
        if(job.getJobId() == jobId){
            return &job;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for(auto it = jobs.begin(); it != jobs.end(); it++){
        if(it->getJobId() == jobId){
            jobs.erase(it);
            return;
        }
    }
    setMaxJobId(findNewMaxJobId());
}

JobsList::JobEntry *JobsList::getLastJob(int lastJobId) {
    for(auto& job : jobs){
        if(job.getJobId() == lastJobId){
            return &job;
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int jobId) {
    for(auto& job : jobs){
        if(job.getJobId() == jobId && job.isJobStopped()){
            return &job;
        }
    }
    return nullptr;
}

void JobsList::killAllJobs() {
    for(auto it = jobs.begin(); it != jobs.end(); it++){
        if(!it->isJobStopped()){
            if(syscall(SYS_kill, it->getPid(), SIGKILL) == -1) {
                perror("smash error: kill failed");
                return;
            }
            jobs.erase(it);
        }else{
            jobs.erase(it);
        }
    }
    setMaxJobId(0);
}

JobsList::JobsList(): max_job_id(0) {}

bool JobsList::isJobsListEmpty() const {
    return jobs.empty();
}

JobsList::JobEntry::JobEntry(long job_id, Command *cmd, pid_t pid):
job_id(job_id),cmd(cmd),isStopped(false),pid(pid) {}

pid_t SmallShell::get_current_pid_fg() const {
    return current_pid_fg;
}

void SmallShell::set_current_pid_fg(pid_t pid) {
    this->current_pid_fg = pid;
}

void SmallShell::print_alias() const {
    for (const auto& a : this->alias) {
        std::cout << a.first << "=\'" << a.second << "\'" << std::endl;
    }

}

//todo: du command
DiskUsageCommand::DiskUsageCommand(const char *cmd_line) : Command(cmd_line) {}


void DiskUsageCommand::execute() {
    std::string path = ".";

    if (cmd_line) {
        std::string trimmed = _trim(std::string(cmd_line));
        _removeBackgroundSignForString(trimmed);
        char* args[COMMAND_MAX_ARGS];
        int argc = _parseCommandLine(trimmed.c_str(), args);
        if (argc > 2) {
            write(STDERR_FILENO, "smash error: du: too many arguments\n", 36);
            return;
        }
        if (argc == 2)
            path = args[1];
    }

    struct stat st;
    if (syscall(SYS_stat, path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::string msg = "smash error: du: directory " + path + " does not exist\n";
        write(STDERR_FILENO, msg.c_str(), msg.size());
        return;
    }

    pid_t pid = syscall(SYS_fork);
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid == 0) {  // Child
        if (syscall(SYS_setpgid, 0, 0) == -1) {
            perror("smash error: setpgrp failed");
            syscall(SYS_exit, 0);
        }

        size_t total = getDirectorySize(path);
        std::string output = "Total disk usage: " + std::to_string((total + 1023) / 1024) + " KB\n";
        write(STDOUT_FILENO, output.c_str(), output.size());
        syscall(SYS_exit, 0);
    } else {  // Parent
        syscall(SYS_wait4, pid, nullptr, 0, nullptr);
    }
}

size_t DiskUsageCommand::getDirectorySize(const std::string& path) {
    size_t totalSize = 0;
    int fd = syscall(SYS_open, path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        perror("smash error: open failed");
        return 0;
    }

    char buf[8192];
    int nread;

    while ((nread = syscall(SYS_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (int bpos = 0; bpos < nread;) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + bpos);
            std::string name = d->d_name;
            bpos += d->d_reclen;

            if (name == "." || name == "..") continue;

            std::string full_path = path + "/" + name;
            struct stat st;
            if (syscall(SYS_lstat, full_path.c_str(), &st) == -1) {
                perror("smash error: lstat failed");
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                totalSize += getDirectorySize(full_path);
            } else if (!S_ISLNK(st.st_mode)) {
                totalSize += st.st_size;
            }
        }
    }

    if (nread == -1) {
        perror("smash error: getdents64 failed");
    }

    if (syscall(SYS_close, fd) == -1) {
        perror("smash error: close failed");
    }

    return totalSize;
}



//todo: netinfo command
NetInfo::NetInfo(const char *cmd_line) : Command(cmd_line) {}
void NetInfo::printStr(const char* str) {
    syscall(SYS_write, STDOUT_FILENO, str, strlen(str));
}

void NetInfo::printStrLn(const char* str) {
    printStr(str);
    syscall(SYS_write, STDOUT_FILENO, "\n", 1);
}

void NetInfo::printIP(const char* label, struct in_addr addr) {
    printStr(label);
    printStr(inet_ntoa(addr));
    syscall(SYS_write, STDOUT_FILENO, "\n", 1);
}

void NetInfo::execute() {
    std::string cmd = _trim(std::string(cmd_line));
    _removeBackgroundSignForString(cmd);
    char* args[COMMAND_MAX_ARGS];
    int argc = _parseCommandLine(cmd.c_str(), args);

    if (argc == 1) {
        printStrLn("smash error: netinfo: interface not specified");
        return;
    }

    if (argc > 2) {
        printStr("smash error: netinfo: interface");
        for (int i = 1; i < argc; ++i) {
            printStr(" ");
            printStr(args[i]);
        }
        printStrLn(" does not exist");
        return;
    }

    const char* iface = args[1];

    pid_t pid = syscall(SYS_fork);
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid == 0) {
        if (syscall(SYS_setpgid, 0, 0) == -1) {
            perror("smash error: setpgrp failed");
            syscall(SYS_exit, 1);
        }
        runNetInfoInternal(iface);
        syscall(SYS_exit, 0);
    } else {
        syscall(SYS_wait4, pid, nullptr, 0, nullptr);
    }
}

void NetInfo::runNetInfoInternal(const char* iface) {
    int sock = syscall(SYS_socket, AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("smash error: socket failed");
        return;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);  // זהירות: להבטיח null-termination
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    // Check if interface exists via SIOCGIFADDR
    if (syscall(SYS_ioctl, sock, SIOCGIFADDR, &ifr) < 0) {
        printStr("smash error: netinfo: interface ");
        printStr(iface);
        printStrLn(" does not exist");
        syscall(SYS_close, sock);
        return;
    }

    // IP address
    struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
    printIP("IP Address: ", ipaddr->sin_addr);

    // Subnet mask
    if (syscall(SYS_ioctl, sock, SIOCGIFNETMASK, &ifr) < 0) {
        printStrLn("Subnet Mask: ");
    } else {
        struct sockaddr_in* netmask = (struct sockaddr_in*)&ifr.ifr_netmask;
        printIP("Subnet Mask: ", netmask->sin_addr);
    }

    getDefaultGateway(iface);
    printDNSServers();

    syscall(SYS_close, sock);
}


void NetInfo::getDefaultGateway(const char* iface) {
    int fd = syscall(SYS_open, "/proc/net/route", O_RDONLY);
    if (fd < 0) {
        perror("smash error: open failed");
        printStrLn("Default Gateway: ");
        return;
    }

    char buf[8192];
    ssize_t n = syscall(SYS_read, fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        perror("smash error: read failed");
        syscall(SYS_close, fd);
        printStrLn("Default Gateway: ");
        return;
    }
    syscall(SYS_close, fd);
    buf[n] = '\0';

    char* line = strtok(buf, "\n");
    line = strtok(nullptr, "\n");  // Skip header

    while (line) {
        char ifaceName[IFNAMSIZ], dest[32], gateway[32];
        sscanf(line, "%s %s %s", ifaceName, dest, gateway);

        if (strcmp(ifaceName, iface) == 0 && strcmp(dest, "00000000") == 0) {
            unsigned long gw;
            if (sscanf(gateway, "%lx", &gw) == 1) {
                struct in_addr gaddr;
                gaddr.s_addr = gw;
                printIP("Default Gateway: ", gaddr);
                return;
            }
        }

        line = strtok(nullptr, "\n");
    }

    printStrLn("Default Gateway: ");
}

void NetInfo::printDNSServers() {
    int fd = syscall(SYS_open, "/etc/resolv.conf", O_RDONLY);
    if (fd < 0) {
        perror("smash error: open failed");
        printStrLn("DNS Servers: ");
        return;
    }

    char buf[8192];
    ssize_t n = syscall(SYS_read, fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        perror("smash error: read failed");
        syscall(SYS_close, fd);
        printStrLn("DNS Servers: ");
        return;
    }
    syscall(SYS_close, fd);
    buf[n] = '\0';

    char* line = strtok(buf, "\n");

    bool printedAny = false;
    printStr("DNS Servers: ");
    while (line) {
        if (strncmp(line, "nameserver", 10) == 0) {
            char tag[16], ip[64];
            if (sscanf(line, "%s %s", tag, ip) == 2) {
                if (printedAny) {
                    printStr(", ");
                }
                printStr(ip);
                printedAny = true;
            }
        }
        line = strtok(nullptr, "\n");
    }

    syscall(SYS_write, STDOUT_FILENO, "\n", 1);
}

//todo: RedirectionCommand
RedirectionCommand::RedirectionCommand(const char *cmd_line, Command *command) : Command(cmd_line), command(command) {

    isOverride = !contains(cmd_line,">>");
    string cmdl = string(cmd_line);
    _removeBackgroundSignForString(cmdl);
    size_t pos = cmdl.find('>');
    file = _trim( cmdl.substr(isOverride?pos+1:pos+2));
}


void RedirectionCommand::execute() {
    int saved_stdout = syscall(SYS_dup, STDOUT_FILENO);
    if (saved_stdout == -1) {
        perror("smash error: dup failed");
        return;
    }

    int flags = O_CREAT | O_WRONLY | (isOverride ? O_TRUNC : O_APPEND);
    int fd = syscall(SYS_open, file.c_str(), flags, 0644);
    if (fd == -1) {
        perror("smash error: open failed");
        syscall(SYS_dup2, saved_stdout, STDOUT_FILENO); // שחזור stdout
        syscall(SYS_close, saved_stdout);
        return;
    }

    if (syscall(SYS_dup2, fd, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        syscall(SYS_close, fd);
        syscall(SYS_dup2, saved_stdout, STDOUT_FILENO); // שחזור stdout
        syscall(SYS_close, saved_stdout);
        return;
    }

    if (syscall(SYS_close, fd) == -1) {
        perror("smash error: close failed");
    }

    command->execute();

    if (syscall(SYS_dup2, saved_stdout, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 restore failed");
        syscall(SYS_close, saved_stdout);
        return;
    }

    if (syscall(SYS_close, saved_stdout) == -1) {
        perror("smash error: close failed");
    }
}


//todo: PipeCommand
PipeCommand::PipeCommand(const char *cmd_line, Command* firstCommand,
                         Command* secondCommand, bool isContainsCerrPipe) :
                         Command(cmd_line), firstCommand(firstCommand),
                         secondCommand(secondCommand), containsCerrPipe
                         (isContainsCerrPipe){}

void PipeCommand::execute() {
    int fd[2];
    if (syscall(SYS_pipe, fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }
    int pipe_output = containsCerrPipe?2:1;

    pid_t pid1 = syscall(SYS_fork);
    if (pid1 < 0) {
        perror("smash error: fork failed");
        syscall(SYS_close, fd[0]);
        syscall(SYS_close, fd[1]);
        return;
    } else if (pid1 == 0) {
        if (setpgrp()==-1) {
            perror("smash error: setpgrp failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_close, fd[0]) == -1) {
            perror("smash error: close failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_dup2, fd[1], pipe_output) == -1) {
            perror("smash error: dup2 failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_close, fd[1]) == -1) {
            perror("smash error: close failed");
            syscall(SYS_exit, 0);
        }

        firstCommand->execute();
        syscall(SYS_exit, 0);
    }

    pid_t pid2 = syscall(SYS_fork);
    if (pid2 < 0) {
        perror("smash error: fork failed");
        syscall(SYS_close, fd[0]);
        syscall(SYS_close, fd[1]);
        waitpid(pid1, nullptr, 0);
        return;
    } else if (pid2 == 0) {
        if (setpgrp()==-1) {
            perror("smash error: setpgrp failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_close, fd[1]) == -1) {
            perror("smash error: close failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_dup2, fd[0], 0) == -1) {
            perror("smash error: dup2 failed");
            syscall(SYS_exit, 0);
        }

        if (syscall(SYS_close, fd[0]) == -1) {
            perror("smash error: close failed");
            syscall(SYS_exit, 0);
        }

        secondCommand->execute();
        syscall(SYS_exit, 0);
    }

    if (syscall(SYS_close, fd[0]) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (syscall(SYS_close, fd[1]) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (waitpid(pid1, nullptr, 0) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    if (waitpid(pid2, nullptr, 0) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}


//todo: whoamicommand
WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line) {}

void WhoAmICommand::execute() {
    uid_t uid = syscall(SYS_getuid);

    int fd = syscall(SYS_open, "/etc/passwd", O_RDONLY, 0);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buf[8192];
    ssize_t bytesRead = syscall(SYS_read, fd, buf, sizeof(buf) - 1);
    if (bytesRead == -1) {
        perror("smash error: read failed");
        syscall(SYS_close, fd);
        return;
    }

    buf[bytesRead] = '\0';
    syscall(SYS_close, fd);

    char* line = strtok(buf, "\n");
    while (line != nullptr) {
        char* saveptr = nullptr;
        char* name = strtok_r(line, ":", &saveptr);
        strtok_r(nullptr, ":", &saveptr); // skip x
        char* uidStr = strtok_r(nullptr, ":", &saveptr);
        strtok_r(nullptr, ":", &saveptr); // skip gid
        strtok_r(nullptr, ":", &saveptr); // skip info
        char* home = strtok_r(nullptr, ":", &saveptr);

        if (name && uidStr && home) {
            uid_t entry_uid = static_cast<uid_t>(atoi(uidStr));
            if (entry_uid == uid) {
                std::cout << name << " " << home << std::endl;
                return;
            }
        }

        line = strtok(nullptr, "\n");
    }

    std::cerr << "smash error: failed to retrieve user info" << std::endl;
}

