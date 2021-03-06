
#include "mpc.h"
#include "blisp.h"
/* Use these in Windows*/
#ifdef _WIN32

#include <string.h>

static char input[2048];

/* Fake readlin function */
char* readline(char* prompt){
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy- malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

/*
 ** Fake add_history function
 */
void add_history(char* unused){}

#else

#include <editline/readline.h>

#endif

/* Enumeration for possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR, LVAL_DBL, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

/* Enumeration for possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


/*
 ** Define Language
 */
void define_lang(void){
    
    Number  = mpc_new("number");
    Symbol  = mpc_new("symbol");
    String  = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
    Expr    = mpc_new("expr");
    Blisp   = mpc_new("blisp");
    
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                    \
              number   : /-?[0-9]+([.][0-9]+)?/ ;                  \
              symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/  ;       \
              string   : /\"(\\\\.|[^\"])*\"/ ;                    \
              comment  : /;[^\\r\\n]*/   ;                         \
              sexpr    : '(' <expr>* ')' ;                         \
              qexpr    : '{' <expr>* '}' ;                         \
              expr     : <number> | <symbol> | <string>            \
                        | <sexpr> | <qexpr> ;                      \
              blisp    : /^/ <expr>* /$/ ;                         \
              ",
              Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Blisp);
    
}
/*
 ** Enviorment Functions
 */
lenv* lenv_new(void){
    lenv* e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e){
    for(int i = 0; i < e->count; i++){
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k){
    
    /* Find key in array */
    for(int i = 0; i < e->count; i++){
        if(strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    
    if(e->par) {
        return lenv_get(e->par, k);
    } else {
        return lval_err("ERROR: Unbound Symbol '%s'", k->sym);
    }
}

void lenv_put(lenv* e, lval* k, lval* v){
    
    /* Check if variable alredy been defined */
    for(int i = 0; i < e->count; i++){
        
        /* if variables is found
         ** delete the previous value and assign the new */
        if(strcmp(e->syms[i], k->sym) == 0){
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    
    /* Copy content */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v){
    
    while(e->par) {
        e = e->par;
    }
    
    lenv_put(e, k, v);
}

lenv* lenv_copy(lenv* e){
    lenv* n = malloc(sizeof(lenv));
    
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for(int i = 0; i < n->count; i++){
        n->syms[i] = malloc(strlen(e->syms[i] + 1));
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    
    return n;
}



/*
 ** LVAL Functions
 */

/* Create a new lval type num*/
lval* lval_num(double x, int type){
    lval* v = malloc(sizeof(lval));
    v->type = type;
    v->num = x;
    return v;
}


lval* lval_sym(char* s){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_sexpr(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_fun(lbuiltin func){
    lval* v= malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

/* Create a new lval type error*/
lval* lval_err(char* fmt, ...){
    
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    va_list va;
    
    va_start(va, fmt);
    
    /* Allocate space */
    v->err = malloc(512);
    
    vsnprintf(v->err, 511, fmt, va);
    
    v->err = realloc(v->err, strlen(v->err)+1);
    
    va_end(va);
    
    return v;
}

lval* lval_str(char* s){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

lval* lval_lambda(lval* formals, lval* body){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    
    /* Set builtin to NULL */
    v->builtin = NULL;
    
    /* Build new enviorment */
    v->env = lenv_new();
    
    /* Set Formals and body */
    v->formals = formals;
    v->body = body;
    
    return v;
    
}

/* Delete lval and free memory */
void lval_del(lval* v){
    
    switch(v->type){
        case LVAL_NUM:
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for(int i = 0; i < v->count; i++){
                lval_del(v->cell[i]);
            }
            
            free(v->cell);
            break;
        case LVAL_FUN:
            if(!v->builtin){
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        case LVAL_STR:
            if(v->str){
                free(v->str);
            }
            break;
    }
    
    free(v);
}

lval* lval_copy(lval* v){
    
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    
    switch(v->type){
            
        case LVAL_FUN:
            if(v->builtin){
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for(int i = 0; i < x->count; i++){
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;
    }
    
    return x;
}

lval* lval_add(lval* v, lval* x){
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}



void lval_expr_print(lval* v, char open, char close){
    putchar(open);
    
    for(int i = 0; i < v->count; i++){
        /* Print value */
        lval_print(v->cell[i]);
        
        /* Don't print space */
        if(i != (v->count-1)){
            putchar(' ');
        }
    }
    
    putchar(close);
}

void lval_print(lval* v){
    long temp;
    switch(v->type){
            /* if lval type is a number use regular printf */
        case LVAL_NUM:
            temp = (long)(v->num);
            printf("%1ld", temp);
            break;
        case LVAL_DBL:
            printf("%f", v->num);
            break;
            /* if lval is an error print the appropriate error message*/
        case LVAL_ERR:
            printf("ERROR: %s", v->err);
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        case LVAL_FUN:
            if(v->builtin){
                printf("<builtin>");
            } else {
                printf("\\ ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_STR:
            lval_print_str(v);
            break;
    }
}

void lval_print_str(lval* v){
    
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    
    escaped = mpcf_escape(escaped);
    
    printf("\"%s\"", escaped);
    
    free(escaped);
}

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}

lval* lval_pop(lval* v, int i){
    
    lval* x = v->cell[i];
    
    // Shift memory
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
    
    // Decrease count
    v->count--;
    
    // Reallocate memory
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    
    return x;
}

lval* lval_take(lval* v, int i){
    lval* x = lval_pop(v,i);
    lval_del(v);
    return x;
}


lval* builtin_add(lenv* e, lval* a){
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a){
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a){
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a){
    return builtin_op(e, a, "/");
}

lval* builtin_op(lenv* e, lval* a, char* op){
    
    /* Ensure all arguments are numbers */
    for(int i = 0; i < a->count; i++ ){
        LASSERT(a, (a->cell[i]->type == LVAL_NUM || a->cell[i]->type == LVAL_DBL),
                "Function '%s' passed incorrect types for argument %i. Got %s, Expected %s or %s",
                op,
                i,
                ltype_name(a->cell[0]->cell[i]->type),
                ltype_name(LVAL_NUM),ltype_name(LVAL_DBL));
    }
    
    // Pop first element
    lval* x = lval_pop(a, 0);
    
    // if no arguments perform unary op
    if((strcmp(op, "-") == 0) && a->count == 0){
        x->num = -x->num;
    }


    // While there are element remaning
    while( a->count > 0){
        
        lval* y = lval_pop(a,0);
        
        if(strcmp(op, "/") == 0 && y->num == 0) {
            lval_del(x);
            lval_del(y);
            x = lval_err("Division By Zero!");
            break;
        }

        // Perform operation

        x->num = builtin_op_helper(op, x->num,y->num);
        
        lval_del(y);
    }
    
    if(fmod(x->num , 1) != 0){
        x->type = LVAL_DBL;
    }
    lval_del(a);
    
    return x;
}

double builtin_op_helper(char* op, double x, double y){
    
    if(strcmp(op, "+") == 0){
        x += y;
    }
    
    if(strcmp(op, "-") == 0) {  x -= y;}
    
    if(strcmp(op, "*") == 0) {  x *= y;}
    
    if(strcmp(op, "/") == 0) {
        // if value is equal to zero, return error
        
        x /= y;
    }  

    return x;
}

lval* builtin_ord(lenv* e, lval* a,char* op){
    
    LASSERT_ARGS(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);
    
    int r;
    if(strcmp(op, "<") == 0) {
        r = (a->cell[0]->num < a->cell[1]->num);
    }
    if(strcmp(op, ">") == 0) {
        r = (a->cell[0]->num > a->cell[1]->num);
    }
    if(strcmp(op, ">=") == 0) {
        r = (a->cell[0]->num >= a->cell[1]->num);
    }
    if(strcmp(op, "<=") == 0) {
        r = (a->cell[0]->num <= a->cell[1]->num);
    }
    
    return lval_num(r,LVAL_NUM);
}

lval* builtin_gt(lenv* e, lval* a){
    return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a){
    return builtin_ord(e, a, "<");
}

lval* builtin_gte(lenv* e, lval* a){
    return builtin_ord(e, a, ">=");
}

lval* builtin_lte(lenv* e, lval* a){
    return builtin_ord(e, a, "<=");
}

lval* builtin_cmp(lenv* e, lval* a, char* op){
    LASSERT_ARGS(op, a, 2);
    
    int r;
    
    if(strcmp(op, "==") == 0){
        r = lval_eq(a->cell[0], a->cell[1]);
    }
    
    if(strcmp(op, "!=") == 0){
        r = !lval_eq(a->cell[0],a->cell[1]);
    }
    
    lval_del(a);
    
    return lval_num(r, LVAL_NUM);
}

lval* builtin_eq(lenv* e, lval* a){
    return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a){
    return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a){
    LASSERT_ARGS("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);
    
    /* Mark both expressions as evaluable */
    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;
    
    
    if(a->cell[0]->num){
        /* If condition is true, evaluate first */
        x = lval_eval(e, lval_pop(a,1));
    } else {
        /* Otherwise evalute second expression */
        x = lval_eval(e, lval_pop(a,2));
    }
    
    lval_del(a);
    
    return x;
}

lval* builtin_head(lenv* e, lval* a){
    
    // Check for errors
    LASSERT_ARGS("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT(a, (a->cell[0]->count != 0), "Function 'head' passed {}!");
    
    
    lval* v = lval_take(a, 0);
    
    while( v->count > 1) {
        lval_del(lval_pop(v,1));
    }
    
    return v;
}

lval* builtin_tail(lenv* e, lval* a){
    
    // Check for errors
    LASSERT_ARGS("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT(a, (a->cell[0]->count != 0), "Function 'tail' passed {}!");
    
    
    // Take first argument
    lval* v= lval_take(a, 0);
    
    // Delete first element and return
    lval_del(lval_pop(v,0));
    
    return v;
}

lval* builtin_list(lenv* e, lval* a){
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_eval(lenv* e, lval* a){
    LASSERT_ARGS("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
    
    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* lval_join_str(lval* a){
    
    
    lval* x = lval_pop(a, 0);
    
    while(a->count){
        lval* y = lval_pop(a, 0);
        char* temp = (char *)malloc(sizeof(strlen(x->str) + strlen(y->str) + 1));
        strcpy(temp, x->str);
        strcat(temp,y->str);
        strcpy(x->str, temp);
        free(temp);
    }
    
    

    return x;
}


lval* builtin_join(lenv* e, lval* a){
    lval* x = NULL;
    
    if(a->cell[0]->type == LVAL_QEXPR){
        ltype_check("join", a, LVAL_QEXPR);
        x = lval_pop(a, 0);
        
        while(a->count){
            x = lval_join(x, lval_pop(a, 0));
        }
    } else if(a->cell[0]->type == LVAL_STR){
        ltype_check("join", a, LVAL_STR);
        x = lval_join_str(a);

    }
    
    
    
    lval_del(a);
    
    return x;
}

lval* builtin_var(lenv* e, lval* a, char* func){
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
    
    lval* syms = a->cell[0];
    for(int i = 0; i < syms->count; i++){
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "Function '%s' cannot define non-symbole. Got %s, Expected %s.",
                func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }
    
    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. Got %i, Expected %i",
            func, syms->count, a->count-1);
    
    for(int i = 0; i < syms->count; i++){
        if(strcmp(func, "def") == 0){
            lenv_def(e, syms->cell[i], a->cell[i+1]);
        }
        
        if(strcmp(func, "=") == 0){
            lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }
    
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a){
    return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a){
    return builtin_var(e, a, "=");
}

lval* builtin_lambda(lenv* e, lval* a){
    
    
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);
    
    /* Check Q-Expression contains only symbols */
    for(int i = 0; i < a->cell[0]->count; i++ ){
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, Expected %s",
                ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }
    
    /* Pop first two arguments and pass them to lval_lambda */
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);
    
    return lval_lambda(formals, body);
    
}

lval* builtin_load(lenv* e, lval* a){
    LASSERT_ARGS("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);
    
    /* Parse FIle given */
    mpc_result_t r;
    if(mpc_parse_contents(a->cell[0]->str, Blisp, &r)) {
        
        /* Read Contents*/
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);
        
        /* Evaluate each expression */
        while(expr->count){
            lval* x = lval_eval(e, lval_pop(expr, 0));
            
            /* if error print it */
            if(x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
        
        /* Delete expression and arguments */
        lval_del(expr);
        lval_del(a);
        
        /* Return empty list */
        return lval_sexpr();
    } else {
        
        /* Get Parse error as String */
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);
        
        /* Create new error msg */
        lval* err = lval_err("Could not load File %s", err_msg);
        free(err_msg);
        lval_del(a);
        
        return err;
    }
    
}

lval* builtin_print(lenv* e, lval* a){
    
    /* Print each argument followed by space */
    for(int i = 0; i < a->count; i++){
        lval_print(a->cell[i]);
        putchar(' ');
    }
    
    /* Print new line */
    putchar('\n');
    lval_del(a);
    
    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a){
    LASSERT_ARGS("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);
    
    /* Build error object */
    lval* err = lval_err(a->cell[0]->str);
    
    /* Delete argument */
    lval_del(a);
    
    return err;
}

lval* lval_join(lval* x, lval* y){
    
    while(y->count){
        x = lval_add(x, lval_pop(y, 0));
    }
    
    lval_del(y);
    return x;
}

int lval_eq(lval* x, lval* y){
    
    if(x->type != y->type){
        return 0;
    }
    
    switch(x->type){
        case LVAL_NUM:
            return (x->num == y->num);
            
        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);
            
        case LVAL_FUN:
            if(x->builtin || y->builtin){
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
            }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if(x->count != y->count) {
                return 0;
            }
            
            /* Check that all elements are equal*/
            for(int i = 0; i < x->count; i++){
                /* If one is not equal than expression is false */
                if(!lval_eq(x->cell[i], y->cell[i])){
                    return 0;
                }
            }
            
            /* Otherwise return true */
            return 1;
        case LVAL_STR:
            return (strcmp(x->str,y->str) == 0);
    }
    
    return 0;
}

lval* lval_call(lenv* e, lval* f, lval* a){
    
    /* if builtin, apply */
    if(f->builtin){
        return f->builtin(e, a);
    }
    
    /* Argument count */
    int given = a->count;
    int total = f->formals->count;
    
    /* Loop while remain argument to process */
    while(a->count){
        
        if(f->formals->count == 0){
            lval_del(a);
            return lval_err("Function passed too many arguments. Got %i, Expected %i",
                            given, total);
        }
        
        /* Pop symbole from formals */
        lval* sym = lval_pop(f->formals, 0);
        
        if(strcmp(sym->sym, "&") == 0){
            /* Ensure '&' followed by symbol */
            if(f->formals->count != 1){
                lval_del(a);
                return lval_err("Function format invalid. Symbole '&' not followed by single symbole.");
            }
            
            /* Next formal should be bound to remainig arguments */
            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
        }
        /* Pop argement */
        lval* val = lval_pop(a, 0);
        
        /* Push var to function stack */
        lenv_put(f->env, sym, val);
        
        /* Delete symbole and value */
        lval_del(sym);
        lval_del(val);
        
        
        
    }
    
    /* Argument list was pushed to stack, can be cleaned */
    lval_del(a);
    
    /* if '&' remains in formal list it should be bound to empty list */
    if(f->formals->count > 0
       && strcmp(f->formals->cell[0]->sym,"&") == 0){
        
        /* Check '&' is not passed invalidily */
        if(f->formals->count != 2){
            return lval_err("Function format invalid. Symbol '&' not followed by signle symbol!");
        }
        
        /* Pop and delete '&' symbol */
        lval_del(lval_pop(f->formals, 0));
        
        /* Pop next symbol and create empty list */
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();
        
        /* Bind to enviorment and delete */
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
        
    }
    
    /* All formals have been bound */
    if(f->formals->count == 0){
        f->env->par = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        /* Return partialy evaluted function */
        return lval_copy(f);
    }
    
    
}

lval* lval_read_num(mpc_ast_t* t){
    errno = 0;
    lval* v = NULL;
    double x = strtod(t->contents, NULL);
    
    if(strstr(t->contents,".")){
        v = lval_num(x, LVAL_DBL);
    } else {
        v = lval_num(x, LVAL_NUM);
    }

    return errno != ERANGE ?  v : lval_err("invalid number");
}

lval* lval_read_str(mpc_ast_t* t){
    
    /* Replace final quet with terminating char*/
    t->contents[strlen(t->contents)-1] = '\0';
    
    /* Copy string without the leading quote */
    char* unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    
    /* Pass thru unescape function*/
    unescaped = mpcf_unescape(unescaped);
    
    lval* v = lval_str(unescaped);
    
    free(unescaped);
    
    return v;
    
}

/* Walks thru the mst tree and creates a S-Expr tree*/
lval* lval_read(mpc_ast_t* t){
    
    // TODO: Add verbose
        
    if(strstr(t->tag, "number")){
        return lval_read_num(t);
    }
    
    if(strstr(t->tag, "symbol")){
        return lval_sym(t->contents);
    }
    
    if(strstr(t->tag, "string")) {
        return lval_read_str(t);
    }
    
    lval* x = NULL;
    
    if(strstr(t->tag,"qexpr")){
        x = lval_qexpr();
    }
    
    if(strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")){
        x = lval_sexpr();
    }
    
    
    /* Fill the list*/
    for(int i = 0; i < t->children_num; i++){
        if(strstr(t->children[i]->tag, "comment")) { continue; }
        if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if(strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }
    
    return x;
    
}

lval* lval_eval(lenv* e, lval* v){
    
    // TODO: Add verbose
    
    if(v->type == LVAL_SYM){
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    
    // Evaluate S-expression
    if( v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }
    
    return v;
}


lval* lval_eval_sexpr(lenv* e, lval* v){
    
    // Evaluate children
    for(int i = 0; i < v->count; i++){
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    
    // Check for Errors
    for(int i = 0; i < v->count; i++){
        if( v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }
    
    // Empty expression
    if( v->count == 0){
        return v;
    }
    
    // Single expression
    if( v->count == 1){
        return lval_take(v, 0);
    }
    
    // Ensure first elemnt is a function
    lval* f = lval_pop(v, 0);
    if(f->type != LVAL_FUN){
        lval* err = lval_err("S-Expression starts with incorrect type. Got %s, Expexted %s",
                             ltype_name(f->type),ltype_name(LVAL_FUN));
        lval_del(v);
        lval_del(f);
        return err;
    }
    
    // Call to function
    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}


lval* ltype_check(char* func, lval* a, int expect){
    
    for(int i = 0; i < a->count; i++){
        LASSERT_TYPE(func, a, i, expect);
    }
    
    return lval_num(1, LVAL_NUM);
}

char* ltype_name(int t){
    switch(t){
        case LVAL_QEXPR: return "Q-Expression";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_NUM:   return "Number";
        case LVAL_SYM:   return "Symbole";
        case LVAL_FUN:   return "Function";
        case LVAL_ERR:   return "Error";
        case LVAL_STR:   return "String";
        default: return "Unknown";
    }
}

/* Add single item to enviorment */
void lenv_add_builtin(lenv* e, char* name, lbuiltin func){
    
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k ,v);
    
    lval_del(k);
    lval_del(v);
}

/* Register all builtin function to the global enviorment */
void lenv_add_builtins(lenv* e){
    
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
    
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_gte);
    lenv_add_builtin(e, "<=", builtin_lte);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}

void run_REPL(void){
     /* Build enviorment*/
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    while(1) {
        char* input = readline("blisp> ");
        add_history(input);
        
        /* Parse user input */
        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Blisp, &r)) {
             
            lval* result = lval_eval(e, lval_read(r.output));
            //TODO: Add verdose mpc_ast_print(r.output);
            lval_println(result);
            lval_del(result);
            
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        
        free(input);
    }  

    /* Clean enviorment */
    lenv_del(e);  
}

void run_FILE(int argc,char** argv){
    /* Build enviorment*/
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    /* Loop thru suplid filenames */
    for(int i = 1; i < argc; i++){
        /* Create an argument list */
        lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
        
        /* Pass to builtin load */
        lval* x = builtin_load(e, args);
        
        /* if the result is an error, print it */
        if(x->type == LVAL_ERR) {
            lval_println(x);
        }
        lval_del(x);
    } 

    /* Clean enviorment */
    lenv_del(e);     
}

int main(int argc,char** argv){
    puts("Lisp version 0.0.13");
    puts("Precc Command + C to Exit\n");
    
    /* Define language */
    define_lang();
    
    
    
    if(argc >= 2){
        run_FILE(argc, argv);
    } else {
        run_REPL();    
    }
    
    
    
    /* Clean up language defenition */
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Blisp); 
    
    return 0;
}