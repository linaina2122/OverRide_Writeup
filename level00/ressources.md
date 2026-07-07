## Vulnerability Analysis

A basic reverse engineering pass over the binary's assembly reveals the following comparison:

```asm
cmp    $0x149c,%eax
```

This instruction compares the `eax` register against the value `5276` (decimal equivalent of `0x149c`).

- If the comparison succeeds (i.e., the input matches), the program executes:

  ```c
  system("/bin/sh");
  ```

- If it fails, the program prints:

  ```
  Invalid Password!
  ```

  and returns without granting access.

---

## Step-by-Step Exploitation

Run the binary and supply the recovered password:

```bash
level00@OverRide:~$ ./level00
```

```
***********************************
* 	     -Level00 -		  *
***********************************
Password: 5276

Authenticated!
```

Confirm privilege escalation and retrieve the next level's credentials:

```bash
$ whoami
level01
$ cat /home/users/level01/.pass
```