#pragma once

#include <iostream>
#include <string>

#include "2105057_symbolInfo.hpp"
#include "2105057_Hash.hpp"
using namespace std;

// Defined a function pointer 
typedef uint64_t (*HashFunction)(string, uint64_t);

class ScopeTable
{
private:
    // properties of hashtable
    int numOfBuckets;
    // attributes for scope table identification
    string id;
    int childCount;
    int childNum;

    SymbolInfo **hashTable;

    //hashFunc pointer is used to incorporate different hash func from command line
    HashFunction hashFunc;

    ostream& out;

public:
    ScopeTable *parentScope;

    // constructor
    ScopeTable(int n, int childNum,HashFunction hashFunc=&Hash::SDBMHash, ScopeTable *parentScope = nullptr,ostream& outFile = cout):out(outFile)
    {
        // initialization of the hash table
        this->numOfBuckets = n;

        this->parentScope = parentScope;

        this->hashTable = new SymbolInfo *[numOfBuckets];
        this->childNum=childNum;

        for (int i = 0; i < numOfBuckets; i++)
        {
            hashTable[i] = nullptr;
        }
        if(this->parentScope == nullptr){
            this->id="1";
        }
        else{
            this->id = parentScope->getId()+"."+to_string(childNum);
        }
        this->childCount=0;
        this->hashFunc = hashFunc;
    }

    // destructor
    ~ScopeTable()
    {

        // We will delete the first Symbol of any bucket and the rest of the symbol will be deleted recursively
        for (int i = 0; i < numOfBuckets; i++)
        {
            if (hashTable[i] != nullptr)
                delete hashTable[i];
        }

        delete[] hashTable;
    }

    string getId()
    {
        return id;
    }
    int getChildNum(){
        return childNum;
    }
    void incrementChildCount(){
        childCount++;
    }
    int getChildCount(){
        return childCount;
    }
    int calculateBucketIndex(string name)
    {
        if (!this->hashFunc) {
            cerr << "Error: hashFunc is null in calculateBucketIndex" << endl;
            return 0; // Return a safe default index or handle error appropriately
        }
        uint64_t index = hashFunc(name, (uint64_t)numOfBuckets);
        return static_cast<int>(index);
    }


    SymbolInfo *LookUp(string name, bool verbose = false)
    {
        int index = calculateBucketIndex(name);
        int indexInChain = 1;
        SymbolInfo *currentSymbol = hashTable[index];

        while (currentSymbol != nullptr)
        {
            if (currentSymbol->get_name() == name)
            {
                if (verbose)
                    out << "\t'" << name << "' found in ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;
                // chainPos = indexInChain;
                return currentSymbol;
            }
            currentSymbol = currentSymbol->next;
            indexInChain++;
        }

        return nullptr;
    }


    bool Insert(string name, string type,int& bucket, int& chainPos, bool verbose = false)
    {

        int index = calculateBucketIndex(name);
        bucket = index;
        SymbolInfo *existingSymbol = LookUp(name);

        int indexInChain = 1;

        if (existingSymbol != nullptr)
        {
            // Symbol exists, find its chain position
            int indexInChain = 1;
            SymbolInfo *current = hashTable[index];
            while (current != nullptr)
            {
                if (current->get_name() == name)
                {
                    chainPos = indexInChain;
                    if (verbose)
                        out << "\t'" << name << "' already exists in ScopeTable# " << getId() << " at position " << (index + 1) << ", " << chainPos << endl;
                    return false;
                }
                current = current->next;
                indexInChain++;
            }
            // Should not reach here if LookUp found the symbol
            chainPos = -1; // Safety default
            return false;
        }
        else
        {
            SymbolInfo *newSymbol = new SymbolInfo(name, type, nullptr,out);

            SymbolInfo *current = hashTable[index];

            // if first entry of that bucket is empty then insert
            if (current == nullptr)
            {
                hashTable[index] = newSymbol;
            }
            else
            {
                SymbolInfo *nextInfo = current;
                while (nextInfo->next != nullptr)
                {
                    nextInfo = nextInfo->next;
                    indexInChain++;


                }
                nextInfo->next = newSymbol;
                indexInChain++;

            }
        }

        chainPos = indexInChain;

        if (verbose)
            out << "\tInserted in ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;
        return true;
    }

    bool Remove(string name, bool verbose = false)
    {
        // first search if the symbol already exist if not then you can't delete

        SymbolInfo *existingSymbol = LookUp(name);
        int index = calculateBucketIndex(name);
        int indexInChain = 1;

        if (existingSymbol != nullptr)
        {
            // remove

            SymbolInfo *CurrentSymbol = hashTable[index];
            // SymbolInfo *prev;
            SymbolInfo *temp;

            // in case we remove the first symbol from a bucket then update the reference in hashtable .
            if (CurrentSymbol == existingSymbol)
            {
                hashTable[index] = CurrentSymbol->next;
                temp = CurrentSymbol;
            }
            else
            {
                while (CurrentSymbol->next != existingSymbol)
                {
                    CurrentSymbol = CurrentSymbol->next;
                    indexInChain++;
                }
                temp = CurrentSymbol->next; // want to delete CurrentSymbol->next;
                CurrentSymbol->next = CurrentSymbol->next->next;
            }

            // to avoid recursive deletion
            temp->next = nullptr;
            delete temp;

            if (verbose)
                out << "\tDeleted '" << name << "' from ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;

            return true;
        }
        else
        {
            // doesn't exist cant remove
            if (verbose)
                out << "\tNot found in the current ScopeTable" << endl;
            return false;
        }
    }

    void printScopeTable(int scopeCount)
    {

        for (int j = 0; j < scopeCount; j++)
        {
            out << "\t";
        }

        out << "ScopeTable# " << this->id << endl;
        for (int i = 0; i < numOfBuckets; i++)
        {
            for (int j = 0; j < scopeCount; j++)
            {
                out << "\t";
            }
            out << i + 1;
            out << "--> ";
            SymbolInfo *curr = hashTable[i];
            while (curr != nullptr)
            {
                out << "<" << curr->get_name() << "," << curr->get_type() << "> ";
                curr = curr->next;
            }
            out << endl;
        }
    }

    void printSimpleScopeTable() {
        out << "ScopeTable # " << id << endl;
        for (int i = 0; i < numOfBuckets; i++) {
            SymbolInfo *curr = hashTable[i];
            if (curr==nullptr) continue;
            out << i << " --> ";
            while (curr) {
                out << "< " << curr->get_name() << " : " << curr->get_type() << " >";
                curr = curr->next;
            }
            out << endl;
        }
    }


    // Function to get the collision ratio for a single scope
    double getCollisionRatio(){
        int nonEmptyBuckets = 0;
        int totalInsertions = 0;

        for (int i = 0; i<numOfBuckets;i++){
            SymbolInfo* curr = hashTable[i];
            if(curr != nullptr){
                nonEmptyBuckets++;
                int count = 0;
                while(curr != nullptr){
                    count++;
                    curr = curr->next;
                }
                totalInsertions += count;
            }
        }

        int collisions = totalInsertions - nonEmptyBuckets;
        return static_cast<double>(collisions) / numOfBuckets;
    }
};
