#include <queue>
#include <string>
#include <pthread.h>

using namespace std;

class MyQueue {
    std::queue<char *> storage;
    pthread_cond_t new_element;
    pthread_cond_t empty_element;
    pthread_mutex_t get_mutex;
    pthread_mutex_t put_mutex;
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
