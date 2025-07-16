#include "config_proxy.h"
#include <rtthread.h>
#include <string.h>
#include <stdint.h>

#include "qactive_demo.h"

Q_DEFINE_THIS_MODULE("config_proxy")

/* Forward declaration for read_config implementation */
void read_config(uint32_t key, uint8_t *buf);

#define CFG_MQ_NAME      "cfgMq"
#define CFG_SEND_TIMEOUT_MS  50  /* Short timeout for send operation */

/* message queue for config requests */
static rt_mq_t config_mq;

static void config_thread(void *arg) {
    ConfigReqEvt *req;
    (void)arg;

    Q_REQUIRE(config_mq != RT_NULL);  /* Assert valid message queue */

    for (;;) {
        /* block until an AO posts a request */
        if (rt_mq_recv(config_mq, &req, CONFIG_REQ_MSG_SIZE, RT_WAITING_FOREVER) == RT_EOK) {
            rt_kprintf("[ConfigProxy] Processing config request, key=%lu\n", (unsigned long)req->key);

            /* single-threaded access to persistent config */
            read_config(req->key, req->buf);

            /* post confirmation back into QP */
            ConfigCfmEvt *cfm = Q_NEW(ConfigCfmEvt, CONFIG_CFM_SIG);
            if (cfm != (ConfigCfmEvt *)0) {
                cfm->key = req->key;
                (void)memcpy(cfm->buf, req->buf, sizeof(req->buf));
                rt_kprintf("[ConfigProxy] Posting config confirmation, key=%lu\n", (unsigned long)cfm->key);
                QACTIVE_POST((QActive*)req->sender, &cfm->super, 0U);
            }

            /* recycle the request event - will be auto-recycled by QF dispatch */
            QF_gc(&req->super);
        }
    }
}

void config_init(void) {
    rt_kprintf("[ConfigProxy] Initializing config proxy system\n");

    config_mq = rt_mq_create(CFG_MQ_NAME,
                             CONFIG_REQ_MSG_SIZE,
                             CONFIG_PROXY_QUEUE_SIZE,
                             RT_IPC_FLAG_FIFO);
    Q_REQUIRE(config_mq != RT_NULL);  /* Assert config_mq creation success */

    if (config_mq != RT_NULL) {
        rt_thread_t tid = rt_thread_create("cfgTh",
                                           config_thread,
                                           RT_NULL,
                                           1024U,
                                           8U,    /* Priority 8: below AOs (1-4) but above background tasks */
                                           10U);
        Q_REQUIRE(tid != RT_NULL);  /* Assert thread creation success */
        if (tid != RT_NULL) {
            rt_thread_startup(tid);
            rt_kprintf("[ConfigProxy] Config proxy thread started successfully\n");
        }
    }
}

void post_config_request(uint32_t key, uint8_t *buf, QActive *sender) {
    ConfigReqEvt *req = Q_NEW(ConfigReqEvt, CONFIG_REQ_SIG);
    if (req != (ConfigReqEvt *)0) {
        req->key = key;
        if (buf != (uint8_t *)0) {
            memcpy(req->buf, buf, sizeof(req->buf));
        }
        req->sender = sender;

        /* Post to proxy thread with timeout - non-blocking from AO perspective */
        rt_err_t result = rt_mq_send(config_mq, &req, CONFIG_REQ_MSG_SIZE);
        if (result != RT_EOK) {
            /* Failed to send (queue full or other error), cleanup the request event to prevent leak */
            rt_kprintf("[ConfigProxy] WARNING: Failed to send config request, result=%d (queue full?)\n", result);
            QF_gc(&req->super);
        }
    }
}
