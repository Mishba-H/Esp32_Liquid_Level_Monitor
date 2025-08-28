#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

namespace Scheduler {
    typedef void (*command)();

    const int MAX_TASKS = 10;

    // A structure to hold all the info about a single task
    struct Task {
        command function;         // Pointer to the function to be called
        unsigned long interval;   // The interval in milliseconds
        unsigned long lastRun;    // The timestamp of the last execution
    };

    // The queue that holds all of our tasks
    Task taskQueue[MAX_TASKS];
    // The number of tasks currently in the queue
    int taskCount = 0;

    template <command c>
    void addTask(unsigned long wait_ms) {
        if (taskCount < MAX_TASKS) {
            taskQueue[taskCount].function = c;
            taskQueue[taskCount].interval = wait_ms;
            taskQueue[taskCount].lastRun = millis(); // Set initial timestamp
            taskCount++;
        }
    }

    void update() {
        unsigned long now = millis();
        for (int i = 0; i < taskCount; i++) {
            // Check if the interval has passed
            if (now - taskQueue[i].lastRun >= taskQueue[i].interval) {
                // Update the last run time. Using += prevents timing drift.
                taskQueue[i].lastRun += taskQueue[i].interval;
                // Execute the task function
                taskQueue[i].function();
            }
        }
    }
}

#endif