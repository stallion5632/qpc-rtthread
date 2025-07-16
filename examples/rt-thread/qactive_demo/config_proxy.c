#include "config_proxy.h"
#include <rtthread.h>
#include <string.h>
#include "qactive_demo.h"

// XXX 声明 read_config，实际实现应在其它文件
void read_config(uint32_t key, uint8_t *buf);

#define CFG_QUEUE_SIZE    4
#define CFG_MQ_NAME      "cfgMq"
#define CFG_REQ_SIZE     sizeof(ConfigReqEvt*)

/* mailbox for config requests */
static rt_mq_t config_mq;

static void config_thread(void *arg) {
    ConfigReqEvt *req;
    for (;;) {
        /* block until an AO posts a request */
        rt_mq_recv(config_mq, &req, CFG_REQ_SIZE, RT_WAITING_FOREVER);

        /* single-threaded access to persistent config */
        read_config(req->key, req->buf);

        /* post confirmation back into QP */
        ConfigCfmEvt *cfm = Q_NEW(ConfigCfmEvt, CONFIG_CFM_SIG);
        cfm->key = req->key;
        memcpy(cfm->buf, req->buf, sizeof(req->buf));
        QACTIVE_POST((QActive*)req->sender, &cfm->super, 0U);

        /* recycle the request event */
        QF_gc(&req->super);
    }
}

void config_init(void) {
    config_mq = rt_mq_create(CFG_MQ_NAME,
                             CFG_REQ_SIZE,
                             CFG_QUEUE_SIZE,
                             RT_IPC_FLAG_FIFO);
    rt_thread_t tid = rt_thread_create("cfgTh",
                                       config_thread,
                                       RT_NULL,
                                       1024,
                                       10,
                                       10);
    rt_thread_startup(tid);
}
