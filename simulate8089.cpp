/*  bits -> cpu -> decoded bits into instruction -> simulate instructions 
    move to/from memory to/from registers, perform some basic operations
    move out of memory

    in this example we expect the instructions have been decoded and now we'll just try to simulate it
*/
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_map> 
#include <bitset> 

using namespace std;

std::unordered_map<std::string, int> registers = {
  {"ax", 0},
  {"cx", 0},
  {"dx", 0},
  {"bx", 0},
  {"sp", 0},
  {"bp", 0},
  {"si", 0},
  {"di", 0}
};

// flag registers has the following order 
// D7 D6 D5 D4 D3 D2 D1 D0
// S  Z     AC    P     CY
std::bitset<8> flag_registers;

int main(int argc, char *argv[]){
  if (argc != 2){
    std::cerr << "Usage: " << argv[0] << " <filename.asm" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1]);
  if (!file){
    std::cerr << "Error opening file: " << argv[1] << std::endl;
    return 1;
  }
  
  std::cout << "Values of registers before simulation: " << std::endl;

  // print the register values before the start of simulation 
  for (const auto& pair: registers){
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  
  std::string line;
  // simulate the each instruction
  while (std::getline(file, line)){
    if (line.empty()) continue;

    std::istringstream iss(line);
    std::string instruction, reg, val;

    iss >> instruction >> reg >> val;

    if (reg.back() == ','){
      reg.pop_back();
    }
    
    // std::cout << "Inst: " << instruction << ", Reg: " << reg << ", Val: " << val << std::endl;
    if (instruction == "mov"){
      if (registers.find(val) != registers.end()){
        // register to register move
        registers[reg] = registers[val];
      } else {
        // immediate
        registers[reg] = stoi(val);
      }
    } else if (instruction == "sub") {
       if (registers.find(val) != registers.end()){
        // register to register
        registers[reg] -= registers[val];
       } else {
        // immediate
        registers[reg] -= stoi(val);    
       }
       
       // set flags 
       if (registers[reg] == 0){
          flag_registers[6] = true;
       } else {
         flag_registers[6] = false;
       }

       uint8_t result = static_cast<uint8_t>(registers[reg]);
       if (result & (1<<7)) {
          flag_registers[7] = true;
       } else {
         flag_registers[7]  = false;
       }
    } else if (instruction == "add") {
       if (registers.find(val) != registers.end()){
        // register to register
        registers[reg] += registers[val];
       } else {
        // immediate
        registers[reg] += stoi(val);    
       }
       
       // set flags 
       if (registers[reg] == 0){
          flag_registers[6] = true;
       } else {
         flag_registers[6] = false;
       }

       uint8_t result = static_cast<uint8_t>(registers[reg]);
       if (result & (1<<7)) {
          flag_registers[7] = true;
       } else {
         flag_registers[7]  = false;
       }
    } else if (instruction == "cmp"){
       uint8_t temp_val {0};
       if (registers.find(val) != registers.end()){
        // register to register subregisters
        temp_val = registers[reg] - registers[val];
       } else {
        // immediate
        temp_val = registers[reg] - stoi(val);    
       }
       
       // set flags 
       if (temp_val == 0){
          flag_registers[6] = true;
       } else {
          flag_registers[6] = false;
       }

       if (temp_val < 0) {
          flag_registers[7] = true;
       } else {
          flag_registers[7]  = false;
       }
    }
  }
  std::cout << "Values of registers after simulation: " << std::endl;
  for (const auto& pair: registers){
    std::cout << pair.first << ": " << pair.second << std::endl;
  }

  return 0;
}

