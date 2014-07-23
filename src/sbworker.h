#ifndef _SB_WORKER_
#define _SB_WORKER_

#include "sample.pb.h"

class Sbworker
{
public:
    Sbworker();
    ~Sbworker();

    int Init(const char *name, const char *vbox_conf);
    int Start();

private:
    int NewSocket();

    int RunTask(unsigned char *data, size_t len, Result *res);

    int ScanFile(Sample *sample, Result *res);

    int m_running;

    void *m_ctx;
    void *m_worker;
    char *m_host;

    char *m_name;
    int m_heartbeat;

    Result m_result;
};

#endif
