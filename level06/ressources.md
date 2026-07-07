

## Vulnerability Analysis

Reversing the `auth()` function reveals three critical characteristics:

1. **Anti-Debugging Implementation:** It utilizes the `ptrace` system call to detect if a debugger is attached. If detected, it immediately terminates execution to prevent reverse engineering.
2. **Deterministic Hashing:** The serial key is not randomly generated or fetched securely from a remote database. Instead, it is computed locally using a predictable mathematical algorithm based entirely on the characters of the provided **Login** string.
3. **Logic Defect & Local Storage:** The calculated serial key is stored in a local stack variable within the `auth` function frame (`$ebp-0x10`) right before performing the final comparison against the user's input.

---

## Architectural Pitfalls Faced During Debugging

### 1. The Ptrace Bypass Trap

When attempting to debug the program naturally, `ptrace` returns `-1` (`0xffffffff`), triggering a tampering warning. We bypass this by setting a breakpoint immediately following the `ptrace` system call and manually forcing the return value register (`$eax`) to `0`. This tricks the binary into thinking no debugger is present.

### 2. SUID Privilege Stripping

A primary trap encountered during this challenge is attempting to spawn the shell *inside* GDB. The Linux kernel enforces a strict security boundary: **if a non-root user traces or debugs an SUID binary via `ptrace`, the kernel strips all elevated privileges.**

Consequently, spawning a shell within GDB only yields the permissions of the base user (`level06`). To successfully elevate to `level07`, the valid credentials must be extracted via GDB, but executed natively *outside* the debugger environment.

### 3. Stack Frame Misplacement & Loop Interpolation

* Inspecting `$ebp-0x10` inside the `main()` function frame yields irrelevant addresses because the calculation variable only exists inside the `auth()` function's frame.
* Inspecting variables mid-loop yields raw, incomplete mathematical state updates that show up as junk signed integers. The value must be extracted *after* the calculation loop concludes, but *before* the function frame is destroyed—specifically at the comparison instruction jump (`je`).

---

## Step-by-Step Exploitation

### Step 1: Initialize Debugger and Set Breakpoints

Fire up GDB with the target binary:

```bash
gdb ./level06
```

Here we set a breakpoint where we can set eax register 0 to pass to execute the rest of the code:

```
(gdb) break *0x080487ba
```

```
run
-> Enter Login: aaaaaa
-> Enter Serial: 123456
set $eax = 0
continue
```

Here, after the program executed the hashing algorithm and stored the final value in this address `-0x10(%ebp)`, we will read this value directly from the address:

```
break *0x08048869
```

```
print /u *(unsigned int*)($ebp - 0x10)
```

result :

```
6231562
```

```
exit
```

```bash
./level06
-> Enter Login: aaaaaa
-> Enter Serial: 6231562
Authenticated!
```

```bash
$ whoami
level07
cat /home/users/level07/.pass
```