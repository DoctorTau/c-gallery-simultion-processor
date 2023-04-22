#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>

#define NUMBER_OF_PICTURES 5

const char vahter_sem_name[] = "/vahter_sem";
const char picture_sem_name_template[] = "/picture_sem";

char picture_sem_names[NUMBER_OF_PICTURES][sizeof(picture_sem_name_template) + 1];
sem_t *gallery_sem;
sem_t *picture_sems[NUMBER_OF_PICTURES];

void generatePictureSemNames() {
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        sprintf(picture_sem_names[i], "%s%d", picture_sem_name_template, i);
    }
}

void closeAllSems() {
    sem_close(gallery_sem);
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        sem_close(picture_sems[i]);
    }

    sem_unlink(vahter_sem_name);
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        sem_unlink(picture_sem_names[i]);
    }
}

void stopSignalHandler(int signal) {
    if (signal == SIGINT) {
        printf("SIGINT received. Exiting.\n");
        closeAllSems();
        exit(0);
    }
}

void printError(char *message) {
    printf("Error: %s\nExiting", message);
    closeAllSems();
    exit(1);
}

int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void clearConsole() {
    printf("\033[H\033[J");
}

void printGalleryInfo() {
    clearConsole();
    int number_of_visitors;

    if (sem_getvalue(gallery_sem, &number_of_visitors) == -1) {
        perror("Could not get value of gallery semaphore.");
        exit(EXIT_FAILURE);
    }

    printf("Gallery info:\n");
    printf("There are %d visitors in the gallery.\n", 50 - number_of_visitors);

    int number_of_visitors_in_picture;
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        if (sem_getvalue(picture_sems[i], &number_of_visitors_in_picture) == -1) {
            perror("Could not get value of picture semaphore.");
            exit(EXIT_FAILURE);
        }
        printf("There are %d visitors looking at picture %d.\n", 10 - number_of_visitors_in_picture,
               i + 1);
    }
}

bool isAllTrue(const bool *val) {
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        if (!val[i]) {
            return false;
        }
    }
    return true;
}

int main(int argc, char const *argv[]) {
    closeAllSems();
    (void)signal(SIGINT, stopSignalHandler);
    generatePictureSemNames();

    int number_of_visitors;
    // Check if the number of visitors was given as an argument
    if (argc == 2) {
        number_of_visitors = atoi(argv[1]);
    } else {
        printError("Number of visitors was not given as an argument.");
    }

    // Create semaphores
    gallery_sem = sem_open(vahter_sem_name, O_CREAT, 0666, 50);
    if (gallery_sem == SEM_FAILED) {
        printError("Could not create gallery semaphore.");
    }

    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        picture_sems[i] = sem_open(picture_sem_names[i], O_CREAT, 0666, 10);
        if (picture_sems[i] == SEM_FAILED) {
            printError("Could not create picture semaphore.");
        }
    }
    printGalleryInfo();

    // Create visitors
    for (int i = 0; i < number_of_visitors; i++) {
        int pid = fork();
        if (pid == 0) {
            // Visitor
            bool visited_pictures[NUMBER_OF_PICTURES];
            int time_to_stay = getRandomNumber(1, 5);

            // Wait for the gallery to be free
            sem_wait(gallery_sem);
            for (;;) {
                int picture_number = getRandomNumber(0, NUMBER_OF_PICTURES - 1);
                // Wait for the picture to be free
                sem_wait(picture_sems[picture_number]);

                // Look at the picture
                sleep(time_to_stay);

                // Leave the picture
                sem_post(picture_sems[picture_number]);

                // Mark the picture as visited
                visited_pictures[picture_number] = true;

                // If all pictures have been visited, leave the gallery
                if (isAllTrue(visited_pictures)) {
                    break;
                }
            }
            printGalleryInfo();
            // Leave the gallery
            sem_post(gallery_sem);

            return 0;
        }
    }

    // Wait for all visitors to leave
    for (int i = 0; i < number_of_visitors; i++) {
        wait(NULL);
    }

    printGalleryInfo();
    closeAllSems();
    return 0;
}