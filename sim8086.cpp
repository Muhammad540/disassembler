// A partial 8086 emulator
// objective: read the binary file and disassemble it to match the original source assemmbly 

// single_register_mov.asm -> source assembly 
// single_register_mov -> binary file 

#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <vector>


/*
lookup table for 8086 register names, where 
the first dimension (2) represents register size (8-bit/16-bit) and 
the second dimension (8) represents the register index.
*/
static constexpr const char regTable[2][8][3] = {
    // W = 0 -> 8-bit registers
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    // W = 1 -> 16-bit registers
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

const char* getRegName(uint8_t w, uint8_t reg) {
    if (w > 1 || reg > 7){
        throw std::out_of_range("Invalid register index");
    }
    return regTable[w][reg];
}

/*
lookup for register/memory field encoding 
FOR MOD = 11 (register mode no memory displacement)
*/
static constexpr const char regMemTable[2][8][3] = {
    // W = 0 -> 8-bit registers
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    // W = 1 -> 16-bit registers
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

const char* getRegMemName(uint8_t w, uint8_t regMem) {
    if (w > 1 || regMem > 7){
        throw std::out_of_range("Invalid register index");
    }
    return regMemTable[w][regMem];
}

int main(int argc, char *argv[]){
    if (argc !=2){
        std::cerr << "Usage: " << argv[0] << " <binary_file>" << std::endl;
        return 1;
    }

    // open the binary file
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    // get the file size 
    in.seekg(0, std::ios::end);
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    // read the file into a buffer
    std::vector<char> buffer(size);
    if (!in.read(buffer.data(), size)) {
        std::cerr << "Error: only read " << in.gcount() << " bytes" << std::endl;
        return 1;
    }
    in.close();

    // process the bytes
    // for (unsigned char byte: buffer){
    //     std::cout << std::hex 
    //               << std::uppercase 
    //               << static_cast<int>(byte) << ' ';
    // }
    // std::cout << std::dec << std::endl;

    // 10001001 
    //   100010 ( bit shift right by 2)
    // 00000011 ( mask for last two bits Ox03 & with 10001001)
    
    // 11011001
    //       11 ( bit shift right by 6)
    //    11011 ( bit shift right by 3, then mask with 0x07 0111) 
    //      001 ( mask with 0x07)
    size_t pc = 0;
    int inst_len = 2;   
    while (pc+1 < buffer.size()){
        uint8_t b0 = buffer[pc];
        uint8_t b1 = buffer[pc+1];
        uint8_t b2 = buffer[pc+2];
        uint8_t b3 = buffer[pc+3];
        // first byte decoding
        uint8_t opcode = b0 >> 2;

        // Register/memory to/from register
        if (opcode == 0b100010) {
            uint8_t d = b0 & 0x02;
            uint8_t w = b0 & 0x01;
            // second byte decoding
            uint8_t mod = b1 >> 6;
            uint8_t reg = (b1 >> 3) & 0x07;
            uint8_t r_m = b1 & 0x07;

            if (mod == 0b01) inst_len = 3;
            else if (mod == 0b10) inst_len = 4;
            else if (mod == 0b00 && r_m == 0b110) inst_len = 4;
            if (pc + inst_len - 1 >= buffer.size()) break;
            
            std::cout << "mov ";

            if (mod == 0b11) { // mod 0b11 is a register to register move
                if (d == 0) {
                    // reg field is the source operand
                    // r_m field is the destination operand
                    // register mode with memory displacement
                    const char *regMemName = getRegMemName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                    std::cout << ", ";
                    // get register name 
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << std::endl;
                } else {
                    // reg field is the destination operand
                    // r_m field is the source operand
                    // get register name 
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    // register mode with memory displacement
                    const char *regMemName = getRegMemName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                    std::cout << std::endl;
                }
            } else if (mod == 0b00){
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si]";
                            break;                                  
                        case 0b001:
                            std::cout << "[bx + di]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di]";
                            break;
                        case 0b100:
                            std::cout << "[si]";
                            break;
                        case 0b101:
                            std::cout << "[di]";
                            break;
                        case 0b110:{
                            uint8_t direct_address = uint8_t(b3 << 8 | b2);
                            std::cout << "[" << direct_address << "]";
                            break;
                        }
                        case 0b111:
                            std::cout << "[bx]";
                            break;
                    }
                    std::cout << ", ";
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << std::endl;
                } else {
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di]";
                            break;
                        case 0b100:
                            std::cout << "[si]";
                            break;
                        case 0b101:
                            std::cout << "[di]";
                            break;
                        case 0b110:{
                            int16_t direct_address = int16_t(b3 << 8 | b2);
                            std::cout << "[" << direct_address << "]";
                            break;
                        }
                        case 0b111:
                            std::cout << "[bx]";
                            break;
                    }
                    std::cout << std::endl;
                }
            } else if (mod == 0b01){
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << (int8_t)b2 << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << (int8_t)b2 << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << (int8_t)b2 << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << (int8_t)b2 << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << (int8_t)b2 << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << (int8_t)b2 << "]";
                            break;
                        case 0b110:
                            std::cout << "[bp + " << (int8_t)b2 << "]";
                            break;
                        case 0b111:
                            std::cout << "[bx + " << (int8_t)b2 << "]";
                            break;
                    }
                    std::cout << ", ";
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << std::endl;
                } else {
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << (int8_t)b2 << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << (int8_t)b2 << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << (int8_t)b2 << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << (int8_t)b2 << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << (int8_t)b2 << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << (int8_t)b2 << "]";
                            break;
                        case 0b110:
                            std::cout << "[bp + " << (int8_t)b2 << "]";
                            break;
                        case 0b111:
                            std::cout << "[bx + " << (int8_t)b2 << "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            } else if (mod == 0b10){
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b100:
                            std::cout << "[si + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b101:
                            std::cout << "[di + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b110:
                            std::cout << "[bp + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                        case 0b111:
                            std::cout << "[bx + " << (int16_t)(b3 << 8 | b2) << "]" << std::endl;
                            break;
                    }
                    std::cout << ", ";
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << std::endl;
                } else {
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    switch (r_m){    
                        case 0b000:
                            std::cout << "[bx + si + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b110:
                            std::cout << "[bp + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                        case 0b111:
                            std::cout << "[bx + " << (int16_t)(b3 << 8 | b2) << "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            }
        }

        pc += inst_len;
    }
    return 0;
}