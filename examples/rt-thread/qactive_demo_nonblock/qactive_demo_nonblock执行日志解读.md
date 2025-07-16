请你根据日志更新 “## 日志解读”
```
=== QActive Demo Enhanced Auto-Initialize ===
[qactive_demo_start] Starting QActive Demo with enhanced RT-Thread integration...
[QActiveDemo_init] *** QActive Demo Enhanced v2.0.0-enhanced ***
[QActiveDemo_init] Build: Jul 16 2025 13:28:39
[QActiveDemo_init] QF framework initialized
[QActiveDemo_init] Publish-subscribe system initialized
[QActiveDemo_init] sizeof(QEvt)=4
[QActiveDemo_init] sizeof(SensorDataEvt)=8
[QActiveDemo_init] sizeof(ProcessorResultEvt)=8
[QActiveDemo_init] sizeof(WorkerWorkEvt)=16
[QActiveDemo_init] sizeof(ConfigReqEvt)=76
[QActiveDemo_init] sizeof(StoreReqEvt)=268
[QActiveDemo_init] Basic event pool initialized
[QActiveDemo_init] Shared 8-byte event pool initialized for SensorDataEvt, ProcessorResultEvt
[QActiveDemo_init] Worker 16-byte event pool initialized for WorkerWorkEvt
[QActiveDemo_init] Config 64-byte event pool initialized for Config events
[QActiveDemo_init] Storage 256-byte event pool initialized for Storage events
RT-Integration: Initializing RT-Thread components
[ConfigProxy] Initializing config proxy system
[ConfigProxy] Config proxy thread started successfully
[StorageProxy] Initializing storage proxy system
[StorageProxy] Storage proxy thread started successfully
RT-Integration: All synchronization objects created successfully
[QActiveDemo_init] RT-Thread integration initialized successfully
[SensorAO_ctor] Constructing Sensor Active Object
[SensorAO_ctor] Sensor AO constructed successfully
[ProcessorAO_ctor] Constructing Processor Active Object
[ProcessorAO_ctor] Processor AO constructed successfully
[WorkerAO_ctor] Constructing Worker Active Object
[WorkerAO_ctor] Worker AO constructed successfully
[MonitorAO_ctor] Constructing Monitor Active Object
[MonitorAO_ctor] Monitor AO constructed successfully
[QActiveDemo_init] All Active Objects constructed
[qactive_demo_start] Starting 4 QActive objects with RT-Thread scheduling...
[SensorAO_initial] Initializing Sensor AO - Thread: main, QActive_Prio: 1, RT_Prio: 10
[SensorAO_initial] Subscribed to SENSOR_READ_SIG
[SensorAO_active] ENTRY - Thread: main, Prio: 10, Addr: 0x6008fadc
[SensorAO_active] Starting periodic sensor readings
[QActive_start_] AO: 60080238, name: , registered, QHSM: 60080238
[QActive_start_] Thread startup result: 0, state: 1
[qactive_demo_start] Sensor AO started (prio=1, thread=sensor_)
[ProcessorAO_initial] Initializing Processor AO - Thread: main, QActive_Prio: 2, RT_Prio: 10
[ProcessorAO_initial] Subscribed to SENSOR_DATA_SIG, PROCESSOR_START_SIG, CONFIG_CFM_SIG
[ProcessorAO_idle] ENTRY - Thread: main, Prio: 10, Addr: 0x6008fadc
[ProcessorAO_idle] Processor idle, waiting for data
[QActive_start_] AO: 60080330, name: , registered, QHSM: 60080330
[QActive_start_] Thread startup result: 0, state: 1
[qactive_demo_start] Processor AO started (prio=2, thread=process)
[WorkerAO_initial] Initializing Worker AO - Thread: main, QActive_Prio: 3, RT_Prio: 10
[WorkerAO_initial] Subscribed to WORKER_WORK_SIG, STORE_CFM_SIG
[WorkerAO_idle] ENTRY - Thread: main, Prio: 10, Addr: 0x6008fadc
[WorkerAO_idle] Worker idle, waiting for work
[QActive_start_] AO: 60080414, name: , registered, QHSM: 60080414
[QActive_start_] Thread startup result: 0, state: 1
[qactive_demo_start] Worker AO started (prio=3, thread=worker_)
[MonitorAO_initial] Initializing Monitor AO - Thread: main, QActive_Prio: 4, RT_Prio: 10
[MonitorAO_initial] Subscribed to MONITOR_CHECK_SIG
[MonitorAO_monitoring] ENTRY - Thread: main, Prio: 10, Addr: 0x6008fadc
[MonitorAO_monitoring] Starting periodic monitoring
[QActive_start_] AO: 6008050c, name: , registered, QHSM: 6008050c
[QActive_start_] Thread startup result: 0, state: 1
[qactive_demo_start] Monitor AO started (prio=4, thread=monitor)
RT-Integration: Starting RT-Thread components
RT-Integration: Storage thread started successfully
RT-Integration: All RT-Thread components started successfully
[qactive_demo_start] RT-Thread integration components started successfully
[qactive_demo_start] *** QActive Demo Enhanced Started Successfully ***
[qactive_demo_start] All components running under RT-Thread scheduling
[main] *** QActive Demo Enhanced v2.0.0-enhanced ***
[main] MStorage: Thre Thread staard started - Managing located - Managing lol data storagecal data storage
 e
d by RT-Thread
Smain] Type 'qactive[main_control start' for manual co] Type 'ntrolqac iftive_control start' for manual control if needed
msh />[[tthread_function] AO thread_function] AO thhread started: 60080414, name: rworker_, prio: 29, stat: 3
ead started: 60080414, name: worker_, prio: 29, stat: 3
[thread_function] AO thread started: 60080330, name: process, prio: 30, stat: 3
[I/SDIO] SD card capacity 65536 KB.
[thread_function] AO thread started: 60080238, name: sensor_, prio: 31, stat: 3

msh />Storage: Saving data batch 1 to local storage
Storage: Save completed (total: 1)
Shell: System health check completed
[SensorAO_active] TIMEOUT - Reading #1, data = 51 (tick=2089)
[SensorAO_active] Posting sensor data to Processor AO
[SensorAO_active] Updated sensor readings count: 1
[SensorAO_active]   datUpdated sensor ra = 51
[ProcessorAO_processing] ENTRY - Processing data (count: 1)
[ProcessorAO_processing] Generated result: 100
[ProcessorAO_processing] Created ProcessorResultEvt with result: 100
[ProcessorAO_processing] About to create WorkerWorkEvt (size=16 bytes)
[ProcessorAO_processing] Posting work to Worker AO (id=1, size=8, prio=1)
[Pr[ProcessorAO_processing] About ocesto create second WorkerWorksEvortAO_
processing] Posting second[Pr workocessorAO_processin g]to  Worker AO (id=1001, siPoProcessing work (to[WorkerAO_working] ENTRY - Processing work (total: 1)
[WorkerAO_working] Armed timeout for 500ms wort kSto simurage: Saving data batch 2 to local storage
lation
simulation
 d=1001, size=8, prio=2)
[WorkerAO_working] WORKER_WORK_SIG - Additional work ID 1001 queued
SConfigProxy] Processing config reqShell[hell: System health check completed
 t, key=4660
: System health check completed
[stub] read_config called, key=[ProcessorAO_processing] Updated processed data count: 1
4660
[ConfigProxy] Posting config confirmation, key=4660
[ProcessorAO_processing] CONFIG_CFM_SIG - Config received during processing: key=4660
e_SIG - Config re[WorkerAO_working] WORKER_TIMEOUT_SIG - Work completed
[WorkerA[WorkerAO_working] EXIT - Disarming work timer
O_idle] ENTRY - Thread: worker_, Prio:  3
[WorkerAO_idle] ENTRY - Thread: worker_, Prio: 29, Addr:[stub ] flash_write cal0x600804led, len=50
[WorkerAO_idle] Worker idle, waiting for work
13, first_byte=0x57
[StorageProxy] Posting storage confirmation, result=0
[WorkerAO_idle] STORE_CFM_SIG - Storage completed with result: 0

msh />cmd_[MonitorAO_monitoring] MONITOR_TIMEOUT_SIG - System check #1 - All systems operational
[QF_Monitor] PoolMin(4B)=50, PoolMin(8B)=58, PoolMin(16B)=38, PoolMin(64B)=28, PoolMin(256B)=19
[MonitorAO_monitoring] Posting self-check signal
[MonitorAO_monitoring] Updated health checks count: 1
[MonitorAO_monitoring] MONITOR_CHECK_SIG - Health check completed
aStorage: Saving data batch 3 to local storage
Storage: Save completed (total: 3)
Shell: System health check completed
active_control start[SensorAO_active] TIMEOUT - Reading #2, data = 269 (tick=4089)
[SensorAO_active] Posting sensor data to Processor AO
ctrocessorAO_processing] SENSOR_DATA_SIGs count[:S ensorAO2_a
 iveg ] Updated sensor readings count: 2
additional sensor data = 269
Storage: Saving data batch 4 to local storage
Storage: Save completed (total: 4)
Shell: System health check completed

cmd_aactive_control: command not found.
msh />cmd_qactive_control Storage: Saving data batch 5 to local storage
Storage: Save completed (total: 5)
Shell: System health check completed
start
QActive: Starting QActive components
QActive: Start commands sent to QActive components
msh />[SensorAO_active] SENSOR_READ_SIG - Manual read triggered
[SensorAO_active] TIMEOUT - Reading #3, data = 125 (tick=5471)
[SensorAO_active] Posting sensor data to Processor AO
[ProcessorAO_processing] SENSOR_DATA_SIG - Processing additional sensor data = 125
[SensorAO_active] Updated sensor readings count: 3
[SensorAO_amonitori[SensorAO_amonitng] MONITOR_TIMEoriOU4, data = 241 (tick=6089)
nsting sensor da[SensorAO_active] Posting senstora to Pr data to Processor AO
a = 241sorAO_processing] SENSOR_DATA_SIG -o Processing additional sensor datrocessing additional sensor data = 241

 al
[QF_Monitor] PoolMin(4B)=48, PoolMin(8B)=58, PoolMin(16B)=38, PoolMin(64B)=28, PoolMin(256B)=19
[MonitorAO_monitoring] Posting self-check signal
[MonitorAO_monitoring] Updated health checks count: 2
[MonitorAO_monitoring] MONITOR_CHECK_SIG - Health check completed
[SensorAO_active] Updated sensor readings count: 4
Storage: Saving data batch 6 to local storage
Storage: Save completed (total: 6)
Shell: System health check completed
Storage: Saving data batch 7 to local storage
Storage: Save completed (total: 7)
Shell: System health check completed
```

## 执行日志解读：


### 1. 框架与事件池初始化

- `=== QActive Demo Enhanced Auto-Initialize ===`
- `[qactive_demo_start] Starting QActive Demo with enhanced RT-Thread integration...`
- `[QActiveDemo_init] QF framework initialized`
- `[QActiveDemo_init] Publish-subscribe system initialized`
- `[QActiveDemo_init] sizeof(QEvt)=4 ... sizeof(StoreReqEvt)=268`
- `[QActiveDemo_init] Basic event pool initialized`
- `[QActiveDemo_init] Shared 8-byte event pool initialized for SensorDataEvt, ProcessorResultEvt`
- `[QActiveDemo_init] Worker 16-byte event pool initialized for WorkerWorkEvt`
- `[QActiveDemo_init] Config 64-byte event pool initialized for Config events`
- `[QActiveDemo_init] Storage 256-byte event pool initialized for Storage events`

**说明：**
- QP/C 框架和 RT-Thread 集成初始化成功。
- 所有事件池按事件大小递增顺序初始化，且同尺寸事件只用一个池，完全符合 QPC 要求。
- 事件池覆盖所有事件类型，内存分配合理。



### 2. RT-Thread 组件与代理线程初始化

- `RT-Integration: Initializing RT-Thread components`
- `[ConfigProxy] ...`
- `[StorageProxy] ...`
- `RT-Integration: All synchronization objects created successfully`
- `[QActiveDemo_init] RT-Thread integration initialized successfully`

**说明：**
- 配置代理、存储代理等 RT-Thread 线程和同步对象初始化成功。
- 代理线程用于处理配置和存储等耗时操作，保证主业务线程无阻塞。


### 3. Active Object（AO）构造与启动

- `[SensorAO_ctor] ...`
- `[ProcessorAO_ctor] ...`
- `[WorkerAO_ctor] ...`
- `[MonitorAO_ctor] ...`
- `[QActiveDemo_init] All Active Objects constructed`
- `[qactive_demo_start] Starting 4 QActive objects with RT-Thread scheduling...`
- `[SensorAO_initial] ...`
- `[QActive_start_] AO: ... registered, QHSM: ...`
- `[QActive_start_] Thread startup result: 0, state: 1`
- `[thread_function] AO thread started: ...`

**说明：**
- Sensor、Processor、Worker、Monitor 四个 AO 均已构造、注册并以独立线程方式启动，线程优先级分配合理。
- 各 AO 的状态机初始化、信号订阅、定时器设置等均已完成。



### 4. 业务事件流与异步处理

- `[SensorAO_active] TIMEOUT - Reading #N, data = ...`
- `[SensorAO_active] Posting sensor data to Processor AO`
- `[ProcessorAO_processing] ...`
- `[WorkerAO_working] ...`
- `[ConfigProxy] Processing config request, key=...`
- `[StorageProxy] ...`
- `[MonitorAO_monitoring] MONITOR_TIMEOUT_SIG - System check #N - All systems operational`
- `[QF_Monitor] PoolMin(4B)=..., PoolMin(8B)=..., ...`

**说明：**
- **Sensor AO** 周期性采集数据并投递 Processor AO。
- **Processor AO** 处理数据，生成工作事件投递 Worker AO，并发起配置请求。
- **ConfigProxy** 代理线程异步处理配置请求，回送确认事件。
- **Worker AO** 处理工作，完成后触发存储，**StorageProxy** 代理线程异步写入并回送确认。
- **Monitor AO** 定期健康检查，统计事件池最小剩余量，确保系统健康。
- 所有事件流转均为异步、无阻塞，充分利用 QP/C 事件驱动和 RT-Thread 多线程优势。


