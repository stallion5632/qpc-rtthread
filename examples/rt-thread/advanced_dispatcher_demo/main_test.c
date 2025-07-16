/*============================================================================
* Product: Advanced Dispatcher Demo for RT-Thread - Test Version
* Last updated for version 7.2.0
* Last updated on  2024-12-19
============================================================================*/
#include "qactive_demo.h"
#include <stdio.h>

Q_DEFINE_THIS_FILE

/* QF publish-subscribe table */
#define MAX_PUB_SIG     32U
static QSubscrList subscrSto[MAX_PUB_SIG];

/*==========================================================================*/
/* Simplified Test Version */
/*==========================================================================*/
void AdvancedDemo_init_test(void) {
    static uint8_t isInitialized = 0;
    if (isInitialized != 0) {
        printf("Advanced Demo Test: Already initialized, skipping...\n");
        return;
    }
    isInitialized = 1;

    printf("=== Advanced Dispatcher Demo Test Initialize ===\n");

    /* Initialize QF */
    QF_init();

    /* Initialize publish-subscribe */
    QF_psInit(subscrSto, Q_DIM(subscrSto));

    /* Initialize event pools in ascending order of event size */
    /* Event pools must be initialized in ascending order of event size. */
    /* Only one pool is allowed for events of the same size. */

    /* Calculate actual event sizes for debugging */
    printf("Event sizes: QEvt=%lu, DataEvt=%lu, StrategyEvt=%lu, BulkDataEvt=%lu\n",
           (unsigned long)sizeof(QEvt),
           (unsigned long)sizeof(DataEvt),
           (unsigned long)sizeof(StrategyEvt),
           (unsigned long)sizeof(BulkDataEvt));

    static QF_MPOOL_EL(QEvt) basicEventPool[20];              /* 4-byte event pool for QEvt */
    /* 12-byte event pool shared by DataEvt and BulkDataEvt */
    static QF_MPOOL_EL(DataEvt) shared12Pool[70];

    /* Initialize pools in ascending order */
    QF_poolInit(basicEventPool, sizeof(basicEventPool), sizeof(basicEventPool[0]));
    printf("Basic event pool initialized: %lu bytes per event\n", (unsigned long)sizeof(basicEventPool[0]));

    QF_poolInit(shared12Pool, sizeof(shared12Pool), sizeof(shared12Pool[0]));
    printf("Shared 12-byte pool initialized: %lu bytes per event\n", (unsigned long)sizeof(shared12Pool[0]));

    printf("Advanced Demo Test: Initialization completed successfully\n");
}

/*==========================================================================*/
/* Test Main Function */
/*==========================================================================*/
int main_test(void) {
    (void)Q_this_module_;
    printf("Starting Advanced Dispatcher Demo Test...\n");

    AdvancedDemo_init_test();

    printf("Test completed - no assertion failures!\n");
    return 0;
}
