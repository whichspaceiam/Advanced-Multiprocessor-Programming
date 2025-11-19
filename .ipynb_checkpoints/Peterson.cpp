#include <iostream>
#include <thread>

class Peterson
{
private:
// is this thread interested in the critical section
// Needs to be volatile to prevent caching in registers
    volatile int interested[2] = {0,0};

    // who's turn is it?
    // Needs to be volatile to prevent caching in registers
    volatile int turn = 0;
public:
// Method for locking w/ Peterson's alogrithm
    void lock(int tid){
        //mark that this thread wants to enter the critical section
        interested[tid] = 1;
        //assume the other thread has priority
        int other = 1 - tid;
        turn = other;
        //wait until the other thread finishes or is not interested
        while(turn == other && interested[other]);

    }
    //method for unlocking w/ Peterson's algorithm
    void unlock(int tid){
        //Mark that this thread is no longer interested
        interested[tid] = 0;
    }
};


//work function
void work(Peterson &p, int &val, int tid){
    for(int i=0; i<100000000; i++){
        //lock using peterson's algorithm
        p.lock(tid);
        //critical section
        val++;
        //unlock using peterson's algorithm
        p.unlock(tid);
    }
}

int main(){
    //shared value
    int val = 0;
    Peterson p;

    // create threads
    std::thread t0([&] {work(p,val,0);});
    std::thread t1([&] {work(p,val,1);});

    //wait for the thread to finish
    t0.join();
    t1.join();

    //print the result
    std::cout << "final value is:" << val <<"\n";
    return 0;
}