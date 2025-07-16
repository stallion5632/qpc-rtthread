#ifndef CONFIG_PROXY_H_
#define CONFIG_PROXY_H_

#include "qpc.h"
#include <rtthread.h>

/* Config proxy signals */
enum ConfigProxySignals {
    CONFIG_REQ_SIG = Q_USER_SIG + 20,
    CONFIG_CFM_SIG,
    STORE_REQ_SIG,
    STORE_CFM_SIG
};

/* Config request event */
typedef struct {
    QEvt super;
    uint32_t key;
    uint8_t buf[64];
    void *sender;  /* QActive that sent the request */
} ConfigReqEvt;

/* Config confirmation event */
typedef struct {
    QEvt super;
    uint32_t key;
    uint8_t buf[64];
} ConfigCfmEvt;

/* Storage request event */
typedef struct {
    QEvt super;
    uint8_t data[256];
    uint32_t len;
    void *requester;  /* QActive that sent the request */
} StoreReqEvt;

/* Storage confirmation event */
typedef struct {
    QEvt super;
    int result;
    void *requester;
} StoreCfmEvt;

/* Initialization functions */
void config_init(void);
void storage_init(void);

/* Post request to proxy threads */
void post_config_request(uint32_t key, uint8_t *buf, QActive *sender);
void post_storage_request(uint8_t *data, uint32_t len, QActive *requester);

#endif /* CONFIG_PROXY_H_ */
