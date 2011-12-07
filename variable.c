#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct {
	char *var;
	char *val;
} Variable;

int vCnt=0;
//~ Variable ** pool = NULL;


Variable ** var_add(Variable ** pool, char *var, char *val);
int check_variable(Variable ** pool, char *var); // if exist, return value, if not null
char * get_value(Variable ** pool, char *var);



int check_variable(Variable ** pool, char *var) {
	int i=0;
	
	while(i<vCnt) {
		if(strcmp(pool[i++]->var, var)==0) return --i;
	}
	
	
	return -1;
}

char * get_value(Variable ** pool, char *var) {
	int i=0;
	
	while(i<vCnt) {
		if(strcmp(pool[i]->var, var)==0) return pool[i]->val;
		i++;
	}
	
	
	return NULL;
	
}
Variable ** var_add(Variable ** pool, char *var, char *val) {
	
	if(pool==NULL) {
		pool = (Variable **)realloc(pool,  sizeof(Variable *));
		pool[0] = (Variable *)malloc(sizeof(Variable));
		
		pool[0]->var = var;
		pool[0]->val = val;
		vCnt++;
		return pool;
	} 
	
	int pos = check_variable(pool, var);
	if(pos!=-1) {
		pool[pos]->val = val;
		return pool;
	}
	
	pool = (Variable **)realloc(pool, vCnt * sizeof(Variable *));
	pool[vCnt] = (Variable *)malloc(sizeof(Variable));
	
	pool[vCnt]->var = var;
	pool[vCnt]->val = val;
	vCnt++;
	return pool;
}


//~ int main() {
	//~ 
	//~ Variable ** pool = NULL;
	//~ pool = var_add(pool, "x", "5");
	//~ pool = var_add(pool, "y", "5");
	//~ pool = var_add(pool, "x", "10");
	//~ 
	//~ int c=0;
	//~ while(c<vCnt){
		//~ printf("%s : \"%s\"\n", pool[c]->var, pool[c]->val);
		//~ c++;
	//~ }
	//~ 
	//~ printf("x : \"%s\"\n", get_value(pool, "x"));
	//~ 
	//~ return 0;
//~ }

