#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<dirent.h>
#include<unistd.h>
struct dirent *dirent;
char *comanda;
int ok; //ok va fi pus pe 1 in situatia in care avem comanda p in sirul comenzilor (argumentul 2)
int isC(char *name){
 	char *ext=strchr(name,'.');
 	if(ext==NULL) return 0;
 	// functia verifica daca dupa primul . este doar litera c, nu sunt acceptate fisiere de genul fisier.rar.c pentru ca apar probleme la crearea fisierului .rar.out in urma compilarii
 	if(strcmp(ext,".c")==0) return 1;
 	return 0;
}

//functia verifica daca argumentul 2 contine doar literele a, d, c, g, u sau p, iar daca exista comanda p, ok va fi setat pe 1

void check_comanda(char *comanda){
 	int i;
  	if(comanda[0]!='-') {
		perror("Comanda invalida\n");
		exit(EXIT_FAILURE);
 	}
 	int length=strlen(comanda);
 	for(i=1;i<length;i++){
		if(comanda[i]!='n' && comanda[i]!='a' && comanda[i]!='d' && comanda[i]!='c' && comanda[i]!='g' && comanda[i]!='u' && comanda[i]!='p') {
			perror("Comanda invalida\n");
			exit(EXIT_FAILURE);	
		}
		if(comanda[i]=='p') ok=1;
 }
}
 //functia afiseaza drepturile in formatul cerut in enunt
void afisareDrepturi(struct stat stat){
 	printf("Utilizator:\nRead - ");
 	if(stat.st_mode & S_IRUSR) {
 		printf("DA");
 	}
		else printf("NU");
 	printf("\nWrite - ");
 	if(stat.st_mode & S_IWUSR) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nExec - ");
 	if(stat.st_mode & S_IXUSR) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nGrup:\nRead - ");
 	if(stat.st_mode & S_IRGRP) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nWrite - ");
 	if(stat.st_mode & S_IWGRP) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nExec - ");
 	if(stat.st_mode & S_IXGRP) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nAltii:\nRead - ");
 	if(stat.st_mode & S_IROTH) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nWrite - ");
 	if(stat.st_mode & S_IWOTH) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\nExec - ");
 	if(stat.st_mode & S_IXOTH) {
		printf("DA");
 	}
		else printf("NU");
 	printf("\n");
}

//functia creeaza denumirea fisierului obtinut in urma compilarii fisierelor .c, schimband extensia .c in extensia .out
char* change(char *name){
 	char outputFile[255];
 	char *p=name;
 	int n=0;
 	while(strcmp(p,"c")!=0){
  		p++;
  		outputFile[n]=name[n];
  		n++;
 	}
 	outputFile[n]='\0';
 	strcat(outputFile,"out");
 	p=outputFile;
 	return p;
}
//functia waitFunction verifica daca exista eroare la waitpid si afiseaza mesajul corespunzator incheierii procesului
void waitFunction(pid_t pid){
	pid_t child;
	int status;
	child=waitpid(pid,&status,0);
	if(child<0){
		perror("Eroare la waitpid\n");
		exit(1);
	}
	if(WIFEXITED(status)){
		printf("Procesul fiu cu PID %d s-a terminat cu codul %d.\n",child,WEXITSTATUS(status));
	}
}
//functia creeaza un proces care compileaza fiecare fisier c
void do_gCommand(char *name,char *path){
 	pid_t pid;
 	char outputFile[255];
	//obtinem fisierul .out in locul extensiei .c
  	strcpy(outputFile,change(name));
 	char *args[] = {"cc", "-o", outputFile, path, NULL};
 	// cream un nou proces folosind fork()
 	if( (pid=fork()) < 0) {
		perror("Eroare pid\n");
		exit(1);
 	}
 	if(pid == 0) {	
		//lansam gcc in executie folosind execv
   		execvp("cc", args);
		perror("Eroare la exec\n");
		exit(-1);
 	}
 	waitFunction(pid);
}

// functia do_gpCommand se apeleaza atunci cand sunt prezente atat g, cat si p in al doilea argument.
void do_gpCommand(char *name, char *path){
 	pid_t pid,pid2;
 	int pfd[2],pfd2[2],errorOk=0,countWarning=0,newfd;
 	float nota;
 	char cuvant[1000];
 	char outputFile[255];
	//obtinem fisierul .out in locul extensiei .c
 	strcpy(outputFile,change(name));
 	char *args[] = {"cc", "-o", outputFile, path, NULL};
 	char *argv[]={"grep", "\\(error\\|warning\\)", NULL};
 	FILE *stream;
	//cream pipe-ul folosit pentru redirectarea output-ului din gcc (stderr) catre celalalt proces
 	if(pipe(pfd)<0) {
  		perror("Eroare la crearea pipe-ului\n");
		exit(1);
 	}
 	if( (pid=fork()) < 0) {
		perror("Eroare pid\n");
		exit(1);
 	}
 	if(pid == 0) {	
		close(pfd[0]);
		//redirectam stderr catre pipe (gcc va scrie in pipe)
		if((newfd=dup2(pfd[1],STDERR_FILENO))<0){
			perror("Eroare la dup\n");
			exit(1);
		}
		close(pfd[1]);
		//compilam programul
   		execvp("cc", args);
		perror("Eroare la exec\n");
		exit(-1);
 	}
	//cream pipe-ul folosit pentru redirectarea output-ului functiei grep
 	if(pipe(pfd2)<0) {
  		perror("Eroare la crearea pipe-ului\n");
		exit(1);
 	}
 	if( (pid2=fork()) < 0) {
		perror("Eroare pid\n");
		exit(1);
 	}
 //acest proces urmeaza sa filtreze liniile care contin error sau warning
 	if(pid2==0){
		close(pfd[1]);
		close(pfd2[0]);
		//redirectam stdin catre capatul de citire al primului pipe (grep citeste din stdin)
		if((newfd=dup2(pfd[0],STDIN_FILENO))<0){
			perror("Eroare la dup\n");
			exit(1);	
		}
		close(pfd[0]);
		//redirectam stdout catre capatul de scriere al celui de-al doilea pipe (grep scrie in stdout)
		if((newfd=dup2(pfd2[1],STDOUT_FILENO))<0){
			perror("Eroare la dup\n");
			exit(1);
		}
		close(pfd2[1]);
		//filtram liniile cu grep
		execvp("grep",argv);
		perror("Eroare la exec\n");
		exit(1);
 	}
 //aici urmeaza sa se calculeze nota pe baza formulei din enunt
 	close(pfd[1]);
 	close(pfd[0]);
 	close(pfd2[1]);
	//citim din pipe output -ul functiei grep
 	stream=fdopen(pfd2[0],"r");
	if(!stream) {
		perror("Eroare la fdopen\n");
		exit(1);
	} 
	//verificam daca intalnim cuvintele error si warning si calculam nota
 	while(fscanf(stream,"%s",cuvant)!=EOF){
		if(strcmp(cuvant,"error:")==0) {
			errorOk=1;
			break;	
		}
		if(strcmp(cuvant,"warning:")==0) {
			countWarning++;	
		}
		if(countWarning>10) {
			break;	
		}
 	}
 	if(errorOk==1) {
		printf("Nota este 1\n");
	}
 		else if(countWarning>10) {
			printf("Nota este 2\n");
 			}
 				else if(countWarning==0) {
					printf("Nota este 10\n");
 					}
 						else {
							nota=(float)(2+(0.8*(10-countWarning)));
 							printf("Nota este %g\n",nota);
 						}
 	waitFunction(pid);
 	waitFunction(pid2);
	fclose(stream);
 	close(pfd2[0]);
}
//functia do_arg2 analizeaza optiunile introduse in al doilea argument si afiseaza rezultatele corespunzatoare
void do_arg2(struct stat stat, char *name, char *path){
	int length=strlen(comanda);
 	int i;
 	for(i=1;i<length;i++){
		//in cazul in care optiunea este n, se afiseaza calea relativa a fisierului
		if(comanda[i]=='n') {
			printf("Denumirea fisierului: %s\n",path);
		}
		if(comanda[i]=='a') {
			afisareDrepturi(stat);
		}
		if(comanda[i]=='d') {
			printf("Fisierul are %ld octeti\n",stat.st_size);
		}
		if(comanda[i]=='u') {
			printf("Identificatorul utilizatorului este %d\n",stat.st_uid);
		}
		if(comanda[i]=='c') {
			printf("Numarul de legaturi este: %ld\n",stat.st_nlink);
 		}
		//se analizeaza daca exista atat comanda g, cat si comanda p (ok este pe 0 sau pe 1 in functie de existenta optiunii p)
		if(comanda[i]=='g') {
			if(ok==1) do_gpCommand(name,path);
		     	else do_gCommand(name,path);
		}
 	} 
}
//se creeaza denumirea legaturii simbolice pt fiecare fisier prin eliminarea extensiei .c
char* withoutExtension(char *name,char *new_name){
 	int i,n=strlen(name);
 	for(i=0;i<n;i++){
 		if(name[i]=='.') {
			new_name[i]='\0';
			break;	
		}
		new_name[i]=name[i];
 	}
 	return new_name;
}

void parcurgere_fisier(DIR *director,char *dir_name){
 	char *name,path[1000],*new_name;
 	pid_t pid,pid2;
 	while(1){
		dirent=readdir(director);
		if(dirent==NULL) {
			break;
        }
	//se creeaza calea relativa a fisierului analizat pentru a putea folosi functia lstat
		name=dirent->d_name;
		sprintf(path,"%s/%s",dir_name,name);
		struct stat stat;
		if(lstat(path,&stat)<0){
			perror("Eroare lstat\n");
			exit(1);
		}
	//verificam daca fisierul este obisnuit si are extensia .c, iar atunci cream procesul care va analiza optiunile prezente in al doilea argument al functiei
		if(S_ISREG(stat.st_mode) && isC(name)){
			if( (pid=fork()) < 0) {
				perror("Eroare pid\n");
				exit(1);
 			}
 			if(pid == 0) {	
   				do_arg2(stat, name, path);
				exit(0);
 			}

		//cream procesul care creeaza legaturi simbolice pt fisierele cu dimensiunea mai mica decat 102400 bytes
			if( (pid2=fork()) < 0) {
				perror("Eroare pid\n");
				exit(1);
			}
			if(pid2 == 0) {
				new_name=(char*)malloc(sizeof(char));
				if(!new_name){
					perror("Eroare malloc\n");
					exit(1);
				}
		//legatura simbolica va avea denumirea fisierului fara extensia .c
				new_name=withoutExtension(name,new_name);
				if(stat.st_size<102400) symlink(path,new_name);
				free(new_name);
				exit(0);
			}
			waitFunction(pid);
 			waitFunction(pid2);
		}
	}
	closedir(director);
}


int main(int args, char **argv){
	char *dir_name;
//verificam daca sunt exact 2 argumente si ca primul argument este director
	if(args!=3){
		perror("Numar gresit de argumente\n");
		exit(EXIT_FAILURE);
	}

	dir_name=argv[1];
	DIR *director=opendir(dir_name);
	if(director==NULL){
		perror("Primul argument nu este director\n");
		exit(EXIT_FAILURE);
	}
	comanda=argv[2];
	//verificam al doilea argument
	check_comanda(comanda);
	//parcurgem directorul
	parcurgere_fisier(director,dir_name);
	return 0;
}