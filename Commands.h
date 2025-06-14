// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>
#include <list>
#include <unordered_map>
#include <netinet/in.h>
#include <string>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class Command {
protected:
    const char* cmd_line;

public:
    Command(const char *cmd_line);
    virtual void execute() = 0;

    const char* getCmdLine() const;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);
    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
    bool complex;
    bool background;
public:
    ExternalCommand(const char *cmd_line);
    void execute() override;
};


class RedirectionCommand : public Command {
    bool isOverride;
    Command* command;
    std::string file;
public:
    explicit RedirectionCommand(const char *cmd_line,Command *command);
    virtual ~RedirectionCommand() = default;
    void execute() override;
};

class PipeCommand : public Command {
    bool containsCerrPipe;
    Command* firstCommand;
    Command* secondCommand;
public:
    PipeCommand(const char *cmd_line, Command* firstCommand,
                Command* secondCommand,bool isContainsCerrPipe);
    virtual ~PipeCommand() = default;
    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);
    size_t calc_blocks_rec(const std::string &directory);
    virtual ~DiskUsageCommand() = default;
    void execute() override;

};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);
    virtual ~WhoAmICommand() = default;
    void execute() override;
};

class NetInfo : public Command {
public:
    NetInfo(const char *cmd_line);

    void printStr(const char *str);

    virtual ~NetInfo() = default;

    void printStrLn(const char *str);

    void printIP(const char *label, struct in_addr addr);

    void execute() override;

    void runNetInfoInternal(const char *iface);

    void getDefaultGateway(const char *iface);

    void printDNSServers();
};

class ChangeDirCommand : public BuiltInCommand {
    public:
    std::string* plastPwd;
    ~ChangeDirCommand() override = default;
    ChangeDirCommand(const char *cmd_line, std::string* plastPwd);
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    ~GetCurrDirCommand() override = default;
    GetCurrDirCommand(const char *cmd_line);
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
    public:
    virtual ~ShowPidCommand()= default;
    ShowPidCommand(const char *cmd_line);
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    JobsList *jobs;
    public:
    QuitCommand(const char *cmd_line, JobsList *jobs);
    void execute() override;
    virtual ~QuitCommand() = default;
};

class Chprompt : public BuiltInCommand {
public:
    Chprompt(const char *cmd_line);
    virtual ~Chprompt() = default;
    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        long job_id;
        Command* cmd;
        bool isStopped;
       pid_t pid;
    public:
        JobEntry(long job_id,Command *cmd, pid_t pid);
        pid_t getPid() const;
        long getJobId() const;
        bool isJobStopped() const;
        Command *getCmd() const;

    };
    int max_job_id;
    std::list<JobEntry> jobs;

public:
    JobsList();

    ~JobsList() = default;

    void KillForQuitCommand();

    int findNewMaxJobId () const; //searching max after delete

    void setMaxJobId(int max_job_id);

    void addJob(Command *cmd, pid_t pid);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    int get_max_job_id() const; //returns current max

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int lastJobId);

    JobEntry *getLastStoppedJob(int jobId);

    bool isJobsListEmpty() const;


    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs;

public:
    JobsCommand(const char *cmd_line, JobsList *jobs);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);
    void execute() override;
    virtual ~KillCommand() = default;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~ForegroundCommand() = default;
    void execute() override;
};

class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);
    virtual ~AliasCommand() = default;
    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);
    virtual ~UnAliasCommand() = default;
    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);
    virtual ~UnSetEnvCommand() = default;
    void execute() override;
};

class WatchProcCommand : public BuiltInCommand {
public:
    WatchProcCommand(const char *cmd_line);

    double memoryUsageMB(int pid);

    long long CPUtime();

    long long processCPUtime(pid_t pid);

    double calculateCPUUsagePercent(int pid);

    void execute() override;

    virtual ~WatchProcCommand() = default;
};

class SmallShell {
private:
    SmallShell();
    std::string current_dir;
    std::string last_dir;
    std::string chprompt;
    JobsList jobs;
    pid_t current_pid_fg;
    std::vector<std::pair<std::string,std::string>> alias;

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell()= default;
    pid_t get_current_pid_fg() const ;
    void set_current_pid_fg(pid_t pid);

    JobsList& getJobs();
    void executeCommand(const char *cmd_line);
    void setChprompt(std::string newChprompt = "smash");
    std::string getChprompt() const;
    std::string alias_preparse_Cmd(const char *cmd_line) const;
    bool find_alias(std::string alias) const;
    std::string get_alias(std::string& alias) const;
    void print_alias() const;
    void set_alias(std::string& alias,std::string& command);
    void remove_alias(std::string alias);
    bool isBuiltInCommand(const char *cmd_line) const;
    bool isSpecialCommand(const char *cmd_line) const;
    Command* CommandByFirstWord(const char *cmd_line);
};

#endif //SMASH_COMMAND_H_
