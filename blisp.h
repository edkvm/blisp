

#ifndef blisp_h
#define blisp_h

/*
 ** Types
 */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;



/* Function pointers*/
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    
    /* Number , Error and Symbol*/
    double num;
    char* err;
    char* sym;
    char* str;
    
    
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;
    
    
    int count;
    lval** cell;
    
};

struct lenv{
    lenv* par;
    
    int count;
    char** syms;
    lval** vals;
};

/*
 ** Global Declaration
 */
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Blisp;

/*
 ** Function Declaration
 */
lenv* lenv_new(void);
void  lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void  lenv_put(lenv* e, lval* k, lval* v);
void  lenv_def(lenv* e, lval* k, lval* v);
lenv* lenv_copy(lenv* e);

lval* lval_num(double x, int type);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
lval* lval_str(char* s);


lval* lval_read_num(mpc_ast_t* t);
lval* lval_read_str(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);


lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);

lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* lval_copy(lval* v);
void  lval_del(lval* v);
lval* lval_add(lval* v, lval* x);

int   lval_eq(lval* x, lval* y);

void  lval_print(lval* v);
void  lval_println(lval* v);
void  lval_expr_print(lval* v, char open, char close);
void  lval_print_str(lval* v);

lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin(lval* a, char* func);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);
lval* builtin_print(lenv* e, lval* a);
lval* builtin_error(lenv* e, lval* a);

lval* builtin_cmp(lenv* e, lval* a, char* op);
lval* builtin_var(lenv* e, lval* a, char* func);
double builtin_op_helper(char* op, double x, double y);

lval* builtin_load(lenv* e, lval* a);


char* ltype_name(int t);
lval* ltype_check(char* func, lval* a, int expect);

/*
 ** Macro
 */
#define LASSERT(args, cond, fmt, ...)  \
if(!(cond)) {  \
lval* err = lval_err(fmt, ##__VA_ARGS__); \
lval_del(args); \
return err; \
}

#define LASSERT_TYPE(func, args, index, expect) \
LASSERT(args,(args->cell[index]->type == expect), \
"Function '%s' passed incorrect types for argument %i. Got %s, Expected %s", \
func, index, ltype_name(args->cell[index]->type), ltype_name(expect))


#define LASSERT_ARGS(op, args, expect) \
LASSERT(args, (args->count == expect), \
"Function '%s' passed wrong number of arguments, Got %i, Expected %i", \
op, args->count, expect)

#endif
