e#summary anim64 file formats

## Datastorm 2012 version ##

### Onefiler format ###

  * Player code
  * For each animation:
    * Load address (big-endian)
    * Animation size (big-endian)
    * Animation
  * End of file marker (0 byte)

### Anim file ###

  * Byte 0: Use interframe (xor) compression (1 = yes, 0 = no)
  * Byte 1 and up: All frames, compressed in one go.

The compression is done in these steps:

  1. xor frames with previous frames (if using interframe compression)
  1. pack color nibbles so that 0x9000-0xa000 is packed to 0x9000-0x97d0
  1. RLE pack area 0x8000-0x97d0

When frames are again unpacked to e.g. 0x8000, following bytes have special meaning...

```
0x83e8: border color, frame 0
0x83e9: bg color, frame 0 
0x83ea: end frame (0-3) 
0x83eb: version (1)
0x87e8: border color, frame 1
0x87e9: bg color, frame 1 
0x8be8: border color, frame 2
0x8be9: bg color, frame 2 
0x8fe8: border color, frame 3
0x8fe9: bg color, frame 3
```

## New version ##

### Animation ###

  * Byte 0: Version = 2
  * Byte 1: Frame count

For each frame...

  * Byte 0 - ?: Packed data

For following frames:

  * Byte 0 - ?: Packed data
  * Extra byte: XOR with previous frame (0 = no, 1 = yes)

Special meanings within unpacked frame...

```
0x3e8: (border_color << 4) | (bg_color)
```

### Onefiler format ###

  * Player code (including music)
  * For each animation:
    * Pointer to next animation
    * Animation
  * End of file marker (0 int)