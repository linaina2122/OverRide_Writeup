# OverRide — Level03 Writeup

## Goal
`level03` is a SUID binary that asks for a numeric password. Enter the correct number and it spawns a `/bin/sh` shell running with `level04` privileges, letting you read `level04`'s password.

## The Big Picture
The program does this:
```
key = magic_number - your_input
```
Then it uses that `key` to XOR-decrypt a hidden string. If the decrypted string equals `"Congratulations!"`, you win. Your job is to pick the `your_input` that produces the exact `key` that decrypts correctly.

---

## Step 1 — The argument setup (`test` function)

Disassembly:
```
<+6>:  mov 0x8(%ebp),%eax    ; eax = arg1 = your_input
<+9>:  mov 0xc(%ebp),%edx    ; edx = arg2 = magic_number
<+12>: mov %edx,%ecx         ; ecx = magic_number (a copy)
<+14>: sub %eax,%ecx         ; ecx = magic_number - your_input   <- the KEY
```

**AT&T reminder:** `sub %eax,%ecx` means `ecx = ecx - eax`. So the key is `magic_number - your_input`.

The magic number is a hardcoded constant, visible in the disassembly as an immediate:
```
movl $0x1337d00d, ...    ->   0x1337d00d = 322424845
```

C equivalent:
```c
void test(int user_input, int magic_number) {
    int result = magic_number - user_input;   // the key

    if ((unsigned int)result > 21) {
        decrypt(rand());   // out of range -> random key -> FAIL
        return;
    }
    decrypt(result);       // in range -> decrypt with YOUR key
}
```

---

## Step 2 — The 0–21 gate (important trick)

The check is **unsigned**: `(unsigned int)result > 21`.

- If your input makes `result` land in **0..21** -> `decrypt(result)` runs with a key you control.
- If `result > 21` -> it decrypts with `rand()` instead. Dead end.
- If your input is **larger** than the magic number, `result` goes negative — but cast to unsigned, a negative number becomes huge (e.g. `-1` -> `4294967295`), which is `> 21`. Also fails.

So the valid key window is exactly **0 to 21**. (The big switch statement in the decompiled code is a red herring — every case, including `default`, just calls `decrypt(result)`. It has no effect on the logic.)

This narrows the answer to 22 candidates. Only **one** is correct — that's decided inside `decrypt()`.

---

## Step 3 — The decryption (`decrypt` function)

```c
void decrypt(int code) {
    char encrypted_str[] = "\x51\x7d\x7c\x75\x60\x73\x66\x67"
                           "\x7e\x73\x66\x7b\x7d\x7c\x61\x33";
    int len = strlen(encrypted_str);

    for (int i = 0; i < len; i++)
        encrypted_str[i] ^= code;      // XOR every byte with the key

    if (strcmp(encrypted_str, TARGET) == 0)   // TARGET = "Congratulations!"
        system("/bin/sh");
    else
        puts("...try again...");
}
```

The ciphertext bytes are:
```
51 7d 7c 75 60 73 66 67 7e 73 66 7b 7d 7c 61 33
```

Each byte is XORed with the single `code` value (the key). We need the `code` (0–21) where the result spells the target.

---

## Step 4 — Finding the key

XOR every ciphertext byte with each candidate 0–21 until the output is the target string:

| code | decrypted result   |
|------|--------------------|
| 0    | `Q}|u...` (garbage) |
| ...  | ...                |
| **18** | **`Congratulations!`** (correct) |

`code = 18` (`0x12`) produces `"Congratulations!"`. That's our key.

Quick way to verify:
```python
cipher = bytes([0x51,0x7d,0x7c,0x75,0x60,0x73,0x66,0x67,
                0x7e,0x73,0x66,0x7b,0x7d,0x7c,0x61,0x33])
for code in range(22):
    print(code, bytes(b ^ code for b in cipher))
# code 18 -> b'Congratulations!'
```

---

## Step 5 — Solving for the password

We need `key = 18`, and `key = magic_number - your_input`:
```
your_input = magic_number - key
your_input = 322424845 - 18
your_input = 322424827
```

**Password: `322424827`**

---

## Step 6 — Getting the shell

A SUID binary drops its elevated privileges when run under a debugger (ptrace), so the exploit must run in a **normal shell**. Pipe the password in, and keep stdin open with `cat` so the spawned shell doesn't immediately die:

```bash
./level03
password: 322424827
```

The terminal goes quiet — that's your `/bin/sh`. Now read the next flag:
```bash
whoami
cat /home/users/level04/.pass
```

---

## One-line summary
`key = 322424845 - input` -> in-range check (0..21) -> XOR-decrypt ciphertext with key -> must equal `"Congratulations!"`. That happens at `key = 18`, so **input = 322424827**.