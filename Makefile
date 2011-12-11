CC65BASE = /usr/local/lib/cc65

CRT0   	= $(CC65BASE)/lib/c64.o
CLIB	= $(CC65BASE)/lib/c64.lib
CC	= cc65 -O --create-dep -Cl -tc64 -T -I $(CC65BASE)/include/ 
AS	= ca65 --cpu 6502x # -l
LD	= ld65 -C anim64.cfg -m anim64.map -Ln anim64.lbl 
C1541  	= ~/bin/c1541
DEPDIR = .dep


all:   	anim64

# --------------------------------------------------------------------------
# Generic rules

%.o : %.c

%.a : %.c
	@echo $<
	@$(CC) -o $(basename $<).a $(basename $<).c
	@mkdir -p $(DEPDIR)
	@mv $(basename $<).u $(DEPDIR)/

%.o : %.a
	@$(AS) $(basename $<).a

# Don't delete intermediate .a files.
.PRECIOUS : %.a 

%.o : %.s
	@echo $<
	@$(AS) $(basename $<).s

OBJS = anim64.o colcpy.o music.o loader.o movie.o

-include $(OBJS:%.o=$(DEPDIR)/%.u)

# --------------------------------------------------------------------------
# Rules how to make each one of the binaries

EXELIST=anim64

anim64.d64:
	$(C1541) -format anim64,AA  d64 anim64.d64 > /dev/null

anim64: 		$(OBJS) $(CLIB) anim64.d64
	@$(LD) -o $@.prg $(OBJS) $(CLIB)
	@for exe in $(EXELIST); do\
	    $(C1541) -attach anim64.d64 -delete $$exe.prg  > /dev/null;\
	    $(C1541) -attach anim64.d64 -write $$exe.prg  > /dev/null;\
	done;

run: anim64
	x64 anim64.d64

# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	rm -f *~ *.d64 *.map *.o *.lbl *.prg *.lst *.a *.u $(DEPDIR)/*
	
# ------------------


