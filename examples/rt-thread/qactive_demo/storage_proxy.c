#include "config_proxy.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>

// XXX 声明 flash_write，实际实现应在其它文件
int flash_write(uint8_t *data, uint32_t len);

#define STR_QUEUE_SIZE    4
#define STR_MQ_NAME      "strMq"
#define STR_REQ_SIZE     sizeof(StoreReqEvt*)

/* mailbox for storage requests */
static rt_mq_t storage_mq;

static void storage_thread(void *arg) {
    StoreReqEvt *req;
    (void)arg;

    for (;;) {
        if (rt_mq_recv(storage_mq, &req, STR_REQ_SIZE, RT_WAITING_FOREVER) == RT_EOK) {
            /* perform blocking flash write */
            int res = flash_write(req->data, req->len);

            /* post confirmation back to AO */
            StoreCfmEvt *cfm = Q_NEW(StoreCfmEvt, STORE_CFM_SIG);
            if (cfm != (StoreCfmEvt *)0) {
                cfm->result = res;
                cfm->requester = req->requester;
                QACTIVE_POST((QActive*)cfm->requester, &cfm->super, 0U);
            }

            QF_gc(&req->super);
        }
    }
}

void storage_init(void) {
    storage_mq = rt_mq_create(STR_MQ_NAME,
                              STR_REQ_SIZE,
                              STR_QUEUE_SIZE,
                              RT_IPC_FLAG_FIFO);
    if (storage_mq != RT_NULL) {
        rt_thread_t tid = rt_thread_create("strTh",
                                           storage_thread,
                                           RT_NULL,
                                           2048,
                                           8,
                                           10);
        if (tid != RT_NULL) {
            rt_thread_startup(tid);
        }
    }
}

void post_storage_request(uint8_t *data, uint32_t len, QActive *requester) {
    StoreReqEvt *req = Q_NEW(StoreReqEvt, STORE_REQ_SIG);
    if (req != (StoreReqEvt *)0) {
        if (len > sizeof(req->data)) {
            len = sizeof(req->data);
        }
        if (data != (uint8_t *)0) {
            memcpy(req->data, data, len);
        }
        req->len = len;
        req->requester = requester;

        /* Post to proxy thread - non-blocking from AO perspective */
        if (rt_mq_send(storage_mq, &req, STR_REQ_SIZE) != RT_EOK) {
            /* Failed to send, cleanup */
            QF_gc(&req->super);
        }
    }
}
