#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <set>
#include <fstream>
#include <iterator>
#include <regex>
#include <csignal>
#include <pwd.h>
#include <unistd.h>


using namespace std;

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

std::string SmallShell::alias_preparse_Cmd(const char *cmd_line) {
    string trimmed = _trim(string(cmd_line));
    istringstream iss(trimmed);
    string firstWord;
    iss >> firstWord;

    string restOfLine;
    getline(iss, restOfLine);

    auto it = alias.find(firstWord);
    if (it != alias.end()) {
        firstWord = it->second;
    }

    string newCommandLine = firstWord + " " + restOfLine;
    return newCommandLine;
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(SmallShell::getInstance().alias_preparse_Cmd
    (cmd_line)));
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

bool _isComplexComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return (str[str.find_last_not_of(WHITESPACE)] == ('*') || str[str.find_last_not_of(WHITESPACE)] == ('?'));
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


SmallShell::SmallShell():chprompt("smash"), last_dir(""), current_pid_fg(-1) {
    char cwd[COMMAND_MAX_LENGTH];
    if (getcwd(cwd, COMMAND_MAX_LENGTH) == nullptr) {
        std::perror("smash error:getcwd failed\n");
    }
    current_dir = string(cwd);
}

Command *SmallShell::CommandByFirstWord(const char *cmd_line){
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new Chprompt(cmd_line);

    }else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);

    }else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);

    }else if(firstWord.compare("cd") == 0){
        return new ChangeDirCommand(cmd_line,&last_dir);

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

    }else {
        return new ExternalCommand(cmd_line);
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(alias_preparse_Cmd(cmd_line)));
    if(contains(cmd_s,">") && (cmd_s.rfind(">") == cmd_s.find(">") || cmd_s
    .rfind(">") - cmd_s.find(">") == 1)){
        return new RedirectionCommand(cmd_s.c_str(),CommandByFirstWord(cmd_s.c_str()));
    }else if(contains(cmd_s,"|")){
        int pos = cmd_s.find("|");
        string first_command = _trim(cmd_s.substr(0,pos));
        string second_command = _trim(cmd_s.substr(pos+1));

        return new PipeCommand(cmd_line,CommandByFirstWord(first_command
        .c_str()),CommandByFirstWord(second_command.c_str()), contains(cmd_s,
                                                                       "|&"));

    }else{
        return CommandByFirstWord(cmd_s.c_str());
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
     Command* cmd = CreateCommand(cmd_line);
     cmd->execute();
     delete cmd;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

//todo: chprompt
Chprompt::Chprompt(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void SmallShell::setChprompt(std::string newChprompt) {
    chprompt = newChprompt;
}

std::string SmallShell::getChprompt() const {
    return chprompt;
}

void Chprompt::execute() {
    char* args[20];
    _parseCommandLine(cmd_line,args);
    if (args[1] == nullptr){
        SmallShell::getInstance().setChprompt();
    }else{
        SmallShell::getInstance().setChprompt(args[1]);
    }
}


//todo: showpid command
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute()  {
    std::cout << "smash pid is " << getpid() << std::endl;
}

//todo: GetCurrDirCommand
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void GetCurrDirCommand::execute()  {
    char* cwd = new char[COMMAND_MAX_LENGTH];
    if(getcwd(cwd, COMMAND_MAX_LENGTH) == nullptr){
        std::perror("smash error:getcwd failed\n");
        delete[] cwd;
        return;
    }
    std::cout << cwd << std::endl;
    delete[] cwd;
}

//todo:ChangeDirCommand
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) , plastPwd(plastPwd) {}
void ChangeDirCommand::execute() {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);

    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);
    //if there are more than 2 args
    if (num_of_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }
    //if there is 1 arg (only cd)
    if (num_of_args == 1) {
        char* going_to_be_last_dir = new char[COMMAND_MAX_LENGTH];
        if(getcwd(going_to_be_last_dir, COMMAND_MAX_LENGTH) == nullptr){
            perror("smash error: getcwd failed");
            delete[] going_to_be_last_dir;
            for(int i = 0; i < num_of_args; i++){
                free(args[i]);
            }
            return;
        }
        delete[] *plastPwd;
        *plastPwd = strdup(going_to_be_last_dir);
        for(int i = 0; i < num_of_args; i++){
            free(args[i]);
        }
        return;
    }

    //there are exactly 2 args
    if (strcmp(args[1], "-") == 0) {
        if (*plastPwd == nullptr || (*plastPwd)[0] == '\0') { //if there isnt last pwd
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        char curr_dir[COMMAND_MAX_LENGTH];
        if (getcwd(curr_dir, sizeof(curr_dir)) == nullptr) {
            perror("smash error: getcwd failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        if (chdir(*plastPwd) == -1) {
            perror("smash error: chdir failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        delete[] *plastPwd;
        *plastPwd = strdup(curr_dir); // מעתיקים את ה-current לתוך lastPwd
    }
    else { //reglar cd
        char curr_dir[COMMAND_MAX_LENGTH];
        if (getcwd(curr_dir, sizeof(curr_dir)) == nullptr) {
            perror("smash error: getcwd failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        if (chdir(args[1]) == -1) {
            perror("smash error: chdir failed");
            for (int i = 0; i < num_of_args; i++) {
                free(args[i]);
            }
            return;
        }

        delete[] *plastPwd;
        *plastPwd = strdup(curr_dir); // מעדכנים את ה-last directory
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
    char* args[20];
    int arg_num = _parseCommandLine(cmd_line,args);
    char* end;
    long val = strtol(args[1], &end, 10);

    if ((args[1] == end && arg_num == 2)|| arg_num > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
    }else{
        JobsList::JobEntry* job = args[1] == end?jobs->getLastJob(jobs->get_max_job_id()):jobs->getJobById(val);
        if(jobs->getJobById(val) == nullptr){
            if(arg_num == 1){
                std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            }else{
                std::cerr << "smash error: fg: job-id " <<val <<" does not exist" << std::endl;
            }
        }else{
            pid_t pid = job->getPid();
            SmallShell::getInstance().set_currentt_pid_fg(pid);
            waitpid(pid,nullptr,0);
            SmallShell::getInstance().set_currentt_pid_fg(-1);
            jobs->removeJobById(job->getJobId());
        }
    }
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
        for(int i=0; i<num_of_args; i++){
            free(args[i]);
        }
        exit(0);
    }
    if (num_of_args >= 1) {
        for(int i=0; i<num_of_args; i++){
            free(args[i]);
        }
        exit(0);
    }
}

void JobsList::KillForQuitCommand() {
    this->removeFinishedJobs();
    std::cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << std::endl;
    for (auto job : jobs) {
        std::cout << job.getPid() << ": " << job.getCmd()->getCmdLine() << std::endl;
        if (kill(job.getPid(), SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    }
    if (jobs.size() == 0) {
        max_job_id = 0;
    }
    else {
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
void KillCommand::execute()  {
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
    signal_number = abs(signal_number);

    if (signal_number> 31 || signal_number < 1 || job_id < 0) {
        std::cout << "smash error: kill: invalid arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

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
    if (kill(job->getPid(), signal_number) == -1) {
        perror("smash error: kill failed");
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    if (signal_number == SIGKILL) {
        jobs->removeJobById(job_id);
        jobs->setMaxJobId(jobs->findNewMaxJobId());
    }

    for (int i = 0; i < num_of_args; i++) {
        free(args[i]);
    }
    return;
}

//todo: alias command
AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void AliasCommand::execute() {
    char *args[20];
    int arg_num = _parseCommandLine(cmd_line, args);
    std::string alias;
    std::string alias_value;
    bool seen = false;
    for (int i = 1; i < arg_num; i++) {
        if (seen) {
            alias_value += args[i];
            //seen = string(args[i]).find('=') == -1?false:true;
        }else{
            int j = 0;
            while (args[i][j] != '\0') {
                if (args[i][j] == '=') {
                    seen = true;
                    continue;
                } else if (seen) {
                    alias_value += args[i][j];
                } else {
                    alias += args[i][j];
                }
            }
        }
    }

    if(SmallShell::getInstance().find_alias(alias) || SmallShell::getInstance
    ().isBuiltInCommand(alias.c_str())){
        std::cerr << "smash error: alias: <name> already exists or is a reserved command" << std::endl;
        return;
    }
    string cmd_line_str(cmd_line);
    regex pattern(R"(^alias [a-zA-Z0-9_]+='[^']*'$)");
    if (!regex_search(cmd_line_str, pattern)) {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }
    SmallShell::getInstance().set_alias(alias,alias_value);
}

bool SmallShell::find_alias(std::string alias) {
    auto it = this->alias.find(alias);
    return it != this->alias.end();
}
std::string SmallShell::get_alias(std::string alias) {
    auto it = this->alias.find(alias);
    if (it != this->alias.end()) {
        return it->second;
    }
}
void SmallShell::set_alias(std::string alias,std::string command) {
    this->alias[alias] =  command;
}

//todo: unalias command
UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void UnAliasCommand::execute() {
char *args[20];
int arg_num = _parseCommandLine(cmd_line, args);
if (arg_num < 2){
    std::cerr << "smash error: unalias: not enough arguments" << std::endl;
}
for (int i = 1; i < arg_num; ++i) {
    if(SmallShell::getInstance().find_alias(args[i])){
        SmallShell::getInstance().remove_alias(args[i]);
    }else{
        std::cerr << "smash error: unalias: <name> alias does not exist"
        << std::endl;
    }
}
}

void SmallShell::remove_alias(std::string alias) {
    auto it = this->alias.find(alias);
    if(it != this->alias.end()){
        this->alias.erase(it);
    }

}

//todo: UnSetEnvCommand
UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void UnSetEnvCommand::execute()  {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);
    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);
    if (num_of_args == 1) {
        std::cout << "smash error: unsetenv: not enough arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }
    for (int i = 0; i < num_of_args; i++) {
        const char* var_name = args[i];
        if (getenv(var_name) == nullptr) {
            std::cerr << "smash error: unsetenv: " << var_name << " does not exist" << std::endl;
            for (int j = 0; j < num_of_args; j++) {
                free(args[j]);
            }
            return;
        }
        if (unsetenv(var_name) != 0) {
            perror("smash error: unsetenv failed");
            for (int j = 0; j < num_of_args; j++) {
                free(args[j]);
            }
            return;
        }
    }
}

//todo:WatchProcCommand
WatchProcCommand::WatchProcCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void WatchProcCommand::execute()  {
    std::string cmd_string = string(cmd_line);
    if (_isBackgroundComamndForString(cmd_string)) {
        _removeBackgroundSignForString(cmd_string);
    }
    cmd_string = _trim(cmd_string);
    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);

    // if not 2 args
    if (num_of_args != 2) {
        std::cout << "smash error: watchproc: invalid arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    // second arg needs to be a number
    if (!isNumber(args[1])) {
        std::cout << "smash error: watchproc: invalid arguments" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    //check if process exists
    pid_t pid = stoi(args[1]);
    if (kill(pid, 0) == -1 && errno == ESRCH) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    //  /proc/<pid>/stat
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    int stat_fd = open(stat_path.c_str(), O_RDONLY);
    if (stat_fd == -1) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    char stat_buffer[4096];
    ssize_t stat_bytes = read(stat_fd, stat_buffer, sizeof(stat_buffer) - 1);
    if (stat_bytes < 0) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        close(stat_fd);
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }
    stat_buffer[stat_bytes] = '\0';
    close(stat_fd);

    std::istringstream iss(stat_buffer);
    std::vector<std::string> stats(istream_iterator<std::string>{iss}, {});

    if (stats.size() < 24) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    long utime = std::stol(stats[13]);
    long stime = std::stol(stats[14]);
    long total_time = utime + stime;

    long clk_ticks_per_sec = sysconf(_SC_CLK_TCK);
    double cpu_usage = (double)(total_time) / clk_ticks_per_sec * 100.0;

    //  /proc/<pid>/status
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    int status_fd = open(status_path.c_str(), O_RDONLY);
    if (status_fd == -1) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }

    char status_buffer[4096];
    ssize_t status_bytes = read(status_fd, status_buffer, sizeof(status_buffer) - 1);
    if (status_bytes < 0) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        close(status_fd);
        for (int i = 0; i < num_of_args; i++) {
            free(args[i]);
        }
        return;
    }
    status_buffer[status_bytes] = '\0';
    close(status_fd);

    std::istringstream status_iss(status_buffer);
    std::string line;
    double memory_mb = 0.0;
    while (getline(status_iss, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream mem_iss(line);
            std::string key;
            long mem_kb;
            mem_iss >> key >> mem_kb;
            memory_mb = mem_kb / 1024.0;
            break;
        }
    }

    std::cout << "PID: " << pid
              << " | CPU Usage: " << std::fixed << std::setprecision(1) << cpu_usage << "%"
              << " | Memory Usage: " << std::fixed << std::setprecision(1) << memory_mb << " MB"
              << std::endl;

    for (int i = 0; i < num_of_args; i++) {
        free(args[i]);
    }

    return;
}

ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line){}
void ExternalCommand:: execute()  {
    std::string cmd_string = string(cmd_line);
    cmd_string = _trim(cmd_string);
    _removeBackgroundSignForString(cmd_string);
    background = _isBackgroundComamnd(cmd_line);
    complex = _isComplexComamnd(cmd_line);

    char* args[COMMAND_MAX_ARGS];
    int num_of_args = _parseCommandLine(cmd_string.c_str(), args);


}

bool SmallShell::isBuiltInCommand(const char *cmd_line) {
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

void JobsList::removeFinishedJobs(){
    int new_max_job_id = 0;
    int job_id = 0;
    for (std::list<JobEntry>::iterator it = jobs.begin(); it != jobs.end();)
    {
        if (it->isJobStopped() || waitpid(it->getPid(),nullptr,WNOHANG)){
            it = jobs.erase(it);
        }else
            job_id = it->getJobId();
        new_max_job_id = std::max(new_max_job_id,job_id);
        ++it;
    }
    max_job_id = new_max_job_id;
}

int JobsList::get_max_job_id() const{
    return max_job_id;
};
void JobsList::addJob(Command *cmd, bool isStopped){
    jobs.push_back(JobEntry(++max_job_id,cmd,isStopped));
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for(auto job : jobs){
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
}

JobsList::JobEntry *JobsList::getLastJob(int lastJobId) {
    for(auto job : jobs){
        if(job.getJobId() == lastJobId){
            return &job;
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int jobId) {
    for(auto job : jobs){
        if(job.getJobId() == jobId && job.isJobStopped()){
            return &job;
        }
    }
    return nullptr;
}

void JobsList::killAllJobs() {
    for(auto it = jobs.begin(); it != jobs.end(); it++){
        if(!it->isJobStopped()){
            kill(it->getPid(), SIGKILL);
            jobs.erase(it);
        }else{
            jobs.erase(it);
        }
    }
}

JobsList::JobsList(): max_job_id(0) {}

JobsList::JobEntry::JobEntry(long job_id, Command *cmd, bool isStopped):
job_id(job_id),cmd(cmd),isStopped(isStopped) {}


RedirectionCommand::RedirectionCommand(const char *cmd_line, Command *command) : Command(cmd_line), command(command) {
    isOverride = !contains(cmd_line,">>");
    string cmdl = string(cmd_line);
    size_t pos = cmdl.find('>');
    file = cmdl.substr(isOverride?pos+2:pos+1);
}

void RedirectionCommand::execute() {
    int saved_stdout = dup(1);
    if(saved_stdout < 0){
        std::cerr << "smash error: " << "dup" << " failed" << endl;
    }
    if(close(1) < 0){
        std::cerr << "smash error: " << "close" << " failed" << endl;
    }
    int fd = open(file,  O_CREAT | O_WRONLY | (isOverride ? O_TRUNC : O_APPEND) , 0644);
    dup2(fd, 1);
    close(fd);
    command->execute();
    dup2(saved_stdout, 1);
    close(saved_stdout);
}

PipeCommand::PipeCommand(const char *cmd_line, Command* firstCommand,
                         Command* secondCommand, bool isContainsCerrPipe) :
                         Command(cmd_line), firstCommand(firstCommand),
                         secondCommand(secondCommand), containsCerrPipe
                         (isContainsCerrPipe){}

void PipeCommand::execute() {
    int fd[2];
    pipe(fd);
    pid_t pid1 = fork();
    if(pid1 < 0){
        perror("smash error: fork failed");
    }else if(pid1 == 0){
        setpgrp();
        close(fd[0]);
        dup2(fd[1],1);
        close(fd[1]);
        firstCommand->execute();
        exit(1);
    }
    pid_t pid2 = fork();
    if(pid2 < 0){
        perror("smash error: fork failed");
    }else if(pid2 == 0){
        setpgrp();
        close(fd[1]);
        dup2(fd[0],0);
        close(fd[0]);
        secondCommand->execute();
        exit(1);
    }
    SmallShell::getInstance().set_currentt_pid_fg(pid1);
    SmallShell::getInstance().set_currentt_pid_fg(pid2);
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
    SmallShell::getInstance().set_currentt_pid_fg(-1);

}

void WhoAmICommand::execute() {
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw == nullptr) {
        perror("smash error: getpwuid failed");
        return;
    }
    std::cout << pw->pw_name << " " << pw->pw_dir << std::endl;
}
