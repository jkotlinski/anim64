
ifeq ($(PLATFORM),)
_PLATFORM_     = c64
else ifeq ($(PLATFORM),c116)
_PLATFORM_     = c16
else ifeq ($(PLATFORM),plus4)
_PLATFORM_     = c16
else
_PLATFORM_     = $(PLATFORM)
endif

ifeq ($(NO_VICE),)
NO_VICE        = 0
endif


ifneq ($(_PLATFORM_),c64)
ifneq ($(_PLATFORM_),c16)
$(error invalid platform $(_PLATFORM_) specified)
endif
endif


ARCH           = $(shell uname | tr "[a-z]" "[A-Z]" | tr -c -d "[A-Z]")

ifneq ($(findstring CYGWINNT,$(ARCH)),)
    ifeq (CYGWINNT,$(ARCH))
ARCH           = WIN32
    else
ARCH           = WIN64
    endif    
endif
ifneq ($(findstring DARWIN,$(ARCH)),)
ARCH           = MACOSX
endif


ifeq ($(_PLATFORM_),c16)
    ifneq ($(NO_VICE),0)
        ifneq ($(findstring WIN,$(ARCH)),)
USE_PLUS4EMU   = 0
USE_YAPE       = 1
        else
USE_PLUS4EMU   = 1
USE_YAPE       = 0
        endif
    else
USE_PLUS4EMU   = 0
USE_YAPE       = 0
    endif
else
USE_PLUS4EMU   = 0
USE_YAPE       = 0
endif


ifeq ($(_PLATFORM_),c16)
    ifeq ($(ARCH),MACOSX)
    # MacOSX, these programs must be installed as applications
VICE           = /Applications/VICE/xplus4.app/Contents/MacOS/xplus4
PLUS4EMU       = /Applications/plus4emu.app/Contents/MacOS/plus4emu
    else    
VICE           = xplus4
PLUS4EMU       = plus4emu
        ifeq ($(ARCH),WIN64)
YAPE           = YapeWin64
        else
YAPE           = Yape
        endif
    endif
else ifeq ($(ARCH),MACOSX)
    # MacOSX, these programs must be installed as applications
VICE           = /Applications/VICE/x64.app/Contents/MacOS/x64
else
VICE           = x64
endif

ifneq ($(USE_PLUS4EMU),0)
EMU          = $(PLUS4EMU) -disk
else
EMU          = $(VICE) -drive8type 1541 -drive9type 0 -autostart
endif


ECHO         = echo
PRINTF       = printf

AS           = ca65
LD           = ld65
PU           = pucrunch
C1541        = c1541

MKDIR        = mkdir -p
RM           = rm -f
ifeq ($(ARCH),MACOSX)
RMDIR        = rmdir # XXX TODO xargs to remove .DS_Store
else
RMDIR        = rmdir --ignore-fail-on-non-empty
endif
CAT          = cat


.PHONY: default loader assemble link compress diskimage run clean distclean wipe
.PHONY: tellarch


BUILDDIR     = ../../build
INTERMDIR    = ../../build/intermediate
LOADER_SRC   = ../../src
LOADER       = $(BUILDDIR)/loader-$(_PLATFORM_).a

RESOURCESDIR = ../resources
PIC1         = $(INTERMDIR)/sheba.bin
PIC2         = $(INTERMDIR)/prlogo.bin

NAME         = minexample

SOURCE       = $(NAME).s
LOADERCFG    = loaderconfig.inc
ASSEMBLE     = $(INTERMDIR)/$(NAME)-$(_PLATFORM_).o
LINK         = $(INTERMDIR)/$(NAME)-uncompressed-$(_PLATFORM_).prg
COMPRESS     = $(INTERMDIR)/$(NAME)-$(_PLATFORM_).prg
DISKIMAGE    = $(BUILDDIR)/$(NAME)-$(_PLATFORM_).d64

AS_FLAGS     = -I ../../../shared -I ../../include -I $(LOADER) -D EXTCONFIGPATH --cpu 6502X
PU_FLAGS     = -d -l 0x1000 -x 0x1000 -i 0 


default: diskimage


tellarch:
	@$(ECHO) $(ARCH)


loader: $(LOADER)

$(LOADER):
	make -C $(LOADER_SRC) EXTCONFIGPATH=../samples/minexample


assemble: $(ASSEMBLE)

$(ASSEMBLE): $(SOURCE) $(LOADERCFG)
	$(MKDIR) $(BUILDDIR)
	$(MKDIR) $(INTERMDIR)
ifeq ($(_PLATFORM_),c64)
	$(AS) $(AS_FLAGS) -t c64 -D PLATFORM=64 -o $@ $<
else
	$(AS) $(AS_FLAGS) -t c16 -D PLATFORM=16 -o $@ $<
endif


link: $(LINK)

$(LINK): Linkfile $(ASSEMBLE) $(LOADER)
	$(LD) -o $@ -C $^


compress: $(COMPRESS)

$(COMPRESS): $(LINK)
ifeq ($(_PLATFORM_),c64)
	$(PU) $(PU_FLAGS) -c64 -g 0x37 $^ $@
else
	$(PU) $(PU_FLAGS) -c16 $^ $@
endif


diskimage: $(DISKIMAGE)

$(DISKIMAGE): $(COMPRESS) $(PIC1) $(PIC2)
	$(C1541) -format "normal is boring,+h" d64 $@
	$(C1541) -attach $@ \
	 -write $(COMPRESS) "minexample" \
	 -write $(PIC1) "pic1" \
	 -write $(PIC2) "pic2"


ifneq ($(USE_YAPE),0)
run: $(DISKIMAGE)
	$(YAPE) "..\..\build\$(NAME)-$(_PLATFORM_).d64"
else
run: $(DISKIMAGE)
	$(EMU) $^
endif


$(INTERMDIR)/%.bin: $(RESOURCESDIR)/%.bin
	$(PRINTF) "\x00\x40" | $(CAT) - $? > $@


clean:
	-$(RM) $(ASSEMBLE) $(LINK) $(COMPRESS) $(DISKIMAGE)
	-$(RMDIR) $(INTERMDIR)
	-$(RMDIR) $(BUILDDIR)

distclean:
	-$(MAKE) -C $(LOADER_SRC) clean

wipe: distclean clean
