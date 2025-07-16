# Thread Priority Assignment Table

This document defines the thread priority assignment for the QActive Demo with RT-Thread integration.

## Priority Hierarchy (Lower number = Higher Priority)

| Priority | Component | Thread Name | Purpose | Notes |
|----------|-----------|-------------|---------|-------|
| 1 | SensorAO | sensor_ao | Sensor data acquisition | Highest priority for responsiveness |
| 2 | ProcessorAO | processor_ao | Data processing | High priority for real-time processing |
| 3 | WorkerAO | worker_ao | Work execution | Medium priority |
| 4 | MonitorAO | monitor_ao | System monitoring | Medium priority |
| 7 | Storage Proxy | strTh | Flash/Storage operations | Below AOs, above background |
| 8 | Config Proxy | cfgTh | Configuration access | Below AOs, above background |
| 10 | RT-Thread Storage | storage | RT-Thread storage thread | Background service |
| 11 | RT-Thread Shell | shell | RT-Thread shell thread | Background service |
| 15+ | Background | idle, etc. | System idle and other tasks | Lowest priority |

## Design Principles

1. **Active Objects (1-4)**: Highest priority to ensure Run-to-Completion semantics
2. **Proxy Threads (7-8)**: Medium priority to service AO requests promptly
3. **RT-Thread Services (10-11)**: Lower priority for system services
4. **Background Tasks (15+)**: Lowest priority for non-critical operations

## Queue Depth Configuration

| Component | Queue Size | Reasoning |
|-----------|------------|-----------|
| Config Proxy | 8 | Increased from 4 to reduce queue full probability |
| Storage Proxy | 8 | Increased from 4 to reduce queue full probability |

## Event Pool Configuration

| Pool Type | Size | Event Types | Reasoning |
|-----------|------|-------------|-----------|
| Basic (4B) | 50 | QEvt | Basic events |
| Shared (8B) | 60 | SensorDataEvt, ProcessorResultEvt | Data events |
| Worker (16B) | 40 | WorkerWorkEvt | Work events |
| Config (64B) | 30 | ConfigReqEvt, ConfigCfmEvt | Increased for concurrent config requests |
| Storage (256B) | 20 | StoreReqEvt, StoreCfmEvt | Increased for concurrent storage requests |

## Memory Management

- All proxy threads use QF_gc() for proper event cleanup
- Failed send operations clean up allocated events to prevent leaks
- Event pools are sized to handle maximum expected concurrent requests
