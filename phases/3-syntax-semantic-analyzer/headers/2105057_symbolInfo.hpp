#ifndef SYMBOL_INFO_HPP
#define SYMBOL_INFO_HPP

#include <string>
#include <vector>
#include <iostream>

class SymbolInfo {
private:
    std::string name;               // Identifier name
    std::string type;               
    std::string originalType;
    std::string returnType;         // For functions: return type
    std::vector<std::string> paramTypes; // For functions: parameter types
    bool isFunction;       
    bool isArray;         

public:
    SymbolInfo* next;      // Linked list pointer for chaining symbols in a hash bucket

    // Constructor
    SymbolInfo(const std::string& name, const std::string& type, SymbolInfo* next = nullptr)
        : name(name), type(type), originalType(type), next(next), isFunction(false), isArray(false) {
        parseType();
    }

    // Destructor
    ~SymbolInfo() {
        // Note: 'next' is managed by the hash table
    }

    std::string getDisplayType() const {
        if (type.find("ARRAY,") == 0) {
            return "ID";  // Display arrays as ID
        } else if (type == "int" || type == "float" || type == "void") {
            return "ID";  // Display variables as ID
        } else if (type.find("FUNCTION,") == 0) {
            return "FUNCTION";  // Display functions as FUNCTION
        }
        return type;  // Default case
    }

    std::string getActualType() const {
        return type;  // Return the stored type for type checking
    }

    void parseType() {
        if (originalType.find("FUNCTION") == 0) {
            isFunction = true;
            size_t comma = originalType.find(",");
            size_t arrow = originalType.find("<==");
            if (comma != std::string::npos && arrow != std::string::npos) {
                returnType = originalType.substr(comma + 1, arrow - comma - 1);
                std::string paramStr = originalType.substr(arrow + 4, originalType.length() - arrow - 5);
                
                // Clear any existing parameter types
                paramTypes.clear();
                
                // Skip empty parameter lists
                if (paramStr.empty() || paramStr == ")") {
                    // No parameters
                    type = "ID";
                    return;
                }
                
                // Parse parameters
                size_t pos = 0;
                while (pos < paramStr.length()) {
                    size_t nextComma = paramStr.find(",", pos);
                    if (nextComma == std::string::npos) {
                        nextComma = paramStr.length();
                    }
                    
                    std::string param = paramStr.substr(pos, nextComma - pos);
                    
                    // Trim whitespace safely
                    size_t start = param.find_first_not_of(" \t");
                    if (start == std::string::npos) {
                        param = ""; // All whitespace
                    } else {
                        size_t end = param.find_last_not_of(" \t");
                        param = param.substr(start, end - start + 1);
                    }
                    
                    // Skip empty parameters or closing parenthesis
                    if (!param.empty() && param != ")") {
                        size_t space = param.find(" ");
                        if (space != std::string::npos) {
                            // Format: "type name" - extract type only
                            std::string paramType = param.substr(0, space);
                            // Trim paramType safely
                            size_t typeStart = paramType.find_first_not_of(" \t");
                            if (typeStart != std::string::npos) {
                                size_t typeEnd = paramType.find_last_not_of(" \t");
                                paramType = paramType.substr(typeStart, typeEnd - typeStart + 1);
                            }
                            if (!paramType.empty()) {
                                paramTypes.push_back(paramType);
                            }
                        } else {
                            // Format: "type" only - use as is (but check it's not ")")
                            if (param != ")") {
                                paramTypes.push_back(param);
                            }
                        }
                    }
                    pos = nextComma + 1;
                }
            }
            type = "ID";
        } else if (originalType.find("ARRAY") == 0) {
            isArray = true;
            // Keep the full ARRAY,type format for proper type checking
            // Don't modify type here
        }
    }

    // Getters
    std::string getName() const { return name; }
    std::string getType() const { return type; }
    std::string getReturnType() const { return returnType; }
    std::vector<std::string> getParamTypes() const { return paramTypes; }
    bool isFunctionSymbol() const { return isFunction; }
    bool isArraySymbol() const { return isArray; }
    SymbolInfo* getNext() { return next; }
    std::string getOriginalType() const { return originalType; }

    // Setters
    void setName(const std::string& name) { this->name = name; }
    void setType(const std::string& type) { 
        this->type = type; 
        parseType();
    }
    void setNext(SymbolInfo* next) { this->next = next; }

    // Debugging utility
    void show() {
        std::cout << "SymbolInfo: Name=" << name << ", Type=" << type;
        if (isFunction) {
            std::cout << ", ReturnType=" << returnType << ", Params=";
            for (const auto& param : paramTypes) {
                std::cout << param << " ";
            }
        }
        std::cout << std::endl;
    }
};

#endif // SYMBOL_INFO_HPP