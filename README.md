# Code optimization

## 8086 Partial Emulator

This is a partial 8086 emulator that can disassemble 8086 instructions.

### Usage

```bash
    nasm <single_register_mov.asm or many_register_mov.asm>
    g++ -o sim8086 sim8086.cpp
    ./sim8086 <binary_file>
```

### Input file

The input file should be a binary file that contains the 8086 instructions.

### Output

The output will be the disassembled instructions.


