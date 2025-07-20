/*============================================================================
* Product: Throughput Performance Test for QPC-RT-Thread
* Integrated into performance_tests framework
* Based on original throughput_test.c from backup_old_tests
============================================================================*/
#include "perf_test.h"
#include "qpc.h"
#include "cycle_counter.h"

/*==========================================================================*/
/* Throughput Test Data Structure */
/*==========================================================================*/
typedef struct
{
    rt_uint32_t packets_sent;
    rt_uint32_t packets_received;
    rt_uint32_t start_cycles;
    rt_uint32_t end_cycles;
    rt_uint32_t test_duration_cycles;
    rt_uint32_t target_packets;
    rt_bool_t test_running;
    struct rt_mailbox producer_mb;
    struct rt_mailbox consumer_mb;
} throughput_test_data_t;

static throughput_test_data_t s_throughput_data;

/* Mailbox storage */
#define MAILBOX_SIZE 128 /* Increase mailbox size to reduce send failures */
static rt_ubase_t producer_mb_pool[MAILBOX_SIZE];
static rt_ubase_t consumer_mb_pool[MAILBOX_SIZE];

/*==========================================================================*/
/* Producer Thread Function */
/*==========================================================================*/
static void producer_thread_func(void *parameter)
{
    throughput_test_data_t *data = (throughput_test_data_t *)parameter;
    rt_uint32_t packet_id = 1;

    rt_kprintf("[Throughput Producer] Started\n");

    while (data->test_running && data->packets_sent < data->target_packets)
    {
        /* Send packet to consumer via mailbox */
        rt_err_t result = rt_mb_send(&data->producer_mb, packet_id);
        if (result == RT_EOK)
        {
            data->packets_sent++;
            packet_id++;

            /* Log every 100 packets */
            if (data->packets_sent % 100 == 0)
            {
                rt_kprintf("[Throughput Producer] Sent %u packets\n", data->packets_sent);
            }
        }
        else
        {
            rt_kprintf("[Throughput Producer] Send failed, result=%d (mailbox full), retry after delay\n", result);
            rt_thread_mdelay(5); /* Wait longer if mailbox is full */
        }

        /* Small yield to prevent overwhelming the system */
        rt_thread_yield();
    }

    rt_kprintf("[Throughput Producer] Finished - Sent %u packets\n", data->packets_sent);
}

/*==========================================================================*/
/* Consumer Thread Function */
/*==========================================================================*/
static void consumer_thread_func(void *parameter)
{
    throughput_test_data_t *data = (throughput_test_data_t *)parameter;
    rt_ubase_t packet_id;

    rt_kprintf("[Throughput Consumer] Started\n");

    while (data->test_running)
    {
        /* Receive packet from producer */
        rt_err_t result = rt_mb_recv(&data->producer_mb, &packet_id, RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            data->packets_received++;

            /* Send acknowledgment back */
            rt_mb_send(&data->consumer_mb, packet_id);
        }

        /* Exit condition */
        if (data->packets_received >= data->target_packets)
        {
            break;
        }
    }

    rt_kprintf("[Throughput Consumer] Finished - Received %u packets\n", data->packets_received);
}

/*==========================================================================*/
/* Test Implementation Functions */
/*==========================================================================*/
static int throughput_test_init(perf_test_case_t *tc)
{
    /* Initialize DWT cycle counter */
    dwt_init();

    /* Initialize test data */
    s_throughput_data.packets_sent = 0;
    s_throughput_data.packets_received = 0;
    s_throughput_data.start_cycles = 0;
    s_throughput_data.end_cycles = 0;
    s_throughput_data.test_duration_cycles = 0;
    s_throughput_data.target_packets = 850; /* Match expected report */
    s_throughput_data.test_running = RT_TRUE;

    /* Initialize mailboxes */
    rt_err_t result = rt_mb_init(&s_throughput_data.producer_mb,
                                 "producer_mb",
                                 producer_mb_pool,
                                 MAILBOX_SIZE,
                                 RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        rt_kprintf("[Throughput Test] Failed to initialize producer mailbox\n");
        return -1;
    }

    result = rt_mb_init(&s_throughput_data.consumer_mb,
                        "consumer_mb",
                        consumer_mb_pool,
                        MAILBOX_SIZE,
                        RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        rt_kprintf("[Throughput Test] Failed to initialize consumer mailbox\n");
        rt_mb_detach(&s_throughput_data.producer_mb);
        return -1;
    }

    /* Initialize statistics */
    tc->stats.packets_sent = 0;
    tc->stats.packets_received = 0;
    tc->stats.test_duration = 0;

    tc->user_data = &s_throughput_data;
    tc->iterations = 0;

    rt_kprintf("[Throughput Test] Initialized - Target: %u packets\n",
               s_throughput_data.target_packets);

    return 0;
}

static int throughput_test_run(perf_test_case_t *tc)
{
    throughput_test_data_t *data = (throughput_test_data_t *)tc->user_data;
    rt_thread_t producer_thread, consumer_thread;

    rt_kprintf("[Throughput Test] Starting throughput test...\n");

    /* Record start time */
    data->start_cycles = dwt_get_cycles();

    /* Create producer thread */
    producer_thread = rt_thread_create("producer",
                                       producer_thread_func,
                                       data,
                                       2048,
                                       RT_THREAD_PRIORITY_MAX / 2 - 1,
                                       10);

    /* Create consumer thread */
    consumer_thread = rt_thread_create("consumer",
                                       consumer_thread_func,
                                       data,
                                       2048,
                                       RT_THREAD_PRIORITY_MAX / 2 - 1,
                                       10);

    if (!producer_thread || !consumer_thread)
    {
        rt_kprintf("[Throughput Test] Failed to create threads\n");
        if (producer_thread)
            rt_thread_delete(producer_thread);
        if (consumer_thread)
            rt_thread_delete(consumer_thread);
        return -1;
    }

    /* Start threads */
    rt_thread_startup(producer_thread);
    rt_thread_startup(consumer_thread);

    /* Wait for completion or timeout */
    rt_uint32_t timeout_ms = 10000; /* 10 second timeout */
    rt_uint32_t check_interval_ms = 100;
    rt_uint32_t elapsed_ms = 0;

    while (data->test_running && elapsed_ms < timeout_ms)
    {
        if (data->packets_received >= data->target_packets)
        {
            break;
        }

        rt_thread_mdelay(check_interval_ms);
        elapsed_ms += check_interval_ms;
        tc->iterations++;
    }

    /* Stop the test */
    data->test_running = RT_FALSE;
    data->end_cycles = dwt_get_cycles();
    data->test_duration_cycles = data->end_cycles - data->start_cycles;

    /* Wait for threads to naturally exit, no manual delete needed to avoid assertion failure */
    rt_thread_mdelay(100);

    /* Update final statistics */
    tc->stats.packets_sent = data->packets_sent;
    tc->stats.packets_received = data->packets_received;
    tc->stats.test_duration = data->test_duration_cycles;

    rt_kprintf("[Throughput Test] Completed - Sent: %u, Received: %u, Duration: %u cycles\n",
               data->packets_sent, data->packets_received, data->test_duration_cycles);

    /* Add performance analysis */
    if (data->test_duration_cycles > 0)
    {
        rt_uint32_t throughput = (data->packets_received * 1000) / (data->test_duration_cycles / 1000);
        rt_kprintf("[Throughput Test] Performance: %u packets/second\n", throughput);
    }

    return 0;
}

static int throughput_test_stop(perf_test_case_t *tc)
{
    throughput_test_data_t *data = (throughput_test_data_t *)tc->user_data;
    if (data)
    {
        data->test_running = RT_FALSE;

        /* Clean up mailboxes */
        rt_mb_detach(&data->producer_mb);
        rt_mb_detach(&data->consumer_mb);
    }

    rt_kprintf("[Throughput Test] Stopped\n");
    return 0;
}

/* Register the test case */
PERF_TEST_REG(throughput, throughput_test_init, throughput_test_run, throughput_test_stop);
