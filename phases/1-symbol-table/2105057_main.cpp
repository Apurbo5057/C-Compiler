// Only one corner case is left to be handled : duplicate variable in a same STRUCT/UNION
// Report generation is not implemented yet

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include "2105057_SymbolTable.hpp"
#include "2105057_Hash.hpp"

using namespace std;

bool fileExists(const string &filename)
{
    ifstream file(filename);
    return file.good();
}

int main(int argc, char *argv[])
{
    string inputFileName = "input.txt"; // default input file name
    string outputFileName = "output.txt";

    HashFunction hashFunc = &Hash::SDBMHash; // Default hash function

    // parse command line arguments for hash function and input file
    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        if (arg.find("--hash=") == 0)
        {
            string hash = arg.substr(7);
            if (hash == "djb2")
            {
                hashFunc = &Hash::DJB2Hash;
            }
            else if (hash == "fnv1a")
            {
                hashFunc = &Hash::FNV1AHash;
            }
            else if (hash == "sum")
            {
                hashFunc = &Hash::SumHash;
            }
        }
        else if (arg.find("--input=") == 0)
        {
            inputFileName = arg.substr(8);
        }
    }

    // Verify hashFunc is not null
    if (!hashFunc)
    {
        cerr << "Error: hashFunc is null. Check Hash.hpp definitions." << endl;
        return 1;
    }


    // check if input file exists
    if (!fileExists(inputFileName))
    {
        cout << "Input file does not exist" << inputFileName << endl;
        return 1;
    }

    // Redirect input and output streams

    if (!freopen(inputFileName.c_str(), "r", stdin))
    {
        cout << "Error opening input file: " << inputFileName << endl;
        return 1;
    }
    if (!freopen(outputFileName.c_str(), "w", stdout))
    {
        cout << "Error opening output file: " << outputFileName << endl;
        return 1;
    }
    

    int numOfBuckets;
    cin >> numOfBuckets;
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Skip the entire line

    SymbolTable *ST = new SymbolTable(numOfBuckets, hashFunc, true);

    int commandLine = 0;
    string line;
    string wrongParameter = "\tNumber of parameters mismatch for the command ";

    // process command
    while (getline(cin, line))
    {
        if(ST->getCurrentScope()==nullptr){
            cout<<"There is no Scope left"<<endl;
            break;
        }
        commandLine++;
        cout << "Cmd " << commandLine << ": " << line << endl;

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "I")
        {
            string name;
            string type;

            if (!(ss >> name >> type))
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            // For FUNCTION, STRUCT or UNION types, process additional arguments
            if (type == "FUNCTION")
            {
                string returnType;
                if (!(ss >> returnType))
                {
                    cout << wrongParameter << command << endl;
                    continue;
                }

                // Build the complete type string including return type and parameters
                string fullType = type + " " + returnType;
                string param;
                while (ss >> param)
                {
                    fullType += " " + param;
                }

                ST->Insert(name, fullType, true);
            }
            else if (type == "STRUCT" || type == "UNION")
            {
                // Count arguments and build the complete type string
                int argCount = 0;
                string fullType = type;
                string field;

                while (ss >> field)
                {
                    fullType += " " + field;
                    argCount++;
                }

                // Check if we have an even number of arguments (type-name pairs)
                if (argCount % 2 != 0 || argCount < 4)
                {
                    cout << wrongParameter << command << endl;
                    continue;
                }

                ST->Insert(name, fullType, true);
            }
            else
            {
                // For simple types, ensure no additional arguments
                string extraArg;
                if (ss >> extraArg)
                {
                    cout << wrongParameter << command << endl;
                    continue;
                }

                ST->Insert(name, type, true);
            }
        }
        else if (command == "L")
        {
            string name;
            if (!(ss >> name) || (ss >> ws && !ss.eof()))
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            ST->LookUp(name, true);
        }
        else if (command == "D")
        {
            string name;
            if (!(ss >> name) || (ss >> ws && !ss.eof()))
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            ST->Remove(name, true);
        }
        else if (command == "E")
        {
            if (ss >> ws && !ss.eof())
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            ST->exitScope(true);
        }
        else if (command == "S")
        {
            if (ss >> ws && !ss.eof())
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            ST->enterScope(true);
        }
        else if (command == "Q")
        {
            if (ss >> ws && !ss.eof())
            {
                cout << wrongParameter << command << endl;
                continue;
            }
            ST->endProgram(true);
        }
        else if (command == "P")
        {
            string operation;
            if (!(ss >> operation) || (ss >> ws && !ss.eof()))
            {
                cout << wrongParameter << command << endl;
                continue;
            }

            if (operation == "A")
            {
                ST->printAllScopeTable();
            }
            else if (operation == "C")
            {
                ST->printCurrentScopeTable();
            }
            else
            {
                cout << "\tInvalid argument for the command P";
                cout << endl;
            }
        }
        else
        {
            cout << "\tInvalid command " << command << endl;
        }
    }

    // ST->endProgram(true);
    delete ST;

    return 0;
}

// To run on linux machine
// To compile this program try : g++ -fsanitize=address -g 2105057_main.cpp -o myProgram
// To run this program try :./myProgram --hash=[hashFunctionName] --input=[inputFileName]


//To run on windows machine
//g++ -g 2105057_main.cpp -o myProgram
//myProgram --hash=[hashFunctionName] --input=[inputFileName]