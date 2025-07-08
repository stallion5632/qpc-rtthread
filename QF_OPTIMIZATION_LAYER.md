# QPC-RT-Thread Optimization Layer Implementation

## Overview
This implementation addresses all requirements from Pull Request #1 to create a coordination and optimization layer for QPC on RT-Thread, providing fast-path event dispatch and improved real-time performance.

## Files Created/Modified

### New Files
- `ports/rt-thread/qf_opt_layer.c` - Main optimization layer implementation
- `ports/rt-thread/qf_opt_layer.h` - Public API for optimization layer

### Modified Files
- `ports/rt-thread/qf_port.c` - Added fast-path logic to QActive_post_()
- `ports/rt-thread/qf_port.h` - Added configuration macros and QEvt extension
- `ports/rt-thread/qep_port.h` - Added platform-specific QEvt extension support
- `SConscript` - Added optimization layer to build

## Key Features Implemented

### A. Build Errors and Structure Consistency
- **A-1**: Fixed QActive thread.stat access by using rt_thread_ready() function
- **A-2**: Implemented parallel mapping array for QEvt target pointers instead of modifying core QEvt structure

### B. Real-time Efficiency and Design Consistency
- **B-1**: Added fast-path dispatch in QActive_post_() and QF_postFromISR() before rt_mb_send()
- **B-2**: Implemented QF_zeroCopyPost() with user-space ring buffer and rt_sem protection
- **B-3**: Dispatcher thread runs at configurable highest priority (default 0) with no timeslice
- **B-4**: Direct QHSM_DISPATCH() calls without posting back to AO mailbox

### C. Thread Safety
- **C-1**: Moved rt_sem_release() outside critical sections, used __sync_fetch_and_add() for atomic operations
- **C-2**: Single-producer/single-consumer ring buffer with atomic front/rear index management

### D. Resource Management and Robustness
- **D-1**: Static dispatcher stack allocation with RT_ALIGN_SIZE
- **D-2**: Configurable QF_STAGING_BUFFER_SIZE with overflow handling and lost event counter
- **D-3**: Extended fast-path to dynamic events with proper reference counting

### E. Enhanced Responsiveness
- **E-1**: Idle hook integration to ensure timely processing of staged events
- **E-2**: Automatic dispatcher triggering when system becomes idle to prevent event starvation

## Configuration Options

```c
#define QF_STAGING_BUFFER_SIZE 32U        // Configurable staging buffer size
#define QF_DISPATCHER_STACK_SIZE 2048U    // Configurable dispatcher stack size  
#define QF_DISPATCHER_PRIORITY 0U         // Highest priority for dispatcher
```

## Public API

```c
// Core functions
void QF_initOptLayer(void);                           // Initialize optimization layer
bool QF_isEligibleForFastPath(QActive const * const me, QEvt const * const e);
bool QF_zeroCopyPost(QActive * const me, QEvt const * const e);
bool QF_postFromISR(QActive * const me, QEvt const * const e);

// Control functions
void QF_enableOptLayer(void);                         // Enable optimization
void QF_disableOptLayer(void);                        // Disable optimization
uint32_t QF_getLostEventCount(void);                  // Get lost event statistics
```

## Architecture

### Fast-Path Decision Flow
1. Check if optimization layer is enabled
2. Verify target thread readiness 
3. Support both immutable (poolId == 0) and dynamic events (poolId != 0)
4. Direct dispatch via QHSM_DISPATCH() if eligible
5. Fallback to normal rt_mb_send() if not eligible

### Ring Buffer Design
- Single-producer (ISR/task), single-consumer (dispatcher) design
- Atomic index updates using __sync_fetch_and_add()
- Parallel event-target mapping to avoid modifying core QEvt
- Overflow handling with statistics and optional fallback

### Dispatcher Thread
- Highest configurable priority for real-time responsiveness
- Batch processing of staged events
- Direct state machine dispatch without mailbox round-trip
- Proper event garbage collection

## Benefits

1. **Reduced Latency**: Fast-path eliminates mailbox overhead for eligible events
2. **Better Real-time**: Highest priority dispatcher ensures timely processing
3. **Scalable**: Configurable buffer sizes and priorities
4. **Safe**: Lock-free operations and proper error handling
5. **Compatible**: Preserves existing QP/C API and behavior
6. **Monitorable**: Lost event counters and enable/disable controls

## Integration

The optimization layer is automatically initialized in QF_run() and integrates seamlessly with existing QActive_post_() calls. No changes to application code are required, but applications can optionally use the new ISR wrapper and configuration APIs for enhanced performance.

### Idle Hook Integration
The optimization layer includes an idle hook mechanism to ensure timely event processing:

```c
// Automatically installed in QF_initOptLayer()
rt_thread_idle_sethook(QF_idleHook);
```

**Benefits:**
- Prevents event starvation when non-ISR threads post to staging buffer
- Ensures events are processed even when system is idle
- Provides additional safety net for real-time responsiveness
- Works transparently without requiring application changes

**Implementation Details:**
- Idle hook checks if staging buffer has pending events
- Only triggers semaphore release if events are waiting
- Lightweight implementation suitable for idle context
- Respects optimization layer enable/disable state

## Testing

Basic syntax verification and configuration checks are included. Full functional testing requires RT-Thread environment with actual hardware or QEMU simulation.