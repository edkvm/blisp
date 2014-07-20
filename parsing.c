
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

 
/*
** Function Declaration 
*/
lval* lval_num(long x);
lval* lval_err(char* m);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* builtin_op(lval* a, char* op);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* builtin(lval* a, char* func);


/*
** Macro
*/
#define LASSERT(args, cond, err) if(!(cond)) { lval_del(args); return lval_err(err); }

/* Create a new lval type num*/
lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* Create a new lval type error*/
lval* lval_err(char* m){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);

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
	v-type = LVAL_FUN;
	v->fun = func;
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
			printf("Error: %s", v->err);
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

lval* builtin_op(lval* a, char* op){
	
	// Ensure all arguments are numbers
	for(int i = 0; i < a->count; i++){
		if(a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on non-number!");
		}
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

lval* builtin_head(lval* a){
	
	// Check for errors
	LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments");
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'head' passed incorrect types!");
	LASSERT(a, (a->cell[0]->count != 0), "Function 'head' passed {}!");
	

	lval* v = lval_take(a, 0);

	while( v->count > 1) {
		lval_del(lval_pop(v,1));
	}

	return v;
}

lval* builtin_tail(lval* a){
	
	// Check for errors
	LASSERT(a, (a->count == 1), "Function 'tail' passed too many arguments");
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'tail' passed incorrect types!");
	LASSERT(a, (a->cell[0]->count != 0), "Function 'tail' passed {}!");
	

	// Take first argument
	lval* v= lval_take(a, 0);

	// Delete first element and return
	lval_del(lval_pop(v,0));

	return v;
}

lval* builtin_list(lval* a){
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a){
	LASSERT(a, (a->count == 1), "Function 'eval' passed too many arguments!");
	LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!");

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(x);
}

lval* builtin_join(lval* a){
	
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

lval* builtin(lval* a, char* func){
	if(strcmp("list", func) == 0){
		return builtin_list(a);
	}

	if(strcmp("head", func) == 0){
		return builtin_head(a);
	}

	if(strcmp("tail", func) == 0){
		return builtin_tail(a);
	}

	if(strcmp("eval", func) == 0){
		return builtin_eval(a);
	}

	if(strcmp("join", func) == 0){
		return builtin_join(a);
	}

	if(strstr("+*-/", func) == 0){
		return builtin_op(a, func);
	}

	lval_del(a);
	return lval_err("Unkown function!");

}

lval* lval_join(lval* x, lval* y){
	
	while(y->count){
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

lval* lval_eval(lval* v){

	// Evaluate S-expression
	if( v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	} 

	return v;
}


lval* lval_eval_sexpr(lval* v){

	// Evaluate children
	for(int i = 0; i < v->count; i++){
		v->cell[i] = lval_eval(v->cell[i]);
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

	// Ensure first elemnt is symbol
	lval* f = lval_pop(v, 0);
	if(f->type != LVAL_SYM){
		lval_del(f);
		lval_del(v);

		return lval_err("S-expression does not start with symbol!");

	}

	// Call builtin with operator
	lval* result = builtin(v, f->sym);
	lval_del(f);
	return result;
}

int main(int argc,char** argv){
	puts("Lisp version 0.0.5");
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
	
	

	while(1){
		char* input = readline("blisp> ");
		add_history(input);


		/* Parse user input */
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Blisp, &r)) {
			lval* result = lval_eval(lval_read(r.output));
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

	/* Clean up language defenition */
	mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expr, Blisp); 

	return 0;
}