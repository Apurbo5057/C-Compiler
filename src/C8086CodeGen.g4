parser grammar C8086CodeGen;

options {
    tokenVocab = C8086Lexer;
}

@parser::header {
    #include <iostream>
    #include <fstream>
    #include <string>
    #include <cstdlib>
    #include "C8086Lexer.h"
	#include "CodeGenerator.hpp"

    extern std::ofstream parserLogFile;
    extern std::ofstream errorFile;
	extern std::ofstream dataSectionFile;
    extern std::ofstream codeSectionFile; 

}

@parser::members {
    void writeIntoparserLogFile(const std::string message) {
        if (!parserLogFile) {
            std::cout << "Error opening parserLogFile.txt" << std::endl;
            return;
        }

        parserLogFile << message << std::endl;
        parserLogFile.flush();
    }

    void writeIntoErrorFile(const std::string message) {
        if (!errorFile) {
            std::cout << "Error opening errorFile.txt" << std::endl;
            return;
        }
        errorFile << message << std::endl;
        errorFile.flush();
    }
}


start : {

	writeIntoparserLogFile("Starting parsing process...");

	// Generate the program header and data and code section headers
	if (codeGen) {
		codeGen->GenProgramHeader();
		codeGen->GenDataSectionHeader();
		codeGen->GenCodeSectionHeader();
	}
	
	
}program
	{
		// Generate the program footer
		if(codeGen) codeGen->GenProgramFooter();
        writeIntoparserLogFile("Parsing completed successfully ");
	}
	;

program : program unit 
	| unit
	;
	
unit : var_declaration
     | {
		if(codeGen)codeGen->setIsGlobalScope(false);
	 }func_declaration{
		if(codeGen)codeGen->setIsGlobalScope(true);
	 }
     | {
		if(codeGen)codeGen->setIsGlobalScope(false);
	 }func_definition{
		if(codeGen)codeGen->setIsGlobalScope(true);
	 }
     ;
     
func_declaration : type_specifier ID LPAREN parameter_list RPAREN SEMICOLON
		| type_specifier ID LPAREN RPAREN SEMICOLON
		;
		 
func_definition : type_specifier ID {
        if(codeGen) codeGen->GenFunctionHeader($ID.text, $type_specifier.text, $ID->getLine());
    } LPAREN {
        if(codeGen) codeGen->EnterNewScope();
    } parameter_list RPAREN {
        // After parsing all parameters, assign their correct offsets
        if(codeGen) codeGen->GenFunctionParametersComplete($RPAREN->getLine());
    } compound_statement {
        if(codeGen) codeGen->GenFunctionFooter($ID.text, $type_specifier.text, $ID->getLine());
        if(codeGen) codeGen->ExitCurrentScope();
    }
    | type_specifier ID {
        if(codeGen) codeGen->GenFunctionHeader($ID.text, $type_specifier.text, $ID->getLine());
    } LPAREN RPAREN {
        // No parameters to process
    } compound_statement {
        if(codeGen) codeGen->GenFunctionFooter($ID.text, $type_specifier.text, $ID->getLine());
    }
    ;

parameter_list  : parameter_list COMMA type_specifier ID {
            if(codeGen) {
                codeGen->GenFunctionParameter($ID.text, $type_specifier.text, $ID->getLine());
            }
        }
        | parameter_list COMMA type_specifier
        | type_specifier ID{
            if(codeGen) {
                codeGen->GenFunctionParameter($ID.text, $type_specifier.text, $ID->getLine());
            }
        }
        | type_specifier
        ;
 		
compound_statement : LCURL {
				if(codeGen) codeGen->EnterNewScope();
			} statements RCURL{
				if(codeGen) {
					codeGen->ExitCurrentScope();
				}
			}
 		    | LCURL RCURL
 		    ;
 		    
var_declaration 
    : t=type_specifier dl=declaration_list[$t.text] sm=SEMICOLON {
        
    }

    | t=type_specifier de=declaration_list_err sm=SEMICOLON {
        writeIntoErrorFile(
            std::string("Line# ") + std::to_string($sm->getLine()) +
            " with error name: " + $de.error_name +
            " - Syntax error at declaration list of variable declaration"
        );

      }
    ;

declaration_list_err returns [std::string error_name]: {
        $error_name = "Error in declaration list";
    };

 		 
type_specifier returns [std::string text]	
        : INT {
            $text = "int";
        }
 		| FLOAT {
            $text = "float";
        }
 		| VOID {
            $text = "void";
        }
 		;
 		
declaration_list [std::string type]
    : ID {
        if(codeGen ){ 
			if(codeGen->isInGlobalScope() ){
				codeGen->GenGlobalVariableDeclaration($ID.text, $type, $ID->getLine());
			}
			else{
				codeGen->GenLocalVariableDeclaration($ID.text, $type, $ID->getLine());
			}
		}
		
      } declaration_list_rest[$type]
    | ID LTHIRD CONST_INT RTHIRD {
		if(codeGen) {
			if(codeGen->isInGlobalScope()) {
				codeGen->GenGlobalArrayDeclaration($ID.text, $type, std::stoi($CONST_INT.text), $ID->getLine());
			} else {
				codeGen->GenLocalArrayDeclaration($ID.text, $type, std::stoi($CONST_INT.text), $ID->getLine());
			}
		}
	} declaration_list_rest[$type]
    ;

declaration_list_rest [std::string type]
    : COMMA ID {
        if(codeGen ){ 
			if(codeGen->isInGlobalScope() ){
				codeGen->GenGlobalVariableDeclaration($ID.text, $type, $ID->getLine());
			}
			else{
				codeGen->GenLocalVariableDeclaration($ID.text, $type, $ID->getLine());
			}
		}
      } declaration_list_rest[$type]
    | COMMA ID LTHIRD CONST_INT RTHIRD {
		if(codeGen) {
			if(codeGen->isInGlobalScope()) {
				codeGen->GenGlobalArrayDeclaration($ID.text, $type, std::stoi($CONST_INT.text), $ID->getLine());
			} else {
				codeGen->GenLocalArrayDeclaration($ID.text, $type, std::stoi($CONST_INT.text), $ID->getLine());
			}
		}
	} declaration_list_rest[$type]
    | // empty - end of list
    ;
 		  
statements : statement
	   | statements statement
	   ;
	   
statement : var_declaration
	  | expression_statement
	  | compound_statement
	  | FOR LPAREN expression_statement {
			std::string startLabel = codeGen->getNextLabel();
			codeGen->writeToCodeSection(startLabel + ":", $FOR->getLine());
	  }expression SEMICOLON{
			codeGen->writeToCodeSection( "\tPOP AX\n", $SEMICOLON->getLine());
			codeGen->writeToCodeSection("\tCMP AX, 0\n", $SEMICOLON->getLine());
			std::string endLabel = codeGen->getNextLabel();
			codeGen->writeToCodeSection("\tJE " + endLabel + "\n", $SEMICOLON->getLine());
			std::string continueLabel = codeGen->getNextLabel();
			codeGen->writeToCodeSection("\tJMP " + continueLabel + "\n", $SEMICOLON->getLine());
			std::string loopLabel = codeGen->getNextLabel();
			codeGen->writeToCodeSection(loopLabel + ":", $SEMICOLON->getLine());
	  } expression RPAREN{
			codeGen->writeToCodeSection("\tPOP AX\n", $RPAREN->getLine());
			codeGen->writeToCodeSection("\tJMP " + startLabel + "\n", $RPAREN->getLine());
			codeGen->writeToCodeSection(continueLabel + ":", $RPAREN->getLine());
	  }statement{
			codeGen->writeToCodeSection("\tJMP " + loopLabel + "\n", $SEMICOLON->getLine());
			codeGen->writeToCodeSection(endLabel + ":", $SEMICOLON->getLine());
	  }
	  | IF LPAREN expression RPAREN {
	  		std::string endLabel;
	  		if(codeGen) {
	  			endLabel = codeGen->getNextLabel();
	  			codeGen->GenIfStart($IF->getLine());
	  			codeGen->GenIfElse(endLabel, $IF->getLine());
	  		}
	  	} statement {
	  		if(codeGen) {
	  			codeGen->GenLabel(endLabel, $IF->getLine());
	  		}
	  	}
	  | IF LPAREN expression RPAREN {
	  		std::string elseLabel, endLabel;
	  		if(codeGen) {
	  			elseLabel = codeGen->getNextLabel();
	  			endLabel = codeGen->getNextLabel();
	  			codeGen->GenIfStart($IF->getLine());
	  			codeGen->GenIfElse(elseLabel, $IF->getLine());
	  		}
	  	} statement ELSE {
	  		if(codeGen) {
	  			codeGen->GenIfEnd(endLabel, $ELSE->getLine());
	  			codeGen->GenLabel(elseLabel, $ELSE->getLine());
	  		}
	  	} statement {
	  		if(codeGen) {
	  			codeGen->GenLabel(endLabel, $ELSE->getLine());
	  		}
	  	}
	  | WHILE {
	  		std::string startLabel, endLabel;
	  		if(codeGen) {
	  			startLabel = codeGen->getNextLabel();
	  			endLabel = codeGen->getNextLabel();
	  			codeGen->GenWhileStart(startLabel, $WHILE->getLine());
	  		}
	  	} LPAREN expression RPAREN {
	  		if(codeGen) {
	  			codeGen->GenWhileCondition(endLabel, $RPAREN->getLine());
	  		}
	  	} statement {
	  		if(codeGen) {
	  			codeGen->GenWhileEnd(startLabel, endLabel, $WHILE->getLine());
	  		}
	  	}
	  | PRINTLN LPAREN ID RPAREN SEMICOLON {
			if(codeGen) codeGen->callPrintlnFunction($ID.text);
	  }
	  | RETURN expression SEMICOLON {
		if(codeGen) codeGen->GenReturnStatement(true, $RETURN->getLine());
		}
		| RETURN SEMICOLON {
			if(codeGen) codeGen->GenReturnStatement(false, $RETURN->getLine());
		}
	;


	  
expression_statement : SEMICOLON			
    | expression SEMICOLON {
        if(codeGen) {
            // Pop AX to free stack space used by expression evaluation
            codeGen->GenPopFromStack("AX", $SEMICOLON->getLine());
        }
    }
    ;
	  
variable returns [std::string var_name, int line, bool is_array] 
	 : ID {
		$var_name = $ID.text;
		$line = $ID->getLine();
		$is_array = false;
	 }
	 | ID LTHIRD expression RTHIRD {
		$var_name = $ID.text;
		$line = $ID->getLine();
		$is_array = true;
		// The expression (array index) result is already on the stack from expression parsing
	 }
	 ;
	 
expression : logic_expression	
	   | v=variable ASSIGNOP logic_expression {
		   if(codeGen) {
			   // Pop the value to assign from the stack
			   codeGen->GenPopFromStack("AX", $ASSIGNOP->getLine());
			   
			   if ($v.is_array) {
				   // For array assignment, we need to handle the index properly
				   // The array index is already on the stack from the variable parsing
				   codeGen->GenAssignmentOp($v.var_name, "AX", $ASSIGNOP->getLine());
			   } else {
				   // Regular variable assignment
				   codeGen->GenAssignmentOp($v.var_name, "AX", $ASSIGNOP->getLine());
			   }
			   
			   // Push the assigned value back to the stack for potential chained assignments
			   codeGen->GenPushToStack("AX", $ASSIGNOP->getLine());
		   }
	   }	
	   ;
			
logic_expression : rel_expression 	
		 | rel_expression LOGICOP rel_expression {
			if(codeGen) {
				codeGen->GenLogicalOp($LOGICOP.text, $LOGICOP->getLine());
				codeGen->GenPushToStack("AX", $LOGICOP->getLine());
			}
		 }
		 ;
			
rel_expression	: simple_expression 
		| simple_expression RELOP simple_expression {
			if(codeGen) {
				codeGen->GenRelationalOp($RELOP.text, $RELOP->getLine());
				codeGen->GenPushToStack("AX", $RELOP->getLine());
			}
		}
		;

	
simple_expression : term 
		  | simple_expression ADDOP term {
			if(codeGen) {
				// First of all pop two arguments from stack and perform addtion
				codeGen->GenPopFromStack("BX", $ADDOP->getLine());
				codeGen->GenPopFromStack("AX", $ADDOP->getLine());
				if ($ADDOP.text == "+") {
					codeGen->GenAdditionOp("AX", "BX", $ADDOP->getLine());
				} else if ($ADDOP.text == "-") {
					codeGen->GenSubtractionOp("AX", "BX", $ADDOP->getLine());
				}

				// Then push back the result to stack
				codeGen->GenPushToStack("AX", $ADDOP->getLine());
			}
		  }
		  ;
					
term :	unary_expression
     |  term MULOP unary_expression {

			if(codeGen){
				// First of all pop two arguments from stack and perform multiplication
				codeGen->GenPopFromStack("BX", $MULOP->getLine());
				codeGen->GenPopFromStack("AX", $MULOP->getLine());
				if ($MULOP.text == "*") {
					codeGen->GenMultiplicationOp("BX", $MULOP->getLine());
				} else if ($MULOP.text == "/") {
					codeGen->GenDivisionOp("BX", $MULOP->getLine());
				} else if ($MULOP.text == "%") {
					codeGen->GenModulusOp("BX", $MULOP->getLine());
				}

				// Then push back the result to stack
				codeGen->GenPushToStack("AX", $MULOP->getLine());
			}
	 }
     ;

unary_expression : ADDOP unary_expression {
		if(codeGen && $ADDOP.text == "-") {
			codeGen->GenPopFromStack("AX", $ADDOP->getLine());
			codeGen->GenUnaryMinus($ADDOP->getLine());
			codeGen->GenPushToStack("AX", $ADDOP->getLine());
		}
		// For positive unary, no operation needed
	}
	| NOT unary_expression {
		if(codeGen) {
			codeGen->GenPopFromStack("AX", $NOT->getLine());
			codeGen->GenLogicalNot($NOT->getLine());
			codeGen->GenPushToStack("AX", $NOT->getLine());
		}
	}
	| factor 
	;




factor	: 
	variable {
		if(codeGen) {
			if ($variable.is_array) {
				// For array access, the index is on the stack
				codeGen->GenLoadIdentifier($variable.var_name, $variable.line);
			} else {
				// For regular variable
				codeGen->GenLoadVariable($variable.var_name, "AX", $variable.line);
			}
			codeGen->GenPushToStack("AX", $variable.line);
		}
	}
	| ID LPAREN argument_list RPAREN {
		if(codeGen) {
			codeGen->GenFunctionCall($ID.text, $ID->getLine());
		}
	}
	| LPAREN expression RPAREN
	| CONST_INT {
		if(codeGen){
			codeGen->GenLoadConstInt(std::stoi($CONST_INT.text), $CONST_INT->getLine());
			codeGen->GenPushToStack("AX", $CONST_INT->getLine());
		}
	}
	| CONST_FLOAT {
		if(codeGen) {
			codeGen->GenLoadConstFloat(std::stoi($CONST_FLOAT.text), $CONST_FLOAT->getLine());
			codeGen->GenPushToStack("AX", $CONST_FLOAT->getLine());
		}
	}
	| variable INCOP {
		if(codeGen) {
			if ($variable.is_array) {
				// For array post-increment, the index is already handled in variable parsing
				codeGen->GenPostIncrement($variable.var_name, $INCOP->getLine());
			} else {
				// Regular variable post-increment
				codeGen->GenPostIncrement($variable.var_name, $INCOP->getLine());
			}
			codeGen->GenPushToStack("AX", $INCOP->getLine());
		}
	}
	| variable DECOP {
		if(codeGen) {
			if ($variable.is_array) {
				// For array post-decrement
				codeGen->GenPostDecrement($variable.var_name, $DECOP->getLine());
			} else {
				// Regular variable post-decrement
				codeGen->GenPostDecrement($variable.var_name, $DECOP->getLine());
			}
			codeGen->GenPushToStack("AX", $DECOP->getLine());
		}
	}
	;

	
argument_list : arguments
              |
              ;
	
arguments : arguments COMMA logic_expression {
        // Arguments are parsed left to right and left on stack
        // Don't call GenPushArgument here - just leave them on stack
    }
    | logic_expression {
        // First argument parsed - just leave it on stack
    }
    ;










