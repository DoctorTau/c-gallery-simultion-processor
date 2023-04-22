# Использование семафоров

## Выполнил

Струдент группы БПИ212: **Шатравка Даниил Александрович**

## Условие задачи

Задача о картинной галерее. Вахтер–процесс следит за тем,
чтобы в картинной галерее одновременно было не более 50 процессов посетителей. Для обозрения представлены 5 картин. Посетитель
случайным образом ходит от картины к картине, и если на картину любуются более чем десять посетителей, он стоит в стороне
и ждет, пока число желающих увидеть картину не станет меньше.
Посетитель может покинуть галерею, когда просмотрит все картины. В галерею также пытаются постоянно зайти новые посетители, которые ожидают своей очереди, если та заполнена. Создать
приложение, моделирующее однодневную работу картинной галереи. Время нахождения возле картины для каждого посетителя является случайной величиной в некотором диапазоне.

## Решение

Для решения задачи использовался язык программирования C.

---

## Общая схема работы для решения задачи на 4-6

Создается 6 семафоров:

1. `gallery_sem` - семафор контролирующий доступ к галерее. Максимальное количество посетителей в галерее - 50.
2. 5 семафров `picture_sem` - семафоры контролирующие доступ к картинам. Максимальное количество посетителей возле картин - 10.

При запуске программы через коммандную строку передается количество писетителей. Далее созданется `n` процессов посетителей с помощью `fork()`.

Посетители пытаются войти в галерею. Войдя в галерею, посетитель уменьшает семафор `gallery_sem` на 1. После этого он выбирает случайную картину и уменьшает семафор `picture_sem` на 1. После этого он увеличивает семафор `gallery_sem` на 1. После этого он увеличивает семафор `picture_sem` на 1. После этого он покидает галерею.

И так пока не закончатся посетители.

## На 4 балла

Сначала инициализируются семафоры:

```c
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
```

После этого создаются `n` процессов посетителей:

В каждом процессе посетителя происходит следующее:

1. Посетитель пытается войти в галерею. Для этого он уменьшает семафор `gallery_sem` на 1.
2. Выбирает случайную картину и уменьшает семафор `picture_sem` на 1.
3. Пользователь ждет случайное время (от 5 до 10 секунд).
4. Пользователь увеличивает семафор `gallery_sem` на 1.
5. Картина помечается как посещенная.
6. Если пользователь посетил все картины - он покидает галерею. Иначе он выбирает случайную картину и уменьшает семафор `picture_sem` на 1.

Код процесса посетителя:

```c
bool visited[number_of_pictures];

            // Visitor process
            // Wait for the gallery semaphore to be available
            operation = setSemOperation(0, -1, 0);
            semop(gallery_sem, &operation, 1);

            for (;;) {
                int picture = getRandomNumber(0, number_of_pictures - 1);
                // Wait for the semaphore of the picture to be available
                operation = setSemOperation(picture, -1, 0);
                semop(picture_sems, &operation, 1);

                // printf("Visitor %d is looking at picture %d\n", getpid(), picture + 1);
                sleep(getRandomNumber(5, 10));

                // Release the semaphore of the picture
                operation = setSemOperation(picture, 1, 0);
                semop(picture_sems, &operation, 1);

                visited[picture] = true;
                if (isAllTrue(visited)) {
                    break;
                }
            }

            // Release the gallery semaphore
            operation = setSemOperation(0, 1, 0);
            semop(gallery_sem, &operation, 1);

            // printf("Visitor %d left the gallery.\n", getpid());

            // Exit the visitor process
            printGalleryInfo();
            return 0;
```


В конце работы программы удаляются семафоры:

```c
void deleteAllSemaphores() {
    semctl(gallery_sem, 0, IPC_RMID);
    for (int i = 0; i < number_of_pictures; i++)
        semctl(picture_sems, i, IPC_RMID);
}
```