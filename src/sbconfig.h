#include <string>
#include <vector>
using namespace std;

struct Worker
{
    char *name;
    pid_t pid;
};

class Sbconfig
{
public:
    static Sbconfig* GetConfig();
    static Sbconfig* GetConfig(const char *fname);

    int m_front_port;
    int m_back_port;
    unsigned m_heartbeat;
    bool m_isssl;

    //char *m_host;
    //char *m_ipc;
    //char *m_data_path;
    char *m_log_path;
    //char *m_result_bin;
    char *m_vbox_conf;

    char *m_workers_name;
    Worker *m_workers;
    int m_workers_count;

    pid_t m_center;

private:
    int Load(const char *fname);

    Sbconfig();
    ~Sbconfig();
    static Sbconfig *m_config;
};
