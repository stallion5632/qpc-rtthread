/*============================================================================
* QP/C RT-Thread Integration Configuration
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* This file provides configuration options for QP/C RT-Thread integration
* optimizations including heartbeat/watchdog and memory pool management.
============================================================================*/
/*!
* @file
* @brief QF/C RT-Thread integration configuration
*/
#ifndef QF_RT_CONFIG_H
#define QF_RT_CONFIG_H

/*==========================================================================*/
/* Heartbeat and Watchdog Configuration */
/*==========================================================================*/

/*! Enable heartbeat and watchdog integration.
* When enabled, AO threads will use timeout-based event waiting and
* perform periodic heartbeat and watchdog operations.
*/
#ifndef QF_ENABLE_HEARTBEAT
#define QF_ENABLE_HEARTBEAT 1
#endif

/*! Heartbeat interval in RT-Thread ticks.
* Default is 100ms (RT_TICK_PER_SECOND/10)
*/
#ifndef QF_HEARTBEAT_TICKS
#define QF_HEARTBEAT_TICKS (RT_TICK_PER_SECOND/10)
#endif

/*! Enable watchdog feeding during heartbeat.
* When enabled, the system will automatically feed the hardware watchdog
* during heartbeat cycles.
*/
#ifndef QF_ENABLE_WATCHDOG_FEED
#define QF_ENABLE_WATCHDOG_FEED 1
#endif

/*==========================================================================*/
/* RT-Thread Memory Pool Integration Configuration */
/*==========================================================================*/

/*! Enable RT-Thread memory pool integration.
* When enabled, QP/C will use RT-Thread memory pools instead of
* its native event pools for better integration and monitoring.
*/
#ifndef QF_ENABLE_RT_MEMPOOL
#define QF_ENABLE_RT_MEMPOOL 1
#endif

/*! Number of different memory pools.
* Currently supports 3 pools: small, medium, and large.
*/
#ifndef QF_RT_MEMPOOL_NUM_POOLS
#define QF_RT_MEMPOOL_NUM_POOLS 3U
#endif

/*! Small event pool configuration */
#ifndef QF_RT_MEMPOOL_SMALL_SIZE
#define QF_RT_MEMPOOL_SMALL_SIZE    64U   /*!< Small event size in bytes */
#endif

#ifndef QF_RT_MEMPOOL_SMALL_COUNT
#define QF_RT_MEMPOOL_SMALL_COUNT   256U   /*!< Number of small events */
#endif

/*! Medium event pool configuration */
#ifndef QF_RT_MEMPOOL_MEDIUM_SIZE
#define QF_RT_MEMPOOL_MEDIUM_SIZE   128U  /*!< Medium event size in bytes */
#endif

#ifndef QF_RT_MEMPOOL_MEDIUM_COUNT
#define QF_RT_MEMPOOL_MEDIUM_COUNT  128U   /*!< Number of medium events */
#endif

/*! Large event pool configuration */
#ifndef QF_RT_MEMPOOL_LARGE_SIZE
#define QF_RT_MEMPOOL_LARGE_SIZE    256U  /*!< Large event size in bytes */
#endif

#ifndef QF_RT_MEMPOOL_LARGE_COUNT
#define QF_RT_MEMPOOL_LARGE_COUNT   32U    /*!< Number of large events */
#endif

/*! Enable memory pool statistics collection.
* When enabled, detailed allocation/deallocation statistics are maintained.
*/
#ifndef QF_ENABLE_MEMPOOL_STATS
#define QF_ENABLE_MEMPOOL_STATS 1
#endif

/*==========================================================================*/
/* Debugging and Monitoring Configuration */
/*==========================================================================*/

/*! Enable QP/C RT-Thread debug output.
* When enabled, detailed debug information will be printed to console.
*/
#ifndef Q_RT_DEBUG
#define Q_RT_DEBUG 1  /* Disabled by default to reduce console noise */
#endif

/*! Enable RT-Thread shell commands for QP/C monitoring.
* When enabled, provides shell commands for monitoring dispatcher metrics,
* memory pool status, and AO status.
*/
#ifndef QF_ENABLE_SHELL_COMMANDS
#define QF_ENABLE_SHELL_COMMANDS 1
#endif

/*==========================================================================*/
/* Advanced Configuration */
/*==========================================================================*/

/*! Maximum number of events that can be staged for batch processing */
#ifndef QF_STAGING_BUFFER_SIZE
#define QF_STAGING_BUFFER_SIZE 32U
#endif

/*! Dispatcher thread stack size */
#ifndef QF_DISPATCHER_STACK_SIZE
#define QF_DISPATCHER_STACK_SIZE 2048U
#endif

/*! Dispatcher thread priority (0 = highest priority) */
#ifndef QF_DISPATCHER_PRIORITY
#define QF_DISPATCHER_PRIORITY 0U
#endif

/*==========================================================================*/
/* Validation */
/*==========================================================================*/

#if QF_ENABLE_RT_MEMPOOL && (QF_RT_MEMPOOL_NUM_POOLS > 15U)
#error "QF_RT_MEMPOOL_NUM_POOLS cannot exceed 15"
#endif

#if QF_ENABLE_HEARTBEAT && (QF_HEARTBEAT_TICKS < 1U)
#error "QF_HEARTBEAT_TICKS must be at least 1"
#endif

#if QF_RT_MEMPOOL_SMALL_SIZE > QF_RT_MEMPOOL_MEDIUM_SIZE
#error "Small pool size must be <= medium pool size"
#endif

#if QF_RT_MEMPOOL_MEDIUM_SIZE > QF_RT_MEMPOOL_LARGE_SIZE
#error "Medium pool size must be <= large pool size"
#endif

/*==========================================================================*/
/* Usage Information */
/*==========================================================================*/

/*!
* @brief How to use these optimizations:
*
* 1. Heartbeat and Watchdog:
*    - Set QF_ENABLE_HEARTBEAT to 1
*    - Configure QF_HEARTBEAT_TICKS for desired interval
*    - AO threads will automatically perform heartbeat operations
*    - Hardware watchdog will be fed automatically if available
*
* 2. RT-Thread Memory Pool Integration:
*    - Set QF_ENABLE_RT_MEMPOOL to 1
*    - Configure pool sizes and counts as needed
*    - QP/C events will use RT-Thread memory pools
*    - Use qf_mempool shell command to monitor usage
*
* 3. Shell Commands (when QF_ENABLE_SHELL_COMMANDS is enabled):
*    - qf_metrics: Display dispatcher metrics
*    - qf_aos: Display Active Object status
*    - qf_mempool: Display memory pool statistics
*    - qf_help: Display help information
*    - qf_test: Run integration example
*
* 4. Example Usage:
*    See qf_example.c for a complete example demonstrating both features.
*/



#endif /* QF_RT_CONFIG_H */
