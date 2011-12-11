
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
VICE128        = $(VICE)
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
VICE128        = /Applications/VICE/x128.app/Contents/MacOS/x128
else
VICE           = x64
VICE128        = x128
endif

ifeq ($(_PLATFORM_),c16)
# for testing, VICE only
DRIVE9TYPE     = 1551
else
DRIVE9TYPE     = 0
endif

ifneq ($(USE_PLUS4EMU),0)
EMU41          = $(PLUS4EMU) floppy.a.driveType=0 floppy.b.driveType=1 -disk
EMU51          = $(PLUS4EMU) floppy.a.driveType=1 floppy.b.driveType=1 -disk # C-16 only
else
EMU41          = $(VICE) -drive8type 1541 -drive9type $(DRIVE9TYPE) -autostart
EMU51          = $(VICE) -drive8type 1551 -drive9type $(DRIVE9TYPE) -autostart # C-16 only
endif
# the following mode is only supported by VICE and YAPE
EMU81          = $(VICE) -drive8type 1581 -drive9type $(DRIVE9TYPE) -autostart
# the following modes are only supported by VICE
EMU42          = $(VICE) -drive8type 1542 -drive9type $(DRIVE9TYPE) -autostart # 1541-II
EMU70          = $(VICE) -drive8type 1570 -drive9type $(DRIVE9TYPE) -autostart
EMU71          = $(VICE) -drive8type 1571 -drive9type $(DRIVE9TYPE) -autostart
EMU7141        = $(VICE) -drive8type 1571 -drive9type $(DRIVE9TYPE) -autostart # 1571 with .d64
EMU73          = $(VICE128) -drive8type 1573 -drive9type 0 -autostart # 1571CR
EMU7341        = $(VICE128) -drive8type 1573 -drive9type 0 -autostart # 1571CR with .d64
EMU2000        = $(VICE) -drive8type 2000 -drive9type 0 -autostart
EMU4000        = $(VICE) -drive8type 4000 -drive9type 0 -autostart
EMU1001        = $(VICE) -drive8type 1001 -drive9type $(DRIVE9TYPE) -autostart

ifneq ($(USE_PLUS4EMU),0)
EMU941         = $(PLUS4EMU) floppy.a.driveType=0 floppy.b.driveType=0 -disk
EMU951         = $(PLUS4EMU) floppy.a.driveType=1 floppy.b.driveType=0 -disk # C-16 only
else
EMU941         = $(VICE) -drive8type 1541 -drive9type 1541 -autostart
EMU951         = $(VICE) -drive8type 1551 -drive9type 1541 -autostart # C-16 only
endif
# the following modes are only supported by VICE
EMU942         = $(VICE) -drive8type 1542 -drive9type 1541 -autostart # 1541-II
EMU970         = $(VICE) -drive8type 1570 -drive9type 1541 -autostart
EMU971         = $(VICE) -drive8type 1571 -drive9type 1541 -autostart
EMU97141       = $(VICE) -drive8type 1571 -drive9type 1541 -autostart # 1571 with .d64
EMU973         = $(VICE128) -drive8type 1573 -drive9type 1541 -autostart # 1571CR
EMU97341       = $(VICE128) -drive8type 1573 -drive9type 1541 -autostart # 1571CR with .d64
EMU981         = $(VICE) -drive8type 1581 -drive9type 1541 -autostart
EMU92000       = $(VICE) -drive8type 2000 -drive9type 1541 -autostart
EMU94000       = $(VICE) -drive8type 4000 -drive9type 1541 -autostart
EMU91001       = $(VICE) -drive8type 1001 -drive9type 1541 -autostart


SVNVERSION     = svnversion
ifneq ($(wildcard ../../.svn/format),)
VERSION        = $(shell $(SVNVERSION) | tr -d [:cntrl:])
else
VERSION        =
endif

ECHO           = echo
PRINTF         = printf

MAKE           = make
CP             = cp
MV             = mv
RM             = rm -f
MKDIR          = mkdir
ifeq ($(ARCH),MACOSX)
RMDIR          = rmdir # XXX TODO xargs to remove .DS_Store
else
RMDIR          = rmdir --ignore-fail-on-non-empty
endif
TOUCH          = touch
CAT            = cat

AS             = ca65
LD             = ld65
C1541          = c1541

CC1541         = ../../tools/cc1541

PU             = pucrunch
BB             = BB

# CRUSH.EXE is a DOS program to which no source has been published
ifneq ($(findstring WIN,$(ARCH)),)
    ifeq (WIN64,$(ARCH))
            # WIN64, no native DOS support
DOSBOX         = DOSBox
LC             = ..\..\tools\crush $(LCSPEED)
    else
            # WIN32, CRUSH.EXE does not understand / instead of \
LC             = ../../tools/crush $(LCSPEED)
    endif
else ifeq ($(ARCH),MACOSX)
    # MacOSX, DOSBox must be installed as an application
DOSBOX         = /Applications/DOSBox.app/Contents/MacOS/DOSBox
LC             = ..\..\tools\crush $(LCSPEED)
else
    # default: use wine to run CRUSH.EXE
LC             = wine ../../tools/crush $(LCSPEED) # XXX TODO try dosemu
endif
LCSPEED        = 6

CONV           = java -cp ../../tools CompressedFileConverter
EXO            = exomizer


LOADER         = ../../src
LDRBIN         = ../../build
LDRINC         = ../../include

BUILDDIR       = ../../build
INTERMDIR      = ../../build/intermediate

RESOURCESDIR   = ../resources


AS_FLAGS       = -I $(LDRINC) -I ../../../shared
PU_FLAGS       = -d -l 0x1000 -x 0x1000 -i 0
ifeq ($(_PLATFORM_),c16)
AS_FLAGS      += -t c16 -D PLATFORM=16
PU_FLAGS      += -c16
else
AS_FLAGS      += -t c64 -D PLATFORM=64
PU_FLAGS      += -c64 -g 0x37
endif


default: all

tellarch:
	@$(ECHO) $(ARCH)


$(BUILDDIR):
	$(MKDIR) $(BUILDDIR)

$(INTERMDIR):
	$(MKDIR) $(INTERMDIR)


SHAREDDEPS     = $(LOADER)/Makefile \
                 $(LDRINC)/common-features.inc \
                 $(LDRINC)/config.inc

LOADERDEPS     = $(SHAREDDEPS) $(BUILDDIR) $(INTERMDIR) \
                 $(LOADER)/decompress/pudecomp.s $(LOADER)/decompress/lcdecomp.s $(LOADER)/decompress/bbdecomp.s $(LOADER)/decompress/exodecomp.s \
                 $(LOADER)/drives/drivecode1541-kernel.inc $(LOADER)/drives/drivecode1541.s $(LOADER)/drives/drivecode1541-loadfile.s $(LOADER)/drives/drivecode1571.s $(LOADER)/drives/drivecode1581.s \
                 $(LOADER)/hal/hal.inc $(LOADER)/hal/hal-c64.inc $(LOADER)/hal/hal-c16.inc \
                 $(LOADER)/install/install.s \
                 $(LOADER)/resident/resident.s $(LOADER)/resident/zp-resident.s \
                 $(LDRINC)/resident-jumptable.inc $(LDRINC)/install-jumptable.inc $(LDRINC)/loader.inc $(LDRINC)/diskio.inc

MODULE1541DEPS = $(SHAREDDEPS) \
                 $(LOADER)/drives/drivecode1541-kernel.inc $(LOADER)/drives/drivecode1541.s $(LOADER)/drives/drivecode1541-loadfile.s \
                 $(INTERMDIR)/kernelsymbols1541.inc

DISKIMAGEDEPS  = Makefile $(CC1541) \
                 $(INTERMDIR)/test-$(_PLATFORM_).prg $(LDRBIN)/loader-$(_PLATFORM_).so \
                 $(INTERMDIR)/sheba.bin $(INTERMDIR)/sheba.pu $(INTERMDIR)/sheba.bbconv $(INTERMDIR)/sheba.lcconv $(INTERMDIR)/sheba.exo \
                 $(INTERMDIR)/prlogo.bin $(INTERMDIR)/prlogo.pu $(INTERMDIR)/prlogo.bbconv $(INTERMDIR)/prlogo.lcconv $(INTERMDIR)/prlogo.exo \
                 $(INTERMDIR)/bothgfx.pu $(INTERMDIR)/bothgfx.bbconv $(INTERMDIR)/bothgfx.lcconv $(INTERMDIR)/bothgfx.exo \
                 $(INTERMDIR)/shebahi.bin $(INTERMDIR)/shebahi.pu $(INTERMDIR)/shebahi.bbconv $(INTERMDIR)/shebahi.lcconv $(INTERMDIR)/shebahi.exo \
                 $(INTERMDIR)/prlogohi.bin $(INTERMDIR)/prlogohi.pu $(INTERMDIR)/prlogohi.bbconv $(INTERMDIR)/prlogohi.lcconv $(INTERMDIR)/prlogohi.exo \
                 $(INTERMDIR)/bothgfxhi.pu $(INTERMDIR)/bothgfxhi.bbconv $(INTERMDIR)/bothgfxhi.lcconv $(INTERMDIR)/bothgfxhi.exo


all: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64 $(BUILDDIR)/loadertest-$(_PLATFORM_).d71 $(BUILDDIR)/loadertest-$(_PLATFORM_).d81



$(LDRBIN)/loader-$(_PLATFORM_).a: $(LOADERDEPS) $(LOADER)/install/dynlink.s $(LOADER)/decompress/exostreamdecr1.s $(LDRBIN)/loader-$(_PLATFORM_).so
	$(MAKE) -C $(LOADER) DEBUG=1 lib-full

$(LDRBIN)/loader-$(_PLATFORM_).so: $(LOADERDEPS)
	$(MAKE) -C $(LOADER) DEBUG=1 so-full

$(LDRBIN)/drivecode1541-loadfile.prg: $(MODULE1541DEPS)
	$(MAKE) -C $(LOADER) DEBUG=1 mods

$(INTERMDIR)/kernelsymbols1541.inc: $(LOADER)/drives/drivecode1541.s $(LOADER)/drives/drivecode1541-loadfile.s
	$(MAKE) -C $(LOADER) ../build/intermediate/kernelsymbols1541.inc # no usage of $(INTERMDIR) to match the path as seen from loader/src


../../.svn/entries:
ifeq ($(VERSION),)
	$(MKDIR) ../../.svn
	$(TOUCH) $@
endif

../../version.inc: ../../.svn/entries
	$(MAKE) -C ../.. version.inc


$(INTERMDIR)/test-$(_PLATFORM_).o: test.s zp.inc $(LOADERDEPS) $(LOADER)/install/dynlink.s ../../version.inc Makefile
	$(AS) $(AS_FLAGS) -o $@ $<

$(INTERMDIR)/test-$(_PLATFORM_)-uncompressed.prg: Linkfile $(INTERMDIR)/test-$(_PLATFORM_).o $(LDRBIN)/loader-$(_PLATFORM_).a
	$(LD) -C Linkfile -vm -m $@.map -Ln $@.lbl -o $@ $(INTERMDIR)/test-$(_PLATFORM_).o $(LDRBIN)/loader-$(_PLATFORM_).a

$(INTERMDIR)/test-$(_PLATFORM_).prg: $(INTERMDIR)/test-$(_PLATFORM_)-uncompressed.prg
	$(PU) $(PU_FLAGS) $? $@


$(INTERMDIR)/shebahi.bin: $(RESOURCESDIR)/sheba.bin
	$(PRINTF) "\x00\xC0" | $(CAT) - $? > $@

$(INTERMDIR)/prlogohi.bin: $(RESOURCESDIR)/prlogo.bin
	$(PRINTF) "\x00\xC0" | $(CAT) - $? > $@

$(INTERMDIR)/%.bin: $(RESOURCESDIR)/%.bin
	$(PRINTF) "\x00\x40" | $(CAT) - $? > $@

$(INTERMDIR)/%.pu: $(INTERMDIR)/%.bin
	$(PU) -c0 $? $@

$(INTERMDIR)/%.bb: $(INTERMDIR)/%.bin
	$(BB) $?
	$(MV) $?.bb $@

$(INTERMDIR)/%.bbconv: $(INTERMDIR)/%.bin $(INTERMDIR)/%.bb
	$(CONV) bb $^ $@

ifeq ($(DOSBOX),)
$(INTERMDIR)/%.lc: $(INTERMDIR)/%.bin
	$(LC) $? out
	$(MV) out $@
else
$(INTERMDIR)/%.lc: $(INTERMDIR)/%.bin
	export SDL_VIDEODRIVER=dummy && $(DOSBOX) -noconsole -c "MOUNT C ../.." -c "C:" -c "cd build\interm~1" -c "..\$(LC) $(notdir $?) out" -c "exit"
	$(MV) $(INTERMDIR)/OUT $@
endif

$(INTERMDIR)/%.lcconv: $(INTERMDIR)/%.bin $(INTERMDIR)/%.lc
	$(CONV) lc $^ $@

$(INTERMDIR)/%.exo: $(INTERMDIR)/%.bin
	$(EXO) mem -f $? -o $@

$(INTERMDIR)/bothgfx.%: $(INTERMDIR)/sheba.% $(INTERMDIR)/prlogo.%
	$(CAT) $^ > $@

$(INTERMDIR)/bothgfxhi.%: $(INTERMDIR)/shebahi.% $(INTERMDIR)/prlogohi.%
	$(CAT) $^ > $@


# NAME T/S on the .d64 when using cc1541
# SB-SHEBA.BIN         7/0
# SP-SHEBA.PU          9/0
# SV-SHEBA.BBCONV     10/0
# SL-SHEBA.LCCONV     11/0
# SE-SHEBA.EXO        12/0
# PB-PRLOGO.BIN       13/0
# PP-PRLOGO.PU        15/0
# PV-PRLOGO.BBCONV    16/0
# PL-PRLOGO.LCCONV    17/0
# PE-PRLOGO.EXO       19/0
# BP-BOTHGFX.PU       20/0
# BV-BOTHGFX.BBCONV   21/0
# BL-BOTHGFX.LCCONV   22/0
# BE-BOTHGFX.EXO      23/0
# TB-SHEBAHI.BIN      24/0
# TP-SHEBAHI.PU       26/0
# TV-SHEBAHI.BBCONV   27/0
# TL-SHEBAHI.LCCONV   28/0
# TE-SHEBAHI.EXO      29/0
# QB-PRLOGOHI.BIN     30/0
# QP-PRLOGOHI.PU      32/0
# QV-PRLOGOHI.BBCONV  33/0
# QL-PRLOGOHI.LCCONV  34/0
# QE-PRLOGOHI.EXO     35/0
# CP-BOTHGFXHI.PU     36/0
# CV-BOTHGFXHI.BBCONV 37/0
# CL-BOTHGFXHI.LCCONV 38/0
# CE-BOTHGFXHI.EXO    40/0

$(BUILDDIR)/loadertest-$(_PLATFORM_).d64: $(DISKIMAGEDEPS) $(LDRBIN)/drivecode1541-loadfile.prg
	$(CC1541) -n "NORMAL IS BORING" -i PLUSH -4 -S 8 \
	 \
	 -f "LOADER TEST"               -w $(INTERMDIR)/test-$(_PLATFORM_).prg \
	 -f "LOADER.SO"                 -w $(LDRBIN)/loader-$(_PLATFORM_).so \
	 -f "LOADER DRIVECODE"          -w $(LDRBIN)/drivecode1541-loadfile.prg \
      \
	 -f SB-SHEBA.BIN        -r 7 -e -w $(INTERMDIR)/sheba.bin \
	 -f SP-SHEBA.PU         -r 7 -e -w $(INTERMDIR)/sheba.pu \
	 -f SV-SHEBA.BBCONV     -r 7 -e -w $(INTERMDIR)/sheba.bbconv \
	 -f SL-SHEBA.LCCONV     -r 7 -e -w $(INTERMDIR)/sheba.lcconv \
	 -f SE-SHEBA.EXO        -r 7 -e -w $(INTERMDIR)/sheba.exo \
      \
	 -f PB-PRLOGO.BIN       -r 7 -e -w $(INTERMDIR)/prlogo.bin \
	 -f PP-PRLOGO.PU        -r 7 -e -w $(INTERMDIR)/prlogo.pu \
	 -f PV-PRLOGO.BBCONV    -r 7 -e -w $(INTERMDIR)/prlogo.bbconv \
	 -f PL-PRLOGO.LCCONV    -r 7 -e -w $(INTERMDIR)/prlogo.lcconv \
	 -f PE-PRLOGO.EXO       -r 7 -e -w $(INTERMDIR)/prlogo.exo \
      \
	 -f BP-BOTHGFX.PU       -r 7 -e -w $(INTERMDIR)/bothgfx.pu \
	 -f BV-BOTHGFX.BBCONV   -r 7 -e -w $(INTERMDIR)/bothgfx.bbconv \
	 -f BL-BOTHGFX.LCCONV   -r 7 -e -w $(INTERMDIR)/bothgfx.lcconv \
	 -f BE-BOTHGFX.EXO      -r 7 -e -w $(INTERMDIR)/bothgfx.exo \
      \
	 -f TB-SHEBAHI.BIN      -r 7 -e -w $(INTERMDIR)/shebahi.bin \
	 -f TP-SHEBAHI.PU       -r 7 -e -w $(INTERMDIR)/shebahi.pu \
	 -f TV-SHEBAHI.BBCONV   -r 7 -e -w $(INTERMDIR)/shebahi.bbconv \
	 -f TL-SHEBAHI.LCCONV   -r 7 -e -w $(INTERMDIR)/shebahi.lcconv \
	 -f TE-SHEBAHI.EXO      -r 7 -e -w $(INTERMDIR)/shebahi.exo \
      \
	 -f QB-PRLOGOHI.BIN     -r 7 -e -w $(INTERMDIR)/prlogohi.bin \
	 -f QP-PRLOGOHI.PU      -r 7 -e -w $(INTERMDIR)/prlogohi.pu \
	 -f QV-PRLOGOHI.BBCONV  -r 7 -e -w $(INTERMDIR)/prlogohi.bbconv \
	 -f QL-PRLOGOHI.LCCONV  -r 7 -e -w $(INTERMDIR)/prlogohi.lcconv \
	 -f QE-PRLOGOHI.EXO     -r 7 -e -w $(INTERMDIR)/prlogohi.exo \
      \
	 -f CP-BOTHGFXHI.PU     -r 7 -e -w $(INTERMDIR)/bothgfxhi.pu \
	 -f CV-BOTHGFXHI.BBCONV -r 7 -e -w $(INTERMDIR)/bothgfxhi.bbconv \
	 -f CL-BOTHGFXHI.LCCONV -r 7 -e -w $(INTERMDIR)/bothgfxhi.lcconv \
	 -f CE-BOTHGFXHI.EXO    -r 7 -e -w $(INTERMDIR)/bothgfxhi.exo \
      \
	$@


# NAME T/S on the .d71 when using cc1541
# SB-SHEBA.BIN         7/0
# SP-SHEBA.PU          8/0
# SV-SHEBA.BBCONV     43/0
# SL-SHEBA.LCCONV      9/0
# SE-SHEBA.EXO        44/0
# PB-PRLOGO.BIN       10/0
# PP-PRLOGO.PU        11/0
# PV-PRLOGO.BBCONV    46/0
# PL-PRLOGO.LCCONV    12/0
# PE-PRLOGO.EXO       47/0
# BP-BOTHGFX.PU       13/0
# BV-BOTHGFX.BBCONV   48/0
# BL-BOTHGFX.LCCONV   14/0
# BE-BOTHGFX.EXO      49/0
# TB-SHEBAHI.BIN      15/0
# TP-SHEBAHI.PU       16/0
# TV-SHEBAHI.BBCONV   51/0
# TL-SHEBAHI.LCCONV   17/0
# TE-SHEBAHI.EXO      52/0
# QB-PRLOGOHI.BIN     19/0
# QP-PRLOGOHI.PU      20/0
# QV-PRLOGOHI.BBCONV  55/0
# QL-PRLOGOHI.LCCONV  21/0
# QE-PRLOGOHI.EXO     56/0
# CP-BOTHGFXHI.PU     22/0
# CV-BOTHGFXHI.BBCONV 57/0
# CL-BOTHGFXHI.LCCONV 23/0
# CE-BOTHGFXHI.EXO    58/0

$(BUILDDIR)/loadertest-$(_PLATFORM_).d71: $(DISKIMAGEDEPS)
	$(CC1541) -n "NORMAL IS BORING" -i PLUSH -S 8 \
	 -f "LOADER TEST"                  -w $(INTERMDIR)/test-$(_PLATFORM_).prg \
	 -f "LOADER.SO"                    -w $(LDRBIN)/loader-$(_PLATFORM_).so   \
	 \
	 -f SB-SHEBA.BIN        -r 7 -e -c -w $(INTERMDIR)/sheba.bin \
	 -f SP-SHEBA.PU         -r 7 -e -c -w $(INTERMDIR)/sheba.pu \
	 -f SV-SHEBA.BBCONV     -r 7 -e -c -w $(INTERMDIR)/sheba.bbconv \
	 -f SL-SHEBA.LCCONV     -r 7 -e -c -w $(INTERMDIR)/sheba.lcconv \
	 -f SE-SHEBA.EXO        -r 7 -e -c -w $(INTERMDIR)/sheba.exo \
	 \
	 -f PB-PRLOGO.BIN       -r 7 -e -c -w $(INTERMDIR)/prlogo.bin \
	 -f PP-PRLOGO.PU        -r 7 -e -c -w $(INTERMDIR)/prlogo.pu \
	 -f PV-PRLOGO.BBCONV    -r 7 -e -c -w $(INTERMDIR)/prlogo.bbconv \
	 -f PL-PRLOGO.LCCONV    -r 7 -e -c -w $(INTERMDIR)/prlogo.lcconv \
	 -f PE-PRLOGO.EXO       -r 7 -e -c -w $(INTERMDIR)/prlogo.exo \
	 \
	 -f BP-BOTHGFX.PU       -r 7 -e -c -w $(INTERMDIR)/bothgfx.pu \
	 -f BV-BOTHGFX.BBCONV   -r 7 -e -c -w $(INTERMDIR)/bothgfx.bbconv \
	 -f BL-BOTHGFX.LCCONV   -r 7 -e -c -w $(INTERMDIR)/bothgfx.lcconv \
	 -f BE-BOTHGFX.EXO      -r 7 -e -c -w $(INTERMDIR)/bothgfx.exo \
	 \
	 -f TB-SHEBAHI.BIN      -r 7 -e -c -w $(INTERMDIR)/shebahi.bin \
	 -f TP-SHEBAHI.PU       -r 7 -e -c -w $(INTERMDIR)/shebahi.pu \
	 -f TV-SHEBAHI.BBCONV   -r 7 -e -c -w $(INTERMDIR)/shebahi.bbconv \
	 -f TL-SHEBAHI.LCCONV   -r 7 -e -c -w $(INTERMDIR)/shebahi.lcconv \
	 -f TE-SHEBAHI.EXO      -r 7 -e -c -w $(INTERMDIR)/shebahi.exo \
	 \
	 -f QB-PRLOGOHI.BIN     -r 7 -e -c -w $(INTERMDIR)/prlogohi.bin \
	 -f QP-PRLOGOHI.PU      -r 7 -e -c -w $(INTERMDIR)/prlogohi.pu \
	 -f QV-PRLOGOHI.BBCONV  -r 7 -e -c -w $(INTERMDIR)/prlogohi.bbconv \
	 -f QL-PRLOGOHI.LCCONV  -r 7 -e -c -w $(INTERMDIR)/prlogohi.lcconv \
	 -f QE-PRLOGOHI.EXO     -r 7 -e -c -w $(INTERMDIR)/prlogohi.exo \
	 \
	 -f CP-BOTHGFXHI.PU     -r 7 -e -c -w $(INTERMDIR)/bothgfxhi.pu \
	 -f CV-BOTHGFXHI.BBCONV -r 7 -e -c -w $(INTERMDIR)/bothgfxhi.bbconv \
	 -f CL-BOTHGFXHI.LCCONV -r 7 -e -c -w $(INTERMDIR)/bothgfxhi.lcconv \
	 -f CE-BOTHGFXHI.EXO    -r 7 -e -c -w $(INTERMDIR)/bothgfxhi.exo \
	 \
	$@


# NAME T/S on the .d81 when using c1541
# SB-SHEBA.BIN        $27,$00
# SP-SHEBA.PU         $27,$20
# SV-SHEBA.BBCONV     $29,$00
# SL-SHEBA.LCCONV     $29,$0c
# SE-SHEBA.EXO        $29,$18
# PB-PRLOGO.BIN       $29,$23
# PP-PRLOGO.PU        $26,$03
# PV-PRLOGO.BBCONV    $26,$09
# PL-PRLOGO.LCCONV    $26,$0f
# PE-PRLOGO.EXO       $26,$15
# BP-BOTHGFX.PU       $26,$1a
# BV-BOTHGFX.BBCONV   $2a,$1b
# BL-BOTHGFX.LCCONV   $25,$03
# BE-BOTHGFX.EXO      $25,$15
# TB-SHEBAHI.BIN      $25,$25
# TP-SHEBAHI.PU       $2b,$04
# TV-SHEBAHI.BBCONV   $2b,$0f
# TL-SHEBAHI.LCCONV   $2b,$1b
# TE-SHEBAHI.EXO      $2b,$27
# QB-PRLOGOHI.BIN     $24,$1d
# QP-PRLOGOHI.PU      $2c,$0a
# QV-PRLOGOHI.BBCONV  $2c,$10
# QL-PRLOGOHI.LCCONV  $2c,$16
# QE-PRLOGOHI.EXO     $2c,$1c
# CP-BOTHGFXHI.PU     $2c,$21
# CV-BOTHGFXHI.BBCONV $23,$15
# CL-BOTHGFXHI.LCCONV $23,$26
# CE-BOTHGFXHI.EXO    $2d,$0a

$(BUILDDIR)/loadertest-$(_PLATFORM_).d81 $(BUILDDIR)/loadertest-$(_PLATFORM_).d82: $(DISKIMAGEDEPS)
	$(C1541) -format "normal is boring,+h" $(subst .,,$(suffix $@)) $@
	$(C1541) -attach $@ \
	 -write $(INTERMDIR)/sheba.bin              "sb-sheba.bin" \
	 -write $(INTERMDIR)/sheba.pu               "sp-sheba.pu" \
	 -write $(INTERMDIR)/sheba.bbconv           "sv-sheba.bbconv" \
	 -write $(INTERMDIR)/sheba.lcconv           "sl-sheba.lcconv" \
	 -write $(INTERMDIR)/sheba.exo              "se-sheba.exo" \
	 \
	 -write $(INTERMDIR)/prlogo.bin             "pb-prlogo.bin" \
	 -write $(INTERMDIR)/prlogo.pu              "pp-prlogo.pu" \
	 -write $(INTERMDIR)/prlogo.bbconv          "pv-prlogo.bbconv" \
	 -write $(INTERMDIR)/prlogo.lcconv          "pl-prlogo.lcconv" \
	 -write $(INTERMDIR)/prlogo.exo             "pe-prlogo.exo" \
	 \
	 -write $(INTERMDIR)/bothgfx.pu             "bp-bothgfx.pu" \
	 -write $(INTERMDIR)/bothgfx.bbconv         "bv-bothgfx.bbconv" \
	 -write $(INTERMDIR)/bothgfx.lcconv         "bl-bothgfx.lcconv" \
	 -write $(INTERMDIR)/bothgfx.exo            "be-bothgfx.exo" \
	 \
	 -write $(INTERMDIR)/shebahi.bin            "tb-shebahi.bin" \
	 -write $(INTERMDIR)/shebahi.pu             "tp-shebahi.pu" \
	 -write $(INTERMDIR)/shebahi.bbconv         "tv-shebahi.bbconv" \
	 -write $(INTERMDIR)/shebahi.lcconv         "tl-shebahi.lcconv" \
	 -write $(INTERMDIR)/shebahi.exo            "te-shebahi.exo" \
	 \
	 -write $(INTERMDIR)/prlogohi.bin           "qb-prlogohi.bin" \
	 -write $(INTERMDIR)/prlogohi.pu            "qp-prlogohi.pu" \
	 -write $(INTERMDIR)/prlogohi.bbconv        "qv-prlogohi.bbconv" \
	 -write $(INTERMDIR)/prlogohi.lcconv        "ql-prlogohi.lcconv" \
	 -write $(INTERMDIR)/prlogohi.exo           "qe-prlogohi.exo" \
	 \
	 -write $(INTERMDIR)/bothgfxhi.pu           "cp-bothgfxhi.pu" \
	 -write $(INTERMDIR)/bothgfxhi.bbconv       "cv-bothgfxhi.bbconv" \
	 -write $(INTERMDIR)/bothgfxhi.lcconv       "cl-bothgfxhi.lcconv" \
	 -write $(INTERMDIR)/bothgfxhi.exo          "ce-bothgfxhi.exo" \
	 \
	 -write $(LDRBIN)/loader-$(_PLATFORM_).so   "loader.so" \
	 -write $(INTERMDIR)/test-$(_PLATFORM_).prg "loader test"


ifneq ($(USE_YAPE),0)
run run41 run51: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(YAPE) "..\..\build\loadertest-$(_PLATFORM_).d64"
else
run run41: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU41) $^

run51: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU51) $^
endif

run41ii run42: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU42) $^

run70: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU70) $^

run71: $(BUILDDIR)/loadertest-$(_PLATFORM_).d71
	$(EMU71) $^

run7141: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU7141) $^

run73: $(BUILDDIR)/loadertest-$(_PLATFORM_).d71
	$(EMU73) $^

run7341: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU7341) $^

ifneq ($(USE_YAPE),0)
run81: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(YAPE) "..\..\build\loadertest-$(_PLATFORM_).d81"
else
run81: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU81) "$^:loader test"
endif

run2000: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU2000) "$^:loader test"

run4000: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU4000) "$^:loader test"

run1001: $(BUILDDIR)/loadertest-$(_PLATFORM_).d82
	$(EMU1001) "$^:loader test"

ifneq ($(USE_YAPE),0)
run9 run941 run951: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(YAPE) /DISK9:"..\..\build\loadertest-$(_PLATFORM_).d64" "..\..\build\loadertest-$(_PLATFORM_).d64"
else
run9 run941: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU941) $^

run951: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU951) $^
endif

run941ii run942: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU942) $^

run970: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU970) $^

run971: $(BUILDDIR)/loadertest-$(_PLATFORM_).d71
	$(EMU971) $^

run97141: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU97141) $^

run973: $(BUILDDIR)/loadertest-$(_PLATFORM_).d71
	$(EMU973) $^

run97341: $(BUILDDIR)/loadertest-$(_PLATFORM_).d64
	$(EMU97341) $^

ifneq ($(USE_YAPE),0)
run981: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(YAPE) /DISK9:"..\..\build\loadertest-$(_PLATFORM_).d81" "..\..\build\loadertest-$(_PLATFORM_).d81"
else
run981: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU981) "$^:loader test"
endif

run92000: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU92000) "$^:loader test"

run94000: $(BUILDDIR)/loadertest-$(_PLATFORM_).d81
	$(EMU94000) "$^:loader test"

run91001: $(BUILDDIR)/loadertest-$(_PLATFORM_).d82
	$(EMU91001) "$^:loader test"

	
clean:
	-$(RM) \
	 $(INTERMDIR)/test-$(_PLATFORM_).o $(INTERMDIR)/test-$(_PLATFORM_)-uncompressed.prg $(INTERMDIR)/test-$(_PLATFORM_).prg \
	 $(INTERMDIR)/test-$(_PLATFORM_)-uncompressed.prg.map $(INTERMDIR)/test-$(_PLATFORM_)-uncompressed.prg.lbl \
	 $(INTERMDIR)/*.bin $(INTERMDIR)/*.pu $(INTERMDIR)/*.bb $(INTERMDIR)/*.bbconv $(INTERMDIR)/*.lc $(INTERMDIR)/*.lcconv $(INTERMDIR)/*.exo
	-$(RMDIR) $(INTERMDIR)

distclean:
	$(MAKE) -C $(LOADER) clean
	$(MAKE) -C $(CC1541)_source clean

wipe: distclean clean


$(CC1541): $(CC1541)_source/cc1541.cpp
	$(MAKE) -C $(CC1541)_source
