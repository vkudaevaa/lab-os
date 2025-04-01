#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define MAXLETTERS 6

typedef enum errorCode{
	OPT_SUCCESS,
    OPT_ERROR_OPTION,
    OPT_ERROR_LIMIT,
    OPT_ERROR_INVALID_FORMAT,
    OPT_USER_NOT_FOUND,
    OPT_ERROR_INVALID_LOGIN,
    OPT_ERROR_INVALID_PIN,
    OPT_ERROR_INCORRECT_LOGIN,
    OPT_ERROR_INCORRECT_PIN,
    OPT_ERROR_INCORRECT_CODE,
	OPT_ERROR_MEMORY
} errorCode;

typedef struct{
    char* login;
    unsigned int pin;
    int sanctions_limit;
    int sanctions_count;
}user;

char* readString(FILE *file) {
	char c = fgetc(file);
	while (isspace(c)) {
		c = fgetc(file);
	}
	ungetc(c, file);
	char *str = NULL;
	int length = 0;
	int capacity = 25;
	str = (char *) malloc(capacity * sizeof(char));
	if (!str) {
		return NULL;
	}
	char ch;
	while (fscanf(file, "%c", &ch) == 1 && ch != '\n') {
		if (length + 1 >= capacity) {
			capacity *= 2;
			char *tmp = (char*)realloc(str, capacity * sizeof(char));
			if (!tmp) {
				free(str);
				return NULL;
			}
			str = tmp;
		}
		str[length++] = ch;
	}
	str[length] = '\0';

	return str;
}

void clearStdin(){
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

bool loginValidation(const char* login){
    if (strlen(login) > MAXLETTERS){
        return false;
    }
    for (int i = 0; login[i] != '\0'; i++) {
        if (!isalnum((unsigned char)login[i])) {
            return false;
        }
    }
    
    return true; 

}

int pinValidation(const char *pin) {
    char *endptr;
    long user_pin = strtol(pin, &endptr, 10);
    if (user_pin < 0 || user_pin > INT_MAX || user_pin > 100000) {
        return -1;
    }
    if (endptr == pin || *endptr != '\0') {
        return -1;
    }
    return (int)user_pin; 
}

void printMenu(){
    printf("\nSelect the option:\n");
    printf("<Time> - current time in <hh:mm:ss> format\n");
    printf("<Date> - current date in the format dd:mm:yyyy\n");
    printf("<Howmuch <time> <flag>> - request for the elapsed time from the specified date\n");
    printf("flags:\n");
    printf("\t<-s> - time in seconds\n");
    printf("\t<-m> - time in minutes\n");
    printf("\t<-h> - time in hours\n");
    printf("\t<-y> - time in years\n");
    printf("<Sanctions <username> <number>> - set limits on the number of requests\n\n");
}

void showTime() {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    printf("Current time: %02d:%02d:%02d\n", t->tm_hour, t->tm_min, t->tm_sec);
}

void showDate() {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    printf("Current date: %02d:%02d:%04d\n", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
}


errorCode howMuch(char* input) {
    char* temp = strdup(input);
    if (temp == NULL){
        return OPT_ERROR_MEMORY;
    }
    
    char* command = strtok(temp, " ");
    char* timee = strtok(NULL, " ");
    char* flag = strtok(NULL, " ");

    if (!command || !timee || !flag) {
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    if (strcmp(command, "Howmuch") != 0) {
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    int day, month, year;
    if (sscanf(timee, "%d:%d:%d", &day, &month, &year) != 3) {
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    if (strcmp(flag, "-s") != 0 && strcmp(flag, "-m") != 0 &&
        strcmp(flag, "-h") != 0 && strcmp(flag, "-y") != 0) {
            free(temp);
        return OPT_ERROR_OPTION;
    }

    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 2025){
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    struct tm date_tm = {0};
    date_tm.tm_mday = day;
    date_tm.tm_mon = month - 1;
    date_tm.tm_year = year - 1900;
    date_tm.tm_isdst = -1;

    time_t input_time = mktime(&date_tm);
    time_t now = time(NULL);
    double diff = difftime(now, input_time);

    if (strcmp(flag, "-s") == 0)
        printf("Elapsed: %.0f seconds\n", diff);
    else if (strcmp(flag, "-m") == 0)
        printf("Elapsed: %.0f minutes\n", diff / 60);
    else if (strcmp(flag, "-h") == 0)
        printf("Elapsed: %.0f hours\n", diff / 3600);
    else if (strcmp(flag, "-y") == 0)
        printf("Elapsed: %.1f years\n", diff / (3600 * 24 * 365.25));
    else {
        free(temp);
        return OPT_ERROR_OPTION;
    }
    free(temp);
    return OPT_SUCCESS;
}


errorCode applySanctions(char* input, user* users, int user_count) {
    
    char* temp = strdup(input);
    if (temp == NULL){
        return OPT_ERROR_MEMORY;
    }
    
    char* command = strtok(temp, " ");
    char* username = strtok(NULL, " ");
    char* number = strtok(NULL, " ");

    if (!command || !username || !number) {
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    if (strcmp(command, "Sanctions") != 0) {
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    int number_sanctions = pinValidation(number);
    if (number_sanctions < 0){
        free(temp);
        return OPT_ERROR_INVALID_FORMAT;
    }

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].login, username) == 0) {
            printf("Enter confirmation code (12345): ");
            char* code = readString(stdin);
            if (code == NULL){
                free(temp);
                return OPT_ERROR_MEMORY;
            }
            
            if (strcmp(code, "12345") != 0) {
                free(temp);
                free(code);
                return OPT_ERROR_INCORRECT_CODE;
            }
            users[i].sanctions_limit = number_sanctions;
            printf("Sanctions applied to user %s, limit = %d\n", username, number_sanctions);
            free(temp);
            free(code);
            return OPT_SUCCESS;
        }
    }
    free(temp);
    return OPT_USER_NOT_FOUND;
}

errorCode mainMenu(user* users, int user_count, int current_count){

    user* current_user = &users[current_count];
    errorCode opt;
    printMenu();
    while(true){
        
        if (current_user->sanctions_limit != -1 && current_user->sanctions_count == current_user->sanctions_limit) {
            return OPT_ERROR_LIMIT;
        }

        
        printf("The selected command >>> ");
        char* command = readString(stdin);
        if (command == NULL){
            return OPT_ERROR_MEMORY;
        }
        if (strncmp(command, "Time", 4) == 0) {
            showTime();
            current_user->sanctions_count++;
        } else if (strncmp(command, "Date", 4) == 0) {
            showDate();
            current_user->sanctions_count++;
        } else if (strncmp(command, "Howmuch", 7) == 0) {
            opt = howMuch(command);
            if (opt == OPT_ERROR_INVALID_FORMAT){
                printf("Error: Invalid format! Please try again\n");
                free(command);
                continue;
            }else if (opt == OPT_ERROR_OPTION){
                printf("Error: Invalid option! Please try again\n");
                free(command);
                continue;
            }else if (opt == OPT_ERROR_MEMORY){
                free(command);
                return OPT_ERROR_MEMORY;

            }else{
                current_user->sanctions_count++;
            }
            
        } else if (strncmp(command, "Logout", 6) == 0) {
            printf("Logging out...\n\n");
            free(command);
            return OPT_SUCCESS;
        } else if (strncmp(command, "Sanctions", 9) == 0) {
            opt = applySanctions(command, users, user_count);
            if (opt == OPT_ERROR_INVALID_FORMAT){
                printf("Error: Invalid format! Please try again\n");
                free(command);
                continue;
            }else if (opt == OPT_ERROR_INCORRECT_CODE){
                printf("Error: Incorrect code! Please try again\n");
                free(command);
                continue;
            }else if (opt == OPT_USER_NOT_FOUND){
                printf("Error: User not found! Please try again\n");
                free(command);
                continue;
            }else if (opt == OPT_ERROR_MEMORY){
                free(command);
                return OPT_ERROR_MEMORY;
            }else{
                current_user->sanctions_count++;
            }
            
        } else {
            printf("Error: Invalid option! Please try again\n");
            free(command);
            continue;
        }  
        free(command);
    }
    
}

void freeUsers(user* users, int count){
    for (int i = 0; i < count; i++){
        free(users[i].login);
    }
    free(users);

}

errorCode signUp(int* user_count, int* capacity, user** users){
    printf("Enter the login >> ");
    char* user_login = readString(stdin);
    if (user_login == NULL){
        return OPT_ERROR_MEMORY;
    }
    
    if (!loginValidation(user_login)){
        
        free(user_login);
        return OPT_ERROR_INVALID_LOGIN;
    }
    printf("Enter the PIN >> ");
    char* pin = readString(stdin);
    if (pin == NULL){
        free(user_login);
        return OPT_ERROR_MEMORY;
    }
    int user_pin = pinValidation(pin);
    if (user_pin < 0){
        free(user_login);
        free(pin);
        return OPT_ERROR_INVALID_PIN;
    }
    free(pin);

    if (*user_count >= *capacity){
        *capacity *= 2;
        user* tmp = (user*)realloc(*users, *capacity * sizeof(user));
        if (tmp == NULL){
            freeUsers(*users, *user_count);
            return OPT_ERROR_MEMORY;
        }
        *users = tmp;
    }

    user* current_user = &(*users)[*user_count];
    current_user->login = user_login;
    current_user->pin = user_pin;
    current_user->sanctions_count = 0;
    current_user->sanctions_limit = -1;
    ++(*user_count);
    return OPT_SUCCESS;
}

errorCode logIn(int* user_count, user** users, int* current_count) {
    printf("Enter the login >> ");
    char* user_login = readString(stdin);
    if (user_login == NULL) {
        return OPT_ERROR_MEMORY;
    }
    
    if (!loginValidation(user_login)) {
        free(user_login);
        return OPT_ERROR_INCORRECT_LOGIN;
    }

    printf("Enter the PIN >> ");
    char* pin = readString(stdin);
    if (pin == NULL) {
        free(user_login);
        return OPT_ERROR_MEMORY;
    }

    int user_pin = pinValidation(pin);
    if (user_pin < 0) {
        free(user_login);
        free(pin);
        return OPT_ERROR_INCORRECT_PIN;
    }
    free(pin);

    

    user* current_user = NULL;
    for (int i = 0; i < *user_count; i++) {
        current_user = &(*users)[i];
        if (strcmp(current_user->login, user_login) == 0 && current_user->pin == user_pin) {
            *current_count = i;
            free(user_login);
            return OPT_SUCCESS;
        }
    }

    free(user_login);
    return OPT_USER_NOT_FOUND;
}




int main(int argc, char* argv[]){
    int user_count = 0;
    int current_count = 0;
    int capacity = 5;
    user* users = (user*)malloc(capacity * sizeof(user));
    if (users == NULL){
        return OPT_ERROR_MEMORY;
    }
    printf("Welcome!\n");
    while(true){
        char action;
        char command[256];
        printf("Select an action:\n");
        printf("<s> - sign up\n");
        printf("<l> - log in\n");
        printf("<e> - exit\n");
        printf(">>> ");
        scanf(" %255[^\n]", command);
        clearStdin();

        sscanf(command, "%c", &action);
        if (action == 's'){
            errorCode opt_s;
            opt_s = signUp(&user_count, &capacity, &users);
            if (opt_s == OPT_ERROR_MEMORY){
                fprintf(stderr, "Error: Memory allocation error\n\n");
                break;;
            }else if (opt_s == OPT_ERROR_INVALID_LOGIN){
                printf("Error: Invalid login! Please try again\n\n");
                continue;
            }else if (opt_s == OPT_ERROR_INVALID_PIN){
                printf("Error: Invalid PIN! Please try again\n\n");
                continue;
            }else if (opt_s == OPT_SUCCESS){
                printf("Registration was successful!\n");
                opt_s = mainMenu(users, user_count, user_count - 1);
                if (opt_s == OPT_ERROR_LIMIT){
                    printf("Warning: The number of available requests has ended!\n\n");
                }else if (opt_s == OPT_ERROR_MEMORY){
                    fprintf(stderr, "Error: Memory allocation error\n\n");
                    break;;
                }
            }

        }else if(action == 'l'){
            errorCode opt_l;
            opt_l = logIn(&user_count, &users, &current_count);
            if (opt_l == OPT_ERROR_MEMORY){
                fprintf(stderr, "Error: Memory allocation error\n\n");
                break;
            }else if (opt_l == OPT_ERROR_INCORRECT_LOGIN){
                printf("Error: Incorrect login! Please try again\n\n");
                continue;
            }else if (opt_l == OPT_ERROR_INCORRECT_PIN){
                printf("Error: Incorrect PIN! Please try againn\n");
                continue;
            }else if (opt_l == OPT_USER_NOT_FOUND){
                printf("Error: The user was not found! Please try again\n\n");
                continue;
            }else if (opt_l == OPT_SUCCESS){
                printf("Authorization was successful!\n");
                opt_l = mainMenu(users, user_count, current_count);
                if (opt_l == OPT_ERROR_LIMIT){
                    printf("Warning: The number of available requests has ended!\n\n");
                }else if (opt_l == OPT_ERROR_MEMORY){
                    fprintf(stderr, "Error: Memory allocation error\n\n");
                    break;
                }
            }

        }else if(action == 'e'){
            printf("Exiting...\n");
            break;

        }else{
            printf("Error: Invalid action!\n");
        }
    }
    freeUsers(users, user_count);

}