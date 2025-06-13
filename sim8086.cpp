/* A partial 8086 emulator: reads the binary file and disassemble it to match the original source assemmbly 

example:
    single_register_mov.asm -> source assembly 
    single_register_mov -> binary file 
*/
#include <iostream>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <vector>

typedef uint8_t u8;
typedef uint16_t u16;

/*
 * lookup table for 8086/88 register names, where 
 */
static constexpr const char regTable[2][8][3] = {
    // W = 0 -> 8-bit registers
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    // W = 1 -> 16-bit registers
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

/* value of 'w' bit and 'reg' is used to index into the regTable
 */
const char* getRegName(u8 w, u8 reg) {
    if (w > 1 || reg > 7){
        throw std::out_of_range("Invalid register index");
    }
    return regTable[w][reg];
}

/* lookup for register/memory field encoding (excluding the MOD=11 case that is covered with register lookup table)
*/
static constexpr const char regMemTable[3][8][10] = {
    // MOD - 00 / 01(with 8 bit disp) / 10(with 16 bit disp)
    {"(BX)+(SI)", "(BX)+(DI)", "(BP)+(SI)", "(BX)+(DI)", "(SI)", "(DI)", "D16", "(BX)"},
    {"(BX)+(SI)", "(BX)+(DI)", "(BP)+(SI)", "(BX)+(DI)", "(SI)", "(DI)", "D16", "(BX)"},
    {"(BX)+(SI)", "(BX)+(DI)", "(BP)+(SI)", "(BX)+(DI)", "(SI)", "(DI)", "D16", "(BX)"}
};

const char* getRegMemName(u8 mod, u8 regmem) {
    if (mod > 2 || regmem > 7){
        throw std::out_of_range("Invalid effective address query");
    }
    return regMemTable[mod][regmem];
}

int main(int argc, char *argv[]){
    if (argc !=2){
        std::cerr << "Usage: " << argv[0] << " <binary_file>" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    in.seekg(0, std::ios::end);
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!in.read(buffer.data(), size)) {
        std::cerr << "Error: only read " << in.gcount() << " bytes" << std::endl;
        return 1;
    }
    in.close();

    // process the bytes
    // 10001001 
    //   100010 ( bit shift right by 2)
    // 00000011 ( mask for last two bits Ox03 & with 10001001)
    
    // 11011001
    //       11 ( bit shift right by 6)
    //    11011 ( bit shift right by 3, then mask with 0x07 0111) 
    //      001 ( mask with 0x07)
    
    size_t pc = 0;
    while (pc+1 < buffer.size()){
        int inst_len = 1; // 1 bytes (default) instruction bytes    
        u8 b0 = buffer[pc];
        u8 b1 = buffer[pc+1];

        u8 b2 = (pc + 2 < buffer.size()) ? buffer[pc+2] : 0;
        u8 b3 = (pc + 3 < buffer.size()) ? buffer[pc+3] : 0;

        // first byte decoding to understand what sort of data operation is it ? 
        // checking first 6 bits (MOV instruction ?)
        u8 mov_opcode = b0 >> 2;

        u8 imm_mov_opcode = b0 >> 4;

        u8 add_opcode = b0 >> 2;
        u8 sub_opcode = b0 >> 2;
        u8 cmp_opcode = b0 >> 2;
        u8 jnz_opcode = b0;

        // Register/memory to/from register
        if (mov_opcode == 0b100010) {
            int inst_len = 2;
            u8 d = b0 & 0x02;
            u8 w = b0 & 0x01; // (b0 >> 3) & 0x01;
            // second byte decoding
            u8 mod = b1 >> 6;
            u8 reg = (b1 >> 3) & 0x07;
            u8 r_m = b1 & 0x07;

            if (mod == 0b01) inst_len = 3;
            else if (mod == 0b10) inst_len = 4;
            else if (mod == 0b00 && r_m == 0b110) inst_len = 4;
            if (pc + inst_len - 1 >= buffer.size()) break;

            std::cout << "mov ";

            if (mod == 0b11) { // mod 0b11 is a register to register move
                if (d == 0) {
                    // reg field is the source operand
                    // r_m field is the destination operand
                    const char *regMemName = getRegName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                    std::cout << ", ";
                    // get register name 
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << std::endl;
                } else {
                    // reg field is the destination operand
                    // r_m field is the source operand
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    // register mode with memory displacement
                    const char *regMemName = getRegName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                    std::cout << std::endl;
                }
                // |Opcode,D,W| |MOD,REG,R/M|  
            } else if (mod == 0b00){ 
                if (d == 0){ // reg is source so RM comes first  
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
                            u16 direct_address = u16(b3 << 8 | b2);
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
                int b2val = (int)b2;
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << b2val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b2val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b2val << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << b2val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b2val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b2val << "]";
                            break;
                        case 0b110:
                            if (b2val != 0){ 
                                std::cout << "[bp + " << b2val << "]";
                            } else {
                                std::cout << "[bp]";
                            }
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b2val << "]";
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
                            std::cout << "[bx + si + " << b2val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b2val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b2val << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << b2val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b2val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b2val << "]";
                            break;
                        case 0b110:
                            if (b2val != 0){ 
                                std::cout << "[bp + " <<b2val<< "]";
                            } else {
                                std::cout << "[bp]";
                            }
                            break;
                        case 0b111:
                            std::cout << "[bx + " <<b2val<< "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            } else if (mod == 0b10){
                int b3val = (int)(b3 << 8 | b2); 
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << b3val << "]" << std::endl;
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b3val << "]" << std::endl;
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b3val << "]" << std::endl;
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << b3val << "]" << std::endl;
                            break;
                        case 0b100:
                            std::cout << "[si + " << b3val << "]" << std::endl;
                            break;
                        case 0b101:
                            std::cout << "[di + " << b3val << "]" << std::endl;
                            break;
                        case 0b110:
                            std::cout << "[bp + " << b3val << "]" << std::endl;
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b3val << "]" << std::endl;
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
                            std::cout << "[bx + si + " << b3val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b3val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b3val << "]";
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << b3val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b3val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b3val << "]";
                            break;
                        case 0b110:
                            std::cout << "[bp + " << b3val << "]";
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b3val << "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            }
        }

        // immediate to register 
        if (imm_mov_opcode == 0b1011){
            int inst_len = 2;
            u8 w   = (b0 >> 3) & 0x01;
            u8 reg = b0 & 0x07;

            const char *regName = getRegName(w, reg);
            std::cout << "mov " << regName[0] << regName[1] << ", ";
            // data is the next byte
            if (w == 0){
                std::cout << (int16_t)b1 << std::endl;
            } else {
                std::cout << (int16_t)(b2 << 8 | b1) << std::endl;
            }

        }

        // Add register/memory to register
        if (add_opcode == 0b000000){ 
            int inst_len = 2;
            u8 d = b0 & 0x02;
            u8 w = b0 & 0x01;
            u8 mod = b1 >> 6;
            u8 reg = (b1 >> 3) & 0x07;
            u8 r_m = b1 & 0x07;

            if (mod == 0b01) inst_len = 3;
            else if (mod == 0b10) inst_len = 4;
            else if (mod == 0b00 && r_m == 0b110) inst_len = 4;
            if (pc + inst_len - 1 >= buffer.size()) break;
            
            std::cout << "add ";
            
            // register to register
            if (mod == 0b11) {
                if (d == 0) {
                    const char *regMemName = getRegName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                    std::cout << ", ";
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                } else {
                    const char *regName = getRegName(w, reg);
                    std::cout << regName[0] << regName[1];
                    std::cout << ", ";
                    const char *regMemName = getRegName(w, r_m);
                    std::cout << regMemName[0] << regMemName[1];
                }
                std::cout << std::endl;
            } else if (mod == 0b00){ 
                if (d == 0){ // reg is source so RM comes first  
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
                            u16 direct_address = u16(b3 << 8 | b2);
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
                int b2val = (int)b2;
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << b2val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b2val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b2val << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << b2val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b2val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b2val << "]";
                            break;
                        case 0b110:
                            if (b2val != 0){ 
                                std::cout << "[bp + " << b2val << "]";
                            } else {
                                std::cout << "[bp]";
                            }
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b2val << "]";
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
                            std::cout << "[bx + si + " << b2val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b2val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b2val << "]";
                            break;
                        case 0b011:
                            std::cout << "[bp + di + " << b2val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b2val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b2val << "]";
                            break;
                        case 0b110:
                            if (b2val != 0){ 
                                std::cout << "[bp + " <<b2val<< "]";
                            } else {
                                std::cout << "[bp]";
                            }
                            break;
                        case 0b111:
                            std::cout << "[bx + " <<b2val<< "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            } else if (mod == 0b10){
                int b3val = (int)(b3 << 8 | b2); 
                if (d == 0){
                    switch (r_m){
                        case 0b000:
                            std::cout << "[bx + si + " << b3val << "]" << std::endl;
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b3val << "]" << std::endl;
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b3val << "]" << std::endl;
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << b3val << "]" << std::endl;
                            break;
                        case 0b100:
                            std::cout << "[si + " << b3val << "]" << std::endl;
                            break;
                        case 0b101:
                            std::cout << "[di + " << b3val << "]" << std::endl;
                            break;
                        case 0b110:
                            std::cout << "[bp + " << b3val << "]" << std::endl;
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b3val << "]" << std::endl;
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
                            std::cout << "[bx + si + " << b3val << "]";
                            break;
                        case 0b001:
                            std::cout << "[bx + di + " << b3val << "]";
                            break;
                        case 0b010:
                            std::cout << "[bp + si + " << b3val << "]";
                            break;  
                        case 0b011:
                            std::cout << "[bp + di + " << b3val << "]";
                            break;
                        case 0b100:
                            std::cout << "[si + " << b3val << "]";
                            break;
                        case 0b101:
                            std::cout << "[di + " << b3val << "]";
                            break;
                        case 0b110:
                            std::cout << "[bp + " << b3val << "]";
                            break;
                        case 0b111:
                            std::cout << "[bx + " << b3val << "]";
                            break;
                    }
                    std::cout << std::endl;
                }
            }
        }

        // immediate to register/memory add
        if ((b0 & 0xFE) == 0x80) {
            int inst_len = 3;
            u8 w = b0 & 0x01;
            u8 mod = b1 >> 6;
            u8 reg = (b1 >> 3) & 0x07;
            u8 r_m = b1 & 0x07;

            if (mod == 0b01) inst_len = 4;
            else if (mod == 0b10) inst_len = 5;
            else if (mod == 0b00 && r_m == 0b110) inst_len = 5;
            if (pc + inst_len - 1 >= buffer.size()) break;

            std::cout << "add ";
            if (w == 0) std::cout << "byte ";
            else std::cout << "word ";

            if (mod == 0b11) {
                const char *regName = getRegName(w, r_m);
                std::cout << regName[0] << regName[1];
            } else if (mod == 0b00) {
                switch (r_m) {
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
                    case 0b110: {
                        u16 direct_address = u16(buffer[pc+3] << 8 | buffer[pc+2]);
                        std::cout << "[" << direct_address << "]";
                        break;
                    }
                    case 0b111:
                        std::cout << "[bx]";
                        break;
                }
            } else if (mod == 0b01) {
                int8_t disp = (int8_t)buffer[pc+2];
                switch (r_m) {
                    case 0b000:
                        std::cout << "[bx + si + " << (int)disp << "]";
                        break;
                    case 0b001:
                        std::cout << "[bx + di + " << (int)disp << "]";
                        break;
                    case 0b010:
                        std::cout << "[bp + si + " << (int)disp << "]";
                        break;
                    case 0b011:
                        std::cout << "[bp + di + " << (int)disp << "]";
                        break;
                    case 0b100:
                        std::cout << "[si + " << (int)disp << "]";
                        break;
                    case 0b101:
                        std::cout << "[di + " << (int)disp << "]";
                        break;
                    case 0b110:
                        if (disp != 0) {
                            std::cout << "[bp + " << (int)disp << "]";
                        } else {
                            std::cout << "[bp]";
                        }
                        break;
                    case 0b111:
                        std::cout << "[bx + " << (int)disp << "]";
                        break;
                }
            } else if (mod == 0b10) {
                int16_t disp = (int16_t)(buffer[pc+3] << 8 | buffer[pc+2]);
                switch (r_m) {
                    case 0b000:
                        std::cout << "[bx + si + " << disp << "]";
                        break;
                    case 0b001:
                        std::cout << "[bx + di + " << disp << "]";
                        break;
                    case 0b010:
                        std::cout << "[bp + si + " << disp << "]";
                        break;
                    case 0b011:
                        std::cout << "[bp + di + " << disp << "]";
                        break;
                    case 0b100:
                        std::cout << "[si + " << disp << "]";
                        break;
                    case 0b101:
                        std::cout << "[di + " << disp << "]";
                        break;
                    case 0b110:
                        std::cout << "[bp + " << disp << "]";
                        break;
                    case 0b111:
                        std::cout << "[bx + " << disp << "]";
                        break;
                }
            }

            std::cout << ", ";
            
            if (w == 0) {
                if (mod == 0b00 && r_m == 0b110) {
                    std::cout << (int)buffer[pc+4];
                } else if (mod == 0b01) {
                    std::cout << (int)buffer[pc+3];
                } else if (mod == 0b10) {
                    std::cout << (int)buffer[pc+4];
                } else {
                    std::cout << (int)buffer[pc+2];
                }
            } else {
                if (mod == 0b00 && r_m == 0b110) {
                    std::cout << (int16_t)(buffer[pc+5] << 8 | buffer[pc+4]);
                } else if (mod == 0b01) {
                    std::cout << (int16_t)(buffer[pc+4] << 8 | buffer[pc+3]);
                } else if (mod == 0b10) {
                    std::cout << (int16_t)(buffer[pc+5] << 8 | buffer[pc+4]);
                } else {
                    std::cout << (int16_t)(buffer[pc+3] << 8 | buffer[pc+2]);
                }
            }
            std::cout << std::endl;
        }

        if ((b0 & 0xF0) == 0x04) {
            // handle immediate to register add
            int inst_len = 2;
            u8 w = (b0 >> 3) & 0x01;
            u8 reg = b0 & 0x07;

            const char *regName = getRegName(w, reg);
            std::cout << "add " << regName[0] << regName[1] << ", ";
            if (w == 0) {
                std::cout << (int16_t)b1;
            } else {
                std::cout << (int16_t)(b2 << 8 | b1);
            }
            std::cout << std::endl;
        }

        // for sub instructions 
        if (sub_opcode == 0b001010) {
            // pass
        }

        // for cmp instructions
        if (cmp_opcode == 0b001110) {
            // pass 
        }

        // for jnz instructions 
        if (jnz_opcode == 0x75) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
            // pass 
        }
        
        pc += inst_len;
    }   
    return 0;
}
