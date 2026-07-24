#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include "antlr4-runtime.h"
#include "C8086Lexer.h"
#include "C8086CodeGen.h"
#include "CodeGenerator.hpp"

using namespace antlr4;
using namespace std;

ofstream parserLogFile; // global output stream
ofstream errorFile; // global error stream
ofstream lexLogFile; // global lexer log stream
ofstream dataSectionFile; // global data section file
ofstream codeSectionFile; // global code section file

// Function to trim whitespace from a string
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Function to extract instruction without comments
string getInstruction(const string& line) {
    string result = line;
    size_t commentPos = result.find(";");
    if (commentPos != string::npos) {
        result = result.substr(0, commentPos);
    }
    return trim(result);
}

// Check if line is a label
bool isLabel(const string& line) {
    string trimmed = trim(line);
    return !trimmed.empty() && trimmed.back() == ':' && trimmed.find('\t') != 0;
}

// Peephole optimization function
vector<string> performPeepholeOptimizations(const vector<string>& lines) {
    vector<string> optimized;
    int totalOptimizations = 0;
    
    cout << "\nPerforming peephole optimizations..." << endl;
    
    for (int i = 0; i < lines.size(); i++) {
        bool skipLine = false;
        string currentLine = lines[i];
        
        // Optimization 1: Remove redundant MOV instructions
        // MOV AX, a followed by MOV a, AX
        if (i < lines.size() - 1) {
            string current = getInstruction(currentLine);
            string next = getInstruction(lines[i + 1]);
            
            if (!current.empty() && !next.empty() && 
                current.find("MOV") == 0 && next.find("MOV") == 0) {
                
                // Extract operands from MOV instructions
                regex movPattern(R"(MOV\s+([^,]+),\s*(.+))");
                smatch match1, match2;
                
                if (regex_search(current, match1, movPattern) && 
                    regex_search(next, match2, movPattern)) {
                    
                    string dest1 = trim(match1[1].str());
                    string src1 = trim(match1[2].str());
                    string dest2 = trim(match2[1].str());
                    string src2 = trim(match2[2].str());
                    
                    // Check for MOV A, B followed by MOV B, A pattern
                    if (dest1 == src2 && src1 == dest2) {
                        cout << "  [OPT 1] Removed redundant MOV: " << next << endl;
                        optimized.push_back(currentLine);
                        i++; // Skip the next line
                        totalOptimizations++;
                        continue;
                    }
                }
            }
        }
        
        // Optimization 2: Remove redundant PUSH/POP pairs
        if (i < lines.size() - 1) {
            string current = getInstruction(currentLine);
            string next = getInstruction(lines[i + 1]);
            
            if (!current.empty() && !next.empty()) {
                regex pushPattern(R"(PUSH\s+(.+))");
                regex popPattern(R"(POP\s+(.+))");
                smatch pushMatch, popMatch;
                
                if (regex_search(current, pushMatch, pushPattern) && 
                    regex_search(next, popMatch, popPattern)) {
                    
                    string pushReg = trim(pushMatch[1].str());
                    string popReg = trim(popMatch[1].str());
                    
                    if (pushReg == popReg) {
                        cout << "  [OPT 2] Removed redundant PUSH/POP pair: " << pushReg << endl;
                        i++; 
                        totalOptimizations++;
                        continue;
                    }
                }
            }
        }
        
        // Optimization 3: Remove redundant arithmetic operations
        string instruction = getInstruction(currentLine);
        if (!instruction.empty()) {
            // Remove ADD reg, 0
            if (regex_search(instruction, regex(R"(ADD\s+\w+,\s*0)"))) {
                cout << "  [OPT 3] Removed redundant ADD with 0: " << instruction << endl;
                totalOptimizations++;
                continue;
            }
            
            // Remove SUB reg, 0
            if (regex_search(instruction, regex(R"(SUB\s+\w+,\s*0)"))) {
                cout << "  [OPT 3] Removed redundant SUB with 0: " << instruction << endl;
                totalOptimizations++;
                continue;
            }
            
            // Remove MUL reg, 1 (note: this might need special handling for 8086)
            if (regex_search(instruction, regex(R"(MUL\s+.*1)"))) {
                cout << "  [OPT 3] Removed redundant MUL with 1: " << instruction << endl;
                totalOptimizations++;
                continue;
            }
        }
        
        // If we reach here, keep the current line
        optimized.push_back(currentLine);
    }
    
    // Optimization 4: Remove redundant consecutive labels
    vector<string> finalOptimized;
    vector<string> consecutiveLabels;
    
    for (int i = 0; i < optimized.size(); i++) {
        if (isLabel(optimized[i])) {
            consecutiveLabels.push_back(optimized[i]);
        } else {
            // If we have consecutive labels, keep only the first one
            if (consecutiveLabels.size() > 1) {
                cout << "  [OPT 4] Removed " << (consecutiveLabels.size() - 1) 
                     << " redundant consecutive labels" << endl;
                totalOptimizations += (consecutiveLabels.size() - 1);
                finalOptimized.push_back(consecutiveLabels[0]); // Keep only first label
            } else if (consecutiveLabels.size() == 1) {
                finalOptimized.push_back(consecutiveLabels[0]);
            }
            consecutiveLabels.clear();
            finalOptimized.push_back(optimized[i]);
        }
    }
    
    // Handle case where file ends with labels
    if (consecutiveLabels.size() > 1) {
        cout << "  [OPT 4] Removed " << (consecutiveLabels.size() - 1) 
             << " redundant consecutive labels at end" << endl;
        totalOptimizations += (consecutiveLabels.size() - 1);
        finalOptimized.push_back(consecutiveLabels[0]);
    } else if (consecutiveLabels.size() == 1) {
        finalOptimized.push_back(consecutiveLabels[0]);
    }
    
    cout << "Total optimizations performed: " << totalOptimizations << endl;
    return finalOptimized;
}

// Function to read file into vector of lines
vector<string> readFileLines(const string& filename) {
    vector<string> lines;
    ifstream file(filename);
    string line;
    
    while (getline(file, line)) {
        lines.push_back(line);
    }
    
    file.close();
    return lines;
}

// Function to write vector of lines to file
void writeFileLines(const string& filename, const vector<string>& lines) {
    ofstream file(filename);
    for (const string& line : lines) {
        file << line << endl;
    }
    file.close();
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // ---- Input File ----
    ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        cerr << "Error opening input file: " << argv[1] << endl;
        return 1;
    }

    string outputDirectory = "output/";
    string parserLogFileName = outputDirectory + "parserLog.txt";
    string errorFileName = outputDirectory + "errorLog.txt";
    string lexLogFileName = outputDirectory + "lexerLog.txt";
    string assemblyFileName = outputDirectory + "AssemblyCode.asm";
    string optimizedAssemblyFileName = outputDirectory + "OptimizedAssemblyCode.asm";
    string tempDataFileName = outputDirectory + "temp_data.asm";
    string tempCodeFileName = outputDirectory + "temp_code.asm";

    // create output directory if it doesn't exist
    system(("mkdir -p " + outputDirectory).c_str());

    // ---- Output Files ----
    parserLogFile.open(parserLogFileName);
    if (!parserLogFile.is_open()) {
        cerr << "Error opening parser log file: " << parserLogFileName << endl;
        return 1;
    }

    errorFile.open(errorFileName);
    if (!errorFile.is_open()) {
        cerr << "Error opening error log file: " << errorFileName << endl;
        return 1;
    }

    lexLogFile.open(lexLogFileName);
    if (!lexLogFile.is_open()) {
        cerr << "Error opening lexer log file: " << lexLogFileName << endl;
        return 1;
    }

    dataSectionFile.open(tempDataFileName);
    if (!dataSectionFile.is_open()) {
        cerr << "Error opening dataSection file: " << tempDataFileName << endl;
        return 1;
    }

    codeSectionFile.open(tempCodeFileName);
    if (!codeSectionFile.is_open()) {
        cerr << "Error opening codeSection file: " << tempCodeFileName << endl;
        return 1;
    }

    // ---- Initialize Code Generator ----
    initializeCodeGenerator(&parserLogFile, &errorFile , &dataSectionFile, &codeSectionFile);
   
    // ---- Parsing Flow ----
    ANTLRInputStream input(inputFile);
    C8086Lexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    C8086CodeGen parser(&tokens);

    // this is necessary to avoid the default error listener and use our custom error handling
    parser.removeErrorListeners();

    try {
        // start parsing at the 'start' rule
        parser.start();
        
        cout << "Parsing completed successfully!" << endl;
        
    } catch (const exception& e) {
        cerr << "Error during parsing: " << e.what() << endl;
        return 1;
    }

    // ---- Cleanup ----
    finalizeCodeGenerator();
    
    inputFile.close();
    parserLogFile.close();
    errorFile.close();
    lexLogFile.close();
    dataSectionFile.close();
    codeSectionFile.close();

    // Now merge the temporary data and code files into the unoptimized assembly file
    std::ifstream tempData(tempDataFileName);
    std::ifstream tempCode(tempCodeFileName);
    std::ofstream assemblyFile(assemblyFileName);

    assemblyFile << tempData.rdbuf();
    assemblyFile << tempCode.rdbuf();

    tempData.close();
    tempCode.close();
    assemblyFile.close();

    cout << "Unoptimized assembly code written to: " << assemblyFileName << endl;

    // ---- PEEPHOLE OPTIMIZATIONS ----
    cout << "\n=== PEEPHOLE OPTIMIZATION PHASE ===" << endl;
    
    // Read the generated assembly file
    vector<string> assemblyLines = readFileLines(assemblyFileName);
    
    // Perform peephole optimizations
    vector<string> optimizedLines = performPeepholeOptimizations(assemblyLines);
    
    // Write optimized assembly to new file
    writeFileLines(optimizedAssemblyFileName, optimizedLines);
    
    cout << "\nOptimized assembly code written to: " << optimizedAssemblyFileName << endl;
    cout << "Original file size: " << assemblyLines.size() << " lines" << endl;
    cout << "Optimized file size: " << optimizedLines.size() << " lines" << endl;
    cout << "Lines saved: " << (assemblyLines.size() - optimizedLines.size()) << endl;

    // Clean up temporary files
    std::remove(tempDataFileName.c_str());
    std::remove(tempCodeFileName.c_str());

    cout << "\nGenerated files:" << endl;
    cout << "  - Original assembly: " << assemblyFileName << endl;
    cout << "  - Optimized assembly: " << optimizedAssemblyFileName << endl;
    cout << "  - Parser log: " << parserLogFileName << endl;
    cout << "  - Error log: " << errorFileName << endl;
    cout << "  - Lexer log: " << lexLogFileName << endl;

    return 0;
}