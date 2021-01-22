# Relocating-Assembler-and-Linker-Loader-for-TMS99000
Stand alone Relocating Assembler and Linker that works with Small-C-Plus for the TMS99000 CPU.

The relocating assembler uses identical syntax to the A99 assembler with the addtion of recognising the pseudo opcodes of ENT and EXT or Entry and External variable references.  Internal references can also be signalled using the syntax of "VariableName::" and external references using the syntax of "VariableName##"

The Relocating assemble using the standar Microsoft REL format with the exception that formated has been modified slightly (3 bits to 4 bits) to allow for variable names to be extened to 16 characters.

The Assembler is invokded using the following syntax

***R99 fileName SCHCLC*** 
1.   SC => Source File (.A99) will be found on drive C in the same directory as R99
2.   HC => HEX File (.R99) output file will be created on drive C in the same directory as R99, and
3.   LC => LST File (.L99) output file will be created on drive C in the same directory as R99
  
