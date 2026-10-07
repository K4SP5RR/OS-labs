#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

const int WORKING_TIME = 10;

typedef struct {
    int id;
    char *message;
} Event;

pthread_mutex_t mutex;
pthread_cond_t condition;
bool ready = false;
bool stop = false;
void* data = NULL;

void* producer(void* arg){
    int id = 1;
    while (true){
        pthread_mutex_lock(&mutex);
        if (stop){
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (!ready){
            Event* event = malloc(sizeof(Event));
            event->id = id++;
            event->message = "Producer";
            data = event;
            ready = true;
            printf("Producer: event provided\n");
            pthread_cond_signal(&condition);
        }
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
    return NULL;
}

void* consumer(void* arg){
    while (true){
        pthread_mutex_lock(&mutex);
        while(!ready && !stop){
            pthread_cond_wait(&condition, &mutex);
        }
        if (stop && !ready){
            pthread_mutex_unlock(&mutex);
            break;
        }
        Event *event = (Event *)data;
        data = NULL;
        ready = false;
        printf("Consumer: event %d consumed: %s\n",event->id,event->message);
        free(event);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main()
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);

    pthread_t producer_th;
    pthread_t consumer_th;
    pthread_create(&producer_th, NULL, producer, NULL);
    pthread_create(&consumer_th, NULL, consumer, NULL);

    sleep(WORKING_TIME);
    pthread_mutex_lock(&mutex);
    stop = true;
    pthread_cond_broadcast(&condition);
    pthread_mutex_unlock(&mutex);

    pthread_join(producer_th, NULL);
    pthread_join(consumer_th, NULL);

    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);

    return 0;
}
