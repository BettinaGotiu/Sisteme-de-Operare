#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> //tipuri de date specifice sistemului, cum ar fi pid_t
#include <sys/stat.h> // funcții legate de manipularea informațiilor despre fișiere lstat
#include <unistd.h>  // sistemului de operare Unix, cum ar fi read, write, close, pipe, lseek.
#include <fcntl.h>//Pentru operații pe descriptori de fișiere, cum ar fi open.
#include <time.h>
#include <string.h>
#include <dirent.h> //manipularea informațiilor despre directoare și fișiere, cum ar fi DIR, readdir.
#include <sys/wait.h> //așteptarea proceselor copil, cum ar fi wait, waitpid


//functie ce primeste calea catre un fisier si returneaza nr de linii din el 
int get_line_count(const char *filename){
    FILE *file = fopen(filename, "r"); //deschidere
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    int line_count = 0;
    char ch;

    while ((ch = fgetc(file)) != EOF) { //cat timp e diferite de EOF cauta end of lines
        if (ch == '\n') {
            line_count++;
        }
    }

    fclose(file); //inchiderea fisierului 
    return line_count;
}

void process_bmp_file(const char *file_path, const char *output_dir) {
    int img = open(file_path, O_RDWR); //citire imagine cu read write
    if (img == -1) {
        perror("eroare la deschiderea imaginii BMP");
        return;
    }

    char header[54];
    int buffer_bytes = read(img, header, 54); //citirea header-ului
    if (buffer_bytes == -1) {
        perror("eroare la citirea header-ului BMP");
        close(img);
        return;
    }
    
    //aflarea inaltimii si lungimii
    int height = *(int *) &header[18];
    int width = *(int *) &header[22];

    // Creare imagine gri

    //alocare memorie pt stocarea datelor pixelilor
    unsigned char *image_data = (unsigned char *) malloc(height * width * 3);
    read(img, image_data, height * width * 3); //citirea datelor pixelilor in buffer-ul image_data
    
    //transformarea in tonuri de gri
    //parcurge tripleti de valori RGB
    for (int i = 0; i < height * width * 3; i += 3) {
        unsigned char red = image_data[i];
        unsigned char green = image_data[i + 1];
        unsigned char blue = image_data[i + 2];

        unsigned char gray = (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);

        //actualizează valorile pentru culroi ale pixelilor cu cele gri .
        image_data[i] = gray;
        image_data[i + 1] = gray;
        image_data[i + 2] = gray;
    }

    // Suprascriere imagine originala cu imaginea gri
    lseek(img, 54, SEEK_SET); //de la inceputul headerului
    write(img, image_data, height * width * 3);
    /*se  deplaseaza la inceputul imaginii (dupa header) si suprascrie imaginea originala
    cu noile date pentru pixeli */

    free(image_data);
    close(img);
    //elibereaza memoria inchide fisieru BMP
}

//structura dirent contine informatii despre intrarea curenta 
void process_file(const char *file_path, const char *output_dir, struct dirent *entry, char c, int pipe_fd[2]) {
    struct stat var;
    int r;

    r = lstat(file_path, &var); //obtine informatii despre fisier

    if (r == -1) {
        perror("lstat");
        return;
    }

    char buff[512];
    // Construire cale completa pentru fisierul de iesire
    //entry->d_name = numele fișierului de intrare 
    char output_path[512];
    sprintf(output_path, "%s/%s_statistica.txt", output_dir, entry->d_name);
    //output_path ==fisier ot statistici


    // Creare proces pentru fiecare intrare din director
    pid_t process = fork();

    if (process == -1) {
        perror("Eroare la fork pentru procesul principal");
        exit(EXIT_FAILURE);
    }

    if (process == 0) { // Proces fiu
        int stat_file = open(output_path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
        if (stat_file == -1) {
            perror("Eroare la deschiderea fisierului de statistica");
            exit(EXIT_FAILURE);
        }

        // scriere informatii statistica pt .bmp
        if (S_ISREG(var.st_mode)) {
            if (strstr(file_path, ".bmp") != NULL) {
                // Fisier BMP
                sprintf(buff, "Nume fisier: %s\n", file_path); //nume
                write(stat_file, buff, strlen(buff)); //dimensiune

                process_bmp_file(file_path, output_dir);
                //pt fiecare BMP proceseaza imaginea

            } else if (strstr(file_path, ".txt") != NULL) {
	      //incepe procesarea pt fisiere .txt
                sprintf(buff, "Nume fisier: %s\n", file_path); //nume
                write(stat_file, buff, strlen(buff)); //dimensiune
		        long dimension = var.st_size;
                sprintf(buff, "Dimensiune fisier: %ld\n", dimension);
                write(stat_file, buff, strlen(buff));

                long user = var.st_uid;
                sprintf(buff, "Identificatorul utilizatorului: %ld\n", user);
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces user
                sprintf(buff, "Drepturi de acces user: %c%c%c\n",
                        (var.st_mode & S_IRUSR) ? 'R' : '-',
                        (var.st_mode & S_IWUSR) ? 'W' : '-',
                        (var.st_mode & S_IXUSR) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces grup
                sprintf(buff, "Drepturi de acces grup: %c%c%c\n",
                        (var.st_mode & S_IRGRP) ? 'R' : '-',
                        (var.st_mode & S_IWGRP) ? 'W' : '-',
                        (var.st_mode & S_IXGRP) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces altii
                sprintf(buff, "Drepturi de acces altii: %c%c%c\n",
                        (var.st_mode & S_IROTH) ? 'R' : '-',
                        (var.st_mode & S_IWOTH) ? 'W' : '-',
                        (var.st_mode & S_IXOTH) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));
		
                execl("/home/bettina/Desktop/Sisteme-de-Operare/script.sh", "/home/bettina/Desktop/Sisteme-de-Operare/script.sh", &c, NULL);
                /*se executa scriptu script.sh 
                inlocuieste procesu curent su scriptul 
                trimitem caracteru c catre script */
                /*ar trb rezultatul sa fie redirectat catre pipe_fd[1]*/
               }
        } else if (S_ISLNK(var.st_mode)) {
            // Fisier legatura simbolica
            char link_name[512]; //o sa fie folosit pt stocarea numelui fisierului simbolic

            ssize_t link_size = readlink(file_path, link_name, sizeof(link_name) - 1);
            /*sizeof(link_name) - 1 = dim max a bufferului -1 
            pentru a avea spatiu pentru terminatorul de sir ('\0')*/

            if (link_size != -1) {
                link_name[link_size] = '\0'; //adaugam terminatoru de sir 

                sprintf(buff, "Nume legatura: %s\n", file_path);
                write(stat_file, buff, strlen(buff));
		        sprintf(buff, "Dimensiune legatura: %zd\n", link_size);
                write(stat_file, buff, strlen(buff));

                sprintf(buff, "Dimensiune fisier target: %ld\n", var.st_size);
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces user
                sprintf(buff, "Drepturi de acces user legatura: %c%c%c\n",
                        (var.st_mode & S_IRUSR) ? 'R' : '-',
                        (var.st_mode & S_IWUSR) ? 'W' : '-',
                        (var.st_mode & S_IXUSR) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces grup
                sprintf(buff, "Drepturi de acces grup legatura: %c%c%c\n",
                        (var.st_mode & S_IRGRP) ? 'R' : '-',
                        (var.st_mode & S_IWGRP) ? 'W' : '-',
                        (var.st_mode & S_IXGRP) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));

                // Drepturi de acces altii
                sprintf(buff, "Drepturi de acces altii legatura: %c%c%c\n",
                        (var.st_mode & S_IROTH) ? 'R' : '-',
                        (var.st_mode & S_IWOTH) ? 'W' : '-',
                        (var.st_mode & S_IXOTH) ? 'X' : '-');
                write(stat_file, buff, strlen(buff));
            }
        } else if (S_ISDIR(var.st_mode)) {
            // Director
            sprintf(buff, "Nume director: %s\n", file_path);
            write(stat_file, buff, strlen(buff));

            long user = var.st_uid; //obtinem identificatorul userului asciat directorului
            sprintf(buff, "Identificatorul utilizatorului: %ld\n", user);
            write(stat_file, buff, strlen(buff));

            // Drepturi de acces user
            sprintf(buff, "Drepturi de acces user: %c%c%c\n",
                    (var.st_mode & S_IRUSR) ? 'R' : '-',
                    (var.st_mode & S_IWUSR) ? 'W' : '-',
                    (var.st_mode & S_IXUSR) ? 'X' : '-');
            write(stat_file, buff, strlen(buff));

            // Drepturi de acces grup
            sprintf(buff, "Drepturi de acces grup: %c%c%c\n",
                    (var.st_mode & S_IRGRP) ? 'R' : '-',
                    (var.st_mode & S_IWGRP) ? 'W' : '-',
                    (var.st_mode & S_IXGRP) ? 'X' : '-');
            write(stat_file, buff, strlen(buff));

            // Drepturi de acces altii
            sprintf(buff, "Drepturi de acces altii: %c%c%c\n",
                    (var.st_mode & S_IROTH) ? 'R' : '-',
                    (var.st_mode & S_IWOTH) ? 'W' : '-',
                    (var.st_mode & S_IXOTH) ? 'X' : '-');
            write(stat_file, buff, strlen(buff));
        }

        close(stat_file);

        // Trimite numarul de linii la procesul parinte
        int line_count = get_line_count(output_path);
         //obitnem linile din fisieru de statistica
        write(pipe_fd[1], &line_count, sizeof(int)); //scriem in pipe valoarea variabilei line_count

        exit(EXIT_SUCCESS);
    } else { // Proces parinte
        int status;
        waitpid(process, &status, 0); //asteapta ca procesul fiu identificat de 'process' sa se inchieie

        if (WIFEXITED(status)) { //devine true daca procesu e gata fiu
            int line_count;
            read(pipe_fd[0], &line_count, sizeof(int)); /*aflam ult valoarea a proesului fiu
            inainte ca acesta sa se incheie*/

            printf("S-a incheiat procesul pentru %s cu codul %d și a scris %d linii în %s\n",
                   entry->d_name, WEXITSTATUS(status), line_count, output_path);
        } else {
            printf("Procesul pentru %s a fost terminat neașteptat\n", entry->d_name);
        }
    }
    //aici ar trebui sa deschida (again) un proces pt a citi output ul de la script
    FILE *fp = popen("/home/bettina/Desktop/Sisteme-de-Operare/script.sh", "r");
    if (fp == NULL) {
        perror("Error opening script");
        exit(EXIT_FAILURE);
    }

    char buffer[128];
    int sentence_count = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        sentence_count = atoi(buffer); //ar trb s aconverteasca in numar (linile gasite in numar)
    }

    fclose(fp);

    // Trimite numărul de propoziții corecte la procesul părinte pentru scrieres
    write(pipe_fd[1], &sentence_count, sizeof(int));

}

int main(int argc, char *argv[]) {
    // Verificare argumente
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <director_intrare> <director_iesire> <c>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Deschidere director de intrare
    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului de intrare");
        exit(EXIT_FAILURE);
    }

    // pipe comuniare intre proces parinte si fiu 
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Eroare la crearea conductei");
        exit(EXIT_FAILURE);
    }

    char c = argv[3][0]; //primu caracter din al treilea argument

    int num_files = 0;

    //se parcurge directorul deschis anterior pt fiecare fiser deschis 
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Construire cale completa pentru intrare
            char input_path[512];
            sprintf(input_path, "%s/%s", argv[1], entry->d_name);

            // Creare proces pentru fiecare intrare din director
            pid_t process = fork();

            if (process == -1) {
                perror("Eroare la fork pentru procesul principal");
                exit(EXIT_FAILURE);
            }

            if (process == 0) { // Proces fiu
                process_file(input_path, argv[2], entry, c, pipe_fd);
                exit(EXIT_SUCCESS);
            }

            num_files++; // Incrementare numar de fișiere
        }
    }

    // Așteaptă terminarea tuturor proceselor fiu
    int status;
    for (int i = 0; i < num_files; i++) {
        wait(&status);
    }

    // Închide directorul de intrare și pipe-ul
    closedir(dir);
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    int total_sentence_count = 0;
    int sentence_count;
    while (read(pipe_fd[0], &sentence_count, sizeof(int)) > 0) {
        //citim rezultatele din captul conductei pip_fd[0]
        //formam nr total de prop corecte gasite 
        total_sentence_count += sentence_count; 
    }

    printf("Au fost identificate in total %d propozitii corecte care contin caracterul %c\n", total_sentence_count, c);

    return 0;
}

