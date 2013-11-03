include build/node-start.mk

SRC_HDR:= \
	CPURegs CPUCore CPUClock Dasm \
	BreakPointBase BreakPoint WatchPoint DebugCondition \
	MSXCPUInterface MSXCPU \
	MSXMultiDevice MSXMultiIODevice MSXMultiMemDevice \
	MSXWatchIODevice \
	VDPIODelay \
	IRQHelper

HDR_ONLY:= \
	Z80 R800 \
	CacheLine

include build/node-end.mk
