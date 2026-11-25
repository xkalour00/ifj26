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
void generate_print_def(){
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
void generate_read_line_def(){
    output("LABEL func_readLine");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@result_string");
    output("READ LF@result_string string");
    output("JUMPIFEQ print_EOF LF@result_string nil@nil");

    output("PUSHS LF@result_string");
    output("POPFRAME");
    output("RETURN");

    output("LABEL print_EOF");
    output("PUSHS string@");
    output("POPFRAME");
    output("RETURN");
    output("");
    // TODO zistit ci je vphoode vraciat nil alebo je potreba generovat prazdny str
}

/**
 * @brief generates the definition of builtin readInt function
 */
void generate_read_int_def(){
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
void generate_read_double_def(){
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
 * @brief generates the definition of builtin readBoolean function
 */
void generate_read_boolean_def(){
    output("LABEL func_readBoolean");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@result_boolean");
    output("READ LF@result_boolean bool");
    output("JUMPIFEQ readBoolean_result_nil LF@result_boolean nil@nil");

    output("PUSHS LF@result_boolean");
    output("POPFRAME");
    output("RETURN");

    output("LABEL readBoolean_result_nil");
    output("PUSHS float@0x0p+0");
    output("POPFRAME");
    output("RETURN");
    output("");
}


/**
 * @brief generates the definition of builtin toStr function
 */
void generate_to_str_def(){
    output("LABEL func_toStr");
    output("CREATEFRAME");
    output("PUSHFRAME");

    output("DEFVAR LF@value");
    output("DEFVAR LF@type_value");
    output("POPS LF@value");
    output("TYPE LF@type_value LF@value");

    output("JUMPIFEQ toStr_value_int LF@type_value string@int");
    output("JUMPIFEQ toStr_value_double LF@type_value string@float");
    output("JUMPIFEQ toStr_value_nil LF@type_value string@nil");
    output("JUMPIFEQ toStr_value_bool LF@type_value string@bool");
    output("JUMP toStr_end");

    output("LABEL toStr_value_int");
    output("INT2STR LF@value LF@value");
    output("JUMP toStr_end");

    output("LABEL toStr_value_double");
    output("FLOAT2STR LF@value LF@value");
    output("JUMP toStr_end");

    output("LABEL toStr_value_nil");
    output("MOVE LF@value string@");
    output("JUMP toStr_end");

    output("LABEL toStr_value_bool");
    output("JUMPIFEQ toStr_is_true LF@value bool@true");
    output("MOVE LF@value string@false");
    output("JUMP toStr_end");
    output("LABEL toStr_is_true");
    output("MOVE LF@value string@true");

    output("LABEL toStr_end");
    output("PUSHS LF@value");
    output("POPFRAME");
    output("RETURN");
    output("");
}

/**
 * @brief generates the definition of builtin toInt function
 */
void generate_to_int_def(){
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
void generate_str_length_def(){
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
    // - 1?
}

/**
 * @brief generates the definition of builtin subStr function
 */
void generate_sub_str_def(){
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
void generate_ord_def(){
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
    output("JUMPIFEQ ord_error LF@value string@");

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
void generate_chr_def(){
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