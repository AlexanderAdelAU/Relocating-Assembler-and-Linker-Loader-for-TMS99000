# Relocating-Assembler-and-Linker-Loader-for-TMS99000
Stand alone Relocating Assembler and Linker that works with Small-C-Plus for the TMS99000 CPU.  The programmes have been built using the Eclipse IDE Version: 2020-03 (4.15.0) and the project files for both R99 and Link99 are included as zip files to allow easy building and execution.

The relocating assembler uses identical syntax to the A99 assembler with the addtion of recognising the pseudo opcodes of ENT and EXT or Entry and External variable references.  Internal references can also be signalled using the syntax of "VariableName::" and external references using the syntax of "VariableName##"

The Relocating assemble using the standar Microsoft REL format with the exception that formated has been modified slightly (3 bits to 4 bits) to allow for variable names to be extened to 16 characters.  A very useful utility **DREL** allows you to dump the .R99, .LIB files to view the generated object files to help debug any external references etc.

The Assembler is invokded using the following syntax

***R99 fileName SCHCLC*** 
1.   SC => Source File (.A99) will be found on drive C in the same directory as R99
2.   HC => HEX File (.R99) output file will be created on drive C in the same directory as R99, and
3.   LC => LST File (.L99) output file will be created on drive C in the same directory as R99

The linker loader which takes the .R99 (object modules) and .LIB files will produce the executable .COM file by resolving all the external references either within the .R99 or .LIB files.

The Linker is invokded using the following syntax

***link99  -M  -S cTest99 abc clib99.LIB iolib99.LIB*** in the case where libraries and object modules are available or 
***link99  -M  -S cTest99 abc call etc *** in the case where object modules are to be used. 

***Note:*** the Microsoft REL format is described here https://www.seasip.info/Cpm/rel.html#:~:text=The%20REL%20format%20is%20generated%20by%20Microsoft's%20M80%20and%20Digital%20Research's%20RMAC.&text=REL%20files%20contain%20information%20encoded,value%20of%20the%20location%20counter.

  
