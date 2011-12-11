
SVNVERSION       = svnversion
ifneq ($(wildcard .svn/format),)
VERSION          = $(shell $(SVNVERSION) | tr -d [:cntrl:])
else
VERSION          =
endif

CD               = cd
PRINTF           = printf
MKDIR            = mkdir
TOUCH            = touch
ZIP              = zip -9
ZIP_NO_PATHS     = $(ZIP) -j
ifneq ($(VERSION),)
ZIPFILE          = loader-v$(VERSION).zip
LOADERTEST       = loadertest-v$(VERSION).zip
else
ZIPFILE          = loader.zip
LOADERTEST       = loadertest.zip
endif


SHAREDFOLDER     = shared
LOADERFOLDER     = $(notdir $(shell pwd))
DOCSFOLDER       = $(LOADERFOLDER)/docs
SRCFOLDER        = $(LOADERFOLDER)/src
INCFOLDER        = $(LOADERFOLDER)/include
SAMPLESFOLDER    = $(LOADERFOLDER)/samples
BUILDFOLDER      = $(LOADERFOLDER)/build
TOOLSFOLDER      = $(LOADERFOLDER)/tools


ZIPLIST         += $(SHAREDFOLDER)/standard.inc
ZIPLIST         += $(SHAREDFOLDER)/cpu.inc
ZIPLIST         += $(SHAREDFOLDER)/vic.inc
ZIPLIST         += $(SHAREDFOLDER)/cia.inc
ZIPLIST         += $(SHAREDFOLDER)/ted.inc
ZIPLIST         += $(SHAREDFOLDER)/via.inc
ZIPLIST         += $(SHAREDFOLDER)/kernal.inc
ZIPLIST         += $(SHAREDFOLDER)/basic.inc
ZIPLIST         += $(SHAREDFOLDER)/float.inc

ZIPLIST         += $(LOADERFOLDER)/Makefile
ZIPLIST         += $(LOADERFOLDER)/version.inc

ZIPLIST         += $(DOCSFOLDER)/Prerequisites.txt
ZIPLIST         += $(DOCSFOLDER)/FastFileFormat.txt
ZIPLIST         += $(DOCSFOLDER)/Protocols.txt
ZIPLIST         += $(DOCSFOLDER)/GCRTiming.txt
ZIPLIST         += $(DOCSFOLDER)/FilenameHashFunction.txt

DECOMPFOLDER     = decompress
DRIVESFOLDER     = drives
HALFOLDER        = hal
INSTALLFOLDER    = install
RESIDENTFOLDER   = resident
ZIPLIST         += $(SRCFOLDER)/Makefile
ZIPLIST         += $(SRCFOLDER)/make-loadersymbolsinc.pl
ZIPLIST         += $(SRCFOLDER)/$(DECOMPFOLDER)/pudecomp.s
ZIPLIST         += $(SRCFOLDER)/$(DECOMPFOLDER)/lcdecomp.s
ZIPLIST         += $(SRCFOLDER)/$(DECOMPFOLDER)/bbdecomp.s
ZIPLIST         += $(SRCFOLDER)/$(DECOMPFOLDER)/exodecomp.s
ZIPLIST         += $(SRCFOLDER)/$(DECOMPFOLDER)/exostreamdecr1.s
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/drivecode1541-kernel.inc
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/drivecode1541.s
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/drivecode1541-loadfile.s
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/drivecode1571.s
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/drivecode1581.s
ZIPLIST         += $(SRCFOLDER)/$(DRIVESFOLDER)/make-kernelsymbolsinc.pl
ZIPLIST         += $(SRCFOLDER)/$(HALFOLDER)/hal.inc
ZIPLIST         += $(SRCFOLDER)/$(HALFOLDER)/hal-c64.inc
ZIPLIST         += $(SRCFOLDER)/$(HALFOLDER)/hal-c16.inc
ZIPLIST         += $(SRCFOLDER)/$(INSTALLFOLDER)/install.s
ZIPLIST         += $(SRCFOLDER)/$(INSTALLFOLDER)/dynlink.s
ZIPLIST         += $(SRCFOLDER)/$(RESIDENTFOLDER)/zp-resident.s
ZIPLIST         += $(SRCFOLDER)/$(RESIDENTFOLDER)/resident.s

ZIPLIST         += $(INCFOLDER)/config.inc
ZIPLIST         += $(INCFOLDER)/loader.inc
ZIPLIST         += $(INCFOLDER)/diskio.inc
ZIPLIST         += $(INCFOLDER)/option-check.inc
ZIPLIST         += $(INCFOLDER)/common-features.inc
ZIPLIST         += $(INCFOLDER)/install-jumptable.inc
ZIPLIST         += $(INCFOLDER)/resident-jumptable.inc

MINEXAMPLEFOLDER = minexample
RESOURCESFOLDER  = resources
TESTFOLDER       = test
ZIPLIST         += $(SAMPLESFOLDER)/$(RESOURCESFOLDER)/sheba.bin
ZIPLIST         += $(SAMPLESFOLDER)/$(RESOURCESFOLDER)/prlogo.bin
ZIPLIST         += $(SAMPLESFOLDER)/$(MINEXAMPLEFOLDER)/Makefile
ZIPLIST         += $(SAMPLESFOLDER)/$(MINEXAMPLEFOLDER)/Linkfile
ZIPLIST         += $(SAMPLESFOLDER)/$(MINEXAMPLEFOLDER)/minexample.s
ZIPLIST         += $(SAMPLESFOLDER)/$(MINEXAMPLEFOLDER)/loaderconfig.inc
ZIPLIST         += $(SAMPLESFOLDER)/$(TESTFOLDER)/Makefile
ZIPLIST         += $(SAMPLESFOLDER)/$(TESTFOLDER)/Linkfile
ZIPLIST         += $(SAMPLESFOLDER)/$(TESTFOLDER)/test.s
ZIPLIST         += $(SAMPLESFOLDER)/$(TESTFOLDER)/zp.inc

CC1541SRCFOLDER  = cc1541_source
ZIPLIST         += $(TOOLSFOLDER)/CompressedFileConverter.java
ZIPLIST         += $(TOOLSFOLDER)/CompressedFileConverter.class
ZIPLIST         += $(TOOLSFOLDER)/CRUSH.EXE
ZIPLIST         += $(TOOLSFOLDER)/cwsdpmi.exe
ZIPLIST         += $(TOOLSFOLDER)/$(CC1541SRCFOLDER)/Makefile
ZIPLIST         += $(TOOLSFOLDER)/$(CC1541SRCFOLDER)/cc1541.cpp
ZIPLIST         += $(TOOLSFOLDER)/$(CC1541SRCFOLDER)/filefinder.cpp
ZIPLIST         += $(TOOLSFOLDER)/$(CC1541SRCFOLDER)/XGetOpt.cpp
ZIPLIST         += $(TOOLSFOLDER)/$(CC1541SRCFOLDER)/XGetOpt.h

DISKIMAGES       = $(BUILDFOLDER)/loadertest-c64.d64
DISKIMAGES      += $(BUILDFOLDER)/loadertest-c64.d71
DISKIMAGES      += $(BUILDFOLDER)/loadertest-c64.d81
DISKIMAGES      += $(BUILDFOLDER)/loadertest-c16.d64
DISKIMAGES      += $(BUILDFOLDER)/loadertest-c16.d71
DISKIMAGES      += $(BUILDFOLDER)/loadertest-c16.d81

ZIPLIST         += $(DISKIMAGES)


default: loader

loader:
	$(MAKE) -C src

run:
	$(CD) ..; $(MAKE) -C $(SAMPLESFOLDER)/$(TESTFOLDER) run

$(BUILDFOLDER)/loadertest-c64.d64:
	$(CD) ..; $(MAKE) -C $(SAMPLESFOLDER)/$(TESTFOLDER) all

$(BUILDFOLDER)/loadertest-c16.d64:
	$(CD) ..; $(MAKE) -C $(SAMPLESFOLDER)/$(TESTFOLDER) PLATFORM=c16 all

zip: $(BUILDFOLDER)/loadertest-c64.d64 $(BUILDFOLDER)/loadertest-c16.d64 version.inc
	$(CD) ..; $(ZIP) $(ZIPFILE) $(ZIPLIST)

loadertest: $(BUILDFOLDER)/loadertest-c64.d64 $(BUILDFOLDER)/loadertest-c16.d64 version.inc
	$(CD) ..; $(ZIP_NO_PATHS) $(LOADERTEST) $(DISKIMAGES)


.svn/entries:
ifeq ($(VERSION),)
	$(MKDIR) ./.svn
	$(TOUCH) $@
endif

version.inc: .svn/entries
ifneq ($(VERSION),)
	$(PRINTF) '%s' '.define REPOSITORY_VERSION "' > $@
	$(PRINTF) '%s' $(VERSION) >> $@
	$(PRINTF) '%s' '"' >> $@
endif
