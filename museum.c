#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// print info
#define DEBUG 1

#define MAX_A 1
#define MAX_B 1
#define THREADS 20

// Arbitrary choices
#define WALK_TIME 1
#define WATCH_TIME (WALK_TIME + 1)

int count_a;
int count_b;

// Mutex shared by both halls
pthread_mutex_t mutex;

pthread_cond_t cond_a;
pthread_cond_t cond_b;

void debug_print()
{
    if (!DEBUG) {
        return;
    }
    printf("A: %d/%d; B: %d/%d\n", count_a, MAX_A, count_b, MAX_B);
}

// continuous sleep
void random_sleep(double seconds)
{
    int microseconds;
    if (seconds <= 0.0)
    {
        microseconds = 0.0;
    }
    else
    {
        microseconds = rand() % (int)(seconds * 1000000);
    }
    // 10^-6
    usleep(microseconds);
    // printf("%f\n", ((double)microseconds)/1000000);
}

void walk_a(int id)
{
    random_sleep(WALK_TIME + 1);
    printf("Person %d finished walking through Hall A\n", id);
}

void watch_a(int id)
{
    random_sleep(WATCH_TIME + 1);
    printf("Person %d finished viewing Hall A\n", id);
}

void watch_b(int id)
{
    random_sleep(WATCH_TIME + 1);
    printf("Person %d finished viewing Hall B\n", id);
}

void enter_hall_a(int id)
{
    count_a++;
    printf("Person %d enters Hall A\n", id);
    debug_print();
}

void enter_hall_b(int id)
{
    count_b++;
    printf("Person %d enters Hall B\n", id);
    debug_print();
}

void leave_hall_a(int id)
{
    count_a--;
    printf("Person %d leaves Hall A\n", id);
}

void leave_hall_b(int id)
{
    count_b--;
    printf("Person %d leaves Hall B\n", id);
}

void leave_museum(int id)
{
    printf("Person %d leaves the museum\n", id);
}

void enter_museum_a(void *arg)
{
    int id = *((int *)arg);
    // critical
    pthread_mutex_lock(&mutex);
    /*
        Assume we cannot predict the following:
            1. which people either entering or having entered A will proceed to B
            2. the time it takes to:
                a. visit A
                b. visit B
                c. walk out of B through A
        We therefore treat equally those wishing to enter, as well as those still in B
        by leaving space for all exiting people.
        This ensures we optimise for both:
            1. visiting the museum at the same time by as many people as possible
            2. leaving the museum by people visiting hall B in the shortest possible time
    */
    while (count_a + count_b >= MAX_A)
    {
        pthread_cond_wait(&cond_a, &mutex);
    }
    enter_hall_a(id);

    pthread_mutex_unlock(&mutex);

    // processing
    watch_a(id);

    // critical
    pthread_mutex_lock(&mutex);

    leave_hall_a(id);
    leave_museum(id);

    pthread_cond_signal(&cond_a);

    pthread_mutex_unlock(&mutex);
}

void enter_museum_b(void *arg)
{
    int id = *((int *)arg);
    // critical
    pthread_mutex_lock(&mutex);

    while (count_a + count_b >= MAX_A)
    {
        pthread_cond_wait(&cond_a, &mutex);
    }
    enter_hall_a(id);
    pthread_mutex_unlock(&mutex);

    // processing
    watch_a(id);

    // critical
    pthread_mutex_lock(&mutex);

    while (count_b >= MAX_B)
    {
        pthread_cond_wait(&cond_b, &mutex);
    }
    leave_hall_a(id);
    enter_hall_b(id);

    pthread_cond_signal(&cond_a);
    pthread_mutex_unlock(&mutex);

    // processing
    watch_b(id);

    // critical
    pthread_mutex_lock(&mutex);

    while (count_a >= MAX_A)
    {
        pthread_cond_wait(&cond_a, &mutex);
    }
    leave_hall_b(id);
    enter_hall_a(id);

    pthread_cond_signal(&cond_b);
    pthread_mutex_unlock(&mutex);

    // exit
    walk_a(id);

    // critical
    pthread_mutex_lock(&mutex);

    leave_hall_a(id);
    leave_museum(id);

    pthread_cond_signal(&cond_a);
    pthread_mutex_unlock(&mutex);
}

int main()
{
    srand(time(NULL));
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_a, NULL);
    pthread_cond_init(&cond_b, NULL);

    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
    {
        int *id = malloc(sizeof(int));
        *id = i;
        /*
            Assume we do know know which guests will decide to visit hall B,
            so we pick at random and do not schedule processes based on the resulting information.
        */
        void *function = (rand() % 2 == 0) ? enter_museum_a : enter_museum_b;
        pthread_create(&threads[i], NULL, function, id);
    }
    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_a);
    pthread_cond_destroy(&cond_b);

    printf("The museum is empty\n");

    return 0;
}
