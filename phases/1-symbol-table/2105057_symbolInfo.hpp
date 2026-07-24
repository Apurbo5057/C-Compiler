#pragma once

#include <iostream>
#include <string>

using namespace std;

class SymbolInfo
{
private:
    string name;
    string type;

public:

    // This pointer is used to chain multiple symbol in same bucket of the hashTable
    SymbolInfo *next;

    // constructor
    SymbolInfo(string name, string type, SymbolInfo *next = nullptr)
    {
        this->name = name;
        this->next = next;
        this->type = formatType(type);
    }

    // destructor
    ~SymbolInfo()
    {

        // Here we are recursively deleting this symbol and the following symbols
        if(next!=nullptr) delete next;

    }

    string formatType(string type) {
        const char *cstr = type.c_str();
        int i = 0;
    
        // Read the first keyword: FUNCTION / STRUCT / UNION
        string keyword = "";
        while (cstr[i] != ' ' && cstr[i] != '\0') {
            keyword += cstr[i++];
        }
    
        while (cstr[i] == ' ') i++;
    
        if (keyword == "FUNCTION") {

            string returnType = "", token = "";
            string args = "";
            bool firstArg = true;

            while (cstr[i] != ' ' && cstr[i] != '\0') {
                returnType += cstr[i++];
            }
    

            while (cstr[i] == ' ') i++;
    
            // Collect arguments if any
            if (returnType == "VOID" && cstr[i] == '\0') {
                return "FUNCTION,VOID<==()";
            }
    
            while (cstr[i] != '\0') {
                token = "";
                while (cstr[i] != ' ' && cstr[i] != '\0') {
                    token += cstr[i++];
                }
                if (!firstArg) args += ",";
                args += token;
                firstArg = false;
    
                while (cstr[i] == ' ') i++;
            }
    
            return "FUNCTION," + returnType + "<==(" + args + ")";
        }
        else if (keyword == "STRUCT" || keyword == "UNION") {
            string result = keyword + ",{";
            bool first = true;
    
            while (cstr[i] != '\0') {

                string fieldType = "";
                while (cstr[i] != ' ' && cstr[i] != '\0') {
                    fieldType += cstr[i++];
                }
    
                while (cstr[i] == ' ') i++;
    

                string fieldName = "";
                while (cstr[i] != ' ' && cstr[i] != '\0') {
                    fieldName += cstr[i++];
                }
    
                while (cstr[i] == ' ') i++;
    
                if (!first) result += ",";
                result += "(" + fieldType + "," + fieldName + ")";
                first = false;
            }
    
            result += "}";
            return result;
        }
    
        return type;
    }
    

    //getters and setters 
    void set_name(string name){
        this->name = name ;
    }

    void set_type(string type){
        this->type = type ;
    }

    void set_next (SymbolInfo* next ){
        this->next = next ;
    }

    string get_name(){
        return name;
    }

    string get_type(){
        return type;
    }

    SymbolInfo* get_next(){
        return next ;
    }

    //Debugg
    void show()
    {
        cout<<endl;
        cout<<"SymbolInfo :";
        cout<<"Name = "<< get_name() <<" ";
        cout<<"Type = "<< get_type() <<endl;
    }


    

};