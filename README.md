# Simulating a Partial 8086 Disassembler

This is a simple Intel 8086/8088 “emulator” that can **disassemble** 8086/88 machine code into human-readable instructions.

---

## Features

- Partial Intel 8086/8088 instruction decoder  
- Prints each instruction and its operands in familiar NASM-style syntax  
- Lightweight, header-only C++ code (no external dependencies beyond STL)  

## Requirements

- NASM (for assembling your `.asm` into binary)  
- A C++ compiler 

## Usage

1. Assemble your `.asm` into binary  
   ```bash
   nasm single_register_mov.asm
   ```
2. Compile the simulator  
   ```bash
   g++ -o sim8086 sim8086.cpp
   ```
3. Run the disassembler  
   ```bash
   ./sim8086 <binary>
   ```

### Input

A binary file containing Intel 8086/8088 machine-code bytes.

### Output

Disassembled instructions are printed to stdout in NASM-style syntax, for example:

```
MOV AL, 0x5A
ADD BX, CX
INT 0x21
...
```

