//  Lazy Pirate client
//  Use zmq_poll to do a safe request-reply
//  To run, start lpserver and then randomly kill/restart it
#include "zmq.h"
#include "sample.pb.h"
#include <string.h>
#define SERVER_ENDPOINT     "tcp://127.0.0.1:9981"

#ifdef __GNUC__
#define C_SP    '/' 
#else
#define C_SP    '\\'
#endif

#define MSG_TYPE        4
#define MSG_LEN         4
#define FILE_LEN        8

enum MsgId {
    kFile   = 0,
    kCmd    = 1
};

static void* GetFileData(const char *fname, size_t *buflen)
{
    FILE *pf = fopen(fname, "rb");
    if (!pf) 
        return NULL;
    fseek(pf, 0L, SEEK_END);
    long flen = ftell(pf);
    rewind(pf);

    void *buf = malloc(flen);
    long rlen = fread(buf, 1, flen, pf);
    fclose(pf);
    if (rlen != flen) {
        fprintf(stderr, "read len != ftell length");
        return NULL;
    }
    *buflen = flen;
    return buf;
}


static void* GetSample(const char *fname, size_t *buflen)
{
    FILE *pf = fopen(fname, "rb");
    if (!pf) 
        return NULL;
    fseek(pf, 0L, SEEK_END);
    long flen = ftell(pf);
    rewind(pf);
    const char *sf = strrchr(fname, C_SP);
    const char *name = sf ? sf+1 : fname;
    int namelen = strlen(name)+1;
    unsigned int sample_len = 4+namelen+8+flen;
    void *buf = malloc(MSG_TYPE+MSG_LEN+sample_len);
    int msg_id = kFile;
    unsigned msg_len = sample_len-8;
    memcpy(buf, &msg_id, 4);
    memcpy((char*)buf+4, &msg_len, 4);
    unsigned char *sample = (unsigned char*)buf + 8;
    memcpy(sample, &namelen, 4);
    memcpy(sample+4, fname, namelen);
    memcpy(sample+4+namelen, &flen, 8);

    long rlen = fread(sample+4+namelen+8, 1, flen, pf);
    fclose(pf);
    if (rlen != flen) {
        fprintf(stderr, "read len != ftell length");
        return NULL;
    }
    *buflen = sample_len + 8;
    return buf;
}

static int SendFile(void *socket, const char *fname)
{
    const char *sf = strrchr(fname, C_SP);
    const char *name = sf ? sf+1 : fname;
    size_t fsize = 0;
    void *fbuf = GetFileData(fname, &fsize);

    Sample sample;
    sample.set_fname(name);
    sample.set_data(fbuf, fsize);
    free(fbuf);

    size_t sample_len = sample.ByteSize();
    unsigned char *sbuf = (unsigned char *)malloc(sample_len + 8);
    int id = 10;
    memcpy(sbuf, &id, 4);
    memcpy(sbuf+4, &sample_len, 4);
    sample.SerializeToArray(sbuf+8, sample_len);
    zmq_send(socket, sbuf, sample_len+8, 0);


    zmq_msg_t msg;
    int rc = 0;
    rc = zmq_msg_init(&msg);
    rc = zmq_msg_recv(&msg, socket,  0);

    Result result;
    result.ParseFromArray(zmq_msg_data(&msg),rc);

    printf("risk: %d\n", result.risk());

    zmq_msg_close(&msg);
    return 0;
}


int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("./sbclient file.\n");
        return 0;
    }
    void *ctx = zmq_ctx_new();
    printf ("I: connecting to server...\n");
    void *client = zmq_socket(ctx, ZMQ_REQ);
    zmq_connect(client, SERVER_ENDPOINT);

    SendFile(client, argv[1]);
    zmq_close(client);
    zmq_ctx_destroy(&ctx);
    return 0;
}
