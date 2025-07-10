# QActive Demo for RT-Thread

This directory contains a comprehensive demonstration of QPC Active Objects (QActive) running on RT-Thread with native RT-Thread integration.

## Overview

The QActive demo showcases the integration of QPC's active objects (QActive) with RT-Thread native threads in a real-world IoT gateway scenario. The demo implements an industrial data logging system that demonstrates practical integration patterns between QPC/QActive and RT-Thread domains.

## Architecture

### QActive Components (QPC Framework)
1. **Sensor AO**: Periodically reads sensor data and publishes it
2. **Processor AO**: Processes received sensor data, handles configuration updates
3. **Worker AO**: Handles background data compression and processing
4. **Monitor AO**: Performs periodic system health monitoring

### RT-Thread Components (Native RT-Thread)
1. **Storage Thread**: Manages local data storage operations
2. **Shell Thread**: Provides RT-Thread MSH commands for system control

### Synchronization Objects
- **Mutex**: RT-Thread mutex for shared configuration protection
- **Semaphore**: RT-Thread semaphore for storage coordination
- **Event Set**: RT-Thread event set for system-wide notifications

## Data Flow

```
QActive Domain (QPC Framework)     RT-Thread Domain (Native RT-Thread)
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Sensor AO     │───▶│  Processor AO   │───▶│   Worker AO     │
│  (QActive P3)   │    │  (QActive P4)   │    │  (QActive P5)   │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                │                       ▼
                                │              ┌─────────────────┐
                                │              │ Storage Thread  │
                                │              │ (RT-Thread P10) │
                                │              └─────────────────┘
                                │                       │
                                │                       ▼
                                │              ┌─────────────────┐
                                │              │   Local File    │
                                │              │    Storage      │
                                │              └─────────────────┘
                                │
                                ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Monitor AO    │◀──▶│ Storage Thread  │◀───│  Shell Thread   │
│  (QActive P6)   │    │ (RT-Thread P10) │    │ (RT-Thread P11) │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Key Integration Features

### QActive → RT-Thread Communication
- **Data Transfer**: Worker AO sends processed data to Storage thread via RT-Thread semaphore
- **Storage Coordination**: Worker AO triggers storage operations
- **System Events**: QActive components signal system-wide events via RT-Thread event set

### RT-Thread → QActive Communication  
- **Configuration Updates**: Shell thread sends configuration changes to Processor AO via QPC events
- **Health Coordination**: Storage thread coordinates with Monitor AO via shared variables protected by RT-Thread mutex

### Bidirectional Integration
- **Statistics Sharing**: Both domains update shared statistics protected by RT-Thread mutex
- **Event Coordination**: System-wide events (health checks, errors) are coordinated via RT-Thread event set
- **MSH Commands**: RT-Thread shell commands control QActive components in real-time

## Runtime Control

### MSH Commands
The demo provides several RT-Thread MSH commands for real-time control:

```bash
# Start QActive components
msh> qactive_control start

# Stop QActive components  
msh> qactive_control stop

# Show system statistics
msh> qactive_control stats

# Configure timing parameters
msh> qactive_control config 100 500 1000

# System monitoring
msh> system_status       # Show thread status
msh> system_reset        # Reset statistics
```

### Individual Component Commands
Direct control over specific components:

```bash
# QActive component control
msh> qactive_start_cmd       # Start QActive sensor/processor
msh> qactive_stop_cmd        # Stop QActive components  
msh> qactive_stats_cmd       # Show system statistics
msh> qactive_config_cmd      # Configure timing parameters
```

## Expected Behavior

1. **Sensor Data Flow**: Sensor AO generates periodic temperature/pressure readings
2. **Data Processing**: Processor AO validates sensor data and forwards to Worker AO
3. **Data Compression**: Worker AO compresses data and triggers storage operations
4. **Storage Management**: RT-Thread Storage thread handles local file operations
5. **System Monitoring**: Monitor AO performs periodic health checks
6. **Interactive Control**: MSH commands allow real-time system configuration

## Integration Benefits

- **Educational Value**: Demonstrates practical QActive/RT-Thread integration patterns
- **Industrial Relevance**: Simulates real-world IoT gateway scenarios  
- **Best Practices**: Shows proper synchronization and resource management
- **Runtime Control**: Enables dynamic system configuration and monitoring
- **Scalability**: Framework easily extended for additional components

## Building and Running

1. **Prerequisites**: RT-Thread environment with QPC package installed
2. **Configuration**: Enable `QPC_USING_QACTIVE_DEMO` in RT-Thread configuration
3. **Build**: Use RT-Thread's `scons` build system
4. **Run**: Deploy to target hardware or simulator

## Files Structure

```
qactive_demo/
├── main.c              # Main application with QActive objects
├── qactive_demo.h      # Header with signals and event definitions
├── rt_integration.c    # RT-Thread integration implementation
├── rt_integration.h    # RT-Thread integration headers
├── SConscript         # Build configuration
└── README.md          # This documentation
```

## Technical Details

### Memory Management
- Uses RT-Thread's memory management for all dynamic allocations
- QPC's publish-subscribe system for event distribution
- No complex memory pools - relies on RT-Thread's heap

### Threading Model
- QActive objects run as RT-Thread threads with QPC state machines
- RT-Thread native threads for storage and shell operations
- Proper priority assignment ensures real-time constraints

### Error Handling
- Graceful degradation on resource allocation failures
- Comprehensive error reporting via RT-Thread console
- Recovery mechanisms for transient failures

This demo represents a production-ready foundation for integrating QPC Active Objects with RT-Thread applications in embedded systems.