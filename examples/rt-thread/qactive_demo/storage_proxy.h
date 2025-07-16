#ifndef STORAGE_PROXY_H
#define STORAGE_PROXY_H

#include "qpc.h"
#include <stdint.h>

/* Request event: send buffer to be written */
typedef struct {
    QEvt      super;
    uint8_t  *data;
    uint32_t  len;
    QMState  *requester;
} StoreReqEvt;

/* Confirmation event: result of write */
typedef struct {
    QEvt      super;
    int       result;
    QMState  *requester;
} StoreCfmEvt;

enum {
    STORE_REQ_SIG = Q_USER_SIG + 2,
    STORE_CFM_SIG
};

void storage_init(void);

#endif /* STORAGE_PROXY_H */
