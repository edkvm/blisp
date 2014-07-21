
#include "mpc.h"

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

/* 
** Types
*/
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Enumeration for possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

/* Enumeration for possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Function pointers*/
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
	int type;
	long num;

	/* Erro and Symbol*/
	char* err;
	char* sym;
	lbuiltin fun;

	int count;
	lval** cell;

};

struct lenv{
	int count;
	char** syms;
	lval** vals;
};
 
/*
** Function Declaration 
*/
lval* lval_num(long x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* lval_copy(lval* v);

lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin(lval* a, char* func);

char* ltype_name(int t);

/*
** Macro
*/
#define LASSERT(args, cond, fmt, ...)  \
	if(!(cond)) {  \
		lval* err = lval_err(fmt, ##__VA_ARGS__); \
		lval_del(args); \
		return err; \
	}


lenv* lenv_new(void){
	lenv* e = malloc(sizeof(lenv));
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

	return lval_err("ERROR: Unbound Symbol '%s'", k->sym);
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

/* Create a new lval type num*/
lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
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
	v->fun = func;
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
			break;
	}

	free(v);
}

lval* lval_add(lval* v, lval* x){
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count - 1] = x;  
	return v;
}

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

/* Walks thru the mst tree and creates a S-Expr tree*/
lval* lval_read(mpc_ast_t* t){

	if(strstr(t->tag, "number")){
		return lval_read_num(t);
	}

	if(strstr(t->tag, "symbol")){
		return lval_sym(t->contents);
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
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if(strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;

}

lval* lval_copy(lval* v){

	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type){

		case LVAL_FUN: 
			x->fun = v->fun; 
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
	}

	return x;
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
	switch(v->type){
		/* if lval typ is a number use regular printf */
		case LVAL_NUM:
			printf("%1ld", v->num);
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
			printf("<function>");
			break;

	}
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
	
	// Ensure all arguments are numbers
	for(int i = 0; i < a->count; i++){
		LASSERT(a,
			(a->cell[i]->type == LVAL_NUM)
			,"Function '%s' passed incorrect type for argument %i. Got %s, Expected %s",
			op, i, ltype_name(a->cell[i]->type), ltype_name(LVAL_NUM));
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
		
		// Perform operation
		if(strcmp(op, "+") == 0) {  x->num += y->num;}
		
		if(strcmp(op, "-") == 0) {  x->num -= y->num;}
		
		if(strcmp(op, "*") == 0) {  x->num *= y->num;}
		
		if(strcmp(op, "/") == 0) { 
			// if value is equal to zero, return error 
			if(y->num == 0){
				lval_del(x);
				lval_del(y);
				x = lval_err("Division By Zero!");
				break;
			}
			x->num /= y->num;
		}

		lval_del(y);
	}

	lval_del(a);
	
	return x;
}

char* ltype_name(int t){
	switch(t){
		case LVAL_QEXPR: return "Q-Expression";
		case LVAL_SEXPR: return "S-Expression";
		case LVAL_NUM: 	 return "Number";
		case LVAL_SYM:   return "Symbole";
		case LVAL_FUN:	 return "Function";
		case LVAL_ERR:	 return "Error";
		default: return "Unknown";
	}
}
lval* builtin_head(lenv* e, lval* a){
	
	// Check for errors
	LASSERT(a, (a->count == 1), 
		"Function 'head' passed too many arguments, Got %i, Expected %i",
		a->count, 1);
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), 
		"Function 'head' passed incorrect types!",
	 	ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
	LASSERT(a, (a->cell[0]->count != 0), "Function 'head' passed {}!");
	

	lval* v = lval_take(a, 0);

	while( v->count > 1) {
		lval_del(lval_pop(v,1));
	}

	return v;
}

lval* builtin_tail(lenv* e, lval* a){
	
	// Check for errors
	LASSERT(a, (a->count == 1), 
		"Function 'tail' passed too many arguments, Got %i, Expected %i",
		a->count, 1);
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), 
		"Function 'tail' passed incorrect types!",
	 	ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
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
	LASSERT(a, (a->count == 1), 
		"Function 'eval' passed too many arguments, Got %i, Expected %i",
		a->count, 1);
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), 
		"Function 'eval' passed incorrect types!",
	 	ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a){
	
	for(int i = 0; i < a->count; i++){
		LASSERT(a, (a->cell[i]->count == LVAL_QEXPR), "Function 'eval' passed incorrect type!");
	}

	lval* x = lval_pop(a, 0);

	while(a->count){
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);

	return x;
}

lval* builtin_def(lenv* e, lval* a){
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'def' passed incorrect type");

	lval* syms = a->cell[0];

	/* Ensure all elments are symbols */
	for(int i = 0; i < syms->count; i++){
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),"Function 'def' cannot define non-symbole");
	}

	/* Check correct number of symbols and values */
	LASSERT(a, (syms->count == a->count-1), "Function 'def' cannot define incorrect number of values to symbols");

	for(int i = 0; i < syms->count; i++){
		lenv_put(e, syms->cell[i], a->cell[i+1]);
	}

	lval_del(a);
	return lval_sexpr();

}

lval* lval_join(lval* x, lval* y){
	
	while(y->count){
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func){

	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k ,v);
	
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv* e){
	
	lenv_add_builtin(e, "def", builtin_def);

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

lval* lval_eval(lenv* e, lval* v){

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
		lval_del(v);
		lval_del(f);
		return lval_err("first element is not a function");
	}

	// Call to function
	lval* result = f->fun(e, v);
	lval_del(f);
	return result;
}


int main(int argc,char** argv){
	puts("Lisp version 0.0.7");
	puts("Precc Command + C to Exit\n");

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Qexpr  = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Blisp = mpc_new("blisp");

	mpca_lang(MPCA_LANG_DEFAULT, 
		"						                				              					  \
		number   : /-?[0-9]+/ ;					             \
		symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/  ;       \
		sexpr    : '(' <expr>* ')' ;	  	          		 \
		qexpr    : '{' <expr>* '}' ;					     \
		expr     : <number> | <symbol> | <sexpr> | <qexpr> ; \
		blisp    : /^/ <expr>* /$/ ;           			     				  					  \
		",
	Number, Symbol, Sexpr, Qexpr, Expr, Blisp); 
	
	
	lenv* e = lenv_new();
	lenv_add_builtins(e);

	while(1){
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

	/* Clean up language defenition */
	mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expr, Blisp); 

	return 0;
}