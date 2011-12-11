
.segment "ZEROPAGE" : zeropage

DRIVETYPE              = $02; unused
.if PLATFORM = diskio::platform::COMMODORE_16
PALNTSC                = $03
.else
PALNTSC                = $a8; RS232 Input Bit count/Tape temporary
.endif

X_SHEBA_UNCOMPRESSED   = $9e; Tape Error log pass 1
Y_SHEBA_UNCOMPRESSED   = $9f; Tape Error log pass 2
X_PRHEI_UNCOMPRESSED   = $a0; Real-time jiffy Clock hi
Y_PRHEI_UNCOMPRESSED   = $a1; Real-time jiffy Clock mid
X_SHEBA_COMPRESSED     = $a2; Real-time jiffy Clock lo
Y_SHEBA_COMPRESSED     = $b2; Pointer: Start Address of Tape Buffer
X_PRHEI_COMPRESSED     = $b3; Pointer: Start Address of Tape Buffer
Y_PRHEI_COMPRESSED     = $b4; RS232 Write bit count/Tape Read timing Flag
X_BOTH_COMPRESSED      = $b5; RS232 Next Bit to send/Tape Read - End of Tape
Y_BOTH_COMPRESSED      = $b6; RS232 Output Byte Buffer/Tape Read Error Flag

POINTERS:              .res 6
