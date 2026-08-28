# Exploit Write-Up: Level04 (Ret2Libc with Ptrace Bypass)

This document provides a comprehensive technical breakdown of the vulnerabilities and mechanics used to solve Level04. It covers the stack architecture, the exploitation methodology, the process isolation quirk that made the bypass possible, and the true purpose of the `prctl` function found within the source code.

## Executive Summary

The objective of this level is to redirect the application's execution flow to spawn an interactive shell (`/bin/sh`). The binary employs a custom process-monitoring loop via `ptrace` intended to catch and terminate any child process executing an `execve` system call (Syscall 11).

By leveraging a standard Return-to-libc (Ret2Libc) technique to call `system()`, the exploit forces the creation of an untraced grandchild process, entirely bypassing the parental security monitoring logic.

## 1. Vulnerability Analysis & Memory Layout

The binary contains a standard stack-based buffer overflow. By writing structured data beyond the bounds of the destination buffer, we can cleanly overwrite the Saved EIP (the Return Address) of the current stack frame.

### The 32-bit x86 Calling Convention

In a 32-bit architecture, the processor relies strictly on the stack to handle function arguments and execution tracking. When a function is called, the stack must conform to a rigid structure:

```
       Memory Address       Stack Contents            Purpose
      +----------------+-----------------------+----------------------------------+
      |  0xffffd800    |  "aaaa" ... "aaaa"    |  Buffer Padding (156 bytes)      |
      +----------------+-----------------------+----------------------------------+
ESP ->|  0xffffd89c    |  0xf7e6aed0           |  Address of system()             |
      +----------------+-----------------------+----------------------------------+
      |  0xffffd8a0    |  0xf7e5eb70           |  Address of exit()               |
      +----------------+-----------------------+----------------------------------+
      |  0xffffd8a4    |  0xf7f897ec           |  Address of "/bin/sh" string     |
      +----------------+-----------------------+----------------------------------+
```

## 2. Step-by-Step Execution Mechanics

The exploit strings together three separate memory coordinates in a specific order to satisfy how the CPU processes functions.

### Step A: The Initial Hijack

When the vulnerable function finishes executing, it triggers a `ret` (return) instruction. The CPU pops the address sitting at the current stack pointer (ESP) into the instruction pointer (EIP).

- Because we placed the address of `system()` in the Saved EIP slot, the CPU jumps directly to `system()`.
- Popping this value off the stack causes the stack pointer (ESP) to automatically move down by exactly 4 bytes.

### Step B: Faking the Stack Frame for `system()`

The `system()` function assumes it was called normally via a standard compilation pattern. Therefore, it reads the memory coordinates relative to the new position of ESP:

- **The Return Path (ESP):** `system()` looks at the top of the stack to determine where it should return when its execution finishes completely. It finds the address of `exit()` and registers it as its future destination.
- **The Argument Pointer (ESP + 4):** Following 32-bit rules, the first argument to any function must sit exactly 4 bytes below its return address. `system()` looks at `ESP + 4`, finds the address pointing to the string `"/bin/sh"`, and passes that pointer to the operating system.

## 3. The Grandchild Process Sandbox Bypass

The level includes a monitoring process designed to stop standard shellcode or direct execution vectors. Understanding why this logic fails explains the elegance of the Ret2Libc approach.

### The Parent-Child Trap

When the program initializes, it splits into two entities: a Parent (Tracer) and a Child (Tracee). The parent implements `ptrace(PTRACE_ATTACH, child_pid, ...)` to watch every system call the child makes. If the child attempts to call `execve` (system call 11), the parent flags it, prevents execution, and outputs `child is exiting...`.

### The `system()` Blindspot

The `system()` function inside the standard C library (libc) is a high-level wrapper, not a raw system call. Internally, `system()` handles execution by executing a new `fork()` instruction.

```
       [ level04 Parent ]  <-- Monitoring only the immediate Child PID
               |
               v
       [ level04 Child ]   <-- Jumps to system(), which calls fork()
               |
               v
         [ /bin/sh ]       <-- Grandchild process (UNTRACED)
```

1. The Child jumps to `system()`.
2. `system()` forks a brand-new Grandchild process.
3. The Child transitions into a sleeping state, waiting for the grandchild to finish.
4. The Grandchild executes the actual `execve("/bin/sh")` system call.

By default, Linux `ptrace` monitoring does not inherit down a process tree. Because the parent is explicitly bound only to the child's PID, the kernel completely hides the grandchild's actions from the parent. The parent continues to see the child sleeping peacefully, completely missing the rogue shell running beneath it.

## 4. Clarification on `prctl` and Security

The code contains the following line:

```c
prctl(PR_SET_DUMPABLE, PR_DUMPABLE);
```

### Decoupling `prctl` from System Call Restrictions

It is a common misconception that `prctl` inherently limits execution or blocks system calls. In modern Linux security, a specific submodule of `prctl` called Seccomp (`PR_SET_SECCOMP`) can be used to sandbox system calls. However, level04 does not use Seccomp.

### The True Purpose of `PR_SET_DUMPABLE`

When a binary is configured with elevated privileges (such as a Setuid program), the Linux kernel automatically tags it as "undumpable" to protect its sensitive memory space from unauthorized inspection or debugging.

By calling `prctl(PR_SET_DUMPABLE, PR_DUMPABLE)`, the child process deliberately lowers this defense, telling the operating system: "I am granting permission for a tracer to monitor my memory." This instruction was not put in place to stop the attacker. Rather, the creator of the level had to include it so that the Parent process would have the necessary operating system permissions to attach to the Child via `ptrace` in the first place. It enabled the flawed security monitor instead of creating a sandbox constraint.

## 5. Understanding Stream Persistence (`cat -`)

When running the exploit payload through a terminal pipe, a standard execution line like `python -c '...' | ./level04` will instantly fail and close the shell. This occurs due to how Linux streams handle the End-of-File (EOF) marker.

### The Pipe Collapse Problem

1. The Python script executes, prints the binary payload string into the pipe, and terminates.
2. The binary receives the payload, processes the buffer overflow, and successfully launches the grandchild shell (`/bin/sh`).
3. The newly spawned shell looks to its input channel (stdin) to receive user commands.
4. Because the Python script closed immediately after printing, the pipe encounters an EOF (End-of-File) signal.
5. The shell interprets EOF as "no more input is coming," and shuts down immediately before the user has a chance to type anything.

### The Solution: Using `cat -`

To prevent the stream from collapsing, the command uses standard shell grouping:

```bash
(python -c 'print "a" * 156 + "\xd0\xae\xe6\xf7" + "\x61\x61\x61\x61" + "\xec\x97\xf8\xf7"'; cat -) | ./level04
```

The semicolon tells the terminal to execute the commands sequentially inside a single shared input stream:

1. First, `python` writes the exploit payload directly into the buffer to trigger the vulnerability.
2. Second, the moment Python exits, `cat -` takes over the stream.
3. The hyphen (`-`) specifies that `cat` should read directly from your live keyboard terminal context. This links your active terminal interface directly into the input pipe of the target program. When the shell spawns, the pipe remains open and interactive, allowing you to run commands like `cat /home/users/level05/.pass` to retrieve the flag.