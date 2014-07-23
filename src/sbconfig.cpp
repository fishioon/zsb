#include "sbconfig.h"
#include "iniparser.h"
#include <string.h>

Sbconfig* Sbconfig::m_config = NULL;

Sbconfig::Sbconfig()
{
    m_workers_name = NULL;
    m_workers_count = 0;
}
Sbconfig::~Sbconfig()
{
    if (m_workers_name)
        free(m_workers_name);
}


Sbconfig* Sbconfig::GetConfig()
{
    return m_config;
}

Sbconfig* Sbconfig::GetConfig(const char *fname)
{
    if (m_config)
        delete m_config;
    m_config = new Sbconfig();
    if (m_config->Load(fname)) {
        delete m_config;
        m_config = NULL;
    }
    return m_config;
}

int Sbconfig::Load(const char* fname)
{
    dictionary *dic = iniparser_load(fname);
    if (!dic)
        return -1;
    m_back_port = iniparser_getint(dic, "center:back_port", 7891);
    m_front_port = iniparser_getint(dic, "center:front_port", 7892);
    m_heartbeat = iniparser_getint(dic, "center:heartbeat", 5000);
    m_vbox_conf = strdup(iniparser_getstring(dic, "local:vbox_conf", (char*)"vbox.conf"));

    m_workers_name = strdup(iniparser_getstring(dic, "local:workers", NULL));

    if (m_workers_name) {
        m_workers_count = 1;
        char *p = m_workers_name;

        while (*p) {
            if (*p == ',') 
                m_workers_count++;
            p++;
        }
        m_workers = (Worker*)malloc(sizeof(Worker)*m_workers_count);
        p = m_workers_name;
        for (int i = 0; i < m_workers_count; i++) {
            m_workers[i].name = p;
            p = strchr(p, ',');
            if (p) *p++ = 0;
        }
    }

    iniparser_freedict(dic);
    return 0;
}
