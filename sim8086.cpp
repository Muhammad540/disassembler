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
    while (pc+1 < buffer.size()){
        uint8_t b0 = buffer[pc];
        uint8_t b1 = buffer[pc+1];
        // first byte decoding
        uint8_t opcode = b0 >> 2;
        uint8_t d = b0 & 0x02;
        uint8_t w = b0 & 0x01;
        // second byte decoding
        uint8_t mod = b1 >> 6;
        uint8_t reg = (b1 >> 3) & 0x07;
        uint8_t r_m = b1 & 0x07;

        if (opcode == 0b100010) {
            std::cout << "mov ";
        } else {
            std::cerr << "Unknown opcode: " << std::hex << static_cast<int>(opcode) << std::endl;
            return 1;
        }


        if (d == 0) {
            // reg field is the source operand
            // r_m field is the destination operand
        if (mod == 0b11) {
            // register mode with memory displacement
            const char *regMemName = getRegMemName(w, r_m);
            std::cout << regMemName[0] << regMemName[1];
        }
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
            if (mod == 0b11) {
            // register mode with memory displacement
            const char *regMemName = getRegMemName(w, r_m);
                std::cout << regMemName[0] << regMemName[1];
            }
            std::cout << std::endl;
        }
        pc += 2;
    }
    return 0;
}