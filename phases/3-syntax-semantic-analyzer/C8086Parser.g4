parser grammar C8086Parser;

options {
    tokenVocab = C8086Lexer;
}

@parser::header {
    #include <iostream>
    #include <fstream>
    #include <string>
    #include <cstdlib>
    #include "C8086Lexer.h"
    #include "str_list.cpp"
    #include "ParseInfo.h"
    #include "headers/2105057_symbolInfo.hpp"
    #include "headers/2105057_ScopeTable.hpp"
    #include "headers/2105057_SymbolTable.hpp"  // ADD THIS LINE

    extern std::ofstream parserLogFile;
    extern std::ofstream errorFile;
    extern int syntaxErrorCount;
    extern int numOfBuckets;
    extern SymbolTable* ST;  // Move declaration here
}

@parser::members {

	int numberOfBuckets = 7;  // Add type declaration
    HashFunction hashFunc = &Hash::SDBMHash;
    SymbolTable *ST = new SymbolTable(numberOfBuckets, hashFunc, true);  // Use numberOfBuckets, not numOfBuckets

    void writeIntoparserLogFile(const std::string message) {
        if (!parserLogFile) {
            std::cout << "Error opening parserLogFile.txt" << std::endl;
            std::cout << std::endl;
            return;
        }
        parserLogFile << message << std::endl;
        parserLogFile << std::endl;
        parserLogFile.flush();
    }

    void writeIntoErrorFile(const std::string message) {
        if (!errorFile) {
            std::cout << "Error opening errorFile.txt" << std::endl;
            return;
        }
        errorFile << message << std::endl;
        errorFile << std::endl;
        errorFile.flush();
    }

    void logLine(int line, const std::string& rule) {
        writeIntoparserLogFile("Line " + std::to_string(line) + ": " + rule);
    }
    void logError(int line, const std::string& message) {
        writeIntoErrorFile("Line " + std::to_string(line) + ": " + message);
    }

    // Helper to check if type is integer
    bool isIntegerType(const std::string& type) {
        return type == "int" || type == "CONST_INT";
    }

    // Helper to check type compatibility

    bool isTypeCompatible(const std::string& left, const std::string& right) {
        // Check if either is an array type
        bool leftIsArray = (left.find("ARRAY,") == 0);
        bool rightIsArray = (right.find("ARRAY,") == 0);
        
        // Arrays and scalars are not compatible
        if (leftIsArray != rightIsArray) {
            return false;
        }
        
        // Get base types for comparison
        std::string leftBase = leftIsArray ? left.substr(6) : left;
        std::string rightBase = rightIsArray ? right.substr(6) : right;
        
        // Same base types are compatible
        return leftBase == rightBase;
    }

    // Helper to get base type from potentially prefixed type
    std::string getBaseType(const std::string& type) {
        if (type.find("ARRAY,") == 0) {
            return type.substr(6);
        }
        return type;
    }


    std::vector<std::string> extractParamTypes(const std::string& paramStr) {
        std::vector<std::string> types;
        std::stringstream ss(paramStr);
        std::string param;
        while (std::getline(ss, param, ',')) {
            size_t start = param.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t end = param.find_last_not_of(" \t");
                param = param.substr(start, end - start + 1);
            }

            if (!param.empty()) {
                types.push_back("int"); // Default assumption
            }
        }
        return types;
    }
}

start : program
    {
        writeIntoparserLogFile("Parsing completed successfully with " + std::to_string(syntaxErrorCount) + " syntax errors.");
        ST->printAllScopeTable(parserLogFile); // Print symbol table at the end
        delete ST;  // Clean up symbol table

        writeIntoparserLogFile("Total Error count: " + std::to_string(syntaxErrorCount));
    }
    ;

program returns [ParseInfo info]
    : p=program u=unit {
        $info.line_number = $u.info.line_number;
        $info.parsed_code = $p.info.parsed_code + "\n" + $u.info.parsed_code;
        logLine($u.info.line_number, "program : program unit");
        writeIntoparserLogFile($info.parsed_code);
    }
    | u=unit {
        $info.line_number = $u.info.line_number;
        $info.parsed_code = $u.info.parsed_code;
        logLine($u.info.line_number, "program : unit");
        writeIntoparserLogFile($info.parsed_code);
    }
    ;

unit returns [ParseInfo info]
    : var_declaration {
        $info = $var_declaration.info;
        logLine($info.line_number, "unit : var_declaration");
        writeIntoparserLogFile($info.parsed_code);
    }
    | func_declaration {
        $info = $func_declaration.info;
        logLine($info.line_number, "unit : func_declaration");
        writeIntoparserLogFile($info.parsed_code);
    }
    | func_definition {
        $info = $func_definition.info;
        logLine($info.line_number, "unit : func_definition");
        writeIntoparserLogFile($info.parsed_code);
    }
    ;



func_declaration returns [ParseInfo info]
    : ts=type_specifier ID LPAREN pl=parameter_list RPAREN sm=SEMICOLON {
        $info.line_number = $sm->getLine();
        std::string funcName = $ID->getText();
        std::string funcType = "FUNCTION," + $ts.name_line + "<==(" + $pl.param_string + ")>";
        if (!ST->Insert(funcName, funcType)) {
            logError($sm->getLine(), "Multiple declaration of function " + funcName);
            syntaxErrorCount++;
        }
        logLine($sm->getLine(), "func_declaration : type_specifier ID LPAREN parameter_list RPAREN SEMICOLON");
        std::string code = $ts.name_line + " " + funcName + "(" + $pl.param_string + ");";
        $info = ParseInfo($sm->getLine(), code);
        writeIntoparserLogFile(code);
    }
    | ts=type_specifier ID LPAREN RPAREN sm=SEMICOLON {
        $info.line_number = $sm->getLine();
        std::string funcName = $ID->getText();
        std::string funcType = "FUNCTION," + $ts.name_line + "<==()>";
        if (!ST->Insert(funcName, funcType)) {
            writeIntoErrorFile("Line " + std::to_string($sm->getLine()) + ": Multiple declaration of function " + funcName);
            syntaxErrorCount++;
        }
        logLine($sm->getLine(), "func_declaration : type_specifier ID LPAREN RPAREN SEMICOLON");
        std::string code = $ts.name_line + " " + funcName + "();";
        $info = ParseInfo($sm->getLine(), code);
        writeIntoparserLogFile(code);
    }
    ;


func_definition returns [ParseInfo info]
    : ts=type_specifier ID LPAREN pl=parameter_list RPAREN { 
        // First check if function already declared
        std::string funcName = $ID->getText();
        std::string funcType = "FUNCTION," + $ts.name_line + "<==(" + $pl.param_string + ")>";
        SymbolInfo* existing = ST->LookUp(funcName);
        
        // Insert function in global scope if not exists
        if (!existing) {
            ST->Insert(funcName, funcType);
        }
        
        // Enter function scope and insert parameters
        ST->enterScope();
        std::stringstream ss($pl.param_string);
        std::string param;
        while (std::getline(ss, param, ',')) {
            std::stringstream ps(param);
            std::string type, name;
            ps >> type >> name;
            if (!name.empty()) {
                // Insert parameter with ACTUAL TYPE, not "ID"
                if (!ST->Insert(name, type)) {  // type is already "int", "float", etc.
                    logError($ts.line_number, "Multiple declaration of parameter " + name);
                    syntaxErrorCount++;
                }
            }
        }
      }cs=compound_statement {
        
        $info.line_number = $ts.line_number;
        // std::string funcName = $ID->getText();
        // std::string funcType = "FUNCTION," + $ts.name_line + "<==(" + $pl.param_string + ")>";
        // SymbolInfo* existing = ST->LookUp(funcName);
        if (existing) {
            if (!existing->isFunctionSymbol()) {
                logError($ts.line_number, funcName + " was not declared as a function");
                syntaxErrorCount++;
            } else {
                // Check return type and parameters compatibility
                std::string existingType = existing->getOriginalType();
                if (existingType != funcType) {
                    logError($ts.line_number, "Function definition doesn't match declaration for " + funcName);
                    syntaxErrorCount++;
                }
            }
        }
        
        std::string code = $ts.name_line + " " + funcName + "(" + $pl.param_string + ")" + $cs.code_string;
        logLine($ts.line_number, "func_definition : type_specifier ID LPAREN parameter_list RPAREN compound_statement");
        writeIntoparserLogFile(code);
        // ST->printAllScopeTable(parserLogFile);  
        $info = ParseInfo($ts.line_number, code);
        ST->exitScope();
    }
    | ts=type_specifier ID LPAREN RPAREN cs=compound_statement {
        $info.line_number = $ts.line_number;
        std::string funcName = $ID->getText();
        std::string funcType = "FUNCTION," + $ts.name_line + "<==()>";
        SymbolInfo* existing = ST->LookUp(funcName);
        if (existing) {
            if (!existing->isFunctionSymbol()) {
                // logError($ts.line_number, funcName + " was not declared as a function");
                // syntaxErrorCount++;
            } else if (existing->getReturnType() != $ts.name_line) {
                logError($ts.line_number, "Return type mismatch for function " + funcName);
                syntaxErrorCount++;
            }
        } else {
            ST->Insert(funcName, funcType);
        }
        
        std::string code = $ts.name_line + " " + funcName + "()" + $cs.code_string;
        logLine($ts.line_number, "func_definition : type_specifier ID LPAREN RPAREN compound_statement");
        writeIntoparserLogFile(code);
        ST->printAllScopeTable(parserLogFile);
        $info = ParseInfo($ts.line_number, code);
    }
    ;


parameter_list returns [std::string param_string]
    : pl=parameter_list COMMA ts=type_specifier ID {
        $param_string = $pl.param_string + "," + $ts.name_line + " " + $ID->getText();
        logLine($ID->getLine(), "parameter_list : parameter_list COMMA type_specifier ID");
        writeIntoparserLogFile($param_string);
    }
    | pl=parameter_list COMMA ts=type_specifier {
        $param_string = $pl.param_string + "," + $ts.name_line;
        logLine($ts.line_number, "parameter_list : parameter_list COMMA type_specifier");
        writeIntoparserLogFile($param_string);
    }
    | ts=type_specifier ID {
        $param_string = $ts.name_line + " " + $ID->getText();
        logLine($ID->getLine(), "parameter_list : type_specifier ID");
        writeIntoparserLogFile($param_string);
    }
    | ts=type_specifier {
        $param_string = $ts.name_line;
        logLine($ts.line_number, "parameter_list : type_specifier");
        writeIntoparserLogFile($param_string);
    }
    ;

compound_statement returns [std::string code_string, int line_number]
    : LCURL {
        ST->enterScope(); // Create new scope
    } st=statements RCURL {
        $code_string = "{\n" + $st.statements_string + "\n}";
        $line_number = $LCURL->getLine();
        logLine($RCURL->getLine(), "compound_statement : LCURL statements RCURL");
        writeIntoparserLogFile($code_string);
        ST->printAllScopeTable(parserLogFile); // Print symbol table before exiting scope
        ST->exitScope(); // Remove scope
    }
    | LCURL {
        ST->enterScope(); // Create new scope
    } RCURL {
        $code_string = "{\n}";
        $line_number = $LCURL->getLine();
        logLine($RCURL->getLine(), "compound_statement : LCURL RCURL");
        writeIntoparserLogFile($code_string);
        ST->printAllScopeTable(parserLogFile); // Print symbol table before exiting scope
        ST->exitScope(); // Remove scope
    }
    ;

var_declaration returns [ParseInfo info]
    : t=type_specifier dl=declaration_list sm=SEMICOLON {
        int line = $sm->getLine();
        std::string code = $t.name_line + " " + $dl.var_list.get_list_as_string() + ";";
        // Insert variables into symbol table with ACTUAL TYPE
        for (const auto& var : $dl.var_list.get_variables()) {
            std::string varType;
            std::string varName;
            
            if (var.find('[') != std::string::npos) {
                // Array variable
                varName = var.substr(0, var.find('['));
                varType = "ARRAY," + $t.name_line;  // Store actual type: ARRAY,int or ARRAY,float
            } else {
                // Regular variable
                varName = var;
                varType = $t.name_line;  // Store actual type: int, float, void
            }
            
            if (!ST->Insert(varName, varType)) {
                writeIntoErrorFile("Line " + std::to_string(line) + ": Multiple declaration of " + varName);
                syntaxErrorCount++;
            }
        }
        logLine(line, "var_declaration : type_specifier declaration_list SEMICOLON");
        writeIntoparserLogFile(code);
        $info = ParseInfo(line, code);
    }
    | t=type_specifier de=declaration_list_err sm=SEMICOLON {
        writeIntoErrorFile(
            std::string("Line ") + std::to_string($sm->getLine()) +
            ": " + $de.error_name + " - Syntax error at declaration list of variable declaration"
        );
        syntaxErrorCount++;
    }
    ;

declaration_list_err returns [std::string error_name]
    : {
        $error_name = "Error in declaration list";
    }
    ;

type_specifier returns [std::string name_line, int line_number]
    : INT {
        $name_line = $INT->getText();
        $line_number = $INT->getLine();
        logLine($INT->getLine(), "type_specifier : INT");
        writeIntoparserLogFile($INT->getText());
    }
    | FLOAT {
        $name_line = $FLOAT->getText();
        $line_number = $FLOAT->getLine();
        logLine($FLOAT->getLine(), "type_specifier : FLOAT");
        writeIntoparserLogFile($FLOAT->getText());
    }
    | VOID {
        $name_line = $VOID->getText();
        $line_number = $VOID->getLine();
        logLine($VOID->getLine(), "type_specifier : VOID");
        writeIntoparserLogFile($VOID->getText());
    }
    ;

declaration_list returns [str_list var_list]
    : dl=declaration_list COMMA ID {
        $var_list.set_variables($dl.var_list.get_variables());
        $var_list.add($ID->getText());
        logLine($ID->getLine(), "declaration_list : declaration_list COMMA ID");
        writeIntoparserLogFile($var_list.get_list_as_string());
    }
    | dl=declaration_list COMMA ID LTHIRD CONST_INT RTHIRD {
        $var_list.set_variables($dl.var_list.get_variables());
        $var_list.add($ID->getText() + "[" + $CONST_INT->getText() + "]");
        logLine($ID->getLine(), "declaration_list : declaration_list COMMA ID LTHIRD CONST_INT RTHIRD");
        writeIntoparserLogFile($var_list.get_list_as_string());
    }
    | ID {
        logLine($ID->getLine(), "declaration_list : ID");
        $var_list.add($ID->getText());
        writeIntoparserLogFile($ID->getText());
    }
    | ID LTHIRD CONST_INT RTHIRD {
        logLine($ID->getLine(), "declaration_list : ID LTHIRD CONST_INT RTHIRD");
        $var_list.add($ID->getText() + "[" + $CONST_INT->getText() + "]");
        writeIntoparserLogFile($ID->getText() + "[" + $CONST_INT->getText() + "]");
    }
    ;

statements returns [std::string statements_string]
    : st=statement {
        $statements_string = $st.statement_string;
        logLine($st.line_number, "statements : statement");
        writeIntoparserLogFile($statements_string);
    }
    | sts=statements st=statement {
        $statements_string = $sts.statements_string + "\n" + $st.statement_string;
        logLine($st.line_number, "statements : statements statement");
        writeIntoparserLogFile($statements_string);
    }
    ;

statement returns [std::string statement_string, int line_number]
    : vd=var_declaration {
        $statement_string = $vd.info.parsed_code;
        $line_number = $vd.info.line_number;
        logLine($line_number, "statement : var_declaration");
        writeIntoparserLogFile($statement_string);
    }
    | es=expression_statement {
        $statement_string = $es.expr_string;
        $line_number = $es.line_number;
        logLine($line_number, "statement : expression_statement");
        writeIntoparserLogFile($statement_string);
    }
    | cs=compound_statement {
        $statement_string = $cs.code_string;
        $line_number = $cs.line_number;
        logLine($line_number, "statement : compound_statement");
        writeIntoparserLogFile($statement_string);
    }
    | FOR LPAREN es1=expression_statement es2=expression_statement e=expression RPAREN st=statement {
        $statement_string = "for(" + $es1.expr_string + $es2.expr_string + $e.expr_string + ")" + $st.statement_string;
        $line_number = $FOR->getLine();
        logLine($line_number, "statement : FOR LPAREN expression_statement expression_statement expression RPAREN statement");
        writeIntoparserLogFile($statement_string);
    }
    | IF LPAREN e=expression RPAREN st=statement {
        $statement_string = "if(" + $e.expr_string + ")" + $st.statement_string;
        $line_number = $IF->getLine();
        logLine($line_number, "statement : IF LPAREN expression RPAREN statement");
        writeIntoparserLogFile($statement_string);
    }
    | IF LPAREN e=expression RPAREN st1=statement ELSE st2=statement {
        $statement_string = "if(" + $e.expr_string + ")" + $st1.statement_string + " else " + $st2.statement_string;
        $line_number = $IF->getLine();
        logLine($line_number, "statement : IF LPAREN expression RPAREN statement ELSE statement");
        writeIntoparserLogFile($statement_string);
    }
    | WHILE LPAREN e=expression RPAREN st=statement {
        $statement_string = "while(" + $e.expr_string + ")" + $st.statement_string;
        $line_number = $WHILE->getLine();
        logLine($line_number, "statement : WHILE LPAREN expression RPAREN statement");
        writeIntoparserLogFile($statement_string);
    }
    | PRINTLN LPAREN ID RPAREN SEMICOLON {
        $statement_string = "println(" + $ID->getText() + ");";
        $line_number = $PRINTLN->getLine();
        if (!ST->LookUp($ID->getText())) {
            writeIntoErrorFile("Line " + std::to_string($line_number) + ": Undeclared variable " + $ID->getText());
            syntaxErrorCount++;
        }
        logLine($line_number, "statement : PRINTLN LPAREN ID RPAREN SEMICOLON");
        writeIntoparserLogFile($statement_string);
    }
    | RETURN e=expression SEMICOLON {
        $statement_string = "return " + $e.expr_string + ";";
        $line_number = $RETURN->getLine();
        logLine($line_number, "statement : RETURN expression SEMICOLON");
        writeIntoparserLogFile($statement_string);
    }
    ;

expression_statement returns [std::string expr_string, int line_number]
    : SEMICOLON {
        $expr_string = ";";
        $line_number = $SEMICOLON->getLine();
        logLine($line_number, "expression_statement : SEMICOLON");
        writeIntoparserLogFile($expr_string);
    }
    | e=expression SEMICOLON {
        $expr_string = $e.expr_string + ";";
        $line_number = $SEMICOLON->getLine();
        logLine($line_number, "expression_statement : expression SEMICOLON");
        writeIntoparserLogFile($expr_string);
    }
    ;

variable returns [std::string var_string, int line_number, std::string var_type]
    : ID {
        $var_string = $ID->getText();
        $line_number = $ID->getLine();
        SymbolInfo* sym = ST->LookUp($ID->getText());
        if (!sym) {
            logError($line_number, "Undeclared variable " + $ID->getText());
            syntaxErrorCount++;
            $var_type = "";
        } else {
            std::string fullType = sym->getOriginalType(); // Use original type
            if (fullType.find("ARRAY,") == 0) {
                // This is array access without brackets - ERROR for assignment ONLY in assignment context
                // For expression context, this should be allowed as array name
                $var_type = fullType; // Keep full array type
            } else {
                $var_type = sym->getType(); // Regular variable type (int/float/void)
            }
        }
        logLine($line_number, "variable : ID");
        writeIntoparserLogFile($var_string);
    }
    | ID LTHIRD e=expression RTHIRD {
        $var_string = $ID->getText() + "[" + $e.expr_string + "]";
        $line_number = $ID->getLine();
        SymbolInfo* sym = ST->LookUp($ID->getText());
        if (!sym) {
            writeIntoErrorFile("Line " + std::to_string($line_number) + ": Undeclared variable " + $ID->getText());
            syntaxErrorCount++;
            $var_type = "";
        } else {
            std::string fullType = sym->getOriginalType();
            if (fullType.find("ARRAY,") != 0) {
                writeIntoErrorFile("Line " + std::to_string($line_number) + ": " + $ID->getText() + " is not an array");
                syntaxErrorCount++;
                $var_type = "";
            } else {
                $var_type = fullType.substr(6); // Remove "ARRAY," to get base type (int/float)
                if (!$e.is_integer) {
                    writeIntoErrorFile("Line " + std::to_string($line_number) + ": Expression inside third brackets not an integer");
                    syntaxErrorCount++;
                }
            }
        }
        logLine($line_number, "variable : ID LTHIRD expression RTHIRD");
        writeIntoparserLogFile($var_string);
    }
    ;

expression returns [std::string expr_string, int line_number, std::string expr_type, bool is_integer]
    : le=logic_expression {
        $expr_string = $le.expr_string;
        $line_number = $le.line_number;
        $expr_type = $le.expr_type;
        $is_integer = $le.is_integer;
        logLine($line_number, "expression : logic expression");
        writeIntoparserLogFile($expr_string);
    }
    | v=variable ASSIGNOP le=logic_expression {
        $expr_string = $v.var_string + "=" + $le.expr_string;
        $line_number = $v.line_number;
        // writeIntoErrorFile("var_type: "+ $v.var_type + "expression_type: " + $le.expr_type);
        
        // Check if trying to assign to array name (without index)
        if ($v.var_type.find("ARRAY,") == 0 && $v.var_string.find("[") == std::string::npos) {
            writeIntoErrorFile("Line " + std::to_string($line_number) + ": Type mismatch, " + $v.var_string + " is an array");
            syntaxErrorCount++;
        } else {
            // Get the actual assignable type
            std::string assignableType = getBaseType($v.var_type);
            if (assignableType!="" &&  !isTypeCompatible(assignableType, $le.expr_type)) {
                writeIntoErrorFile("Line " + std::to_string($line_number) + ": Type Mismatch");
                syntaxErrorCount++;
            }
        }
        
        $expr_type = getBaseType($v.var_type);
        $is_integer = isIntegerType($expr_type);
        logLine($line_number, "expression : variable ASSIGNOP logic expression");
        writeIntoparserLogFile($expr_string);
    }
    ;

logic_expression returns [std::string expr_string, int line_number, std::string expr_type, bool is_integer]
    : re=rel_expression {
        $expr_string = $re.expr_string;
        $line_number = $re.line_number;
        $expr_type = $re.expr_type;
        $is_integer = $re.is_integer;
        logLine($line_number, "logic expression : rel_expression");
        writeIntoparserLogFile($expr_string);
    }
    | re1=rel_expression LOGICOP re2=rel_expression {
        $expr_string = $re1.expr_string + " " + $LOGICOP->getText() + " " + $re2.expr_string;
        $line_number = $re1.line_number;
        $expr_type = "int"; // LOGICOP results in int
        $is_integer = true;
        logLine($line_number, "logic expression : rel_expression LOGICOP rel_expression");
        writeIntoparserLogFile($expr_string);
    }
    ;

rel_expression returns [std::string expr_string, int line_number, std::string expr_type, bool is_integer]
    : se=simple_expression {
        $expr_string = $se.expr_string;
        $line_number = $se.line_number;
        $expr_type = $se.expr_type;
        $is_integer = $se.is_integral;
        logLine($line_number, "rel_expression : simple_expression");
        writeIntoparserLogFile($expr_string);
    }
    | se1=simple_expression RELOP se2=simple_expression {
        $expr_string = $se1.expr_string + $RELOP->getText() + $se2.expr_string;
        $line_number = $se1.line_number;
        $expr_type = "int"; // RELOP results in int
        $is_integer = true;
        logLine($line_number, "rel_expression : simple_expression RELOP simple_expression");
        writeIntoparserLogFile($expr_string);
    }
    ;

simple_expression returns [std::string expr_string, int line_number, std::string expr_type, bool is_integral]
    : t=term {
        $expr_string = $t.expr_string;
        $line_number = $t.line_number;
        $expr_type = $t.t_type;
        $is_integral = $t.is_integer;
        logLine($line_number, "simple_expression : term");
        writeIntoparserLogFile($expr_string);
    }
    | se=simple_expression ADDOP t=term {
        $expr_string = $se.expr_string + $ADDOP->getText() + $t.expr_string;
        $line_number = $se.line_number;
        if ($se.expr_type == "float" || $t.t_type == "float") {
            $expr_type = "float";
            $is_integral = false;
        } else {
            $expr_type = "int";
            $is_integral = true;
        }
        logLine($line_number, "simple_expression : simple_expression ADDOP term");
        writeIntoparserLogFile($expr_string);
    }
    ;

term returns [std::string expr_string, int line_number, std::string t_type, bool is_integer]
    : ue=unary_expression {
        $expr_string = $ue.expr_string;
        $line_number = $ue.line_number;
        $t_type = $ue.expr_type;
        $is_integer = $ue.is_integer;
        logLine($line_number, "term : unary_expression");
        writeIntoparserLogFile($expr_string);
    }
    | t=term MULOP ue=unary_expression {
        $expr_string = $t.expr_string +  $MULOP->getText() + $ue.expr_string;
        $line_number = $t.line_number;
        if ($MULOP->getText() == "%") {
            if (!$t.is_integer || !$ue.is_integer) {
                writeIntoErrorFile("Line " + std::to_string($line_number) + ": Non-Integer operand on modulus operator");
                syntaxErrorCount++;
            }
            $t_type = "int";
            $is_integer = true;
        } else {
            if ($t.t_type == "float" || $ue.expr_type == "float") {
                $t_type = "float";
                $is_integer = false;
            } else {
                $t_type = "int";
                $is_integer = true;
            }
        }
        logLine($line_number, "term : term MULOP unary_expression");
        writeIntoparserLogFile($expr_string);
    }
    ;

unary_expression returns [std::string expr_string, int line_number, std::string expr_type, bool is_integer]
    : ADDOP ue=unary_expression {
        $expr_string = $ADDOP->getText() + $ue.expr_string;
        $line_number = $ADDOP->getLine();
        $expr_type = $ue.expr_type;
        $is_integer = $ue.is_integer;
        logLine($line_number, "unary_expression : ADDOP unary_expression");
        writeIntoparserLogFile($expr_string);
    }
    | NOT ue=unary_expression {
        $expr_string = $NOT->getText() + $ue.expr_string;
        $line_number = $NOT->getLine();
        $expr_type = "int";
        $is_integer = true;
        logLine($line_number, "unary_expression : NOT unary_expression");
        writeIntoparserLogFile($expr_string);
    }
    | f=factor {
        $expr_string = $f.expr_string;
        $line_number = $f.line_number;
        $expr_type = $f.f_type;
        $is_integer = $f.is_integer;
        logLine($line_number, "unary_expression : factor");
        writeIntoparserLogFile($expr_string);
    }
    ;
factor returns [std::string expr_string, int line_number, std::string f_type, bool is_integer]
    : v=variable {
        $expr_string = $v.var_string;
        $line_number = $v.line_number;
        $f_type = $v.var_type;
        $is_integer = isIntegerType($v.var_type);
        logLine($line_number, "factor : variable");
        writeIntoparserLogFile($expr_string);
    }
     | ID LPAREN al=argument_list RPAREN {
        $expr_string = $ID->getText() + "(" + $al.args_string + ")";
        $line_number = $ID->getLine();
        SymbolInfo* sym = ST->LookUp($ID->getText());
        if (!sym) {
            writeIntoErrorFile("Line " + std::to_string($line_number) + ": Undeclared function " + $ID->getText());
            syntaxErrorCount++;
            $f_type = "";
        } else if (sym->isFunctionSymbol() == false) {
            writeIntoErrorFile("Line " + std::to_string($line_number) + ": " + $ID->getText() + " is not a function");
            syntaxErrorCount++;
            $f_type = "";
        } 
        else {
            // Extract return type and parameters
            std::string funcType = sym->getOriginalType();
            size_t comma = funcType.find(",");
            size_t arrow = funcType.find("<==");
            if (comma != std::string::npos && arrow != std::string::npos) {
                $f_type = funcType.substr(comma + 1, arrow - comma - 1);
                if ($f_type == "void") {
                    writeIntoErrorFile("Line " + std::to_string($line_number) + ": Void function " + $ID->getText() + " cannot be called as part of an expression");
                    syntaxErrorCount++;
                }
                // // Use SymbolInfo's paramTypes for declared types
                std::vector<std::string> declaredTypes = sym->getParamTypes();
                // // Use the arg_types from argument_list instead of extractParamTypes
                std::vector<std::string> argTypes = $al.arg_types;


                // writeIntoErrorFile("Line " + std::to_string($line_number) + ": Function " + $ID->getText() + " called with parameters: " + $al.args_string);

                if (declaredTypes.size() != argTypes.size()) {
                    writeIntoErrorFile("Line " + std::to_string($line_number) + ": Parameter count mismatch in call to " + $ID->getText());
                    syntaxErrorCount++;
                } else {
                    for (size_t i = 0; i < declaredTypes.size(); ++i) {

                        // writeIntoErrorFile("Line " + std::to_string($line_number) + ": Argument " + std::to_string(i+1) + " type: " + argTypes[i] + ", expected: " + declaredTypes[i]);

                        if (!isTypeCompatible(declaredTypes[i], argTypes[i])) {
                            writeIntoErrorFile("Line " + std::to_string($line_number) + ": Type mismatch in argument " + std::to_string(i+1) + " of call to " + $ID->getText()
                                + " (expected: " + declaredTypes[i] + ", got: " + argTypes[i] + ")");
                            syntaxErrorCount++;
                            break;
                        }
                    }
                }
            } else {
                $f_type = "";
            }
        }
        $is_integer = isIntegerType($f_type);
        logLine($line_number, "factor : ID LPAREN argument_list RPAREN");
        writeIntoparserLogFile($expr_string);
    }
    | LPAREN e=expression RPAREN {
        $expr_string = "(" + $e.expr_string + ")";
        $line_number = $LPAREN->getLine();
        $f_type = $e.expr_type;
        $is_integer = $e.is_integer;
        logLine($line_number, "factor : LPAREN expression RPAREN");
        writeIntoparserLogFile($expr_string);
    }
    | CONST_INT {
        $expr_string = $CONST_INT->getText();
        $line_number = $CONST_INT->getLine();
        $f_type = "int";
        $is_integer = true;
        logLine($line_number, "factor : CONST_INT");
        writeIntoparserLogFile($expr_string);
    }
    | CONST_FLOAT {
        $expr_string = $CONST_FLOAT->getText();
        $line_number = $CONST_FLOAT->getLine();
        $f_type = "float";
        $is_integer = false;
        logLine($line_number, "factor : CONST_FLOAT");
        writeIntoparserLogFile($expr_string);
    }
    | v=variable INCOP {
        $expr_string = $v.var_string + $INCOP->getText();
        $line_number = $v.line_number;
        $f_type = $v.var_type;
        $is_integer = isIntegerType($v.var_type);
        logLine($line_number, "factor : variable INCOP");
        writeIntoparserLogFile($expr_string);
    }
    | v=variable DECOP {
        $expr_string = $v.var_string + $DECOP->getText();
        $line_number = $v.line_number;
        $f_type = $v.var_type;
        $is_integer = isIntegerType($v.var_type);
        logLine($line_number, "factor : variable DECOP");
        writeIntoparserLogFile($expr_string);
    }
    ;


// Modified argument_list rule
argument_list returns [std::string args_string, std::vector<std::string> arg_types]
    : a=arguments {
        $args_string = $a.args_string;
        $arg_types = $a.arg_types;
        logLine($a.line_number, "argument_list : arguments");
        writeIntoparserLogFile($args_string);
    }
    | {
        $args_string = "";
        $arg_types.clear();
        logLine(-1, "argument_list :");
        writeIntoparserLogFile($args_string);
    }
    ;

// Modified arguments rule
arguments returns [std::string args_string, int line_number, std::vector<std::string> arg_types]
    : a=arguments COMMA le=logic_expression {
        $args_string = $a.args_string + "," + $le.expr_string;
        $line_number = $a.line_number;
        $arg_types = $a.arg_types;
        $arg_types.push_back($le.expr_type);
        logLine($line_number, "arguments : arguments COMMA logic expression");
        writeIntoparserLogFile($args_string);
    }
    | le=logic_expression {
        $args_string = $le.expr_string;
        $line_number = $le.line_number;
        $arg_types.clear();
        $arg_types.push_back($le.expr_type);
        logLine($line_number, "arguments : logic expression");
        writeIntoparserLogFile($args_string);
    }
    ;