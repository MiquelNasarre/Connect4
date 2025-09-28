#include "Thread.h"
#include <windows.h>

// THREAD CLASS SOURCE FILE
// This file will define all the Thread.h functions 
// using the win32 API for threading.

/*
-------------------------------------------------------------------------------------------------------
Constructors and destructors
-------------------------------------------------------------------------------------------------------
*/

// Destructor, detaches the thread if it exists.

Thread::~Thread()
{
    detach();
}

// Constructor copies other's values and resets them.

Thread::Thread(Thread&& other) noexcept
{
    thread_handle_ = other.thread_handle_;
    os_thread_id_ = other.os_thread_id_;
    exit_code_valid_ = other.exit_code_valid_;
    last_exit_code_ = other.last_exit_code_;

    other.thread_handle_ = nullptr;
    other.os_thread_id_ = 0UL;
    other.exit_code_valid_ = false;
}

// Detaches if it has a handles and
// copies other's values and resets them.

Thread& Thread::operator=(Thread&& other) noexcept
{
    if (this != &other)
    {
        detach();

        thread_handle_ = other.thread_handle_;
        os_thread_id_ = other.os_thread_id_;
        exit_code_valid_ = other.exit_code_valid_;
        last_exit_code_ = other.last_exit_code_;

        other.thread_handle_ = nullptr;
        other.os_thread_id_ = 0UL;
        other.exit_code_valid_ = false;
    }
    return *this;
}

// This is the actual start function that calls the thread creation.
// It takes the arguments as expected by CreateThread().

/*
-------------------------------------------------------------------------------------------------------
Active thread functions
-------------------------------------------------------------------------------------------------------
*/

bool Thread::start_raw(unsigned long(__stdcall* proc)(void*), void* args)
{
    thread_handle_ = CreateThread(
        NULL,
        0ULL,
        proc,
        args,
        suspended ? CREATE_SUSPENDED : 0UL,
        &os_thread_id_
    );

    if (thread_handle_)
        return true;

    return false;
}

// Waits for the thread to end to continue.
// So it joins its timeline for a certain time.

bool Thread::join(unsigned long timeout_ms)
{
    if (!thread_handle_) return false;

    DWORD w = WaitForSingleObject((HANDLE)thread_handle_, timeout_ms);

    if (w == WAIT_OBJECT_0) 
    {
        DWORD code = 0;
        GetExitCodeThread((HANDLE)thread_handle_, &code);
        last_exit_code_ = (ExitCode)code;
        exit_code_valid_ = true;
        CloseHandle((HANDLE)thread_handle_);
        thread_handle_ = nullptr;
        os_thread_id_ = 0;
        return true;
    }
    return false; // WAIT_TIMEOUT / WAIT_FAILED
}

// Closes the handle and lets the thread continue independently.

void Thread::detach()
{
    if (thread_handle_)
    {
        CloseHandle(thread_handle_);
        thread_handle_ = nullptr;
        os_thread_id_ = 0;

        last_exit_code_ = THREAD_DETACHED;
        exit_code_valid_ = true;
    }
}

// If it has a handle calls ResumeThread.
// Sets suspended to false for start_raw().

bool Thread::resume()
{
    suspended = false;
    return thread_handle_ && ResumeThread(thread_handle_) != (DWORD)-1;
}

// If it has a handle callse SuspendThread.
// Sets suspended to true for start_raw().

bool Thread::suspend()
{
    suspended = true;
    return thread_handle_ && SuspendThread(thread_handle_) != (DWORD)-1;
}

// Hard kill (strongly discouraged). Prefer cooperative stop.
// Its better if you enter a variable in the thread to call stop.

bool Thread::terminate() {
    if (!thread_handle_) 
        return false;

    if (!TerminateThread((HANDLE)thread_handle_, 0UL)) 
        return false;

    last_exit_code_ = THREAD_TERMINATED;
    exit_code_valid_ = true;

    CloseHandle((HANDLE)thread_handle_);
    thread_handle_ = nullptr;
    os_thread_id_ = 0;
    return true;
}

// Sets the name of the thread to your provided name, which can 
// be seen in the debugger, task manager, etc.

bool Thread::set_name(const wchar_t* name)
{
    if (!thread_handle_ || !name) return false;

    // Available on Windows 10 Anniversary Update (1607, build 14393) and later.
    // OK to call unconditionally; returns HRESULT.
    typedef HRESULT(WINAPI* SetThreadDescriptionFn)(HANDLE, PCWSTR);
    static SetThreadDescriptionFn pSetThreadDescription =
        (SetThreadDescriptionFn)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription");

    if (!pSetThreadDescription) return false;

    return SUCCEEDED(pSetThreadDescription((HANDLE)thread_handle_, name));
}

// Changes the thread’s dynamic priority within the process’s priority.

bool Thread::set_priority(const PriorityLevel level)
{
    if (!thread_handle_) return false;

    switch (level)
    {
    case PRIORITY_LOWEST:
        return SetThreadPriority((HANDLE)thread_handle_, THREAD_PRIORITY_LOWEST) != 0;

    case PRIORITY_BELOW_NORMAL:
        return SetThreadPriority((HANDLE)thread_handle_, THREAD_PRIORITY_BELOW_NORMAL) != 0;

    case PRIORITY_NORMAL:
        return SetThreadPriority((HANDLE)thread_handle_, THREAD_PRIORITY_NORMAL) != 0;

    case PRIORITY_ABOVE_NORMAL:
        return SetThreadPriority((HANDLE)thread_handle_, THREAD_PRIORITY_ABOVE_NORMAL) != 0;

    case PRIORITY_HIGHEST:
        return SetThreadPriority((HANDLE)thread_handle_, THREAD_PRIORITY_HIGHEST) != 0;

    default:
        return false;
    }
}

// Sets the logical CPU's this thread is allowed to use in your machine
// The masks are coded as 1ULL << #CPU.

bool Thread::set_affinity(unsigned long long mask)
{
    if (!thread_handle_) 
        return false;

    DWORD_PTR pm = 0, sm = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &pm, &sm)) 
        return false;

    if ((mask == 0) || ((mask & pm) != mask)) 
        return false;

    return SetThreadAffinityMask((HANDLE)thread_handle_, (DWORD_PTR)mask) != 0;
}

/*
-------------------------------------------------------------------------------------------------------
Class helper functions
-------------------------------------------------------------------------------------------------------
*/

// Checks whether the Thread object holds a handle.

bool Thread::is_joinable() const
{
    return thread_handle_ != nullptr;
}

// Checks whether the GetExitCode returns STILL_ACTIVE.

bool Thread::is_running() const
{
    if (!thread_handle_) 
        return false;

    DWORD code = 0UL;
    if (!GetExitCodeThread((HANDLE)thread_handle_, &code)) 
        return false;

    if (code == STILL_ACTIVE)
        return true;

    return false;
}

// Checks whether the GetExitCode returns STILL_ACTIVE.
// If no handle it checks for exit_code_valid.

bool Thread::has_finished() const
{
    if (!thread_handle_)
        return exit_code_valid_; // joined earlier -> finished if we cached code

    DWORD code = 0UL;
    if (!GetExitCodeThread((HANDLE)thread_handle_, &code))
        return false;

    if (code == STILL_ACTIVE)
        return false;

    return true;
}

// If avaliable or joined recently returns exit code.
// If not returns EXIT_CODE_INVALID.

Thread::ExitCode Thread::get_exit_code() const
{
    if (thread_handle_) {
        DWORD code = 0;
        if (GetExitCodeThread((HANDLE)thread_handle_, &code)) 
            return (ExitCode)code;

        return EXIT_CODE_INVALID;
    }
    return exit_code_valid_ ? last_exit_code_ : EXIT_CODE_INVALID;
}

// Returns the handle to the thread

void* Thread::get_native_handle() const
{
    return thread_handle_;
}

// Returns the OS thread ID

unsigned long Thread::get_id() const
{
    return os_thread_id_;
}

/*
-------------------------------------------------------------------------------------------------------
Static class functions
-------------------------------------------------------------------------------------------------------
*/

// Wrap current thread with a real handle (cannot be joined).
// It can be used to generate a Thread object inside any thread.

Thread Thread::from_current()
{
    Thread current_thread;
    HANDLE pseudo = GetCurrentThread();      // pseudo-handle (no CloseHandle)
    DuplicateHandle(GetCurrentProcess(), pseudo,
        GetCurrentProcess(), &current_thread.thread_handle_, 0, FALSE, DUPLICATE_SAME_ACCESS);
    current_thread.os_thread_id_ = GetCurrentThreadId();
    return current_thread;
}
