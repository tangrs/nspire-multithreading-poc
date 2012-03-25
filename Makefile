GCC = nspire-gcc
LD = nspire-ld
LDFLAGS = -ggdb
LIBS = -lm
GCCFLAGS = -ggdb -Wall -W -mcpu=arm926ej-s
OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
EXE = threadpoc.tns
OBJS = example.o interrupt.o threading.o atomic.o

DISTDIR = .
vpath %.tns $(DISTDIR)

all: $(EXE)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

%.o: %.s
	$(GCC) $(GCCFLAGS) -c $<

$(EXE): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(@:.tns=.elf)
	$(OBJCOPY) -O binary $(@:.tns=.elf) $@

clean:
	rm -f *.o *.elf
	rm -f $(EXE)
