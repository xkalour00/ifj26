/**
 * @file codegen_builtin.c
 *
 * @author Dávid Čech-Gut
 * @brief FIT VUT | IFJ project 2026 - implementation of builtin functions for IFJ26
 * @date 2026-11-18
 */

#include "codegen_builtin.h"
#include "codegen.h"

/**
 * @brief generates the definition of builtin print function
 */
void generatePrintDef(){
    output("LABEL func_print");
    output("CREATEFRAME");
    output("PUSHFRAME");
    
    output("DEFVAR LF@value");
    output("POPS LF@value");
    output("DEFVAR LF@type");
    output("DEFVAR LF@nil_type");
    output("TYPE LF@nil_type nil@nil");

    output("TYPE LF@type LF@value");
    output("JUMPIFEQ print_unit LF@type LF@nil_type");
    output("WRITE LF@value");
    output("JUMP print_end");

    output("LABEL print_unit");
    output("WRITE string@()");
    
    output("LABEL print_end");
    output("PUSHS nil@nil");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin readLine function
 */
void generateReadLineDef(){
    output("LABEL func_readLine");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@result_string");
    output("READ LF@result_string string");

    output("PUSHS LF@result_string");
    output("POPFRAME");
    output("RETURN");
    output("");
    // TODO zistit ci je vphoode vraciat nil alebo je potreba generovat prazdny str
}

/**
 * @brief generates the definition of builtin readInt function
 */
void generateReadIntDef(){
    output("LABEL func_readInt");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@result_int");
    output("READ LF@result_int int");
    output("JUMPIFEQ readInt_result_nil LF@result_int nil@nil");

    output("PUSHS LF@result_int");
    output("POPFRAME");
    output("RETURN");

    output("LABEL readInt_result_nil");
    output("PUSHS int@0");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin readDouble function
 */
void generateReadDoubleDef(){
    output("LABEL func_readDouble");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@result_double");
    output("READ LF@result_double float");
    output("JUMPIFEQ readDouble_result_nil LF@result_double nil@nil");

    output("PUSHS LF@result_double");
    output("POPFRAME");
    output("RETURN");

    output("LABEL readDouble_result_nil");
    output("PUSHS float@0x0p+0");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin toStr function
 */
void generateToStrDef(){
    output("LABEL func_toStr");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("DEFVAR LF@type_value");
    output("POPS LF@value");
    output("TYPE LF@type_value LF@value");

    output("JUMPIFEQ toStr_value_int LF@type_value string@int");
    output("JUMPIFEQ toStr_value_double LF@type_value string@float");
    output("JUMP toStr_end");
    output("LABEL toStr_value_int");
    output("INT2STR LF@value LF@value");
    output("JUMP toStr_end");
    output("LABEL toStr_value_double");
    output("FLOAT2STR LF@value LF@value");

    output("LABEL toStr_end");
    output("PUSHS LF@value");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin toInt function
 */
void generateToIntDef(){
    output("LABEL func_toInt");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("POPS LF@value");
    output("FLOAT2INT LF@value LF@value");

    output("PUSHS LF@value");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin strLength function
 */
void generateStrLengthDef(){
    output("LABEL func_strLength");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("POPS LF@value");
    output("STRLEN LF@value LF@value");

    output("PUSHS LF@value");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin subStr function
 */
void generateSubStrDef(){
    output("LABEL func_subStr");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("DEFVAR LF@start");
    output("DEFVAR LF@end");

    output("POPS LF@value");
    output("POPS LF@start");
    output("POPS LF@end");

    output("DEFVAR LF@char");
    output("DEFVAR LF@result");
    output("MOVE LF@result string@");

    output("DEFVAR LF@error");

    output("LT LF@error LF@start int@0");
    output("JUMPIFEQ subStr_error LF@error bool@true");
    output("LT LF@error LF@end int@0");
    output("JUMPIFEQ subStr_error LF@error bool@true");
    output("LT LF@error LF@end LF@start");
    output("JUMPIFEQ subStr_error LF@error bool@true");
    output("DEFVAR LF@strlen");
    output("STRLEN LF@strlen LF@value");
    output("LT LF@error LF@start LF@strlen");
    output("JUMPIFEQ subStr_error LF@error bool@false");
    output("GT LF@error LF@end LF@strlen");
    output("JUMPIFEQ subStr_error LF@error bool@true");

    output("LABEL subStr_loop");
    output("JUMPIFEQ subStr_end LF@start LF@end");
    output("GETCHAR LF@char LF@value LF@start");
    output("CONCAT LF@result LF@result LF@char");
    output("ADD LF@start LF@start int@1");
    output("JUMP subStr_loop");
    output("LABEL subStr_end");
    output("PUSHS LF@result");
    output("POPFRAME");
    output("RETURN");

    output("LABEL subStr_error");
    output("POPFRAME");
    output("EXIT int@40");
    output("");
}

/**
 * @brief generates the definition of builtin ord function
 */
void generateOrdDef(){
    output("LABEL func_ord");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("DEFVAR LF@index");
    output("POPS LF@value");
    output("POPS LF@index");

    output("DEFVAR LF@error");
    output("LT LF@error LF@index int@0");
    output("JUMPIFEQ ord_error LF@error bool@true");
    output("DEFVAR LF@strlen");
    output("STRLEN LF@strlen LF@value");
    output("LT LF@error LF@index LF@strlen");
    output("JUMPIFEQ ord_error LF@error bool@false");

    output("DEFVAR LF@ascii");
    output("STRI2INT LF@ascii LF@value LF@index");
    output("PUSHS LF@ascii");
    output("POPFRAME");
    output("RETURN");

    output("LABEL ord_error");
    output("PUSHS int@-1");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin chr function
 */
void generateChrDef(){
    output("LABEL func_chr");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@code");
    output("POPS LF@code");
    output("DEFVAR LF@result");
    output("INT2CHAR LF@result LF@code");

    output("PUSHS LF@result");
    output("POPFRAME");
    output("RETURN");
    output("");
}