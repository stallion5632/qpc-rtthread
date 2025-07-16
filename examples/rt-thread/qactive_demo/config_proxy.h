#ifndef CONFIG_PROXY_H
#define CONFIG_PROXY_H

#include "qpc.h"
#include <stdint.h>

/* Request event: sent from AO to proxy thread */
typedef struct {
    QEvt      super;
    uint32_t  key;
    uint8_t   buf[16];
    QMState  *sender;
} ConfigReqEvt;

/* Confirmation event: sent from proxy thread back to AO */
typedef struct {
    QEvt      super;
    uint32_t  key;
    uint8_t   buf[16];
} ConfigCfmEvt;

enum {
    CONFIG_REQ_SIG = Q_USER_SIG,
    CONFIG_CFM_SIG
};

void config_init(void);

#endif /* CONFIG_PROXY_H */
