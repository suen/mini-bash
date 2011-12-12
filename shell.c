#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>

typedef struct {
	char *var;
	char *val;
} Variable;


void displayPrompt();							/* display the current working directory as prompt */
char ** parse(char *line);                      /* parse for the command to run  */
char ** parse_pipe(char *line);                 /* parse for pipe       */
char ** parse_or(char *line);                   /* parse for || operator    */ 
char ** parse_and(char *line);                  /* parse for && operator         */
char ** parse_var(char *line);					/* parse for variables */
char ** parse_seperator(char *line);			/* parse for seperator */

int run_command(char *arg[]);                   /* run command in general   */
int run_pipeCmd(char *arg[]);                   /* run pipe command     */
int run_spCmd(char *arg[]);						/* run ; command */
int run_orCmd(char *arg[]);                     /* run || command       */
int run_andCmd(char *arg[]);                    /* run && command       */


void redirect_in(char *arg[]);                  /* redirection "<" */
void redirect_out(char *arg[]);                 /* redirection ">" */
void redirect_out_append(char *arg[]);          /* redirection ">>" */

int func_builtin(char *arg[]);                  /* execute the built-in command.. invokes functions accordingly*/
void func_cd(char *arg[]);                      /* change directory */
void pwd_func(char *argv[]);                    /* pwd          */
void func_exit(char *argv[]);                   /* exit         */
void func_echo(char *argv[]);					/* function echo */
void func_var(char *argv[]);					/* variable setting */
void func_export(char *argv[]);					/*function for exporting the global variable */

void history_run(char arg[]);                   /* runs commands from history */
void history_display(void);                     /* display history */
void set_history(char arg[]);                   /* saves history command */

int check_background(char *arg[]);               /* check for & */
int check_pipe(char *arg[]);                    /* check for |  */
int check_var(char *line);						/* check for = */


Variable ** var_add(Variable ** pool, char *var, char *val); /* Adds variable to the array */
int has_variable(Variable ** pool, char *var); /* if exist, return value, if not null */
char * get_value(Variable ** pool, char *var); /* returns the value of var from the array */


/* struct for built-in functions */
struct builtincmd {
        char *cmd;
        void (*fptr)(char *arg[]);
}builtin[7] = { {"cd", func_cd},{"pwd",pwd_func}, 
				{"exit",func_exit}, {"var", func_var}, 
				{"echo", func_echo}, {"export", func_export}, 
				{NULL,NULL} };


char history[100][20];                  /* history save command */
char history_Count;                     /* history Command Count */
char env[100][100];                     /*                      */
char temp_homedir[1024];                /* Temp Home directory */
char **pipe_arglist;                    /* pipe argument list   */
char **or_arglist;                      /* || argument list     */
char **and_arglist;                     /* && argument list    */
char **sp_arglist;						/* seperator ';' list */
int pArgCnt;                            /* pipe arg Counter       */
int andCnt;                             /* && Counter             */
int orCnt;                              /* || Counter             */
int spCnt;								/* ; counter */
int vCnt=0;
Variable ** vars=NULL;					/* for storing the variables of type "variable" */


/*
 * The function main displays the prompt, seeks for input, 
 * then runs parse (4 parses for ;, &&, || and command) 
 * then executes the arguments accordingly..
 * at last it sets the counters to null.
 * It runs in a loop unless the 'exit' is encountered.
 * 
 * */
int main(void)
{
        char line[256];                 /* User Input Save array */
        char pipe_line[256];
        char sp_line[256];
        char or_line[256];
        char and_line[256];
        char **arglist;
        
        
        int i;

        while(1) {
				
                displayPrompt();
                fgets(line, 255, stdin);
                if(line[0]=='\n')
                        continue;
                if(line[0]== ' ')
                        continue;
                        
                line[strlen(line)-1]='\0';
                
                if(line[0] =='!'){
                        history_run(line);
                        continue;
                }
                
                strcpy(pipe_line,line);
                strcpy(sp_line, line);
                strcpy(or_line,line);
                strcpy(and_line,line);

                set_history(line);
                
                if(strcmp(line,"history") == 0) {
                        history_display();
                        continue;
                } else if(strcmp(line,"variables") == 0) {
                        variables_display();
                        continue;
                }
        
        
                pipe_arglist = parse_pipe(pipe_line);
                sp_arglist = parse_seperator(sp_line);    
                or_arglist = parse_or(or_line);
                and_arglist = parse_and(and_line);
        
                arglist=parse(line);                    /* command paresing     */
                
                if(spCnt >=2) {
                        if(!func_builtin(sp_arglist)){
                                run_spCmd(sp_arglist);
                                continue;
                        }					
				}
                if(andCnt >= 2){
                        if(!func_builtin(and_arglist)){
                                run_andCmd(and_arglist);
                        }
                }else if(orCnt >= 2){
                        if(!func_builtin(or_arglist)){
                                run_orCmd(or_arglist);
                        }
                }else if(pArgCnt == 2 || pArgCnt == 1){
                        if(!func_builtin(arglist)){
                                run_command(arglist);
                        }
                }
                else if(!func_builtin(pipe_arglist)){
                        run_pipeCmd(pipe_arglist);
                }
                
                pArgCnt = 0;    /* pipe Command init */
                orCnt = 0;      /* || Command init   */
                andCnt = 0;     /* && Command init   */
                
                for(i=0; or_arglist[i]; i++)
                        or_arglist[i] = NULL;
                
                for(i=0; and_arglist[i]; i++)
                        and_arglist[i] = NULL;
                
                for(i=0; arglist[i]; i++)
                        arglist[i]=NULL;
        }       
}

/*
 * DisplayPrompt just gets the current workind directory
 * and display as a prompt.
 * 
 * */

void displayPrompt() {
	char dir[1024];
	getcwd(dir, 1024);
	printf("%s> ", dir); 
}

/*
 * The parse_and takes in a string of character.. it looks for '&&' 
 * and divides the strings into token, which it stores in the array
 * declared dynamically. It returns the resulting array as return value.
 * 
 * */

char ** parse_and(char *line) {
	
	char *atoken;
	char **args=NULL;
	
	int i = 0;

	if(line==NULL) return args;
	
	atoken=strtok(line,"&&"); 
	
	while(atoken) {
		args = (char **)realloc(args, (i+1) * sizeof(char *));
		args[i] = (char *)malloc(sizeof(char));
		strcpy(args[i++],atoken);
		
		atoken=strtok(NULL,"&&");
		andCnt++;
	}
	
	args = (char **)realloc(args, (i+1) * sizeof(char *));
	args[i] = (char *)malloc(sizeof(char));
	strcpy(args[i],"");
	return args;

}

/*
 * The parse takes in a string of character.. it looks for a space, \t, or \n  
 * and accordingly divides the strings into token, which it stores in an array
 * declared dynamically. It returns the resulting array as return value.
 * 
 * */

char ** parse(char *line) {
// parseing     
        char *token;
        static char *arg[80];
        int i=0;

        if(line==NULL)
                return NULL;
        token=strtok(line," \t\n");
        while(token)
        {
                arg[i++]=token;
                token=strtok(NULL, " \t\n");
        }
        arg[i]=NULL;
        return (char **) arg;
}

/*
 * The parse_or takes in a string of character.. it looks for '||' 
 * and divides the strings into token, which it stores in an array
 * declared dynamically. It returns the resulting array as return value.
 * 
 * */

char ** parse_or(char *line) {
	
	char *atoken;
	char **args=NULL;
	
	int i = 0;

	if(line==NULL) return args;
	
	atoken=strtok(line,"||"); 
	
	while(atoken) {
		args = (char **)realloc(args, (i+1) * sizeof(char *));
		args[i] = (char *)malloc(sizeof(char));
		strcpy(args[i++],atoken);
		
		atoken=strtok(NULL,"||");
		orCnt++;
	}
	
	args = (char **)realloc(args, (i+1) * sizeof(char *));
	args[i] = (char *)malloc(sizeof(char));
	strcpy(args[i],"");
	return args;

}

/*
 * The parse_pipe takes in a string of character.. it looks for '|' 
 * and divides the strings into token, which it stores in an array
 * declared dynamically. It returns the resulting array as return value.
 * 
 * */
 
char ** parse_pipe(char *line) {
	char *atoken;
	char **args=NULL;
	
	int i = 0;

	if(line==NULL) return args;
	
	atoken=strtok(line,"|"); 
	
	while(atoken) {
		args = (char **)realloc(args, (i+1) * sizeof(char *));
		args[i] = (char *)malloc(sizeof(char));
		strcpy(args[i++],atoken);
		
		atoken=strtok(NULL,"|");
		pArgCnt++;
	}
	
	args = (char **)realloc(args, (i+1) * sizeof(char *));
	args[i] = (char *)malloc(sizeof(char));
	strcpy(args[i],"");
	return args;
}

/*
 * The parse_and takes in a string of character.. it looks for ';' 
 * and divides the strings into token, which it stores in an array
 * declared dynamically. It returns the resulting array as return value.
 * 
 * */


char ** parse_seperator(char *line) {

	char *atoken;
	char **tokens=NULL;
	
	int i = 0;
	spCnt = 0;

	if(line==NULL) return tokens;
	
	atoken=strtok(line,";");
	
	while(atoken) {
		tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
		tokens[i] = (char *)malloc(sizeof(char));
		strcpy(tokens[i++],atoken);
		spCnt++;
		atoken=strtok(NULL, ";");
	}
	
	tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
	tokens[i] = (char *)malloc(sizeof(char));
	strcpy(tokens[i],"");
	return tokens;

}

/*
 * The parse_and takes in a string of character.. it looks for '=' 
 * and divides the strings into token, which it stores in the array
 * declared dynamically. It is assumed that the first token found would
 * be the variable and the token thereafter would be its value..
 * The resulting array is returned as return value.
 * 
 * */

char ** parse_var(char *line) {

	char *atoken;
	char **tokens=NULL;
	
	int i = 0;

	if(line==NULL) return tokens;
	
	atoken=strtok(line,"=");
	
	while(atoken) {
		tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
		tokens[i] = (char *)malloc(sizeof(char));
		strcpy(tokens[i++],atoken);
		
		atoken=strtok(NULL, "=");
	}
	
	tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
	tokens[i] = (char *)malloc(sizeof(char));
	strcpy(tokens[i],"");
	return tokens;
	

	
}       


/*
 * run_command checks takes in an array of string, check for output 
 * and input redirection (<, > , >>), then runs the command in a child 
 * process.
 * 
 * */

int run_command(char *arg[]) {
        int pid;
        int back_flag = 0;
        int pfd[2], pipe_pos;

        back_flag = check_background(arg);
        
        switch(pid = fork()){
                case -1:
                        perror("fork");
                        break;
                case 0:
                        if(strcmp(arg[0], "history") == 0)
                        {
                                history_display();
                                exit(1);
                        }       

                        redirect_in(arg);
                        redirect_out(arg);
                        redirect_out_append(arg);
                        
                        if((pipe_pos=check_pipe(arg)) != 0){
                                
                                pipe(pfd);
                                switch(fork()){
                                case -1:
                                        perror("fork");
                                        exit(1);
                                case 0:
                                //Child Process
                                        close(1);
                                        dup2(pfd[1], 1); //Âü°í1¹ø page126
                                        close(pfd[1]);
                                        close(pfd[0]);
                                        execvp(arg[0],&arg[0]);
                                        exit(1);
                                }

                                close(0);
                                dup2(pfd[0], 0);
                                close(pfd[0]);
                                close(pfd[1]);
                
                        execvp(arg[pipe_pos+1], &arg[pipe_pos+1]);
                        perror("execvp");
                        break;
                        }
                        else
                        {
                                execvp(arg[0], &arg[0]);
                                exit(1);
                        }                       
                default:
                        if(back_flag==0){
                                waitpid(pid,NULL,0);
                        }

                        waitpid(-1, NULL, WNOHANG);
                        break;

        }
        return 0;
}

/*
 * it takes in an array of arguments and runs the commands
 * writing to pipe and reading from pipe.
 * 
 * */

int run_pipeCmd(char *arg[]) {
        int i,pid;
        char **arglist;
        char **arglist2;
        int pfd[2];
        
        if((pid = fork()) == 0)
        {
                
                for(i = 0; i < pArgCnt-1;i++)
                {
                        pipe(pfd);
                
                        switch(pid = fork())
                        {
                                case -1:
                                        perror("fork");
                                        exit(1);
                                        
                                case 0:
                                        close(STDOUT_FILENO);     
                                        dup2(pfd[1],STDOUT_FILENO);
                                        close(pfd[0]);
                                        close(pfd[1]);
                                        
                                        arglist = parse(arg[i]);
                        
                                        execvp(arglist[0],arglist);
                                        break;
                                default:
                                        close(STDIN_FILENO);
                                        dup2(pfd[0],STDIN_FILENO);
                                        close(pfd[0]);
                                        close(pfd[1]);
                                
                        }
                }
                
                arglist2 = parse(arg[pArgCnt-1]);
                execvp(arglist2[0],arglist2);
        }
        else
        {
                waitpid(pid, NULL, 0);
        }
        
        return 0;
}

/*
 * run_spCmd takens in array of string and execute run_command in a child process
 * for each string in the array. 
 * */

int run_spCmd(char *arg[]) {
	int i, pid;
	char **arglist;
	
	for(i=0; i<spCnt; i++) {
		
		switch(pid = fork()) {
			case -1: perror("fork");
					break;
			case 0: arglist = parse(arg[i]);
					run_command(arglist);
					exit(0);
					break;
			default: waitpid(pid, NULL, 0);
					break;
			
		}
	}
	
}

/*
 * Runs if either of the arguments is true.
 * 
 * */

int run_orCmd(char *arg[]) {
        int i,pid;
        char **arglist;
        
        if((pid = fork()) == 0)
        {
                
                for(i = 0; i < orCnt;i++)
                {
                
                        switch(pid = fork())
                        {
                                case -1:
                                        perror("fork");
                                        exit(1);
                                        
                                case 0:
                                        arglist = parse(arg[i]);
                                        execvp(arglist[0],arglist);
                                        break;
                                default:
                                        waitpid(pid, NULL, 0);
                                        break;
                                        
                        }
                        
                }
                
                exit(0);
        }
        else
        {
                waitpid(pid, NULL, 0);
        }
        
        return 0;
}

/*
 * Runs only if both of the arguments in array is true.
 * */

int run_andCmd(char *arg[]) {
        int i,pid,status = 0;
        char **arglist;
                
        if((pid = fork()) == 0)
        {
                
                for(i = 0; i < andCnt;i++)
                {
                        if(status == 0){                
                                switch(pid = fork())
                                {
                                        case -1:
                                                perror("fork");
                                                exit(1);
                                                
                                        case 0:
                                                arglist = parse(arg[i]);
                                
                                                execvp(arglist[0],arglist);
                                                break;
                                        default:
                                                waitpid(pid, &status, 0);
                                                break;
                                                                                        
                                }
                        }else{
                                break;
                        }
                }
                exit(0);
                
        }
        else
        {
                waitpid(pid, NULL, 0);
        }
        
        return 0;
}

/*
 * Adds the variable to the Variable array
 * 
 * */
int set_var(char *var, char *val) {
	vars = var_add(vars, var, val);
	
	return 0;
}

/* Displays the list of variables 
 * */

int variables_display() {
	if(vars==NULL) {
		 printf("No variables set\n");
		 return 1;
	 }
	int i;
	for(i=0; i<vCnt; i++) {
		printf("%s : %s\n", vars[i]->var, vars[i]->val);
	}
	return 0;	
}



/* Checks for '<', if exists, opens the file. The input will be
 * taken from the file.
 * 
 * */
void redirect_in(char *arg[]) {


        int i,fd;
        for(i = 0; arg[i]; i++)
        {
                if(strcmp(arg[i], "<") == 0)
                        break;
        }       
        if(arg[i]){
                if((fd=open(arg[i+1],O_RDONLY))==-1){
                        perror("open");
                        return ;
                }
                dup2(fd,0);
                close(fd);
                for(;arg[i+2];i++)
                        arg[i] = arg[i+2];
                arg[i] = NULL;
        }

}

/* Checks for '>', if exists, opens the file. If file exists, the 
 * content is overwritten. The output will be redirected to the
 * file.   
 * 
 * */
void redirect_out(char *arg[]) {

        int i, fd;
        
        for(i = 0; arg[i]; i++)
        {
                if(strcmp(arg[i], ">") == 0)
                        break;
        }
        if(arg[i]){
                if((fd = open(arg[i+1],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1){
                        perror("open");
                        return ;
                }
                dup2(fd,1);
                close(fd);
                for(;arg[i+2];i++)
                        arg[i] = arg[i+2];
                arg[i] = NULL;
        }
}

/* Checks for '>>', if exists, opens the file. If file exists, the 
 * content is appended. The output will be redirected to the
 * file.   
 * 
 * */

void redirect_out_append(char *arg[]) {

        int i, fd;
        
        for(i = 0; arg[i]; i++)
        {
                if(strcmp(arg[i], ">>") == 0)
                        break;
        }       
        if(arg[i]){
                if((fd = open(arg[i+1],O_WRONLY|O_APPEND,0644))==-1){
                        perror("open");
                        return ;
                }
                dup2(fd,1);
                close(fd);
                for(;arg[i+2];i++)
                        arg[i] = arg[i+2];
                arg[i] = NULL;
        }
}

/*
 * It checks if the arguments passed in are one of the builtin
 * commands and invokes the implementing function accordingly
 * */
int func_builtin(char *arg[]) {

        int i;
        for(i = 0; builtin[i].cmd ; i++)
        {
                if(strcmp(builtin[i].cmd,arg[0]) == 0){
                        builtin[i].fptr(arg);
                        return 1;
                }
        }
        return 0;       
}

/*
 * The function implementing built-in command cd. 
 * */
void func_cd(char *arg[]) {

        char* homedir;
        char path[1024];

        strcpy(path,getcwd(path,1023));

        if(arg[1]==NULL){
                homedir = getenv("HOME");
        }else if(strcmp(arg[1],"~")== 0){     /* home dir */
                homedir = getenv("HOME");
        }else if(strcmp(arg[1],"-") == 0){    
                chdir(temp_homedir);
        }else
                homedir=arg[1];

        if(strcmp(arg[1],"-") == 0){
                chdir(temp_homedir);
        }else{
                chdir(homedir);
        }

        if(!strcmp(temp_homedir,path)== 0){
                strcpy(temp_homedir,path);
        }
}

/*
 * The function implementing built-in command pwd. 
 * */

void pwd_func(char *arg[]) {
        char path[1024];
        getcwd(path,1023);
        printf("%s\n",path);
}

/*
 * It implements built-in command exit. 
 * */

void func_exit(char *arg[]) {
        exit(0);
}

/*
 * It implements built-in command export.  
 * */
void func_export(char *argv[]) {
	
	char ** env;
	if(check_var(argv[1])>0) {
		env = parse_var(argv[1]);
		
		setenv(env[0], env[1], 1);
		printf("%s <- %s\n", env[0], getenv(env[0]));
	} else {
		char * value = get_value(vars, argv[1]);
		if(value!=NULL) {
			setenv(argv[1], value, 1);
			printf("%s <- %s\n", argv[1], getenv(argv[1]));
		}
	}

}


/*
 * It implements built-in command echo. 
 * */
void func_echo(char *argv[]) {
	
	char *value = get_value(vars, argv[1]);
	if(value!=NULL) {
		printf("%s=%s\n", argv[1], value);
	} else {
		value = getenv(argv[1]);
		if(value!=NULL) printf("%s=%s\n", argv[1], value);
	}
}

/*
 * It implements built-in command var. It is used for variable 
 * declaration. 
 * */
void func_var(char *arg[]) {
	char ** vars;
	if(check_var(arg[1])) {
		vars = parse_var(arg[1]);
		set_var(vars[0], vars[1]);

		}	
}

/*
 * Checks if the string contains a variable expression (string=string)
 * */
int check_var(char *line) {
	int i=0, count=0;
	
	for(i=0; i<strlen(line); i++) {
		if(line[i]=='=') count++;
	}
	return count;
}

/*
 * Takes in argument of form !integer, where the integer is the
 * history number. It executes the command stored in history 
 * denoted by the integer.
 * */
void history_run(char arg[]) {
        int num;
        int i;
        char temp[80];
        char **arglist;
        char *tmp;
        
        strcpy(tmp,arg);
        
        tmp = strtok(tmp,"!");          

        if(isalpha(arg[1]) != 0)  {
                for(i=history_Count; i > 0; i--)
                {
                        if(strstr(history[i],tmp) != NULL)
                        {
                                printf("%s\n",history[i]);
                                strcpy(temp, history[i]);
                                break;
                        }
                }
        }else{
                num = atoi(&arg[1])-1;
                strcpy(temp, history[num]);
        }

        arglist=parse(temp);
        
        if(!func_builtin(arglist)){
                        run_command(arglist);
        }
}

/*
 * Displays history list
 * */
void history_display(void) {
        int i = 0;
        
        while(history[i][0] != '\0')
        {
                printf("%d  %s\n", i+1, *(history + i));
                i++;    
        }
}

/*
 * It saves the command to the history array.
 * */
void set_history(char arg[]) {

        int i = 0;
        
        while(history[i][0] != '\0')
        {
                i++;
        }
        strcpy(history[i], arg);
        history_Count++;
}


/*check for '&' 
 * 
 * */
int check_background(char *arg[]) {
        int i;
        for(i=0;arg[i];i++);
        
        if(strcmp(arg[i-1], "&")==0){
                arg[i-1] = NULL;
                return 1;
        }
        return 0;
}

/* check for pipe '|'
 * */
int check_pipe(char *arg[]) {

        int i;
        for(i=0;arg[i];i++)
        {
                if(strcmp(arg[i], "|")==0){
                        arg[i]=NULL;
                        return i;
                }       
        }
        return 0;
}

/* checks if the given 'var' exists in Variable array or not
 * */
int has_variable(Variable ** pool, char *var) {
	int i=0;
	
	while(i<vCnt) {
		if(strcmp(pool[i++]->var, var)==0) return --i;
	}
	
	
	return -1;
}

/* Returns the value of 'var' from the Variable array
 * Pre-condition: 'var' exists in the Variable.
 * */
char * get_value(Variable ** pool, char *var) {
	int i=0;
	
	while(i<vCnt) {
		if(strcmp(pool[i]->var, var)==0) return pool[i]->val;
		i++;
	}
	
	
	return NULL;
	
}

/*
 * Add 'var' and 'val' to the Variable array. If 'var' exists in 
 * the array, it overwrites its value, if not a new entry is made
 * */
Variable ** var_add(Variable ** pool, char *var, char *val) {
	
	if(pool==NULL) {
		pool = (Variable **)realloc(pool,  sizeof(Variable *));
		pool[0] = (Variable *)malloc(sizeof(Variable));
		
		pool[0]->var = var;
		pool[0]->val = val;
		vCnt++;
		return pool;
	} 
	
	int pos = has_variable(pool, var);
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
