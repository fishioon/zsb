#include <map>
#include <unistd.h>
#include <stdio.h>
#include "sbconfig.h"
#include "sbqueue.h"
#include "sbworker.h"

#include "glog/logging.h"

#define SB_CONFIG   "sandbox.conf"

static int g_running = 1;
static pid_t g_sbqueue;


static int HandleOption(int argc, char* argv[])
{
    return 0;
}

static pid_t StartWorker(const char *name, const char *vbox_conf)
{
    pid_t child = fork();
    if (child < 0) {
        fprintf(stderr, "fork worker failed");
        child = 0;
    } else if (child == 0) {
        Sbworker *sbwkr = new Sbworker();
        if (sbwkr->Init(name, vbox_conf) == 0) {
            sbwkr->Start();
        }
        exit(0);
    }
    return child;
}

static int StartAllWorkers(Sbconfig *sbcnf)
{
    printf("workers count: %d\n", sbcnf->m_workers_count);
    for (int i = 0; i < sbcnf->m_workers_count; i++) {
        sbcnf->m_workers[i].pid = StartWorker(
                sbcnf->m_workers[i].name, sbcnf->m_vbox_conf);
    }
    return 0;
}

static int StartCenter(Sbconfig *sbcnf)
{
    pid_t child = fork();
    if (child < 0) {
        fprintf(stderr, "fork center failed");
        LOG(ERROR) << "fork center failed.";
        child = 0;
    } else if (child == 0) {
        Sbqueue *mq = SbqueueInit(sbcnf->m_front_port, 
                sbcnf->m_back_port, sbcnf->m_heartbeat);
        SbqueueStart(mq);
    }
    sbcnf->m_center = child;
    return 0;
}

static int CheckPidStatus(pid_t pid)
{
    if (pid) {
        return 0;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    const char *conf_name = argc < 2 ? "sandbox.conf" : argv[1];
    Sbconfig *sbcnf = Sbconfig::GetConfig(conf_name);
    if (!sbcnf) {
        fprintf(stderr, "Load config file failed.<%s>\n", conf_name);
        return -1;
    }

    StartCenter(sbcnf);
    StartAllWorkers(sbcnf);

    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Start workers";
    LOG(INFO) << "Start center";
    while (g_running) {
        if (CheckPidStatus(g_sbqueue)) {
            StartCenter(sbcnf);
        }
        for (int i = 0; i < sbcnf->m_workers_count; i++) {
            if (CheckPidStatus(sbcnf->m_workers[i].pid)) {
                StartWorker(sbcnf->m_workers[i].name, sbcnf->m_vbox_conf);
            }
        }
        sleep(3);
    }
    return 0;
}
