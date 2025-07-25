import rtconfig
from building import *

cwd     = GetCurrentDir()
src     = Split("""
./src/qf/qep_hsm.c
./src/qf/qep_msm.c
./src/qf/qf_act.c
./src/qf/qf_defer.c
./src/qf/qf_dyn.c
./src/qf/qf_mem.c
./src/qf/qf_ps.c
./src/qf/qf_qact.c
./src/qf/qf_qeq.c
./src/qf/qf_qmact.c
./src/qf/qf_time.c
./include/qstamp.c
./ports/rt-thread/qf_diagnostics.c
./ports/rt-thread/qf_hooks.c
./ports/rt-thread/qf_port.c
./ports/rt-thread/qf_opt_layer.c
""")


if GetDepend(['QPC_USING_BLINKY_EXAMPLE']):
    src += Glob('examples/rt-thread/blinky/blinky.c')

perf_group = None
if GetDepend(['QPC_USING_QACTIVE_PERFORMANCE_TESTS']):
    perf_group = SConscript('apps/performance_tests/SConscript')

demo_groups = []
if GetDepend(['QPC_USING_QACTIVE_DEMO_LITE']):
    demo_groups.append(SConscript('examples/rt-thread/qactive_demo_lite/SConscript'))

if GetDepend(['QPC_USING_QACTIVE_DEMO_LITE_MERGED']):
    demo_groups.append(SConscript('examples/rt-thread/qactive_demo_lite_merged/SConscript'))

if GetDepend(['QPC_USING_QACTIVE_DEMO_BLOCK']):
    demo_groups.append(SConscript('examples/rt-thread/qactive_demo_block/SConscript'))

if GetDepend(['QPC_USING_QACTIVE_DEMO_NONBLOCK']):
    demo_groups.append(SConscript('examples/rt-thread/qactive_demo_nonblock/SConscript'))

if GetDepend(['QPC_USING_QACTIVE_DEMO_ISR']):
    demo_groups.append(SConscript('examples/rt-thread/qactive_demo_isr/SConscript'))

if GetDepend(['QPC_USING_RTMPOOL_DEMO']):
    demo_groups.append(SConscript('examples/rt-thread/rtmpool_demo/SConscript'))

path = [cwd + "/ports/rt-thread", cwd + "/include", cwd + "/src",
        cwd + "/apps/performance_tests",
        cwd + "/examples/rt-thread/qactive_demo_lite",
        cwd + "/examples/rt-thread/qactive_demo_block",
        cwd + "/examples/rt-thread/qactive_demo_nonblock",
        cwd + "/examples/rt-thread/rtmpool_demo"]

group = DefineGroup('qpc', src, depend = ['RT_USING_MAILBOX', 'PKG_USING_QPC'], CPPPATH = path)

# merge performance_tests group
if perf_group:
    group += perf_group

# merge demo groups
for g in demo_groups:
    group += g

Return('group')
