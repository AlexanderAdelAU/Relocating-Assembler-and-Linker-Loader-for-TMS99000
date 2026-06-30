# TMS99105 BASIC99 Build & Object Dump Documentation

This document logs the compilation workflow, overlay memory configuration, and relocatable object file structure for the **TMS99105 BASIC99** system. It showcases a sample build sequence using the `Link99` relocatable object linker alongside target diagnostic dumps using the `DREL` object library utility.

---

## Example Link99 Build (with `-M` Monitor Switch)

The toolchain orchestrates multi-segmented memory spaces, resolving symbols across independent relocatable core and overlay units.

```text
----------------------------------------------------
TMS9900 Relocatable Object Linker  Version 3.9.27
Original CP/M version: Alexander Cameron, January 1985
MSDOS/PC port:         Alexander Cameron, May 2010 - July 2019
Cleaner command syntax: Version 3.9.27
----------------------------------------------------
  42688 Byte Buffer

Phase 1 - Loading object and library files

Page mode: curpage= 0
Opening R99 File bascore.R99

Loading module: bascore g=4 g=E
      0 Data Bytes at     0"     0 bascore
 g=1
- Note: module bascore spans more than one 4KB segment.

   1064 Code Bytes at     0'  1000 bascore
 g=0  AORG_MARK: addr=1000 base=1000
  PREL seen: field=  0 pagemode=YES pmmin_valid=NO
  [... Pages of intermediate PREL listings truncated for brevity ...]
 g=A g=A g=A g=B g=2  AORG SETLC fixup: base=1000 end=205E

Linking.....
Opening R99 File ovlmgr.R99

Loading module: OVLMGR g=4 g=4 g=4 g=E
      0 Data Bytes at     0"     0 OVLMGR
 g=1
     56 Code Bytes at  105E'  205E OVLMGR
  PREL seen: field=066 pagemode=YES pmmin_valid=NO
  PREL seen: field=066 pagemode=YES pmmin_valid=YES
  PREL seen: field=05E pagemode=YES pmmin_valid=YES
  PREL seen: field=066 pagemode=YES pmmin_valid=YES
 g=B g=B g=B g=2  AORG fixup: base=1000 end=1056

Linking.....
 Resolving external xt =    1 xrloc=1016' to 108E for MAP_SEGMENT  nxr=   0
 Resolving external xt =    1 xrloc=1A62' to 1068 for OVLMGR       nxr=   0
 Resolving external xt =    1 xrloc=1A02' to 1068 for OVLMGR       nxr=1A02
 Resolving external xt =    1 xrloc=19F6' to 1068 for OVLMGR       nxr=19F6
 Resolving external xt =    1 xrloc=1958' to 1068 for OVLMGR       nxr=1958
 Resolving external xt =    1 xrloc=192A' to 1068 for OVLMGR       nxr=192A
 Resolving external xt =    1 xrloc=18F2' to 1068 for OVLMGR       nxr=18F2
 Resolving external xt =    1 xrloc=174E' to 1068 for OVLMGR       nxr=174E
 Resolving external xt =    1 xrloc=16FC' to 1068 for OVLMGR       nxr=16FC
 Resolving external xt =    1 xrloc=148C' to 1068 for OVLMGR       nxr=148C
 Resolving external xt =    1 xrloc=1466' to 1068 for OVLMGR       nxr=1466
 Resolving external xt =    1 xrloc=13DC' to 1068 for OVLMGR       nxr=13DC
 Resolving external xt =    1 xrloc=1316' to 1068 for OVLMGR       nxr=1316
 Resolving external xt =    1 xrloc=12B6' to 1068 for OVLMGR       nxr=12B6
 Resolving external xt =    1 xrloc=1292' to 1068 for OVLMGR       nxr=1292
 Resolving external xt =    1 xrloc=116A' to 1068 for OVLMGR       nxr=116A
 Resolving external xt =    1 xrloc=1146' to 1068 for OVLMGR       nxr=1146
 Resolving external xt =    1 xrloc=1088' to 1068 for OVLMGR       nxr=1088
 Resolving external xt =    1 xrloc=1056' to 1068 for OVLMGR       nxr=1056
 Resolving external xt =    1 xrloc=100A' to 10A6 for OVLMGR_INIT  nxr=   0
Page mode: curpage= 2
Opening R99 File basovl.R99

Loading module: BASOVL g=4 g=4 g=E
      0 Data Bytes at     0"     0 BASOVL
 g=1
    204 Code Bytes at  10B4'  20B4 BASOVL
 g=0  AORG_MARK: addr=2000 base=2000
 g=F g=B g=B g=2  AORG SETLC fixup: base=2000 end=21B2

Linking.....
Page mode: curpage= 3
Opening R99 File basmath.R99

Loading module: basMATH g=4 g=4 g=E
      0 Data Bytes at     0"     0 basMATH
 g=1
    244 Code Bytes at  1266'  2266 basMATH
 g=0  AORG_MARK: addr=2000 base=2000
 g=F g=B g=B g=2  AORG SETLC fixup: base=2000 end=223E

Linking.....
Page mode: curpage= 4
Opening R99 File ovltoken.R99

Loading module: OVLTOKEN g=4 g=E
      0 Data Bytes at     0"     0 OVLTOKEN
 g=1
    2EE Code Bytes at  14A4'  24A4 OVLTOKEN
 g=0  AORG_MARK: addr=2000 base=2000
 g=B g=2  AORG SETLC fixup: base=2000 end=22EE

Linking.....
 Searching for main in cbase=1000 pmmin_global=FFFF

	CODE SIZE   1792 (1000-2791)
	DATA SIZE      0

Phase 2 - Writing execution files

	EXE CHAIN BLOCK pg= 0 start=1000 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=11F8 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=13F0 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=15E8 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=17E0 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=19D8 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=1BD0 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=1DC8 size= 1F8
	EXE CHAIN BLOCK pg= 0 start=1FC0 size=  9E
	EXE CHAIN BLOCK pg= 2 start=2000 size= 1B2
	EXE CHAIN BLOCK pg= 3 start=2000 size= 1F8
	EXE CHAIN BLOCK pg= 3 start=21F8 size=  46
	EXE CHAIN BLOCK pg= 4 start=2000 size= 1F8
	EXE CHAIN BLOCK pg= 4 start=21F8 size=  F6
Finished
