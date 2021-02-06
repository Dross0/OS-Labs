#include <queue>
#include <semaphore.h>
#include <pthread.h>

class MyQueue {
    std::queue<char *> storage;
    sem_t full;
    sem_t empty;
    pthread_mutex_t block;
    bool dropped;

public:
    MyQueue();

    virtual ~MyQueue();

    bool isDropped();

    int mymsgput(const char *msg);

    int mymsgget(char *buf, size_t bufsize);

    void mymsqdrop();
};

