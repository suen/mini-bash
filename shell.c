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

#include "variable.c"

void displayPrompt();
char ** myparse(char *line);

char ** parse(char *line);                      /* parse general  */
char ** parse_pipe(char *line);                 /* parse for pipe       */
char ** parse_or(char *line);                   /* parse for ||         */ 
char ** parse_and(char *line);                  /* parse for &&         */
char ** parse_var(char *line);
char ** parse_seperator(char *line);

int run_command(char *arg[]);                   /* command   */
int run_pipeCmd(char *arg[]);                   /* run pipe command     */
int run_orCmd(char *arg[]);                     /* run || command       */
int run_andCmd(char *arg[]);                    /* run && command       */


void redirect_in(char *arg[]);                  /* redirection "<" */
void redirect_out(char *arg[]);                 /* redirection ">" */
void redirect_out_append(char *arg[]);             /* redirection ">>" */

int func_builtin(char *arg[]);                  /* cd, pwd , exit Command */
void func_cd(char *arg[]);                      /* change dirctory */
void pwd_func(char *argv[]);                    /* pwd          */
void func_exit(char *argv[]);                   /* exit         */
void func_echo(char *argv[]);
void func_var(char *argv[]);
void func_export(char *argv[]);

void history_run(char arg[]);                   /* history command */
void history_display(void);                      /* history display */
void set_history(char arg[]);                   /* history command save */

int check_background(char *arg[]);                     /* &            */
int check_pipe(char *arg[]);                    /* | check      */
int check_var(char *line);

void cycle();

struct builtcmd {
        char *cmd;
        void (*fptr)(char *arg[]);
}builtin[7] = 
{ {"cd", func_cd},{"pwd",pwd_func}, {"exit",func_exit}, {"var", func_var}, {"echo", func_echo}, {"export", func_export}, {NULL,NULL} };

char history[100][20];                  /* history save command */
char history_Count;                     /* history Command Count */
char env[100][100];                     /*                      */
char temp_homedir[1024];                /* Temp Home dirtoary */
extern char **environ;
char **pipe_arglist;                    /* pipe parseing list   */
char **or_arglist;                      /* || parseing list     */
char **and_arglist;                     /* && parseing list     */
int pArgCnt;                            /* pipe arg Count       */
int andCnt;                             /* && Count             */
int orCnt;                              /* || Count             */
int spCnt=0;

Variable ** vars=NULL;

int main(void)
{
        char line[256];                 /* User Input Save array */
        char pipe_line[256];
        char or_line[256];
        char and_line[256];
        char **arglist;
        
        int i;

        while(1)
        {
				
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
                or_arglist = parse_or(or_line);
                and_arglist = parse_and(and_line);
        
                arglist=parse(line);                    /* command paresing     */
                
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

void displayPrompt() {
	char dir[1024];
	getcwd(dir, 1024);
	printf("%s> ", dir); 
}

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


char ** parse_seperator(char *line) {

	char *atoken;
	char **tokens=NULL;
	
	int i = 0;

	if(line==NULL) return tokens;
	
	atoken=strtok(line,";");
	
	while(atoken) {
		tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
		tokens[i] = (char *)malloc(sizeof(char));
		strcpy(tokens[i++],atoken);
		
		atoken=strtok(NULL, ";");
	}
	
	tokens = (char **)realloc(tokens, (i+1) * sizeof(char *));
	tokens[i] = (char *)malloc(sizeof(char));
	strcpy(tokens[i],"");
	return tokens;

}

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

int run_orCmd(char *arg[]) {
        int i,pid;
        char **arglist;
        int pfd[2];
        
        if((pid = fork()) == 0)
        {
                
                for(i = 0; i < orCnt;i++)
                {
                        pipe(pfd);
                
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

int set_var(char *var, char *val) {
	vars = var_add(vars, var, val);
	
	return 0;
}

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



// return value : 0 -- no redirect
//              : 1 -- redirect 
void redirect_in(char *arg[]) {
// redirectin(<) check

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

void redirect_out(char *arg[]) {
// redirectout(>) check 

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

void redirect_out_append(char *arg[]) {
// redirectout(>>) check 

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

int func_builtin(char *arg[]) {
// builtin Command
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


void func_cd(char *arg[]) {
// change dirtory
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

void pwd_func(char *arg[]) {
// pwd
        char path[1024];
        getcwd(path,1023);
        printf("%s\n",path);
}

void func_exit(char *arg[]) {
        exit(0);
}

void func_export(char *argv[]) {
	if(check_var(argv[1])>0) {
		printf("need to export variables\n");
	} else
	printf("just a normal variable\n");
}


void func_echo(char *argv[]) {
	printf("%s\n", argv[1]);
	char *value = get_value(vars, argv[1]);
	if(value!=NULL) {
		printf("%s=%s\n", argv[1], value);
	}
}
void func_var(char *arg[]) {
	char ** vars;
	if(check_var(arg[1])) {
		vars = parse_var(arg[1]);
		set_var(vars[0], vars[1]);

		}	
}

int check_var(char *line) {
	int i=0, count=0;
	
	for(i=0; i<strlen(line); i++) {
		if(line[i]=='=') count++;
	}
	return count;
}


void history_run(char arg[]) {
// history cmmand run
        int num;
        int i;
        char temp[80];
        char **arglist;
        char *tmp;
        
        strcpy(tmp,arg);
        
        tmp = strtok(tmp,"!");          

        if(isalpha(arg[1]) != 0)   // Âü°í 3¹ø 
        {
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

void history_display(void) {
// history Display
        int i = 0;
        
        while(history[i][0] != '\0')
        {
                printf("%d  %s\n", i+1, *(history + i));
                i++;    
        }
}

void set_history(char arg[]) {
// history command save
        int i = 0;
        
        while(history[i][0] != '\0')
        {
                i++;
        }
        strcpy(history[i], arg);
        history_Count++;
}

int check_background(char *arg[]) {
//Background(&)
        int i;
        for(i=0;arg[i];i++);
        
        if(strcmp(arg[i-1], "&")==0){
                arg[i-1] = NULL;
                return 1;
        }
        return 0;
}

int check_pipe(char *arg[]) {
//  pipe(|) check
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
