#pragma once

/* THREAD CLASS HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
In this file you can find my take on writing a wrapper for threading.
While the source file uses the intended win32 functions this header file
is absolutely clean of anything.

The thread generation is templated, so it can take any function with 
any arbitrary amout of arguments please beware that the function needs
to match the arguments provided.

DISCLAIMER!! Make sure not to use references variables inside the functions
since it does not take references, it will just make a copy of them.
You should use pointers instead for shared variables between threads.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Definition of the class, everything in this header is contained inside the class Thread.
// A thread object can be generated anywhere in your code, and a different thread can be
// created by simply calling the function start(*function, function args...).
// This thread can then be controlled by other functions inside the class.
// But control through the input variables is always preferred.

class Thread {
private:

    // Class variables

    void* thread_handle_ = nullptr;     // OS handle (nullptr if not running / detached)

    unsigned long  os_thread_id_ = 0UL; // OS thread ID

    bool suspended = false;             // Stores whether the thread is suspended for start call

public:
    enum ExitCode : unsigned long
    {
        ENDED_SUCCESSFULLY  = 0,
        EXCEPTION_CATCHED   = 1,
        THREAD_TERMINATED   = 2,
        THREAD_DETACHED     = 3,
        EXIT_CODE_INVALID   = 4,
        STILL_ACTIVE        = 259,
    };
private:
    ExitCode last_exit_code_ = STILL_ACTIVE;   // Remembers last exit code in case of joining

    bool exit_code_valid_ = false;      // Whether last_exit_code is valid

    // Threads cannot be simply copied or initialized by anther thread.

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    // Templated function in the format desired for Win32 API for thread creation.
    // a void* to the function is its arguments, it returns a unsigned long,
    // 0 if everything ok, 1 if it catched an exception.

    template<typename hFunction>
    static unsigned long _stdcall trampoline(void* pheap_func)
    {
        hFunction* pfunction = static_cast<hFunction*>(pheap_func);
        unsigned long rc = ENDED_SUCCESSFULLY;
        try { 
            (*pfunction)(); 
        } catch (...) { 
            rc = EXCEPTION_CATCHED; 
        }
        delete pfunction;
        return rc;
    }

    // This is the actual start function that calls the thread creation.
    // It takes the arguments as expected by CreateThread().

    bool start_raw(unsigned long(__stdcall* proc)(void*), void* args);

public:

    // Default constructor without creating any thread

    Thread() = default;

    // Actual copy and constructor functions using another Thread object.
    // Detaches before copying and erases the handle and ID on the copied thread.

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    // Destructor, detaches the thread if it exists.

    ~Thread();

    // Templated constructor that calls the start function

    template<typename Proc, typename... Args>
    Thread(Proc proc, Args... args)
    {
        start(proc, args...);
    }

    // Templated start function, it takes any function and any amount of arguments.
    // Then it creates a wrap with it and a pointer to the wrap, which sends to the
    // trampoline that mimics the arguments expected by CreateThread so it can be 
    // sent to the start_raw function. Returns true if the creation is successful.

    template<typename Proc, typename... Args>
    inline bool start(Proc proc, Args... args) {

        if (thread_handle_)
            return false;
        exit_code_valid_ = false;

        // 0) Build a zero-arg callable that closes over proc/args BY VALUE.
        auto bound = [=]() { proc(args...); };

        // 1) Allocate a stable heap copy so it outlives this stack frame.
        using hFunction = decltype(bound);
        hFunction* pheap_function = new hFunction(bound);  // one copy; OK for small lambdas

        // 2) Start the thread via the Win32-ABI trampoline.
        const bool ok = start_raw(&trampoline<hFunction>, pheap_function);

        // 3) On failure, clean up the heap object to avoid a leak.
        if (!ok) {
            delete pheap_function;
        }
        return ok;
    }

    // Calls the one above but makes sure the thread starts suspended.

    template<typename Proc, typename... Args>
    inline bool start_suspended(Proc proc, Args... args) {
        suspended = true;
        bool ok = start(proc, args...);
        if (!ok) suspended = false; // restore on failure
        return ok;
    }

    // Waits for the thread to end to continue.
    // So it joins its timeline for a certain time.
    // The default value means infinite time.

    bool join(unsigned long timeout_ms = 0xFFFFFFFFUL);

    // Closes the handle and lets the thread continue independently.

    void detach();

    // Resume/Suspend: available but **unsafe** for synchronization.
    // If you use suspend before start or start_suspended the thread
    // will only start after calling resume.

    bool resume();
    bool suspend();

    // Sets the name of the thread to your provided name, which can 
    // be seen in the debugger, task manager, etc.

    bool set_name(const wchar_t* name);

    // Changes the thread’s dynamic priority within the process’s priority.

    enum PriorityLevel : int
    {
        PRIORITY_LOWEST         = -2,
        PRIORITY_BELOW_NORMAL   = -1,
        PRIORITY_NORMAL         = 0,
        PRIORITY_ABOVE_NORMAL   = 1,
        PRIORITY_HIGHEST        = 2,
    };
    bool set_priority(const PriorityLevel level);

    // Sets the logical CPU's this thread is allowed to use in your machine
    // The masks are coded as 1ULL << #CPU, for example if I want this thread
    // to use CPU 0 and 2, I would write mask = (1ULL << 0) | (1ULL << 2) 

    bool set_affinity(unsigned long long mask);

    // Hard kill (strongly discouraged). Prefer cooperative stop.
    // Its better if you enter a variable in the thread to call stop.

    bool terminate();

    // Helpers

    bool            is_joinable() const;
    bool            is_running() const;
    bool            has_finished() const;
    ExitCode        get_exit_code() const;

    void*           get_native_handle() const;
    unsigned long   get_id() const;

    // Wrap current thread with a real handle (cannot be joined).
    // It can be used to generate a Thread object inside any thread.

    static Thread from_current();
};

