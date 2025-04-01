#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_MAX 4096

typedef enum errorCode{
    OPT_SUCCESS,
    OPT_ERROR_OPEN_DIR,
    OPT_INVALID_ARGUMENTS,
    OPT_ERROR_STAT
}errorCode;

const char* fileType(struct stat file_stat){
    if (S_ISREG(file_stat.st_mode))  return "ISREG";
    if (S_ISDIR(file_stat.st_mode))  return "ISDIR";
    if (S_ISLNK(file_stat.st_mode))  return "ISLNK";
    if (S_ISCHR(file_stat.st_mode))  return "ISCHR";
    if (S_ISBLK(file_stat.st_mode))  return "ISBLK";
    if (S_ISFIFO(file_stat.st_mode)) return "ISFIFO";
    if (S_ISSOCK(file_stat.st_mode)) return "ISSOCK";

    return "UNKNOWN";

}

errorCode list_files(const char *dir_name) {
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        return OPT_ERROR_OPEN_DIR;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; 
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        struct stat file_stat;
        if (stat(path, &file_stat) == 0) {
            printf("file: %-20s  disk address: %-20lu  type: %-20s\n", entry->d_name, file_stat.st_ino, fileType(file_stat));
        } else {
            closedir(dir);
            return OPT_ERROR_STAT;
        }
    }

    closedir(dir);
    return OPT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Invalid numbers of arguments!\n");
        fprintf(stderr, "Usage: %s <directory>...\n", argv[0]);
        return OPT_INVALID_ARGUMENTS;
    }
    errorCode opt;
    for (int i = 1; i < argc; i++) {
        printf("directory: %s\n", argv[i]);
        opt = list_files(argv[i]);
        if (opt == OPT_ERROR_OPEN_DIR){
            fprintf(stderr, "Error: couldn't open the directory!\n");
            return OPT_ERROR_OPEN_DIR;
        }else if (opt == OPT_ERROR_STAT){
            fprintf(stderr, "Error: Couldn't access the file!\n");
            return OPT_ERROR_STAT;
        }
        if (i != argc - 1){ printf("\n"); }
    }

    return OPT_SUCCESS;
}
