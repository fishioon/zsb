
struct Sbqueue
{
    int front_port;
    int back_port;
    unsigned heartbeat_msec;
    void *ctx;
};

Sbqueue* SbqueueInit(int fport, int bport, unsigned heartbeat);
int SbqueueStart(Sbqueue *mq);
void SbqueueFree(Sbqueue *mq);
