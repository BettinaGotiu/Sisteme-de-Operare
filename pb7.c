#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>

int get_line_count(const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    int line_count = 0;
    char ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            line_count++;
        }
    }

    fclose(file);
    return line_count;
}

void process_bmp_file(const char *file_path, const char *output_dir) {
    int img = open(file_path, O_RDWR);
    if (img == -1) {
        perror("eroare la deschiderea imaginii BMP");
        return;
    }

    char header[54];
    int buffer_bytes = read(img, header, 54);
    if (buffer_bytes == -1) {
        perror("eroare la citirea header-ului BMP");
        close(img);
        return;
    }

    int height = *(int *) &header[18];
    int width = *(int *) &header[22];

    // Creare imagine gri
    unsigned char *image_data = (unsigned char *) malloc(height * width * 3);
    read(img, image_data, height * width * 3);

    for (int i = 0; i < height * width * 3; i += 3) {
        unsigned char red = image_data[i];
        unsigned char green = image_data[i + 1];
        unsigned char blue = image_data[i + 2];

        unsigned char gray = (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);

        image_data[i] = gray;
        image_data[i + 1] = gray;
        image_data[i + 2] = gray;
    }

    // Suprascriere imagine originala cu imaginea gri
    lseek(img, 54, SEEK_SET);
    write(img, image_data, height * width * 3);

    free(image_data);
    close(img);
}

void process_file(const char *file_path, const char *output_dir, struct dirent *entry, char c, int pipe_fd[2]) {
    struct stat var;
    int r;

    r = lstat(file_path, &var);

    if (r == -1) {
        perror("lstat");
        return;
    }

    char buff[512];
    // Construire cale completa pentru fisierul de iesire
    char output_path[512];
    sprintf(output_path, "%s/%s_statistica.txt", output_dir, entry->d_name);

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
                sprintf(buff, "Nume fisier: %s\n", file_path);
                write(stat_file, buff, strlen(buff));

                process_bmp_file(file_path, output_dir);

            } else if (strstr(file_path, ".txt") != NULL) {
	      //incepe procesarea pt fisiere .txt
                sprintf(buff, "Nume fisier: %s\n", file_path);
                write(stat_file, buff, strlen(buff));
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
		
               }
        } else if (S_ISLNK(var.st_mode)) {
            // Fisier legatura simbolica
            char link_name[512];
            ssize_t link_size = readlink(file_path, link_name, sizeof(link_name) - 1);
            if (link_size != -1) {
                link_name[link_size] = '\0';

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
        }

        close(stat_file);

        // Trimite numarul de linii la procesul parinte
        int line_count = get_line_count(output_path);
        write(pipe_fd[1], &line_count, sizeof(int));

        exit(EXIT_SUCCESS);
    } else { // Proces parinte
        int status;
        waitpid(process, &status, 0);

        if (WIFEXITED(status)) {
            int line_count;
            read(pipe_fd[0], &line_count, sizeof(int));

            printf("S-a incheiat procesul pentru %s cu codul %d și a scris %d linii în %s\n",
                   entry->d_name, WEXITSTATUS(status), line_count, output_path);
        } else {
            printf("Procesul pentru %s a fost terminat neașteptat\n", entry->d_name);
        }
    }
    FILE *fp = popen("/home/bettina/Desktop/Sisteme-de-Operare/script.sh", "r");
    if (fp == NULL) {
        perror("Error opening script");
        exit(EXIT_FAILURE);
    }

    char buffer[128];
    int sentence_count = 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        sentence_count = atoi(buffer);
    }

    fclose(fp);

    // Trimite numărul de propoziții corecte la procesul părinte
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

    // Creare pipe pentru comunicarea între procese
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Eroare la crearea conductei");
        exit(EXIT_FAILURE);
    }

    char c = argv[3][0];

    int num_files = 0;

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

            num_files++; // Incrementare număr de fișiere
        }
    }

    // Așteaptă terminarea tuturor proceselor fiu
    int status;
    for (int i = 0; i < num_files; i++) {
        wait(&status);
    }

    // Închide directorul de intrare și conducta
    closedir(dir);
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    int total_sentence_count = 0;
    int sentence_count;
    while (read(pipe_fd[0], &sentence_count, sizeof(int)) > 0) {
        total_sentence_count += sentence_count;
    }

    printf("Au fost identificate in total %d propozitii corecte care contin caracterul %c\n", total_sentence_count, c);

    return 0;
}

