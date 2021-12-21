PLATFORM = retrofw

CFLAGS += -Ofast
CFLAGS += -mhard-float -mips32 -mno-mips16

include gmenunx.mk
