#pragma once

#include <iostream>
#include <string>

#include "2105057_ScopeTable.hpp"

using namespace std;

class SymbolTable
{
private:
    int numOfBuckets;
    ScopeTable *currentScope;

    ostream& out;
    HashFunction hashFunc;

public:
    // Default hash function is SDBMHash
    SymbolTable(int n,HashFunction hashFunc = &Hash::SDBMHash, ostream& outfile = cout, bool verbose = false): out(outfile)
    {
        this->numOfBuckets = n;
        this->hashFunc = hashFunc;
        this->currentScope = nullptr;

        enterScope(verbose);
    }

    ~SymbolTable()
    {
        endProgram(false);
    }

    ScopeTable *getCurrentScope()
    {
        return currentScope;
    }

    void enterScope(bool verbose = false)
    {
        int childNum = 1; // Default childNum for the first scope
        
        if (currentScope != nullptr)
        {
            currentScope->incrementChildCount();
            childNum = currentScope->getChildCount();
        }
        
        ScopeTable *newScope = new ScopeTable(this->numOfBuckets, childNum, this->hashFunc, this->currentScope, this->out);
        
        this->currentScope = newScope;
        if (verbose)
            out << "\tScopeTable# " << currentScope->getId() << " created" << endl;
    }

    void exitScope(bool verbose = false, bool quit = false)
    {
        if (currentScope->parentScope == nullptr && !quit)
        {
            if (verbose)
                out << "\tScopeTable# " << currentScope->getId() << "  cannot be removed" << endl;
            return;
        }
        ScopeTable *temp = currentScope;
        currentScope = currentScope->parentScope;
        temp->parentScope = nullptr;
        string tempId = temp->getId();
        delete temp;
        if (verbose)
            out << "\tScopeTable# " << tempId << " removed" << endl;
    }

    bool Insert(string name, string type,int& bucket, int& chainPos, bool verbose = false)
    {
        if(currentScope == nullptr){
            out<<"\tCannot Insert anymore . You have removed all Scope."<<endl;
            bucket = -1;
            chainPos = -1;
            return false;
        }
        bool success = currentScope->Insert(name, type,bucket,chainPos, verbose);
        return success;
    }

    bool Remove(string name, bool verbose = false)
    {
        if(currentScope == nullptr){
            out<<"\tCannot Insert anymore . You have removed all Scope."<<endl;
            return false;
        }
        bool success = currentScope->Remove(name, verbose);
        return success;
    }

    SymbolInfo *LookUp(string name, bool verbose = false)
    {
        // first searches currentscope if not found then recursively searches the parentscope.

        ScopeTable *searchScope = currentScope;

        // search until parentscope is not nullptr or main scope
        while (searchScope != nullptr)
        {

            SymbolInfo *searchedSymbol = searchScope->LookUp(name, verbose);
            if (searchedSymbol != nullptr)
            {
                return searchedSymbol;
            }
            searchScope = searchScope->parentScope;
        }
        if (verbose)
            out << "\t'" << name << "' not found in any of the ScopeTables" << endl;
        return nullptr;
    }

    bool endProgram(bool verbose = false)
    {
        if (currentScope == nullptr)
        {
            if (verbose)
                out << "\tNo ScopeTable exists" << endl;
            return false;
        }
        while (currentScope != nullptr)
        {
            exitScope(verbose, true);
        }
        return true;
    }

    void printCurrentScopeTable()
    {
        currentScope->printScopeTable(1);
    }

    void printAllScopeTable()
    {
        ScopeTable *curr = currentScope;
        int scopeCount = 1;
        while (curr != nullptr)
        {
            curr->printSimpleScopeTable();
            curr = curr->parentScope;
            scopeCount++;
        }
    }

    double getMeanCollisionRatio()
    {
        double sum = 0.0;
        double scopeCount = 0.0;

        ScopeTable *curr = currentScope;
        while (curr != nullptr)
        {
            sum += curr->getCollisionRatio();
            scopeCount++;
            curr = curr->parentScope;
        }

        return scopeCount > 0 ? sum / scopeCount : 0.0;
    }
};