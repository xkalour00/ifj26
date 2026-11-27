/**
 * @file parser.c
 *
 * @brief FIT VUT | IFJ Project 2026 - parser implementation
 * @author Patrik Kapec (xkapecp00)
 * @date 14.11.2026
 */



#include "parser.h"
#include "precedence_parser.h"
#include "symtable.h"
#include "scanner.h"
#include "token.h"
#include "ast.h"
#include <stdio.h> //iba na dbug
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


token_t *current_token_parser = NULL;
token_type_t current_token_type_parser;
token_type_t prev_token_type_parser;
static ast_node_t *root;
symtable_t *symtable = NULL;





static void parse_prolog(void);
static ast_node_t* parse_function_list(ast_node_t *root);
static ast_node_t* parse_function(void);
static ast_node_t* parse_main(void);
static ast_node_t* parse_definition(bool is_main);
static ast_node_t* parse_params(void);
static ast_node_t* parse_params_c(ast_node_t *linked_list);
static ast_node_t* parse_param_existing(void);
static return_type_t parse_type(void);
static return_type_t parse_return_type(void);
static ast_node_t* parse_def_body(void);
static ast_node_t* parse_indented_block();
static ast_node_t* parse_block();
static ast_node_t* parse_rest_block(ast_node_t *block);
static ast_node_t* parse_line(void);
static ast_node_t* parse_var(void);
static ast_node_t* parse_val(void);
static ast_node_t* parse_return(void);
static ast_node_t* parse_return_value(void);

static ast_node_t* parse_while(void);
static ast_node_t* parse_id_expr_or_funcall(void);
static ast_node_t* parse_body(bool is_expr);
static ast_node_t* parse_else_part(bool is_expr,ast_node_t*elsifs);
static ast_node_t* parse_else_if_or_else(bool is_expr,ast_node_t* elsifs);
static ast_node_t* parse_optional_eol(void);
static ast_node_t* parse_else_body(bool is_expr);
static ast_node_t* parse_for(void);
static ast_node_t* parse_for_body(void);
static void parse_guards(ast_node_t** end, ast_node_t** by, bool *is_to);
static ast_node_t* parse_by_optional(void);
static ast_node_t* parse_indented_for(void);
static ast_node_t* parse_for_conditions(ast_node_t* guards);





void fail(int code){ 
  if (current_token_parser) token_free(current_token_parser);
  scanner_free();
  free_tree(root);

  symtable_free(symtable);
  exit(code);
}

char *str_dup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if(!copy) fail(MALLOC_FAIL);
    if (copy != NULL){
        memcpy(copy, s, len);
    }
    return copy;
}

void consume(void){
    prev_token_type_parser = current_token_type_parser; // si pamatam pre vyrazovy viacridakovy if (in_expr == true)
  if(current_token_parser){ 
    token_free(current_token_parser); 
    current_token_parser = NULL;
    }

  if(get_next_token(&current_token_parser) != 0){ 
    fail(LEXICAL_ERROR);
    }
  //fprintf(stderr, "token: %s\n", tok_name(current_token_type_parser));
  token_get_type(current_token_parser, &current_token_type_parser);
}   

void expect(token_type_t token) {
  if (current_token_type_parser != token) fail(SYNTAX_ERROR);
  consume();                 
}

/**
 * @brief Parses prolog: KW_IMPORT IDENTIFIER SP_DOT OP_MULTIPLY EOL <FUNCTION_LIST>
 */
static void parse_prolog(void){
    expect(KW_IMPORT);
    
    if(current_token_type_parser != IDENTIFIER) fail(SYNTAX_ERROR);
    const char* ifj26; 
    token_get_string(current_token_parser,&ifj26);

    if(strcmp(ifj26, "ifj26") != 0) fail(SYNTAX_ERROR);
    consume();
    expect(SP_DOT);
    expect(OP_MULTIPLY);
    expect(SP_EOL);
}


/**
 * @brief Parses function list: <FUNCTION_LIST> -> <FUNCTION><FUNCTION_LIST>
 *                              <FUNCTION_LIST> -> EOF
 * adds function definitions into linked list
 * @return root node
 * @param root root node
 */
static ast_node_t* parse_function_list(ast_node_t* root){

    switch (current_token_type_parser){
    case SP_EOF:
        consume();
        return root;
    
    case KW_DEF:
        parse_function();
        parse_function_list(root);
        return root;

    case SP_MAIN:
        parse_function();
        parse_function_list(root);
        return root;
       

    default:
        fail(SYNTAX_ERROR);
        return NULL;   
    }
}


/**
 * @brief Parses function : <FUNCTION> -> kw_def
 *                          <FUNCTION> -> @main <MAIN_EOL>    
 * @return returns func_def type node
 */
static ast_node_t* parse_function(void){
    switch(current_token_type_parser){
      case KW_DEF:
        return parse_definition(false);
    case SP_MAIN:
        consume();
        return parse_main();

    default:
        fail(SYNTAX_ERROR);
        return NULL;     
    }
}

/**
 * @brief parses <TYPE> -> KW_int
 *               <TYPE> -> KW_double
 *               <TYPE> -> KW_string
 *               <TYPE> -> KW_unit
 * @return RETURN_INT RETURN_DOUBLE RETURN_STRING or RETURN_UNIT or fail
 */

static return_type_t parse_type(void){
    
    switch(current_token_type_parser){
        case KW_INT:
            consume();
            return RETURN_INT;
        case KW_STRING:
            consume();  
            return RETURN_STRING;
        case KW_DOUBLE:
            consume();
            return RETURN_DOUBLE;
        case KW_UNIT:
            consume();
            return RETURN_UNIT;
        case KW_BOOLEAN:
            consume();
            return RETURN_BOOLEAN;
        default:
            fail(SYNTAX_ERROR);
            return RETURN_UNKNOWN;
    }
}
///// moze definition nemat specifikovany typ?


/**
 * @brief parses <PARAM_EXISTING> -> id : <TYPE>
 * @return AST_DECLARATION node with the parameter name and type and value set to NULL
 */

static ast_node_t* parse_param_existing(void){
    if(current_token_type_parser != IDENTIFIER) fail(SYNTAX_ERROR);
    const char* param_identifier; 
    token_get_string(current_token_parser,&param_identifier);
    char* name = str_dup(param_identifier);
    consume();
    expect(SP_COLON);
    return_type_t return_type = parse_type();
    ast_node_t *n = make_declaration_node(name,return_type,NULL);

    if(symtable_insert(symtable,n,false,true)!= 0) fail(REDEFINITION);
    return n;
}

/**
 * @brief parses <PARAMS_C> -> epsilon
 *               <PARAMS_C> -> , <PARAM_EXISTING> <PARAMS_C>
 * its an extra non terminal to check wheter atleast one paramater exists
 * @param linked_list parameter list to append to
 * @return the same list
 */


static ast_node_t* parse_params_c(ast_node_t *linked_list){
    switch (current_token_type_parser)
    {
    case SP_COMMA:
        consume();
        add_llist_item(linked_list, parse_param_existing());
        parse_params_c(linked_list);
        return linked_list;
    case SP_CLOSING_PARENTHESIS:
        return linked_list;
    default:
        fail(SYNTAX_ERROR);
        return NULL;
    }
}


/**
 * @brief parses <PARAMS> -> epsilon
 *               <PARAMS> -> <PARAM_EXISTING> <PARAMS_C>
 *        recursion for multiple params
 * @return linked list of AST_DECLARATION parameter nodes
 */


static ast_node_t* parse_params(void){
    ast_node_t* params = init_llist();
    switch (current_token_type_parser)
    {
    case SP_CLOSING_PARENTHESIS:
        return params;

    case IDENTIFIER:
        add_llist_item(params,parse_param_existing());
        parse_params_c(params);
        return params;

    default:
        fail(SYNTAX_ERROR);
        return NULL;
    }
}


/**
 * @brief parses <RETURN_TYPE> -> : <TYPE>
 *               <RETURN_TYPE> -> epsilon
 *      
 * @return type from <TYPE> or RETURN_NONE if no type was given
 */


static return_type_t parse_return_type(void){
    switch (current_token_type_parser)
    {
    case OP_ASSIGN:
        return RETURN_NONE;
        
    case SP_COLON:
        consume();
        return parse_type();   
    default:
        fail(SYNTAX_ERROR);
        return 99;
    }
}

/**
 * @brief Parses function_definition :  <definition> -> def id(<PARAMS>)<RETURN_TYPE> = <DEF_BODY>
 * @return node type of func_def
 */


static ast_node_t* parse_definition(bool is_main){
    expect(KW_DEF);
    if(current_token_type_parser != IDENTIFIER) fail(SYNTAX_ERROR);
    const char* func_identidier; 
    token_get_string(current_token_parser,&func_identidier);
    char* name = str_dup(func_identidier);
    consume();
    expect(SP_OPENING_PARENTHESIS);

    if(symtable_scope_push(symtable) != 0) fail(MALLOC_FAIL);

    ast_node_t * params = parse_params();
    expect(SP_CLOSING_PARENTHESIS);
    return_type_t return_type = parse_return_type();
    expect(OP_ASSIGN);
    ast_node_t *body = parse_def_body();

    symtable_scope_pop(symtable);

    if(is_main && root->data.root.main_func != NULL){  
        free(name);
        free_tree(params);
        free_tree(body);
        fail(REDEFINITION);
    }

    if(add_func_def_tree(root,return_type,name,params,body,is_main) != 0){
        free(name);
        free_tree(params);
        free_tree(body);
        fail(MALLOC_FAIL);
    }

    ast_node_t *n;
    if(is_main){
        n = root->data.root.main_func;
    }
    else{
        n = root->data.root.func_def_list->data.llist.end->data.llist_item.node;
    }
  
    if(symtable_insert(symtable,n,true,true) != 0) fail(REDEFINITION);
    return n;

}

/**
 * @brief Parses @main :  <MAIN_EOL> -> kw_def
                          <MAIN_EOL> -> EOL kw_def
 * @return node type of func_def
 */

static ast_node_t* parse_main(void){
    switch (current_token_type_parser)
    {
    case KW_DEF:
        return parse_definition(true);
    case SP_EOL:
        consume();
        return parse_definition(true);
    default:
        fail(SYNTAX_ERROR);
        return NULL;
    }
}

/**
 * @brief parses <DEF_BODY> -> <EXPR> EOL
 *               <DEF_BODY> -> EOL <INDENTED_BLOCK>
 *       
 * @return linked list of the function body lines
 */

static ast_node_t* parse_def_body(void){
    if(current_token_type_parser == SP_EOL){
        consume();
        return parse_indented_block();
    }
    if(current_token_type_parser == IDENTIFIER ||
        current_token_type_parser == SP_OPENING_PARENTHESIS ||
        current_token_type_parser == OP_MINUS ||
        current_token_type_parser == INTEGER ||
        current_token_type_parser == STRING ||
        current_token_type_parser == DOUBLE ||
        current_token_type_parser == OP_NOT ||
        current_token_type_parser == KW_TRUE ||
        current_token_type_parser == KW_FALSE ||
        current_token_type_parser == KW_IF){ 
        ast_node_t *body = init_llist();
        add_llist_item(body,make_node(AST_RETURN,parse_precedence(NULL)));
        if(prev_token_type_parser != SP_DEDENT) expect(SP_EOL);
        //expect(SP_EOL);
        return body;
    }
    fail(SYNTAX_ERROR);
    return NULL;
}

/**
 * @brief parses <INDENTED_BLOCK> -> INDENT <BLOCK> DEDENT
 * @return linked list of lines in the block
 */

static ast_node_t* parse_indented_block(){
    expect(SP_INDENT);
  
    if(symtable_scope_push(symtable) != 0) fail(MALLOC_FAIL);

    ast_node_t* block = parse_block();

   symtable_scope_pop(symtable);

    expect(SP_DEDENT);
    return block;
}


/**
 * @brief parses <BLOCK> -> <LINE> <REST_BLOCK>
 * nonterminal introduced for recursion
 * @return linked list of statements with at least one item
 */


static ast_node_t* parse_block(){
    ast_node_t* block = init_llist();
    add_llist_item(block,parse_line());
    parse_rest_block(block);
    return block;
}


/**
 * @brief parses <VAR> -> var id <RETURN_TYPE> = <EXPR> EOL 
 * @return AST_DECLARATION node with name type and value
 */


static ast_node_t* parse_var(){
    expect(KW_VAR);
    if (current_token_type_parser != IDENTIFIER) fail(SYNTAX_ERROR);
    const char *tmp;
    token_get_string(current_token_parser, &tmp);
    char *name = str_dup(tmp);
    consume();

    return_type_t type = parse_return_type();
    expect(OP_ASSIGN);
    ast_node_t *expr = parse_precedence(NULL);
    if(prev_token_type_parser != SP_DEDENT) expect(SP_EOL);
    //expect(SP_EOL);

    ast_node_t *n = make_declaration_node(name,type,expr);

    if(symtable_insert(symtable,n,false,false) != 0) fail(REDEFINITION);
    return n;
}

/**
 * @brief parses <VAL> -> val id <RETURN_TYPE> = <EXPR> EOL 
 * @return AST_DECLARATION node with name type and value    
 */


static ast_node_t* parse_val(){
    expect(KW_VAL);
    if (current_token_type_parser != IDENTIFIER) fail(SYNTAX_ERROR);
    const char *tmp;
    token_get_string(current_token_parser, &tmp);
    char *name = str_dup(tmp);
    consume();

    return_type_t type = parse_return_type();
    expect(OP_ASSIGN);
    ast_node_t *expr = parse_precedence(NULL);
    if(prev_token_type_parser != SP_DEDENT) expect(SP_EOL);
    //expect(SP_EOL);

    ast_node_t *n = make_declaration_node(name,type,expr);

    if(symtable_insert(symtable,n,false,true) != 0) fail(REDEFINITION);
    return n;
}

/**
 * @brief parses <RETURN> -> return <RETURN_VALUE> and the EOL after it
 * @return AST_RETURN node 
 */

static ast_node_t* parse_return(){
    expect(KW_RETURN);
    ast_node_t *value = parse_return_value();
    if(prev_token_type_parser != SP_DEDENT) expect(SP_EOL);
    //expect(SP_EOL);
    return make_node(AST_RETURN, value);
}

/**
 * @brief parses <RETURN_VALUE> -> epsilon
 *               <RETURN_VALUE> -> <EXPR>
 * @return expression node or NULL for no return value
 */


static ast_node_t* parse_return_value(){
    if(current_token_type_parser == SP_EOL){
        return NULL;
    }
    else if(current_token_type_parser == IDENTIFIER ||
        current_token_type_parser == SP_OPENING_PARENTHESIS ||
        current_token_type_parser == OP_MINUS ||
        current_token_type_parser == INTEGER ||
        current_token_type_parser == DOUBLE ||
        current_token_type_parser == STRING ||
        current_token_type_parser == OP_NOT ||
        current_token_type_parser == KW_TRUE ||
        current_token_type_parser == KW_FALSE ||
        current_token_type_parser == KW_IF)  { 
        return parse_precedence(NULL);
        
    }
    fail(SYNTAX_ERROR);
    return NULL;
}


/**
 * @brief parses <VAL> -> <OPTIONAL_EOL> -> SP_EOL
                          <OPTIONAL_EOL> -> epsilon
 * @return NULL  
 */



static ast_node_t* parse_optional_eol(void){
    switch(current_token_type_parser){
        case SP_EOL:
            consume();
            return NULL;
        case KW_ELSE:
        case KW_VAL:
        case KW_VAR:
        case KW_IF:
        case KW_WHILE:
        case KW_RETURN:
        case IDENTIFIER:
        case SP_OPENING_PARENTHESIS:
        case OP_MINUS:
        case INTEGER:
        case DOUBLE:
        case STRING:
        case OP_NOT:
        case KW_TRUE:
        case KW_FALSE:
        case KW_FOR:
        case SP_DEDENT:
            return NULL;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }

}

/**
 * @brief parse <ELSE_BODY> -> <EXPR> SP_EOL
                <ELSE_BODY> -> SP_EOL <INDENTED_BLOCK>
 * @return ast_node_t* linked list of else block  
 */

static ast_node_t* parse_else_body(bool is_expr){
    switch(current_token_type_parser){
            case IDENTIFIER:
            case SP_OPENING_PARENTHESIS:
            case OP_MINUS:
            case INTEGER:
            case DOUBLE:
            case STRING:
            case OP_NOT:
            case KW_TRUE:
            case KW_FALSE:
                ast_node_t *block = init_llist();
                if(!block) fail(MALLOC_FAIL);
                add_llist_item(block,parse_precedence(NULL));
                if(!is_expr) expect(SP_EOL);
                return block;

            case SP_EOL:
                consume();
                return parse_indented_block();
                
            default:
                fail(SYNTAX_ERROR);
                return NULL;
    }               
}


/**
 * @brief parse <GUARDS> -> KW_TO <EXPR> <BY_OPTIONAL>
                <GUARDS> -> KW_UNTIL <EXPR> 
 * @return NULL  
 */



static void parse_guards(ast_node_t **end, ast_node_t** by, bool *is_to){
    *by = NULL;
    switch(current_token_type_parser){
        case KW_TO:
            consume(); 
            *end = parse_precedence(NULL);
            *by = parse_by_optional();
            *is_to = true;
            return;
        case KW_UNTIL:
            consume();
            *end = parse_precedence(NULL);
            *is_to = false;
            return;
        default:
            fail(SYNTAX_ERROR);
            return;
    }
}

/**
 * @brief parse <BY_OPTIONAL> -> KW_BY <EXPR>
                <BY_OPTIONAL> -> epsilon  
 * @return NULL  
 */


static ast_node_t* parse_by_optional(void){
    switch(current_token_type_parser){
        case KW_BY:
            consume();
            return parse_precedence(NULL);
        case KW_DO:
        case SP_EOL:
            return NULL;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
}


/**
 * @brief parse <INDENTED_FOR> -> INDENT IDENTIFIER ARROW <EXPR> <GUARDS> EOL <FOR_CONDITIONS> DEDENT KW_DO <FOR_BODY>
        
 * @return ast_node_t* type of for_loop
 */


static ast_node_t* parse_indented_for(void){
    expect(SP_INDENT);
    const char *tmp;
    char* name;
    if(current_token_type_parser == IDENTIFIER){
        token_get_string(current_token_parser, &tmp);
        name = str_dup(tmp);
    }
    expect(IDENTIFIER);
    expect(SP_ARROW);
    ast_node_t* value = parse_precedence(NULL);
    ast_node_t* i = make_declaration_node(name,RETURN_INT,value); 
    if(symtable_insert(symtable,i,false,true) != 0) fail(REDEFINITION);
    ast_node_t *by;
    ast_node_t *end;
    bool is_to;
    parse_guards(&end,&by,&is_to);
    expect(SP_EOL);
    ast_node_t* llist = init_llist();
    if(!llist) fail(MALLOC_FAIL);

    ast_node_t* guards  = parse_for_conditions(llist);
    expect(SP_DEDENT);
    expect(KW_DO);
    ast_node_t* body = parse_for_body();
    ast_node_t* n = make_for_loop_node(i,end,by,is_to,guards,body);
    if(!n) fail(MALLOC_FAIL);
    return n;
}

/**
 * @brief parse <FOR_CONDITIONS> -> KW_IF <EXPR> EOL <FOR_CONDITIONS>
                <FOR_CONDITIONS> -> epsilon

 * @return ast_node_t* linked list with guards
 */




static ast_node_t* parse_for_conditions(ast_node_t* guards){
    switch(current_token_type_parser){
        case KW_IF:
            consume();
            add_llist_item(guards,parse_precedence(NULL));
            expect(SP_EOL);
            
            return parse_for_conditions(guards);
        case SP_DEDENT:
            return guards;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
}

/**
 * @brief parse <FOR> -> IDENTIFIER ARROW <EXPR> <GUARDS> KW_DO <FOR_BODY>
                <FOR> -> EOL <INDENTED_FOR>

 * @return ast_node_t* type of for_loop
 */



static ast_node_t* parse_for(){
    switch(current_token_type_parser){
    case IDENTIFIER:
        
        const char *tmp;
        token_get_string(current_token_parser, &tmp);
        char *name = str_dup(tmp);
        consume();
        expect(SP_ARROW);
        ast_node_t* value = parse_precedence(NULL);
        ast_node_t* i = make_declaration_node(name,RETURN_INT,value);
        if(symtable_scope_push(symtable) != 0) fail(MALLOC_FAIL); 
        if(symtable_insert(symtable,i,false,true)) fail(REDEFINITION);
        ast_node_t* end;
        ast_node_t* by;
        bool is_to;
        parse_guards(&end,&by,&is_to);
        expect(KW_DO);
        ast_node_t *body = parse_for_body();
       symtable_scope_pop(symtable);
        ast_node_t* llist = init_llist(); //mozno doplnim jednoriadkove ifs
        if(!llist) fail(MALLOC_FAIL);
        ast_node_t* n = make_for_loop_node(i,end,by,is_to,llist,body);
        if(!n) fail(MALLOC_FAIL);
        return n;
    case SP_EOL:
        consume();
        if(symtable_scope_push(symtable)) fail(MALLOC_FAIL);
        ast_node_t *m = parse_indented_for();
       symtable_scope_pop(symtable);
        return m;
    default:
        fail(SYNTAX_ERROR);
        return NULL;

    }
        
}

/**
 * @brief parse <FOR_BODY>  -> <EXPR> SP_EOL
                <FOR_BODY>  -> SP_EOL <INDENTED_BLOCK>

 * @return ast_node_t* type of linked list
 */


static ast_node_t* parse_for_body(void){
    switch(current_token_type_parser){
            case SP_EOL:
                consume();
                return parse_indented_block();
               
            case IDENTIFIER:
            case SP_OPENING_PARENTHESIS:
            case OP_MINUS:
            case INTEGER:
            case DOUBLE:
            case STRING:
            case OP_NOT:
            case KW_TRUE:
            case KW_FALSE:
            case KW_IF:
                ast_node_t *block = init_llist();
                if(!block) fail(MALLOC_FAIL);
                add_llist_item(block,parse_precedence(NULL));
                parse_optional_eol();
                return block;
            default:
                fail(SYNTAX_ERROR);
                return NULL;
        }
}


/**
 * @brief parse <BODY> -> <EXPR> <OPTIONAL_EOL>
                <BODY> -> SP_EOL <INDENTED_BLOCK>

 * @return ast_node_t* type of linked list
 */


static ast_node_t* parse_body(bool is_expr){
        switch(current_token_type_parser){
            case SP_EOL:
                consume();
                return parse_indented_block();
            
             case KW_IF:
                if(!is_expr){                    
                    ast_node_t *if_block = init_llist();
                    if(!if_block) fail(MALLOC_FAIL);
                    add_llist_item(if_block, parse_if(false));
                    return if_block;
                }    


            case IDENTIFIER:
            case SP_OPENING_PARENTHESIS:
            case OP_MINUS:
            case INTEGER:
            case DOUBLE:
            case STRING:
            case OP_NOT:
            case KW_TRUE:
            case KW_FALSE:
                ast_node_t *block = init_llist();
                if(!block) fail(MALLOC_FAIL);
                add_llist_item(block,parse_precedence(NULL));
                parse_optional_eol();
                return block;
            default:
                fail(SYNTAX_ERROR);
                return NULL;
        }
}

/**
 * @brief parse <ELSE_IF_OR_ELSE> -> KW_IF <EXPR> KW_THEN <BODY> <ELSE_PART>
                <ELSE_IF_OR_ELSE> -> <ELSE_BODY>
   @param is_expr if the else is in expr
   @param elsifs linked lists of elsifs
 * @return ast_node_t* else_part
 */



static ast_node_t* parse_else_if_or_else(bool is_expr,ast_node_t* elsifs){
    switch(current_token_type_parser){
        case KW_IF:
            consume();
            ast_node_t* cond = parse_precedence(NULL);
            expect(KW_THEN);
            ast_node_t *block = parse_body(is_expr);
            ast_node_t *elsif = make_else_if_node(cond,block);
            if(!elsif) fail(MALLOC_FAIL);
            add_llist_item(elsifs,elsif);
            return parse_else_part(is_expr,elsifs);
        case SP_EOL:
        case IDENTIFIER:
        case SP_OPENING_PARENTHESIS:
        case OP_MINUS:
        case INTEGER:
        case DOUBLE:
        case STRING:
        case OP_NOT:
        case KW_TRUE:
        case KW_FALSE:
            return parse_else_body(is_expr);
            

        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
}

/**
 * @brief parse <ELSE_PART> -> KW_ELSE <ELSE_IF_OR_ELSE>
                <ELSE_PART> -> epsilon

 * @return ast_node_t* else_part
 */


static ast_node_t* parse_else_part(bool is_expr, ast_node_t* elsifs){

    if (current_token_type_parser == KW_ELSE){
            consume();
            return parse_else_if_or_else(is_expr,elsifs);
    }
    if(is_expr) fail(SYNTAX_ERROR); //chyba else
    switch(current_token_type_parser){
        case KW_VAL:
        case KW_VAR:
        case KW_IF:
        case KW_WHILE:
        case KW_RETURN:
        case IDENTIFIER:
        case SP_OPENING_PARENTHESIS:
        case OP_MINUS:
        case INTEGER:
        case DOUBLE:
        case STRING:
        case OP_NOT:
        case KW_TRUE:
        case KW_FALSE:
        case KW_FOR:
        case SP_DEDENT:
            return NULL;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
}

/**
 * @brief parses <IF> -> if <EXPR> then EOL <INDENTED_BLOCK> 
 * @return if else node with the condition and both blocks
 */

ast_node_t* parse_if(bool is_expr){
    expect(KW_IF);
    ast_node_t* cond = parse_precedence(NULL);
    expect(KW_THEN);
    ast_node_t* if_block = parse_body(is_expr);
    ast_node_t* elsifs = init_llist();
    if(!elsifs) fail(MALLOC_FAIL);
    ast_node_t* else_block = parse_else_part(is_expr,elsifs);
    ast_node_t* n = make_if_else_tree(cond,if_block,elsifs,else_block);
    if(!n) fail(MALLOC_FAIL);
    return  n;
}

/**
 * @brief parses <WHILE> -> while <EXPR> do EOL <INDENTED_BLOCK>
 * @return while node with the condition and the body
 */

static ast_node_t* parse_while(){
    expect(KW_WHILE);
    ast_node_t* cond = parse_precedence(NULL);
    expect(KW_DO);
    expect(SP_EOL);
    ast_node_t* while_block = parse_indented_block();
    //expect(SP_EOL);
    return make_while_tree(cond,while_block);
}


/**
 * @brief parses a line starting with an identifier ID <ID_EXPR_OR_FUNCALL> EOL
 *        <ID_EXPR_OR_FUNCALL> -> = <EXPR>      assignment
 *        <ID_EXPR_OR_FUNCALL> -> ( <ARGS> )    function call
 *        <ID_EXPR_OR_FUNCALL> -> <EXPR_REST>   expression starting with id
 *        calls and expressions go to the precedence parser
 * @return AST_ASSIGNMENT node or call or expression node or AST_LEAF for a lone id
 */

static ast_node_t* parse_id_expr_or_funcall(){
    const char *tmp;
    token_get_string(current_token_parser,&tmp);
    char *name = str_dup(tmp);
    consume();
    switch(current_token_type_parser){
        case SP_OPENING_PARENTHESIS: 
            ast_node_t *call = parse_precedence(name);
            expect(SP_EOL);
            return call;
        case OP_ASSIGN:
    
            symbol_t *symbol = symtable_find(symtable,name);
            if(!symbol) fail(USING_NOT_DEFINED_VARIABLE);
            //im in assign cant be const
            if(symbol->is_const) fail(ASSIGN_TO_CONST);
            consume();
            ast_node_t* expr = parse_precedence(NULL);
            if(prev_token_type_parser != SP_DEDENT) expect(SP_EOL);
            ast_node_t* assigment_name = make_node(AST_LEAF,symbol);
            free(name);
            //expect(SP_EOL);
            return make_binary_tree(AST_ASSIGNMENT,assigment_name,expr);
        case SP_EOL:
            expect(SP_EOL);
            symbol_t* sym = symtable_find(symtable,name);
            if(!sym) fail(USING_NOT_DEFINED_VARIABLE);
            ast_node_t *n  = make_node(AST_LEAF,sym);
            free(name);
            return n;
            

        case OP_PLUS: 
        case OP_MINUS: 
        case OP_MULTIPLY:
        case OP_DIVIDE:
        case OP_LESS:
        case OP_LESS_OR_EQUAL:
        case OP_GREATER:
        case OP_GREATER_OR_EQUAL:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
        case OP_AND:
        case OP_OR:
            ast_node_t *expr2 = parse_precedence(name);
            expect(SP_EOL);
            return expr2;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
    
}


/**
 * @brief parses <LINE> -> <VAL> EOL
 *               <LINE> -> <VAR> EOL
 *               <LINE> -> <RETURN> EOL
 *               <LINE> -> <IF>
 *               <LINE> -> <WHILE>
 *               <LINE> -> ID <ID_EXPR_OR_FUNCALL> EOL
 *               <LINE> -> <EXPR_NO_ID> EOL
 * @return ast node of the line
 */

static ast_node_t* parse_line(){
    switch(current_token_type_parser){
        case KW_VAR:
            return parse_var();
           
        case KW_VAL:
            return  parse_val();
        case KW_RETURN:
            return parse_return();
        case KW_IF:
           return  parse_if(false);
        case KW_WHILE:
            return parse_while();
        case KW_FOR:
            consume();
            return parse_for();
        case IDENTIFIER:
            
            return parse_id_expr_or_funcall();
        case SP_OPENING_PARENTHESIS:  
        case OP_MINUS:   
        case INTEGER:  
        case DOUBLE: 
        case STRING:
        case OP_NOT:
        case KW_TRUE:
        case KW_FALSE:
            ast_node_t * line = parse_precedence(NULL);
            expect(SP_EOL);
            return line;
        default:
            fail(SYNTAX_ERROR);
            return NULL;
    }
}

/**
 * @brief parses <REST_BLOCK> -> epsilon
 *               <REST_BLOCK> -> <LINE> <REST_BLOCK>
 * @param block line list to append
 * @return the same list
 */

static ast_node_t* parse_rest_block(ast_node_t* block){
    if(current_token_type_parser == SP_DEDENT){
        return block;
    }
    else if (current_token_type_parser == KW_VAR ||
             current_token_type_parser == KW_VAL ||
            current_token_type_parser == KW_IF ||
            current_token_type_parser == KW_WHILE ||
            current_token_type_parser == KW_RETURN ||
            current_token_type_parser == IDENTIFIER ||
            current_token_type_parser == SP_OPENING_PARENTHESIS ||
            current_token_type_parser == OP_MINUS ||
            current_token_type_parser == INTEGER ||
            current_token_type_parser == DOUBLE ||
            current_token_type_parser == STRING ||
            current_token_type_parser == OP_NOT ||
            current_token_type_parser == KW_TRUE ||
            current_token_type_parser == KW_FALSE ||
            current_token_type_parser == KW_FOR){ 
                add_llist_item(block,parse_line());
                parse_rest_block(block);
                return block;
            }
        
      
    fail(SYNTAX_ERROR);
    return NULL;
}


/**
 * @brief parses the whole program
 * @return root ast_node_t* of the whole AST
 */

ast_node_t* parse(void) {
    consume();
    if(symtable_init(&symtable) != 0) fail(MALLOC_FAIL);              
    parse_prolog();
    root = make_root_node();
    parse_function_list(root);     
    token_free(current_token_parser);
    current_token_parser = NULL;
    return root;
}