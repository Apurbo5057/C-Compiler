#include "CodeGenerator.hpp"

// Globar instance (to be used in parser)
CodeGenerator* codeGen =nullptr;


CodeGenerator::CodeGenerator(std::ofstream* parserLogFile,std::ofstream* errorFile,
                             std::ofstream* dataSectionFile, std::ofstream* codeSectionFile):
    parserLogFile(parserLogFile),
    errorFile(errorFile),
    currentStackOffset(0),
    symbolTable(nullptr),
    numberOfBuckets(7),
    dataSectionFile(dataSectionFile),
    codeSectionFile(codeSectionFile),
    isPrintlnFunctionCalled(false)
{
}


CodeGenerator::~CodeGenerator(){
    if(symbolTable){
        delete symbolTable;
    }
}


void CodeGenerator::initializeSymbolTable(){

    HashFunction hashFunc = &Hash::SDBMHash;
    symbolTable = new SymbolTable(numberOfBuckets, hashFunc, true); 
    
    if (symbolTable) {
        cout << "Symbol table initialized with " << numberOfBuckets << " buckets." << endl;
    } else {
        cerr << "Failed to initialize symbol table." << endl;
    }
}


void CodeGenerator::GenProgramHeader() {

    std::string header = 
        ".MODEL SMALL\n"
        ".STACK 1000H\n";
    writeToFile(dataSectionFile,header, -1, false);
}


void CodeGenerator::GenDataSectionHeader() {
    std::string dataSectionHeader = 
        ".DATA\n"
        "\tnumber DB \"00000$\"\n";
    writeToFile(dataSectionFile, dataSectionHeader, -1, false);
}



void CodeGenerator::GenCodeSectionHeader() {
    std::string codeSectionHeader = 
        ".CODE\n";
    writeToFile(codeSectionFile, codeSectionHeader, -1, false);
}


//============================== Data handling functions =========================

void CodeGenerator::GenLoadConstInt(int value, int lineNumber){
    std::string code = "\tMOV AX, " + std::to_string(value);
    writeToFile(codeSectionFile, code, lineNumber, true);
}


void CodeGenerator::GenLoadConstFloat(int value, int lineNumber){
    writeComment(codeSectionFile, "Float constants not supported in integer operations", lineNumber);
    // For now, convert to int for basic operations
    int intValue = static_cast<int>(value);
    std::string code = "\tMOV AX, " + std::to_string(intValue);
    writeToFile(codeSectionFile, code, lineNumber, true);
}



void CodeGenerator:: GenGlobalVariableDeclaration(const std::string& name, const std::string& type, int lineNumber) {
    
    std::string code ="\t" + name + " DW 1 DUP (0000H)";

    // Insert the variable into the symbol table
    if (symbolTable) {
        symbolTable->Insert(name, type);
        symbolTable->printAllScopeTable(*parserLogFile);
    } else {
        cerr << "Error: Symbol table is not initialized." << endl;
    }
    writeToFile(dataSectionFile, code, lineNumber, true);

}


// When a local variable is created it is inserted in the stack 
void CodeGenerator:: GenLocalVariableDeclaration(const std::string& name,const std::string& type,int lineNumber){
    
    // update the stack offset 
    currentStackOffset += 2;
    GenAllocateStackSpace(2,lineNumber);

    // Insert the symbol in the symbol Table in the current local Scope
    if(symbolTable){
        // Insert symbol with name and type but also set the stack offset for local variables . for global variables it will be set to  -1 .
        symbolTable->Insert(name, type);
        SymbolInfo* sym = symbolTable->LookUp(name);
        sym->setStackOffset(currentStackOffset);
        symbolTable->printAllScopeTable(*parserLogFile);
    } else {
        cerr << "Error: Symbol table is not initialized." << endl;
    }


    writeComment(codeSectionFile, "Local variable " + name + " at [BP-" + std::to_string(currentStackOffset) + "]", lineNumber);

}


void CodeGenerator::GenLoadVariable(const std::string& name,const std::string&reg, int lineNumber){
    std::string variable_name = getVariableAddress(name);
    std::string code = "\tMOV " + reg + ", " + variable_name;
    writeToFile(codeSectionFile, code, lineNumber, true);
}



std::string CodeGenerator::getVariableAddress(const std::string& name){
    SymbolInfo* symbol = symbolTable->LookUp(name);
    if (!symbol) {
        std::cerr << "Error: Variable '" << name << "' not found in symbol table." << std::endl;
        return "";
    }
    
    // DEBUG: Print what we found
    std::cerr << "DEBUG: Looking up '" << name << "' - Found in scope, "
              << "stackOffset: " << symbol->getStackOffset() 
              << ", isParameter: " << (symbol->isParameter() ? "true" : "false") << std::endl;
    
    if(symbol->getStackOffset() != -1){
        if(symbol->isParameter()){
            // For function parameters, we access them relative to BP
            std::string result = "[BP+" + std::to_string(symbol->getStackOffset()) + "]";
            std::cerr << "DEBUG: Parameter address: " << result << std::endl;
            return result;
        } else {
            // Local variable
            std::string result = "[BP-" + std::to_string(symbol->getStackOffset()) + "]";
            std::cerr << "DEBUG: Local variable address: " << result << std::endl;
            return result;
        }
    } else {
        // Global variable
        std::cerr << "DEBUG: Global variable address: " << name << std::endl;
        return name;
    }
}

// void CodeGenerator:: GenAssignmentOp(const std::string& varname, const std::string& reg, int lineNumber) {

//     SymbolInfo * symbol = symbolTable->LookUp(varname);

//     if(symbol){
//         if(symbol->isArray()){
//             if(symbol->getStackOffset()==-1){
//                 // Global array variable
//                 GenPopFromStack("BX",lineNumber);
//                 std::string code = "\tLEA SI, " + symbol->getName() ;
//                 writeToFile(codeSectionFile,code, lineNumber, true);
//                 code = "SHL BX, 1"; // Multiply index by 2 (size of word)
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tADD SI, BX"; // Add index to base address
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tMOV [SI], " + reg; // Store value in array element
//                 writeToFile(codeSectionFile, code, lineNumber, true);

//             }else{
//                 // Local array variable
//                 GenPopFromStack("BX",lineNumber);
//                 std::string code = "\tSHL BX, 1"; // Multiply index by 2 (size of word)
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tMOV CX, BP";
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tSUB CX, " + std::to_string(symbol->getStackOffset());
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tADD BX, CX"; // Add index to base address
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//                 code = "\tMOV [BX], " + reg; // Store value in array element
//                 writeToFile(codeSectionFile, code, lineNumber, true);
//             }
//         }
//         else{
//             // This will handle both global and local non-array variables
//             std::string variable_name = getVariableAddress(varname);

//             std::string code = "\tMOV " + variable_name + ", " + reg;
//             writeToFile(codeSectionFile, code, lineNumber, true);
//         }
//     }

// }



void CodeGenerator::GenAssignmentOp(const std::string& varname, const std::string& reg, int lineNumber) {
    SymbolInfo* symbol = symbolTable->LookUp(varname);
    if (!symbol) {
        std::cerr << "Error: Variable '" << varname << "' not found" << std::endl;
        return;
    }

    if (symbol->isArray()) {
        // For array assignment, we need the index which should be on the stack
        // The value to assign is already in the register
        
        // Pop the index from stack (it was pushed when parsing array access)
        GenPopFromStack("BX", lineNumber);
        
        if (symbol->getStackOffset() == -1) {
            // Global array
            std::string code = "\tLEA SI, " + varname;
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tSHL BX, 1"; // Multiply index by 2 (word size)
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tADD SI, BX";
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tMOV [SI], " + reg;
            writeToFile(codeSectionFile, code, lineNumber, true);
        } else {
            // Local array
            std::string code = "\tSHL BX, 1"; // Multiply index by 2
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tMOV CX, BP";
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tSUB CX, " + std::to_string(symbol->getStackOffset());
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tADD BX, CX";
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tMOV [BX], " + reg;
            writeToFile(codeSectionFile, code, lineNumber, true);
        }
    } else {
        // Regular variable assignment
        if (symbol->getStackOffset() == -1) {
            // Global variable
            std::string code = "\tMOV " + varname + ", " + reg;
            writeToFile(codeSectionFile, code, lineNumber, true);
        } else {
            // Local variable

            if(symbol->isParameter()){
                // For function parameters, we access them relative to BP
                std::string code = "\tMOV [BP+" + std::to_string(symbol->getStackOffset()) + "], " + reg;
                writeToFile(codeSectionFile, code, lineNumber, true);
            } else {
                // Local variable
                std::string code = "\tMOV [BP-" + std::to_string(symbol->getStackOffset()) + "], " + reg;
                writeToFile(codeSectionFile, code, lineNumber, true);
            }
            
        }
    }
}









//======================== Array variables handling ============================
void CodeGenerator:: GenGlobalArrayDeclaration(const std::string& name, const std::string& type, int size, int lineNumber){
    std::string code = "\t" + name + " DW " + std::to_string(size) + " DUP (0000H)";

    // Insert the array variable into the symbol table
    if (symbolTable) {
        symbolTable->Insert(name, type);
        SymbolInfo* sym = symbolTable->LookUp(name);
        sym->setIsArray(true);
        sym->setArraySize(size);
        symbolTable->printAllScopeTable(*parserLogFile);
    } else {
        cerr << "Error: Symbol table is not initialized." << endl;
    }
    writeToFile(dataSectionFile, code, lineNumber, true);
}



void CodeGenerator:: GenLocalArrayDeclaration(const std::string& name, const std::string& type, int size, int lineNumber){
    // update the stack offset 
    currentStackOffset += size * 2; // each element is 2 bytes
    GenAllocateStackSpace(size * 2, lineNumber);

    // Insert the symbol in the symbol Table in the current local Scope
    if(symbolTable){
        // Insert symbol with name and type but also set the stack offset for local variables . for global variables it will be set to  -1 .
        symbolTable->Insert(name, type);
        SymbolInfo* sym = symbolTable->LookUp(name);
        sym->setStackOffset(currentStackOffset);
        sym->setIsArray(true);
        sym->setArraySize(size);
        symbolTable->printAllScopeTable(*parserLogFile);
    } else {
        cerr << "Error: Symbol table is not initialized." << endl;
    }
}



void CodeGenerator:: GenPushToStack(const std::string& reg, int lineNumber){
    std::string code = "\tPUSH " + reg;
    writeToFile(codeSectionFile, code, lineNumber, true);
}


void CodeGenerator:: GenPopFromStack(const std::string& reg, int lineNumber){
    std::string code = "\tPOP "+reg;
    writeToFile(codeSectionFile,code,lineNumber,true);
}


void CodeGenerator:: GenAllocateStackSpace(int bytes,int lineNumber){
    std::string code = "\tSUB SP, "+ std::to_string(bytes);
    writeToFile(codeSectionFile,code,lineNumber,true);
}


void CodeGenerator:: GenDeallocateStackSpace(int bytes,int lineNumber){
    std::string code = "\tADD SP, "+ std::to_string(bytes);
    writeToFile(codeSectionFile,code,lineNumber,true);
}







void CodeGenerator:: GenPrintlnFunction(){
    std::string code = 
        "; Print library\n"
        "new_line proc\n"
        "\tpush ax\n"
        "\tpush dx\n"
        "\tmov ah,2\n"
        "\tmov dl,0Dh\n"
        "\tint 21h\n"
        "\tmov ah,2\n"
        "\tmov dl,0Ah\n"
        "\tint 21h\n"
        "\tpop dx\n"
        "\tpop ax\n"
        "\tret\n"
        "new_line endp\n"
        "print_output proc\t\t;print what is in ax\n"
        "\tpush ax\n"
        "\tpush bx\n"
        "\tpush cx\n"
        "\tpush dx\n"
        "\tpush si\n"
        "\tlea si,number\n"
        "\tmov bx,10\n"
        "\tadd si,4\n"
        "\tcmp ax,0\n"
        "\tjnge negate\n"
        "print:\n"
        "\txor dx,dx\n"
        "\tdiv bx\n"
        "\tmov [si],dl\n"
        "\tadd [si],'0'\n"
        "\tdec si\n"
        "\tcmp ax,0\n"
        "\tjne print\n"
        "\tinc si\n"
        "\tlea dx,si\n"
        "\tmov ah,9\n"
        "\tint 21h\n"
        "\tpop si\n"
        "\tpop dx\n"
        "\tpop cx\n"
        "\tpop bx\n"
        "\tpop ax\n"
        "\tret\n"
        "negate:\n"
        "\tpush ax\n"
        "\tmov ah,2\n"
        "\tmov dl,'-'\n"
        "\tint 21h\n"
        "\tpop ax\n"
        "\tneg ax\n"
        "\tjmp print\n"
        "print_output endp\n";
    writeToFile(codeSectionFile, code, -1, false);
}



void CodeGenerator::GenProgramFooter() {

    if(isPrintlnFunctionCalled)GenPrintlnFunction();
    std::string footer =
        "; Footer\n"
        "END main\n"
        "; Assembly is fun! Keep calm and code on :)\n";
    writeToFile(codeSectionFile, footer, -1, false);
}




//==========================Helper Functions==========================//



void CodeGenerator::writeToFile(std::ofstream* file, const std::string& code, int lineNumber, bool lineComment) {
    if(file && file->is_open()) {
        if (lineComment) {
            *file << code << "\t\t; line " << lineNumber << std::endl;
        } else {
            *file << code << std::endl;
        }
    } else {
        cerr << "Error: File stream is not open." << endl;
    }
}


void CodeGenerator::writeComment(std::ofstream* file, const std::string& comment, int lineNumber) {
    if(file && file->is_open()) {
        *file << "; " << comment << "\t\t; line " << lineNumber << std::endl;
    } else {
        cerr << "Error: File stream is not open." << endl;
    }
}



void initializeCodeGenerator(std::ofstream* parserLogFile, std::ofstream* errorFile,std::ofstream* dataSectionFile, std::ofstream* codeSectionFile) {
    if (codeGen == nullptr) {
        codeGen = new CodeGenerator(parserLogFile, errorFile, dataSectionFile, codeSectionFile);
        codeGen->initializeSymbolTable();
    } else {
        cerr << "CodeGenerator instance already exists." << endl;
    }
}



void finalizeCodeGenerator() {
    if (codeGen) {
        delete codeGen;
        codeGen = nullptr;
    } else {
        cerr << "No CodeGenerator instance to finalize." << endl;
    }
}