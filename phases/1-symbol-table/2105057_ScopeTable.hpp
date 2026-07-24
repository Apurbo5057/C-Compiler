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

    SymbolInfo **hashTable;

    //hashFunc pointer is used to incorporate different hash func from command line
    HashFunction hashFunc;

public:
    ScopeTable *parentScope;

    // constructor
    ScopeTable(int n, int scopeNum,HashFunction hashFunc=&Hash::SDBMHash, ScopeTable *parentScope = nullptr)
    {
        // initialization of the hash table
        this->numOfBuckets = n;

        this->parentScope = parentScope;

        this->hashTable = new SymbolInfo *[numOfBuckets];

        for (int i = 0; i < numOfBuckets; i++)
        {
            hashTable[i] = nullptr;
        }

        this->id = to_string(scopeNum);
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
                    cout << "\t'" << name << "' found in ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;
                return currentSymbol;
            }
            currentSymbol = currentSymbol->next;
            indexInChain++;
        }

        return nullptr;
    }

    bool Insert(string name, string type, bool verbose = false)
    {

        SymbolInfo *existingSymbol = LookUp(name);

        int index = calculateBucketIndex(name);
        int indexInChain = 1;

        if (existingSymbol != nullptr)
        {
            if (verbose)
                cout << "\t'" << name << "' already exists in the current ScopeTable" << endl;
            return false;
        }
        else
        {
            SymbolInfo *newSymbol = new SymbolInfo(name, type, nullptr);

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

        if (verbose)
            cout << "\tInserted in ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;
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
                cout << "\tDeleted '" << name << "' from ScopeTable# " << getId() << " at position " << (index + 1) << ", " << indexInChain << endl;

            return true;
        }
        else
        {
            // doesn't exist cant remove
            if (verbose)
                cout << "\tNot found in the current ScopeTable" << endl;
            return false;
        }
    }

    void printScopeTable(int scopeCount)
    {

        for (int j = 0; j < scopeCount; j++)
        {
            cout << "\t";
        }

        cout << "ScopeTable# " << this->id << endl;
        for (int i = 0; i < numOfBuckets; i++)
        {
            for (int j = 0; j < scopeCount; j++)
            {
                cout << "\t";
            }
            cout << i + 1;
            cout << "--> ";
            SymbolInfo *curr = hashTable[i];
            while (curr != nullptr)
            {
                cout << "<" << curr->get_name() << "," << curr->get_type() << "> ";
                curr = curr->next;
            }
            cout << endl;
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
