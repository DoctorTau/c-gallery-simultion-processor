#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <pthread.h>

#define NUMBER_OF_PICTURES 5

sem_t gallery_sem;
sem_t picture_sems[NUMBER_OF_PICTURES];

sem_t *gallery_sem_pointer = &gallery_sem;
sem_t *picture_sems_pointers[NUMBER_OF_PICTURES];

/// @brief Closes all semaphores.
void closeAllSems() {
    sem_destroy(gallery_sem_pointer);
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        sem_destroy(picture_sems_pointers[i]);
    }
}

/// @brief Handles SIGINT signal.
/// @param signal signal number.
void stopSignalHandler(int signal) {
    if (signal == SIGINT) {
        printf("SIGINT received. Exiting.\n");
        closeAllSems();
        exit(0);
    }
}

/// @brief Prints error message and exits.
/// @param message error message.
void printError(char *message) {
    printf("Error: %s\nExiting", message);
    closeAllSems();
    exit(1);
}

/// @brief Generates random number in range [min, max].
/// @param min minimum value.
/// @param max maximum value.
/// @return random number.
int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

/// @brief Clears console.
void clearConsole() {
    printf("\033[H\033[J");
}

/// @brief Prints gallery info.
void printGalleryInfo() {
    clearConsole();
    int number_of_visitors;

    if (sem_getvalue(gallery_sem_pointer, &number_of_visitors) == -1) {
        perror("Could not get value of gallery semaphore.");
        exit(EXIT_FAILURE);
    }

    printf("Gallery info:\n");
    printf("There are %d visitors in the gallery.\n", 50 - number_of_visitors);

    int number_of_visitors_in_picture;
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        if (sem_getvalue(picture_sems_pointers[i], &number_of_visitors_in_picture) == -1) {
            perror("Could not get value of picture semaphore.");
            exit(EXIT_FAILURE);
        }
        printf("There are %d visitors looking at picture %d.\n", 10 - number_of_visitors_in_picture,
               i + 1);
    }
}

/// @brief Checks if all values in array are true.
/// @param val array of bools.
/// @return true if all values are true, false otherwise.
bool isAllTrue(const bool *val) {
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        if (!val[i]) {
            return false;
        }
    }
    return true;
}

/// @brief The behavior of a visitor in it's process.
/// @param arg
/// @return
void *visitorBehavior(void *arg) {
    bool visited_pictures[NUMBER_OF_PICTURES];
    int time_to_stay = getRandomNumber(1, 5);

    // Wait for the gallery to be free
    sem_wait(gallery_sem_pointer);

    for (;;) {
        int picture_number = getRandomNumber(0, NUMBER_OF_PICTURES - 1);
        // Wait for the picture to be free
        sem_wait(picture_sems_pointers[picture_number]);

        // Look at the picture
        sleep(time_to_stay);
        // printGalleryInfo();

        // Leave the picture
        sem_post(picture_sems_pointers[picture_number]);

        // Mark the picture as visiteD
        visited_pictures[picture_number] = true;

        // If all pictures have been visited, leave the gallery
        if (isAllTrue(visited_pictures)) {
            break;
        }
    }
    printGalleryInfo();
    // Leave the gallery
    sem_post(gallery_sem_pointer);

    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    closeAllSems();
    (void)signal(SIGINT, stopSignalHandler);

    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        picture_sems_pointers[i] = &picture_sems[i];
    }

    int number_of_visitors;
    // Check if the number of visitors was given as an argument
    if (argc == 2) {
        number_of_visitors = atoi(argv[1]);
    } else {
        printError("Number of visitors was not given as an argument.");
    }

    printf("Number of visitors: %d\n", number_of_visitors);

    pthread_t threads[number_of_visitors];

    // Create semaphores
    if (sem_init(gallery_sem_pointer, 1, 50) == -1) {
        printError("Could not create gallery semaphore.");
    }

    // Init picture semaphores
    for (int i = 0; i < NUMBER_OF_PICTURES; i++) {
        if (sem_init(picture_sems_pointers[i], 1, 10) == -1) {
            printError("Could not create picture semaphore.");
        }
    }

    // Print initial gallery info.
    printGalleryInfo();

    // Create visitors
    for (int i = 0; i < number_of_visitors; i++) {
        pthread_create(&threads[i], NULL, visitorBehavior, NULL);
    }

    // Wait for all visitors to leave
    for (int i = 0; i < number_of_visitors; i++) {
        pthread_join(threads[i], NULL);
    }

    printGalleryInfo();
    closeAllSems();
    return 0;
}