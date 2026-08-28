# Understanding "Override"

This project is about **binary exploitation**: finding ways to make a program do something it wasn't supposed to do, usually by messing with memory. It sounds scary at first, but the core ideas are actually pretty intuitive once you see them laid out. Here's the plain-English version.

---

## 1. The big picture

When you run a program, it gets a chunk of memory to work with. Part of that memory is the **stack** — a simple structure that stores local variables, function arguments, and (importantly) the address the program should jump back to once a function finishes.

The whole game of "override" style projects is this: **if you can control what gets written into memory, you can control what the program does next.** That's it. Everything else is just different techniques for doing that safely, reliably, or around a specific protection.

---

## 2. Buffer overflow — the starting point

A **buffer** is just a fixed-size block of memory, like an array of 100 characters.

If a program copies user input into that buffer *without checking the size*, and you give it 200 characters instead of 100, the extra 100 characters spill over into whatever memory comes right after it.

That "whatever comes after" might be:
- another variable (you just changed its value)
- the **return address** of the function (you just hijacked where the program jumps next)

This is the foundation of almost everything else in the project. Every protection you'll learn about exists specifically to make this harder to pull off.

---

## 3. Stack canaries — the tripwire

A **stack canary** (or "stack cookie") is a random value the compiler places right before the return address on the stack.

Think of it like a trip wire:
1. Before the function returns, the program checks: "is the canary value still what I set it to?"
2. If it's been changed (because your overflow wrote over it on the way to the return address), the program knows something's wrong and immediately kills itself (`stack smashing detected`) instead of jumping to a corrupted address.

**Why it matters:** it doesn't stop you from overflowing the buffer — it stops you from *silently* overwriting the return address without noticing you also overwrote the canary. To bypass it, attackers usually need to either leak the canary's value first, or find a way to overflow that skips over it entirely.

---

## 4. ASLR — shuffling the map

**ASLR (Address Space Layout Randomization)** randomizes *where* things are loaded in memory each time the program runs — the stack, the heap, shared libraries.

Without ASLR, an address like "the libc `system()` function" is always at the same spot, so you could just hardcode it into your exploit. With ASLR on, that address changes every run, so a hardcoded address is useless.

**Why it matters:** it doesn't stop the overflow, it stops you from *knowing where to jump*. Bypassing it usually means leaking a real address at runtime (an "info leak") and calculating everything else relative to it.

---

## 5. NX / DEP — no executing the stack

**NX (No-eXecute)**, also called **DEP (Data Execution Prevention)**, marks memory regions like the stack as "data only" — the CPU is told it's not allowed to *execute* instructions that live there.

Before NX existed, a classic trick was: overflow the buffer, write your own tiny bit of machine code (shellcode) into it, then point the return address at your own code sitting right there on the stack.

**Why it matters:** NX kills that trick. You can still overflow and overwrite the return address, but you can no longer just drop shellcode on the stack and jump to it — the CPU will refuse to run it.

---

## 6. ROP — Return Oriented Programming

If you can't inject your own code (because of NX), the workaround is: **reuse code that's already there.**

The binary and its libraries are full of tiny existing instruction sequences that end in `ret` (return) — these are called **gadgets**. Individually they do almost nothing (e.g. "put a value in a register, then return"). But if you chain the *addresses* of many gadgets together on the stack, each `ret` jumps to the next one, and you can string together a sequence of small actions into something meaningful — like calling `system("/bin/sh")`.

**Why it matters:** this is how you get around NX. You're not injecting new code, you're just cleverly reordering code that was already legally there.

---

## 7. PIE — moving the binary itself

**PIE (Position Independent Executable)** applies the same randomization idea as ASLR, but to the *main binary itself*, not just its libraries. Without PIE, the executable always loads at the same fixed address, which is a huge help to an attacker (constant, predictable addresses for gadgets).

**Why it matters:** with PIE on, even the binary's own functions move around each run, so you generally need a leak from the binary itself too, not just from libc.

---

## 8. GOT / PLT — how the program finds its functions

When your program calls a library function like `printf`, it doesn't know that function's real address at compile time (especially with ASLR). So it uses a lookup system:

- **PLT (Procedure Linkage Table):** a small stub the program jumps to first.
- **GOT (Global Offset Table):** a table of real addresses, filled in by the loader (or lazily, the first time the function is called).

If you can find a way to *write* to the GOT (say, via a format string bug or an arbitrary write), you can overwrite the entry for a real function with the address of something else — so every future call to that function actually calls yours instead.

**Why it matters:** it's a classic "arbitrary write → control flow hijack" target, separate from the classic stack return-address overwrite.

---

## 9. Format string bugs — the sneaky one

`printf(user_input)` instead of `printf("%s", user_input)` looks harmless but isn't. If the user controls the format string, they control what `printf` thinks its arguments are.

- `%x` repeated a bunch of times → leaks stack memory (info leak, useful for defeating ASLR).
- `%n` → **writes** the number of characters printed so far to an address you supply.

**Why it matters:** this single bug class can give you both a *read* primitive (leak addresses) and a *write* primitive (overwrite anything, including the GOT). It's one of the most powerful and most subtle bugs you'll deal with.


