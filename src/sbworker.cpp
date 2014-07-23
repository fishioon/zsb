//  Paranoid Pirate worker

#include "sbworker.h"
#include <errno.h>

#include "iniparser.h"
#include "zmq.h"
#include "czmq.h"
#include "glog/logging.h"

#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  3000    //  msecs
#define INTERVAL_INIT       1000    //  Initial reconnect
#define INTERVAL_MAX       32000    //  After exponential backoff

//  Paranoid Pirate Protocol constants
#define PPP_READY       "\001"      //  Signals worker is ready
#define PPP_HEARTBEAT   "\002"      //  Signals worker heartbeat

Sbworker::Sbworker()
{
    m_ctx = NULL;
}
//Sbworker::Sbworker(const char *name, const char *conf_name) 
//{
    //m_conf = strdup(conf_name);
    //m_name = strdup(name);
//}
Sbworker::~Sbworker() 
{
    //free(m_conf);
    free(m_name);
    free(m_host);
    if (m_ctx)
        zmq_ctx_term(m_ctx);
}

int Sbworker::NewSocket()
{
    m_worker = zmq_socket(m_ctx, ZMQ_DEALER);
    int rc = zmq_connect(m_worker, m_host);
    if (rc) {
        LOG(ERROR) << "zmq_connect host <" << m_host << "> failed, errno:" << errno;
        return -1;
    }
    zframe_t *frame = zframe_new (PPP_READY, 1);
    zframe_send (&frame, m_worker, 0);
    return 0;
}

int Sbworker::Init(const char *name, const char *vbox_conf)
{
    google::InitGoogleLogging(name);

    m_name = strdup(name);
    dictionary *dic = iniparser_load(vbox_conf);
    if (!dic) {
        LOG(ERROR) << "iniparser load <" << vbox_conf << "> failed";
        return -1;
    }
    int port = iniparser_getint(dic, "worker:port", 7982);
    if (port > 49651 || port < 1024)
        port = 7982;
    char *ip = iniparser_getstring(dic, "worker:ip", (char*)"127.0.0.1");
    int hlen = strlen(ip) + 16;
    m_host = (char*)malloc(hlen);
    snprintf(m_host, hlen, "tcp://%s:%d", ip, port);

    DLOG(INFO) << "center url: " << m_host;

    m_heartbeat = iniparser_getint(dic, "worker:heartbeat", 5000);
    DLOG(INFO) << "worker heartbeat: " << m_heartbeat;
    iniparser_freedict(dic);

    m_ctx = zmq_ctx_new();
    return NewSocket();
}

int Sbworker::ScanFile(Sample *sample, Result *result)
{
    result->set_score(50);
    result->set_risk(9);
    result->set_crash(true);
    return 0;
}

int Sbworker::RunTask(unsigned char *data, size_t len, Result *result)
{
    size_t sm_len = *(int*)(data + 4);
    printf("len: %zu\n", len);
    printf("sm_len: %zu\n", sm_len);

    if (sm_len + 8 != len)
        return -1;

    Sample sample;
    sample.ParseFromArray(data+8, len);
    printf("sample: %s\n", sample.fname().c_str());
    ScanFile(&sample, result);
    return 0;
}

int Sbworker::Start()
{
    uint64_t hb_at = zclock_time() + m_heartbeat;
    zmq_pollitem_t items[] = { { m_worker, 0, ZMQ_POLLIN, 0 } };
    size_t liveness = HEARTBEAT_LIVENESS;
    size_t interval = m_heartbeat;

    while (m_running) {
        int rc = zmq_poll(items, 1, m_heartbeat * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv(m_worker);
            if (!msg)
                break;          //  Interrupted
            if (zmsg_size(msg) == 3) {
                printf("W:recv sample from queue\n");
                zframe_t *frame = zmsg_last(msg);
                int ret = RunTask(zframe_data(frame), zframe_size(frame), &m_result);
                if (ret) {
                    printf("error msg\n");
                }
                size_t reslen = m_result.ByteSize();
                void *buf = malloc(reslen);
                //zmq_msg_t tmp_msg;
                //zmq_msg_init_size(&tmp_msg, reslen);
                m_result.SerializeToArray(buf, reslen);
                zframe_reset(frame, buf, reslen);
                zmsg_send (&msg, m_worker);
                free(buf);

            } else if (zmsg_size(msg) == 1) {
                zframe_t *frame = zmsg_first(msg);
                if (memcmp(zframe_data(frame), PPP_HEARTBEAT, 1) == 0) {
                    printf("W:recv heartbeat from queue\n");
                    liveness = HEARTBEAT_LIVENESS;
                } else {
                    zmsg_dump(msg);
                }
                zmsg_destroy(&msg);
            }
            else {
                printf ("E: invalid message\n");
                zmsg_dump (msg);
            }

            interval = INTERVAL_INIT;
        }
        else if (--liveness == 0) {
            printf ("W: heartbeat failure, can't reach queue\n");
            printf ("W: reconnecting in %zd msec...\n", interval);
            zclock_sleep (interval);

            if (interval < INTERVAL_MAX)
                interval *= 2;
            zmq_close(m_worker);
            NewSocket();
            liveness = HEARTBEAT_LIVENESS;
        }
        //  Send heartbeat to queue if it's time
        if (zclock_time () > (int64_t)hb_at) {
            hb_at = zclock_time () + HEARTBEAT_INTERVAL;
            zframe_t *frame = zframe_new (PPP_HEARTBEAT, 1);
            zframe_send (&frame, m_worker, 0);
            printf("W: send heartbeat to queue\n");
        }
    }
    return 0;
}
