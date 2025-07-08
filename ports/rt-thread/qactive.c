/**
* @file
* @brief QActive services and RT-Thread port implementation
*/
#include "qf_port.h"
#include "qf_pkg.h"
#include "qassert.h"

Q_DEFINE_THIS_MODULE("qactive")

/**
 * @brief Start execution of an Active Object and register it with the framework.
 *
 * This function initializes the active object's event queue, creates the
 * RT-Thread semaphore for synchronization, and starts the thread that will
 * execute the active object's event loop.
 *
 * @param me       pointer to the active object to start
 * @param prio     priority of the active object (1..QF_MAX_ACTIVE)
 * @param qSto     pointer to the storage for the ring buffer of the event queue
 * @param qLen     length of the event queue (in events)
 * @param stkSto   pointer to the stack storage (must be NULL in RT-Thread port)
 * @param stkSize  stack size (in bytes, not used in RT-Thread port)
 * @param param    pointer to the additional port-specific parameter(s)
 */
void QActive_start_(QActive * const me, uint_fast8_t prio,
                    QEvt const *qSto[], uint_fast16_t qLen,
                    void *stkSto, uint_fast16_t stkSize,
                    void const *param)
{
    char tname[RT_NAME_MAX];
    rt_thread_t thread;
    rt_sem_t sem;
    
    /* Initialize QF priority of this active object */
    me->prio = (uint8_t)prio;
    QF_add_(me);  /* make QF aware of this active object */
    
    /* Initialize the QueueBuffer structure for zero-copy event passing */
    me->eQueue.eventPtrs = (QEvt const **)qSto;
    me->eQueue.end = (uint16_t)qLen;
    me->eQueue.front = 0;
    me->eQueue.rear = 0;
    me->eQueue.count = 0;
    
    /* Create semaphore for this active object */
    rt_snprintf(tname, RT_NAME_MAX, "ao%d", (int)prio);
    sem = rt_sem_create(tname, 0, RT_IPC_FLAG_FIFO);
    Q_ASSERT(sem != RT_NULL);
    me->osObject = sem;
    
    /* Create thread for this active object */
    rt_snprintf(tname, RT_NAME_MAX, "ao%d", (int)prio);
    thread = rt_thread_create(tname,
                              (void (*)(void *))&QActive_eventLoop_,
                              me,
                              THREAD_STACK_SIZE,
                              RT_THREAD_PRIORITY_MAX / 2, /* Default priority, can be changed with QActive_setAttr */
                              THREAD_TIMESLICE);
    Q_ASSERT(thread != RT_NULL);
    
    /* Store thread handle and start thread */
    me->thread = thread;
    rt_thread_startup(thread);
}

/**
 * @brief Posts an event to an active object (FIFO policy)
 * 
 * This function implements zero-copy event posting to an AO's queue.
 * It is used by both normal threads and ISRs (via fast path).
 * 
 * @param me     pointer to the active object
 * @param e      pointer to the event to post
 * @param sender pointer to the sender object
 * @return true if successful, false if queue is full
 */
bool QActive_post_(QActive * const me, QEvt const * const e,
                   void const * const sender)
{
    bool status = true;
    QF_CRIT_STAT_TYPE lockKey;
    
    /* Test probe point for tracing/debugging */
    QS_TEST_PROBE_DEF(&QActive_post_)
    if (QS_TEST_PROBE(&QActive_post_) == 0U) {
        return false;
    }
    
    QF_CRIT_ENTRY(lockKey);
    
    /* Add event pointer to AO's queue (zero-copy) */
    if (me->eQueue.count < me->eQueue.end) {
        me->eQueue.eventPtrs[me->eQueue.rear] = e;
        me->eQueue.rear = (me->eQueue.rear + 1) % me->eQueue.end;
        ++me->eQueue.count;
        
        QF_CRIT_EXIT(lockKey);
        
        /* Release semaphore to wake up AO thread */
        rt_sem_release((rt_sem_t)me->osObject);
        
        /* Log event delivery for debugging if QS tracing is enabled */
        QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST, QS_priv_.locFilter[AO_OBJ], me)
            QS_TIME_();             /* timestamp */
            QS_OBJ_(sender);        /* sender object */
            QS_SIG_(e->sig);        /* signal of the event */
            QS_OBJ_(me);            /* receiver object */
            QS_U8_(me->eQueue.count); /* # of events in the queue */
        QS_END_NOCRIT_()
    }
    else {
        QF_CRIT_EXIT(lockKey);
        status = false; /* queue full */
        
        /* Log event overflow for debugging */
        QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_ATTEMPT, QS_priv_.locFilter[AO_OBJ], me)
            QS_TIME_();             /* timestamp */
            QS_OBJ_(sender);        /* sender object */
            QS_SIG_(e->sig);        /* signal of the event */
            QS_OBJ_(me);            /* receiver object */
            QS_U8_(me->eQueue.count); /* # of events in the queue */
        QS_END_NOCRIT_()
        
        /* Recycle the event to avoid memory leak */
        QF_gc(e);
    }
    
    return status;
}

/**
 * @brief Get an event from the active object's event queue
 * 
 * This function retrieves an event pointer from the AO's queue
 * in a zero-copy manner.
 * 
 * @param me pointer to the active object
 * @return pointer to the event
 */
QEvt const *QActive_get_(QActive * const me) {
    QEvt const *e = NULL;
    QF_CRIT_STAT_TYPE lockKey;
    
    /* Wait for event to arrive */
    rt_sem_take((rt_sem_t)me->osObject, RT_WAITING_FOREVER);
    
    QF_CRIT_ENTRY(lockKey);
    
    /* Get event from queue (zero-copy) */
    if (me->eQueue.count > 0) {
        e = me->eQueue.eventPtrs[me->eQueue.front];
        me->eQueue.front = (me->eQueue.front + 1) % me->eQueue.end;
        --me->eQueue.count;
    }
    
    QF_CRIT_EXIT(lockKey);
    
    return e;
}

/**
 * @brief The active object event loop processing function
 * 
 * This function implements the event loop that processes events
 * delivered to the active object.
 * 
 * @param me pointer to the active object
 */
void QActive_eventLoop_(QActive * const me) {
    for (;;) {
        QEvt const *e = QActive_get_(me);
        
        /* Verify that we received a valid event */
        if (e != NULL) {
            /* Process the event */
            QMSM_DISPATCH(&me->super, e);
            
            /* Recycle the event when processing is complete */
            QF_gc(e);
        }
    }
}