#include "config_proxy.h"
#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

Q_DEFINE_THIS_MODULE("storage_proxy")

/* Forward declaration for flash_write implementation */
int32_t flash_write(uint8_t const *data, uint32_t len);

#define STR_MQ_NAME      "strMq"
#define STR_SEND_TIMEOUT_MS  50  /* Short timeout for send operation */

/* mailbox for storage requests */
static rt_mq_t storage_mq;

static void storage_thread(void *arg) {
    StoreReqEvt *req;
    (void)arg;

    Q_REQUIRE(storage_mq != RT_NULL);  /* Assert valid message queue */

    for (;;) {
        if (rt_mq_recv(storage_mq, &req, STORAGE_REQ_MSG_SIZE, RT_WAITING_FOREVER) == RT_EOK) {
            rt_kprintf("[StorageProxy] Processing storage request, len=%lu\n", (unsigned long)req->len);

            /* perform blocking flash write */
            int32_t res = flash_write(req->data, req->len);

            /* post confirmation back to AO */
            StoreCfmEvt *cfm = Q_NEW(StoreCfmEvt, STORE_CFM_SIG);
            if (cfm != (StoreCfmEvt *)0) {
                cfm->result = res;
                cfm->requester = req->requester;
                rt_kprintf("[StorageProxy] Posting storage confirmation, result=%ld\n", (long)res);
                QACTIVE_POST((QActive*)cfm->requester, &cfm->super, 0U);
            }

            /* recycle the request event - will be auto-recycled by QF dispatch */
            QF_gc(&req->super);
        }
    }
}

void storage_init(void) {
    rt_kprintf("[StorageProxy] Initializing storage proxy system\n");

    storage_mq = rt_mq_create(STR_MQ_NAME,
                              STORAGE_REQ_MSG_SIZE,
                              STORAGE_PROXY_QUEUE_SIZE,
                              RT_IPC_FLAG_FIFO);
    Q_REQUIRE(storage_mq != RT_NULL);  /* Assert storage_mq creation success */

    if (storage_mq != RT_NULL) {
        rt_thread_t tid = rt_thread_create("strTh",
                                           storage_thread,
                                           RT_NULL,
                                           2048U,
                                           7U,    /* Priority 7: below AOs (1-4) but above background tasks */
                                           10U);
        Q_REQUIRE(tid != RT_NULL);  /* Assert thread creation success */
        if (tid != RT_NULL) {
            rt_thread_startup(tid);
            rt_kprintf("[StorageProxy] Storage proxy thread started successfully\n");
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

        /* Post to proxy thread with timeout - non-blocking from AO perspective */
        rt_err_t result = rt_mq_send(storage_mq, &req, STORAGE_REQ_MSG_SIZE);
        if (result != RT_EOK) {
            /* Failed to send (queue full or other error), cleanup the request event to prevent leak */
            QF_gc(&req->super);
        }
    }
}
