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

  
A typical monitoring output that the linker produces is below.  In the example, printf is being loaded, linked and resolved.
```
Linking.....
Searching Library for symbol-> printf    

Loading.....printf\pri
      0 Data Bytes at    0"    0 printf\pri

   1230 Code Bytes at  A9E'  BA2 printf\pri
     1B82 -t-xr -s-_ccgt     
     183E -t-xr -s-_fpgt     
     1160 -t-xr -s-_cceq     
     1A56 -t-xr -s-_fpdiv    
     1780 -t-xr -s-_fpeq     
     181C -t-xr -s-_minusfa  
      AA0 -t-ep -s-_Count    
     1C98 -t-xr -s-putc      
     108A -t-xr -s-_ccswitc  
     1AC4 -t-xr -s-_fload    
      EBA -t-xr -s-itod      
      A9E -t-ep -s-printf\pri
      EE4 -t-xr -s-itox      
        0 -t-xr -s-_ccfloor  
        0 -t-xr -s-_ccfloor  
     19EC -t-xr -s-_ccle     
      E96 -t-xr -s-putchar   
     10C4 -t-xr -s-_ccne     
     1ACC -t-xr -s-_fpsub    
     1AC8 -t-xr -s-_fpmul    
     1A28 -t-xr -s-_fpadd    
     1AB4 -t-xr -s-float     
     18E4 -t-xr -s-_fplt     
     1986 -t-xr -s-_fpge     
     1A30 -t-xr -s-floor     
     131A -t-ep -s-ftoa      
      B92 -t-ep -s-_printf   
      DD0 -t-xr -s-utoi      
     16F2 -t-ep -s-ftoe      
     1C70 -t-ep -s-_outc     
      F58 -t-xr -s-itou      
      A9E -t-ep -s-_String   
        0 -t-xr -s-_ccfloat  
        0 -t-xr -s-_ccfloat  
     1BF2 -t-xr -s-_ccdiv    
      ADC -t-ep -s-fprintf   
      AA2 -t-ep -s-printf    
     1ABA -t-xr -s-_fpush    
        0 -t-xr -s-_ccifix   
        0 -t-xr -s-_ccifix   
     1AD2 -t-xr -s-_fstore   
      B3C -t-ep -s-sprintf   
      B40 -t-xr -s-_argcnt   
     1A5E -t-xr -s-ifix      

Linking.....
 Resolving external xt =    1  A62' to  BA6 for printf    nxr =    0
 Resolving external xt =    1  A2C' to  BA6 for printf    nxr =  A2C
 Resolving external xt =    1  9F6' to  BA6 for printf    nxr =  9F6
 Resolving external xt =    1  740' to  BA6 for printf    nxr =  740
 Resolving external xt =    1  420' to  BA6 for printf    nxr =  420
 Resolving external xt =    1  3F6' to  BA6 for printf    nxr =  3F6
 Resolving external xt =    1  3AE' to  BA6 for printf    nxr =  3AE
 Resolving external xt =    1  354' to  BA6 for printf    nxr =  354
 Resolving external xt =    1  310' to  BA6 for printf    nxr =  310
 Resolving external xt =    1  2C2' to  BA6 for printf    nxr =  2C2
 Resolving external xt =    1  206' to  BA6 for printf    nxr =  206
 Resolving external xt =    1  1E4' to  BA6 for printf    nxr =  1E4
 Resolving external xt =    1  1CC' to  BA6 for printf    nxr =  1CC
 Resolving external xt =    1  15A' to  BA6 for printf    nxr =  15A
 Resolving external xt =    1   E8' to  BA6 for printf    nxr =   E8
 Resolving external xt =    1   76' to  BA6 for printf    nxr =   76utoi  
```
