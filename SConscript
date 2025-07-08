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
./ports/rt-thread/qf_hooks.c
./ports/rt-thread/qf_port.c
./ports/rt-thread/qf_opt_layer.c
""")

# Add QXK kernel sources if QXK is enabled
if GetDepend(['QPC_USING_QXK']):
    src += Split("""
./src/qxk/qxk.c
./src/qxk/qxk_mutex.c
./src/qxk/qxk_sema.c
./src/qxk/qxk_xthr.c
""")

if GetDepend(['QPC_USING_BLINKY_EXAMPLE']):
    src += Glob('examples/rt-thread/blinky/blinky.c')

# Add QXK demo if enabled
if GetDepend(['QPC_USING_QXK_DEMO']):
    src += Glob('examples/rt-thread/qxk_demo/*.c')

# Add performance tests if enabled
if GetDepend(['QPC_USING_PERFORMANCE_TESTS']):
    src += Glob('apps/performance_tests/*.c')

path = [cwd + "/ports/rt-thread", cwd + "/include", cwd + "/src", cwd + "/apps/performance_tests", cwd + "/examples/rt-thread/qxk_demo"]

group = DefineGroup('qpc', src, depend = ['RT_USING_MAILBOX', 'PKG_USING_QPC'], CPPPATH = path)

Return('group')