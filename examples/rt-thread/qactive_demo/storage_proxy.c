#include "qactive_demo.h" // 确保 QActive 类型可用
#include <stdio.h>

// XXX 声明 flash_write，实际实现应在其它文件
int flash_write(uint8_t *data, uint32_t len);
#include "storage_proxy.h"
#include <rtthread.h>

#define STR_QUEUE_SIZE    4
#define STR_MQ_NAME      "strMq"
#define STR_REQ_SIZE     sizeof(StoreReqEvt*)

/* mailbox for storage requests */
static rt_mq_t storage_mq;

static void storage_thread(void *arg) {
    StoreReqEvt *req;
    for (;;) {
        rt_mq_recv(storage_mq, &req, STR_REQ_SIZE, RT_WAITING_FOREVER);

        /* perform blocking flash write */
        int res = flash_write(req->data, req->len);

        /* post confirmation back to AO */
        StoreCfmEvt *cfm = Q_NEW(StoreCfmEvt, STORE_CFM_SIG);
        cfm->result    = res;
        cfm->requester = req->requester;
        QACTIVE_POST((QActive*)cfm->requester, &cfm->super, 0U);

        QF_gc(&req->super);
    }
}

void storage_init(void) {
    storage_mq = rt_mq_create(STR_MQ_NAME,
                              STR_REQ_SIZE,
                              STR_QUEUE_SIZE,
                              RT_IPC_FLAG_FIFO);
    rt_thread_t tid = rt_thread_create("strTh",
                                       storage_thread,
                                       RT_NULL,
                                       2048,
                                       8,
                                       10);
    rt_thread_startup(tid);
}
