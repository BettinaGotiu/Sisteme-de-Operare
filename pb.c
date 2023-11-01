#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
  const char *filename;
  struct stat var;
  int r;
  int ok;

  //verificare nr. argumente
  if(argc!=2){
    exit(EXIT_FAILURE);
  }

  filename = argv[1];
  r = lstat(filename,&var);

  //verificare fisier
  if(r != 1){
    if( S_ISREG(var.st_mode) !=0 ){
      printf("regular file");
    }
    else
      {
        perror("not a file");
	exit(EXIT_FAILURE);
      }
  }

  char statistics[512];
  char f_output[512]="statistics.txt";
  int aux = open(f_output,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP);
  
  if(aux == -1){
    perror("eroare la deschidere");
    exit(EXIT_FAILURE);
  }
  
  
    return 0;
}
