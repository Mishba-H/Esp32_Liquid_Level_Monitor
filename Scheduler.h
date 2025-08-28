#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

namespace Scheduler {
    // A function pointer type for our tasks
    typedef void (*command)();

    // You can change this to allow more scheduled tasks
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

    /**
     * @brief Adds a task to the scheduler queue.
     * @tparam c The function to be executed.
     * @param wait_ms The interval in milliseconds between executions.
     */
    template <command c>
    void addTask(unsigned long wait_ms) {
        // Only add the task if there is space in the queue
        if (taskCount < MAX_TASKS) {
            taskQueue[taskCount].function = c;
            taskQueue[taskCount].interval = wait_ms;
            taskQueue[taskCount].lastRun = millis(); // Set initial timestamp
            taskCount++;
        }
        // Optional: Add an else condition to print an error if the queue is full
    }

    /**
     * @brief Checks all tasks and runs any that are due.
     * This should be called in your main loop().
     */
    void update() {
        unsigned long now = millis();
        // Iterate over all registered tasks
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