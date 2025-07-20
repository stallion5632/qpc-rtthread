#ifndef CYCLE_COUNTER_H__
#define CYCLE_COUNTER_H__
#include <stdint.h>
#include <rtthread.h>

#if defined(__ARM_ARCH_7A__)    /* Cortex-A9/ARMv7-A: use PMU */
    static inline void pmu_enable_user_access(void) {
        /* CP15, c9, c14, 0 使能用户态访问 */
        asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(1));
    }

    static inline void pmu_reset_counters(void) {
        /* 复位所有性能计数器并启动周期计数 */
        /* CP15, c9, c12, 0: 控制寄存器; bit0: reset CCNT, bit1: reset evt counters, bit2: enable all */
        asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0x17));
    }

    static inline uint32_t pmu_read_cycle(void) {
        uint32_t cc;
        /* CP15, c9, c13, 0: 读 CCNT */
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
        return cc;
    }

    static inline void dwt_init(void)
    {
        pmu_enable_user_access();
        pmu_reset_counters();
    }

    static inline rt_uint32_t dwt_get_cycles(void)
    {
        return pmu_read_cycle();
    }

#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    /* Cortex-M: use DWT */
    #define DWT_CTRL        (*(volatile rt_uint32_t*)0xE0001000)
    #define DWT_CYCCNT      (*(volatile rt_uint32_t*)0xE0001004)
    #define CoreDebug_DEMCR (*(volatile rt_uint32_t*)0xE000EDFC)

    static inline void dwt_init(void)
    {
        /* enable DWT and reset cycle counter */
        CoreDebug_DEMCR |= (1 << 24);
        DWT_CYCCNT = 0;
        DWT_CTRL |= 1;

        rt_kprintf("[CycleCounter] DWT initialized, CTRL=0x%08x\n", DWT_CTRL);
        if ((DWT_CTRL & 1) == 0)
        {
            rt_kprintf("[CycleCounter] Warning: DWT not available, using ticks\n");
        }
    }

    static inline rt_uint32_t dwt_get_cycles(void)
    {
        if ((DWT_CTRL & 1) == 0)
        {
            /* fallback: approximate cycles by ticks */
            return rt_tick_get() * 1000;
        }
        return DWT_CYCCNT;
    }

#else
    /* Other architectures: no hardware counter */
    static inline void dwt_init(void)   { /* no-op */ }
    static inline rt_uint32_t dwt_get_cycles(void)
    {
        /* fallback: scale ticks to pseudo-cycles */
        return rt_tick_get() * 1000;
    }
#endif

#endif /* CYCLE_COUNTER_H__ */
