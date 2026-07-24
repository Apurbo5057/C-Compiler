#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <utility>
#include "symbol-table/SymbolInfo.hpp"
#include "symbol-table/Hash.hpp"
#include "symbol-table/SymbolTable.hpp"

class CodeGenerator {
private:
    
    // Declare the file streams 
    std::ofstream* parserLogFile;
    std::ofstream* errorFile;
    std::ofstream* dataSectionFile;
    std::ofstream* codeSectionFile;

    // Symbol table for managing variables and functions
    int numberOfBuckets;
    SymbolTable* symbolTable ;
    

    // Stack management for function calls
    int currentStackOffset ;

    // boolean to check if we are in the global scope or not 
    bool isGlobalScope = true;

    // Show println function when called println
    bool isPrintlnFunctionCalled = false;

    // Label counter for generating unique labels
    int labelCounter = 0;


    // Added for function management
    std::string currentFunctionName = "";
    int localVariableSize = 0; // Total size of local variables in current function
    int currentParameterIndex = 0; // For tracking parameter positions

    std::vector<std::pair<std::string, std::string>> currentFunctionParams;

public:
    // Constructor
    CodeGenerator(std::ofstream* parserLogFile, 
                  std::ofstream* errorFile , std::ofstream* dataSectionFile, std::ofstream* codeSectionFile);
    
    // Destructor
    ~CodeGenerator();

    // Initialization
    void initializeSymbolTable();

    // This Function will generate the header of 8086 assembly code in the dataSectionFile
    void GenProgramHeader();

    // This portion is for Data Section
    void GenDataSectionHeader();

    // This portion is for Code Section
    void GenCodeSectionHeader();

    // Checks if we are currently in global scope or not
    bool isInGlobalScope() const {
        return isGlobalScope;
    }

    void callPrintlnFunction(const std::string& varname) {
        isPrintlnFunctionCalled = true;
        GenLoadVariable(varname , "AX", -1); // Load variable into AX register
        std::string code = "\tCALL print_output\n"
        "\tCALL new_line";
        writeToFile(codeSectionFile, code, -1, true);
    }

    int getIsPrintlnFunctionCalled() {
        return isPrintlnFunctionCalled;
    }

    // set the status of global scope
    void setIsGlobalScope(bool status) {
        isGlobalScope = status;
    }

    int getCurrentStackOffset() const {
        return currentStackOffset;
    }

    void setCurrentStackOffset(int offset) {
        currentStackOffset = offset;
    }

    // Generate unique labels
    std::string getNextLabel() {
        return "L" + std::to_string(labelCounter++);
    }

    // Generate label
    void GenLabel(const std::string& label, int lineNumber) {
        std::string code = label + ":";
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    // Jump operations
    void GenUnconditionalJump(const std::string& label, int lineNumber) {
        std::string code = "\tJMP " + label;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenConditionalJump(const std::string& condition, const std::string& label, int lineNumber) {
        std::string code = "\t" + condition + " " + label;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    //========================== Data handling functions ==========================

    // Constants handling functions
    void GenLoadConstInt(int value, int lineNumber);
    void GenLoadConstFloat(int value, int lineNumber);

    // Non-array variables handling
    void GenGlobalVariableDeclaration(const std::string& name, const std::string& type, int lineNumber) ;
    void GenLocalVariableDeclaration(const std::string& name,const std::string& tyep,int lineNumber);
    void GenLoadVariable(const std::string& name,const std::string& reg, int lineNumber);

    // Array variables handling
    void GenGlobalArrayDeclaration(const std::string& name, const std::string& type, int size, int lineNumber);
    void GenLocalArrayDeclaration(const std::string& name, const std::string& type, int size, int lineNumber);

    // Array element access
    void GenLoadArrayElement(const std::string& name, const std::string& reg, int lineNumber) {
        SymbolInfo* symbol = symbolTable->LookUp(name);
        if (!symbol) {
            std::cerr << "Error: Array '" << name << "' not found" << std::endl;
            return;
        }

        // Pop index from stack
        GenPopFromStack("BX", lineNumber);
        
        if (symbol->getStackOffset() == -1) {
            // Global array
            std::string code = "\tLEA SI, " + name;
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tSHL BX, 1"; // Multiply index by 2 (word size)
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tADD SI, BX";
            writeToFile(codeSectionFile, code, lineNumber, true);
            code = "\tMOV " + reg + ", [SI]";
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
            code = "\tMOV " + reg + ", [BX]";
            writeToFile(codeSectionFile, code, lineNumber, true);
        }
    }


    void GenLoadIdentifier(const std::string& name, int line) {
        SymbolInfo* symbol = symbolTable->LookUp(name);
        if (!symbol) {
            std::cerr << "Error: Identifier '" << name << "' not found" << std::endl;
            return;
        }

        if (symbol->isArray()) {
            GenLoadArrayElement(name, "AX", line);
        } else {
            GenLoadVariable(name, "AX", line);
        }
    }




    // Variable access helpers
    std::string getVariableAddress(const std::string& name);

    // Assignment operation
    void GenAssignmentOp(const std::string& varname, const std::string& reg, int lineNumber) ;

    //=====================ADDOP and MULOP operations=====================
    void GenAdditionOp(const std::string& reg1, const std::string& reg2, int lineNumber){
        std::string code = "\tADD " + reg1 + ", " + reg2;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }
    void GenSubtractionOp(const std::string& reg1, const std::string& reg2, int lineNumber){
        std::string code = "\tSUB " + reg1 + ", " + reg2;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenMultiplicationOp(const std::string& reg, int lineNumber){
        std::string code = "\tMUL " + reg;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenDivisionOp(const std::string& reg, int lineNumber){
        std::string code = "\tXOR DX, DX\n\tDIV " + reg;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenModulusOp(const std::string& reg, int lineNumber){
        std::string code = "\tXOR DX, DX\n\tDIV " + reg + "\n\tMOV AX, DX";
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    //===================== UNARY operations =====================
    void GenUnaryMinus(int lineNumber) {
        std::string code = "\tNEG AX";
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenLogicalNot(int lineNumber) {
        std::string skipLabel = getNextLabel();
        std::string code = "\tCMP AX, 0\n"
                          "\tMOV AX, 0\n"
                          "\tJNE " + skipLabel + "\n"
                          "\tMOV AX, 1\n"
                          + skipLabel + ":";
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    // Increment and Decrement operations
    void GenPreIncrement(const std::string& varName, int lineNumber) {
        std::string address = getVariableAddress(varName);
        std::string code = "\tINC " + address + "\n"
                          "\tMOV AX, " + address;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    // void GenPostIncrement(const std::string& varName, int lineNumber) {
    //     std::string address = getVariableAddress(varName);
    //     std::string code = "\tMOV AX, " + address + "\n"
    //                       "\tINC " + address;
    //     writeToFile(codeSectionFile, code, lineNumber, true);
    // }



    void GenPostIncrement(const std::string& varName, int lineNumber) {
        SymbolInfo* symbol = symbolTable->LookUp(varName);
        if (!symbol) {
            std::cerr << "Error: Variable '" << varName << "' not found" << std::endl;
            return;
        }

        if (symbol->isArray()) {
            // For array post-increment, we need to handle the index
            // The index should be on the stack from the array access parsing
            GenPopFromStack("BX", lineNumber); // Get the index
            
            if (symbol->getStackOffset() == -1) {
                // Global array
                std::string code = "\tLEA SI, " + varName;
                writeToFile(codeSectionFile, code, lineNumber, true);
                code = "\tSHL BX, 1"; // Multiply index by 2 (word size)
                writeToFile(codeSectionFile, code, lineNumber, true);
                code = "\tADD SI, BX";
                writeToFile(codeSectionFile, code, lineNumber, true);
                code = "\tMOV AX, [SI]"; // Get current value
                writeToFile(codeSectionFile, code, lineNumber, true);
                code = "\tINC WORD PTR [SI]"; // Increment the array element
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
                code = "\tMOV AX, [BX]"; // Get current value
                writeToFile(codeSectionFile, code, lineNumber, true);
                code = "\tINC WORD PTR [BX]"; // Increment the array element
                writeToFile(codeSectionFile, code, lineNumber, true);
            }
        } else {
            // Regular variable post-increment
            std::string address = getVariableAddress(varName);
            std::string code = "\tMOV AX, " + address + "\n"
                            "\tINC WORD PTR " + address;
            writeToFile(codeSectionFile, code, lineNumber, true);
        }
    }


    void GenPreDecrement(const std::string& varName, int lineNumber) {
        std::string address = getVariableAddress(varName);
        std::string code = "\tDEC " + address + "\n"
                          "\tMOV AX, " + address;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenPostDecrement(const std::string& varName, int lineNumber) {
        std::string address = getVariableAddress(varName);
        std::string code = "\tMOV AX, " + address + "\n"
                        "\tDEC WORD PTR " + address;  // Ensure word-sized decrement
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    //===================== RELATIONAL operations =====================
    void GenRelationalOp(const std::string& op, int lineNumber) {
        // Pop two operands from stack (right operand first, then left)
        GenPopFromStack("BX", lineNumber); // right operand
        GenPopFromStack("AX", lineNumber); // left operand
        
        std::string falseLabel = getNextLabel();
        std::string code = "\tCMP AX, BX\n"
                          "\tMOV AX, 0\n"; // Default to false
        
        if (op == "<") {
            code += "\tJGE " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        } else if (op == "<=") {
            code += "\tJG " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        } else if (op == ">") {
            code += "\tJLE " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        } else if (op == ">=") {
            code += "\tJL " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        } else if (op == "==") {
            code += "\tJNE " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        } else if (op == "!=") {
            code += "\tJE " + falseLabel + "\n"
                   "\tMOV AX, 1\n"
                   + falseLabel + ":";
        }
        
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    //===================== LOGICAL operations =====================
    void GenLogicalOp(const std::string& op, int lineNumber) {
        // Pop two operands from stack (right operand first, then left)
        GenPopFromStack("BX", lineNumber); // right operand
        GenPopFromStack("AX", lineNumber); // left operand
        
        if (op == "&&") {
            std::string falseLabel = getNextLabel();
            std::string endLabel = getNextLabel();
            std::string code = "\tCMP AX, 0\n"
                              "\tJE " + falseLabel + "\n"
                              "\tCMP BX, 0\n"
                              "\tJE " + falseLabel + "\n"
                              "\tMOV AX, 1\n"
                              "\tJMP " + endLabel + "\n"
                              + falseLabel + ":\n"
                              "\tMOV AX, 0\n"
                              + endLabel + ":";
            writeToFile(codeSectionFile, code, lineNumber, true);
        } else if (op == "||") {
            std::string trueLabel = getNextLabel();
            std::string endLabel = getNextLabel();
            std::string code = "\tCMP AX, 0\n"
                              "\tJNE " + trueLabel + "\n"
                              "\tCMP BX, 0\n"
                              "\tJNE " + trueLabel + "\n"
                              "\tMOV AX, 0\n"
                              "\tJMP " + endLabel + "\n"
                              + trueLabel + ":\n"
                              "\tMOV AX, 1\n"
                              + endLabel + ":";
            writeToFile(codeSectionFile, code, lineNumber, true);
        }
    }

    //===================== CONTROL FLOW STATEMENTS =====================
    
    // IF statement
    void GenIfStart(int lineNumber) {
        // Pop condition from stack
        GenPopFromStack("AX", lineNumber);
        std::string code = "\tCMP AX, 0";
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenIfElse(const std::string& elseLabel, int lineNumber) {
        std::string code = "\tJE " + elseLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenIfEnd(const std::string& endLabel, int lineNumber) {
        std::string code = "\tJMP " + endLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    // WHILE loop
    void GenWhileStart(const std::string& startLabel, int lineNumber) {
        GenLabel(startLabel, lineNumber);
    }

    void GenWhileCondition(const std::string& endLabel, int lineNumber) {
        // Pop condition from stack
        GenPopFromStack("AX", lineNumber);
        std::string code = "\tCMP AX, 0\n"
                          "\tJE " + endLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenWhileEnd(const std::string& startLabel, const std::string& endLabel, int lineNumber) {
        std::string code = "\tJMP " + startLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
        GenLabel(endLabel, lineNumber);
    }

    // FOR loop
    void GenForInit(int lineNumber) {
        // Initialize expression is already handled in expression parsing
        writeComment(codeSectionFile, "FOR loop initialization", lineNumber);
    }

    void GenForConditionStart(const std::string& conditionLabel, int lineNumber) {
        GenLabel(conditionLabel, lineNumber);
    }

    void GenForConditionCheck(const std::string& bodyLabel, const std::string& endLabel, int lineNumber) {
        GenPopFromStack("AX", lineNumber);
        std::string code = "\tCMP AX, 0\n"
                        "\tJNE " + bodyLabel + "\n"  // Jump to body if condition is true
                        "\tJMP " + endLabel;         // Jump to end if condition is false
        writeToFile(codeSectionFile, code, lineNumber, true);
    }

    void GenForUpdate(const std::string& updateLabel, const std::string& conditionLabel, int lineNumber) {
        std::string code = "\tJMP " + updateLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
        GenLabel(updateLabel, lineNumber);
        // Update expression will be handled in expression parsing
    }

    void GenForEnd(const std::string& conditionLabel, const std::string& endLabel, int lineNumber) {
        std::string code = "\tJMP " + conditionLabel;
        writeToFile(codeSectionFile, code, lineNumber, true);
        GenLabel(endLabel, lineNumber);
    }

    //===============================Function handling================================


    void GenFunctionParameter(const std::string& paramName, const std::string& paramType, int lineNumber) {
        // Just collect parameters for now, don't assign offsets yet
        currentFunctionParams.push_back({paramName, paramType});
        currentParameterIndex++;
    }
    
    void GenFunctionParametersComplete(int lineNumber) {
        // Now assign correct offsets in reverse order
        int totalParams = currentFunctionParams.size();
        
        for (int i = 0; i < totalParams; i++) {
            const std::string& paramName = currentFunctionParams[i].first;
            const std::string& paramType = currentFunctionParams[i].second;
            
            // Calculate offset: last parameter gets BP+4, second-to-last gets BP+6, etc.
            int paramOffset = 4 + ((totalParams - 1 - i) * 2);
            
            symbolTable->Insert(paramName, paramType);
            symbolTable->printAllScopeTable(*parserLogFile);
            SymbolInfo* paramSymbol = symbolTable->LookUp(paramName);
            paramSymbol->setStackOffset(paramOffset);
            paramSymbol->setIsParameter(true);
            
            // Add parameter to current function's symbol
            SymbolInfo* funcSymbol = symbolTable->LookUp(currentFunctionName);
            if (funcSymbol) {
                funcSymbol->addParameter(paramName, paramType);
            }
            
            writeComment(codeSectionFile, "Parameter " + paramName + " at [BP+" + std::to_string(paramOffset) + "]", lineNumber);
        }
        
        currentFunctionParams.clear(); // Clean up
    }


    void GenPushArgument(int lineNumber) {
        // Pop argument from expression evaluation and push for function call
        GenPopFromStack("AX", lineNumber);
        GenPushToStack("AX", lineNumber);
    }


    void GenFunctionCall(const std::string& functionName, int lineNumber) {
        SymbolInfo* funcSymbol = symbolTable->LookUp(functionName);
        if (!funcSymbol || !funcSymbol->isFunction()) {
            std::cerr << "Error: Function " << functionName << " not declared." << std::endl;
            return;
        }

        const auto& params = funcSymbol->getParameters();
        int paramCount = params.size();

        writeComment(codeSectionFile, "Pushing arguments for " + functionName, lineNumber);

        // Reverse the order: last parameter first
        for (int i = paramCount - 1; i >= 0; i--) {
            // Pop the evaluated argument value from expression stack
            GenPopFromStack("AX", lineNumber);

            // Push onto actual call stack
            GenPushToStack("AX", lineNumber);

            writeComment(codeSectionFile, "Pushed param " + params[i].first, lineNumber);
        }

        // Call the function
        std::string code = "\tCALL " + functionName;
        writeToFile(codeSectionFile, code, lineNumber, true);


        // ADD SP, paramCount * 2 (if caller cleans stack)

        // Push return value (AX) onto expression stack
        GenPushToStack("AX", lineNumber);
    }


    void GenReturnStatement(bool hasReturnValue, int lineNumber) {
        if (hasReturnValue) {
            // Pop return value into AX
            GenPopFromStack("AX", lineNumber);
        }
        
        // Jump to function return label
        std::string returnLabel = "RETURN_" + currentFunctionName;
        GenUnconditionalJump(returnLabel, lineNumber);
    }







    void resetForNewFunction(const std::string& funcName) {
        currentFunctionName = funcName;
        localVariableSize = 0;
        currentParameterIndex = 0;
        currentStackOffset = 0;
        currentFunctionParams.clear();
    }







    void GenFunctionHeader(const std::string& functionName, const std::string& type, int lineNumber) {
        resetForNewFunction(functionName);

        if(symbolTable){
            symbolTable->Insert(functionName, type);
            symbolTable->printAllScopeTable(*parserLogFile);
            SymbolInfo* funcSymbol = symbolTable->LookUp(functionName);
            if (funcSymbol) {
                funcSymbol->setIsFunction(true);
                funcSymbol->setReturnType(type);
            } else {
                std::cerr << "Error: Function '" << functionName << "' not found in symbol table." << std::endl;
            }
        }

        std::string code = functionName + " PROC\n";
        if(functionName == "main") {
            code += "\tMOV AX, @DATA\n"
                    "\tMOV DS, AX\n";
        }

        code += "\tPUSH BP\n"
                "\tMOV BP, SP\n";



        writeToFile(codeSectionFile, code, lineNumber, true);
    }



    void GenFunctionFooter(const std::string& functionName, const std::string& type, int lineNumber) {

        std::string returnLabel = "RETURN_" + functionName;
        GenLabel(returnLabel, lineNumber);

        std::string code;


        if(functionName == "main") {
        code = "\tMOV AX, 4CH\n"
               "\tINT 21H\n"
               + functionName + " ENDP\n";
        } else {
            if(currentStackOffset > 0) {
                code = "\tADD SP, " + std::to_string(currentStackOffset) + "\n";
            } 
            code += "\tPOP BP\n";

            SymbolInfo* funcSymbol = symbolTable->LookUp(functionName);
            int paramCount = funcSymbol ? funcSymbol->getParameterCount() : 0;

            if (paramCount > 0) {
                code += "\tRET " + std::to_string(paramCount * 2) + "\n";
            } else {
                code += "\tRET\n";
            }
            
            code += functionName + " ENDP\n";
        }

        writeToFile(codeSectionFile, code, lineNumber, true);
    }


    //===============================Stack operations================================

    // Stack operation Functions
    void GenPushToStack(const std::string& reg, int lineNumber);
    void GenPopFromStack(const std::string& reg, int lineNumber);
    void GenAllocateStackSpace(int bytes,int lineNumber);
    void GenDeallocateStackSpace(int bytes,int lineNumber);

    void EnterNewScope() {
        if (symbolTable) {
            symbolTable->enterScope();
        } else {
            std::cerr << "Error: Symbol table is not initialized." << std::endl;
        }
    }

    void ExitCurrentScope(bool verbose = false) {
        if (symbolTable) {
            symbolTable->exitScope(verbose);
        } else {
            std::cerr << "Error: Symbol table is not initialized." << std::endl;
        }
    }

    // Function handling


    //prints a variable 
    void GenPrintlnFunction();

    // This Function will generate the footer of 8086 assembly code in the codeSectionFile
    void GenProgramFooter();

    //============================ Helper functions===========================

    // Helper functions for code generation
    // This will write to file like :
    // MOV AX, 5    ; line 10
    void writeToFile(std::ofstream* file,const std::string& code , int lineNumber , bool lineComment = false);
    
    // Helper function for comment 
    void writeComment(std::ofstream* file, const std::string& comment, int lineNumber);

    void writeToCodeSection(const std::string& code , int lineNumber , bool lineComment = false) {
            if (lineComment) {
                *codeSectionFile << code << "\t\t; line " << lineNumber << std::endl;
            } else {
                *codeSectionFile << code << std::endl;
            }

    }
};

// Global instance (to be used in parser)
extern CodeGenerator* codeGen;

// Helper functions for parser to create CodeGenerator instance 
void initializeCodeGenerator(std::ofstream* parserLogFile, std::ofstream* errorFile,std::ofstream* dataSectionFile, std::ofstream* codeSectionFile);

// Function to finalize or clean up the CodeGenerator instance
void finalizeCodeGenerator();

#endif // CODEGEN_HPP