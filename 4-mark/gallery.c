#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

const key_t vahter_sem_id = 666;
const key_t picture_sems_id = 777;
int gallery_sem, picture_sems;

void stopSignalHandler(int signal) {
    if (signal == SIGINT) {
        printf("SIGINT received. Exiting.\n");
        deleteAllSemaphores();
        exit(0);
    }
}

void printError(char *message) {
    printf("Error: %s\nExiting", message);
    deleteAllSemaphores();
    exit(1);
}

void deleteAllSemaphores() {
    semctl(gallery_sem, 0, IPC_RMID);
    for (int i = 0; i < 5; i++)
        semctl(picture_sems, i, IPC_RMID);
}

int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void clearConsole() {
    printf("\033[H\033[J");
}

void printGalleryInfo() {
    clearConsole();
    printf("Gallery info:\n");
    printf("There are %d visitors in the gallery.\n", 50 - semctl(gallery_sem, 0, GETVAL) - 1);
    for (int i = 0; i < 5; i++) {
        printf("There are %d visitors looking at picture %d.\n",
               10 - semctl(picture_sems, i, GETVAL) - 1, i + 1);
    }
}

struct sembuf operation;

struct sembuf setSemOperation(int sem_num, int sem_op, int sem_flg) {
    struct sembuf semaphore_operation;
    semaphore_operation.sem_num = sem_num;
    semaphore_operation.sem_op = sem_op;
    semaphore_operation.sem_flg = sem_flg;
    return semaphore_operation;
}

int main(int argc, char const *argv[]) {
    (void)signal(SIGINT, stopSignalHandler);

    int number_of_visitors;
    // Check if the number of visitors was given as an argument
    if (argc == 2) {
        number_of_visitors = atoi(argv[1]);
    } else {
        printError("Number of visitors was not given as an argument.");
    }

    // Create a new semaphore for gallery
    gallery_sem = semget(vahter_sem_id, 1, IPC_CREAT | IPC_EXCL | 0644);
    if (gallery_sem == -1) {
        printError("Could not create semaphore for gallery");
    }
    // Set gallery_sem value to 49 (max number of visitors)
    semctl(gallery_sem, 0, SETVAL, 49);

    // Initialize the semaphores for the pictures
    picture_sems = semget(picture_sems_id, 5, IPC_CREAT | IPC_EXCL | 0644);
    if (picture_sems == -1) {
        printError("Could not create semaphore for picture");
    }
    for (int i = 0; i < 5; i++) {
        // Set the value of the semaphore to 10 (max number of visitors)
        semctl(picture_sems, i, SETVAL, 9);
    }

    for (int visitor = 0; visitor < number_of_visitors; ++visitor) {
        // Create a new visitor process
        int visitor_process = fork();

        if (visitor_process == -1) {
            printError("Could not create visitor process");
        } else if (visitor_process == 0) {
            // Visitor process
            // Wait for the gallery semaphore to be available
            operation = setSemOperation(0, -1, 0);
            semop(gallery_sem, &operation, 1);
            // printf("Visitor %d entered the gallery.\n", getpid());

            for (int picture = 0; picture < 5; ++picture) {
                // Wait for the semaphore of the picture to be available
                operation = setSemOperation(picture, -1, 0);
                semop(picture_sems, &operation, 1);

                // printf("Visitor %d is looking at picture %d\n", getpid(), picture + 1);
                sleep(getRandomNumber(5, 10));

                // Release the semaphore of the picture
                operation = setSemOperation(picture, 1, 0);
                semop(picture_sems, &operation, 1);
            }

            // Release the gallery semaphore
            operation = setSemOperation(0, 1, 0);
            semop(gallery_sem, &operation, 1);

            // printf("Visitor %d left the gallery.\n", getpid());

            // Exit the visitor process
            printGalleryInfo();
            return 0;
        }
    }

    for (int visitor = 0; visitor < number_of_visitors; ++visitor) {
        // Parent process
        // Wait for the visitor process to finish
        wait(NULL);
        // Delete all semaphores
    }

    printGalleryInfo();

    deleteAllSemaphores();
    printf("Visitor finished. Closing the gallery.\n");

    return 0;
}
