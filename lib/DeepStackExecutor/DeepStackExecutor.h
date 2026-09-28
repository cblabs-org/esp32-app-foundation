/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/DeepStackExecutor/DeepStackExecutor.h
 * Description: Reusable framework service that runs an arbitrary callable on a
 *              persistent worker task with a PSRAM-backed static stack. Decouples
 *              deep-call-stack work from shared tasks.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <functional>

/**
 * @brief Executes arbitrary callables on a persistent worker task with a PSRAM-backed stack.
 *
 * Designed to offload memory-intensive deep call stack work (e.g., compression) from
 * shared tasks like httpd. The worker is created once and persists. Callables are
 * executed synchronously via a blocking run() method.
 *
 * If PSRAM stack allocation fails or is unsupported (e.g., CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY
 * is off), begin() returns false and available() returns false, allowing the caller to
 * fall back to a shallower execution path.
 *
 * @note Architecture & DRAM tuning details: see .agents/skills/framework-expert/systems/web-portal/dram-tuning.md
 *       and docs/developer/performance-guide.md
 */
class DeepStackExecutor {
public:
    /**
     * @brief Construct a new DeepStackExecutor.
     *
     * @param stackBytes Worker stack depth. Defaults to 16384 (BOARD_DEEPSTACK_BYTES fallback).
     * @param name       FreeRTOS task name for debugging.
     */
    explicit DeepStackExecutor(size_t stackBytes = 16384, const char* name = "deepstack");

    /**
     * @brief Destructor. Deletes the task and frees the caps-allocated stack (used in tests).
     */
    ~DeepStackExecutor();

    /**
     * @brief Lazily allocate the PSRAM stack and create the persistent worker task.
     *
     * @return true if the worker was created successfully on a PSRAM stack.
     */
    bool begin();

    /**
     * @brief Check if the persistent worker task is available.
     *
     * @return true iff the worker is running on a PSRAM stack.
     */
    bool available() const;

    using WorkFn = void (*)(void* ctx);

    /**
     * @brief Run a callable on the worker task and block until completion or timeout.
     *
     * Serialized: only one job executes at a time. Concurrent callers are blocked.
     * If the timeout expires before the job completes, the executor remains marked busy
     * until the in-flight job finishes, and subsequent run() calls will return false.
     *
     * @param work      The std::function containing the work to execute.
     * @param timeoutMs Maximum time to wait for the work to complete, in milliseconds.
     * @return true if the work completed successfully within the timeout.
     */
    bool run(const std::function<void()>& work, uint32_t timeoutMs);

    /**
     * @brief Run a function pointer with context on the worker task and block until completion or timeout.
     *
     * @param work      Function pointer to execute.
     * @param ctx       Context passed to the function pointer.
     * @param timeoutMs Maximum time to wait for the work to complete, in milliseconds.
     * @return true if the work completed successfully within the timeout.
     */
    bool run(WorkFn work, void* ctx, uint32_t timeoutMs);

    /**
     * @brief Get the minimum remaining free stack space since task creation.
     *
     * Useful for empirical stack sizing.
     *
     * @return size_t Free stack space in bytes.
     */
    size_t stackHighWaterMarkBytes() const;

    /**
     * @brief Block until all registered DeepStackExecutor instances are idle.
     *
     * Acquires and immediately releases the mutex of every active instance.
     * Used by OTA to prevent TOCTOU flash write collisions.
     */
    static void awaitAllIdle();

    // Non-copyable
    DeepStackExecutor(const DeepStackExecutor&) = delete;
    DeepStackExecutor& operator=(const DeepStackExecutor&) = delete;

private:
    size_t _stackBytes;
    const char* _name;
    bool _available = false;
    
#ifndef UNIT_TEST
    void* _taskHandle = nullptr;
    void* _mutex = nullptr;
    void* _doneSem = nullptr;
    WorkFn _workFn = nullptr;
    void* _workCtx = nullptr;
    bool _busy = false;

    static void workerLoop(void* param);
#else
public:
    // Stub members for native testing
    bool _stubAvailable = true;
    size_t _stubHighWaterMark = 1024;
#endif
};
