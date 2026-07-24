#ifndef SYMBOL_INFO_HPP
#define SYMBOL_INFO_HPP

#include <string>
#include <vector>
#include <iostream>

class SymbolInfo {
private:
    std::string name;       
    std::string type;
    int stackOffset = -1; // Default value indicating no stack offset assigned               
    bool is_Array = false;
    int arraySize = 0; 

    bool is_Function = false;
    std::vector<std::pair<std::string, std::string>> parameters; // (name, type) pairs
    std::string returnType = "";

    bool is_Parameter = false; // To indicate if this symbol is a function parameter

public:
    SymbolInfo* next;      // Linked list pointer for chaining symbols in a hash bucket

    // Constructor
    SymbolInfo(const std::string& name, const std::string& type, SymbolInfo* next = nullptr)
        : name(name), type(type), next(next){
    }

    // Destructor
    ~SymbolInfo() {
        // Note: 'next' is managed by the hash table
    }


    // Getters
    std::string getName() const { return name; }
    std::string getType() const { return type; }
    SymbolInfo* getNext() { return next; }
    int getStackOffset() const { return stackOffset; }
    bool isArray() const { return is_Array; }
    int getArraySize() const { return arraySize; }

    bool isFunction() const { return is_Function; }
    void setIsFunction(bool isFunc) { this->is_Function = isFunc; }


    std::string getReturnType() const { return returnType; }
    void setReturnType(const std::string& retType) { this->returnType = retType; }
    
    void addParameter(const std::string& paramName, const std::string& paramType) {
        parameters.push_back({paramName, paramType});
        
    }
    bool isParameter() const { return is_Parameter; }
    void setIsParameter(bool isParam) { this->is_Parameter = isParam; }
    
    const std::vector<std::pair<std::string, std::string>>& getParameters() const { 
        return parameters; 
    }
    
    int getParameterCount() const { return parameters.size(); }

    // Setters
    void setName(const std::string& name) { this->name = name; }
    void setNext(SymbolInfo* next) { this->next = next; }
    void setStackOffset(int offset) { this->stackOffset = offset; }
    void setIsArray(bool isArray) { this->is_Array = isArray; }
    void setArraySize(int size){
        if(is_Array) {
            this->arraySize = size;
        } else {
            std::cerr << "Error: Cannot set array size for a non-array symbol." << std::endl;
        }
    }


    // Debugging utility
    void show() {
        std::cout << "SymbolInfo: Name=" << name << ", Type=" << type;
        std::cout << std::endl;
    }
};

#endif // SYMBOL_INFO_HPP