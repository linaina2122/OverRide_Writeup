# Level08 Exploit

This program copies a file into a `./backups/` folder. Because it uses naive string concatenation to build file paths, you can trick it into writing files in a location they were never meant to end up.

## Why You Needed a Nested Folder

When you ran the exploit, your terminal was already inside `/tmp/backups`. Because of this specific starting location, the program's path-building logic forced **two nested layers** to be created.

### 1. The Log File Trap

The code hardcodes the log path as `./backups/.log`. From inside `/tmp/backups`, that concatenation becomes:

```
/tmp/backups + /backups/.log = /tmp/backups/backups/.log
```

**Result:** if that second `backups` folder didn't already exist, the program crashed instantly trying to write the log.

### 2. The Flag File Trap

The code glues `./backups/` onto your argument (`file`). From the same location, that becomes:

```
/tmp/backups + /backups/file = /tmp/backups/backups/file
```

**Result:** the program successfully dumped the flag right inside that nested folder.

## Step-by-Step Execution

### Step A: Set Up the Working Directory

Starting from `/tmp`, create the outer `backups` folder and move into it — this is the directory the program expects you to be running from:

```bash
mkdir backups
cd backups/
```

### Step B: Point a Symlink at the Target File

Rather than passing a real file as the argument, create a symlink named `pass` that points at the next level's protected password file. Since the binary just copies whatever path it's given, following a symlink lets it read a file you wouldn't otherwise have permission to access directly:

```bash
ln -s /home/users/level09/.pass pass
ls -la
```
```
total 0
drwxrwxr-x 2 level08 level08 60 Jul  7 21:33 .
drwxrwxrwt 3 root    root    60 Jul  7 21:33 ..
lrwxrwxrwx 1 level08 level08 25 Jul  7 21:33 pass -> /home/users/level09/.pass
```

### Step C: Create the Nested Folder

Since the program builds its log path as `./backups/.log` relative to your current directory, and you're already sitting inside `/tmp/backups`, the program is really looking for `/tmp/backups/backups/.log`. That inner `backups` folder doesn't exist by default, so it needs to be created — otherwise the program fails before it ever gets to the file-copy logic:

```bash
mkdir backups
```

### Step D: Run the Binary Against the Symlink

With the nested folder in place, running the binary against the `pass` symlink no longer crashes on the log write. The program (running with elevated privileges) follows the symlink, reads the protected file, and — due to the same path-gluing bug — copies its contents one directory deeper than intended, into the nested `backups` folder:

```bash
~/level08 pass
```

### Step E: Read the Result

Because the path-gluing bug placed the output one directory deeper than intended, the copied file is sitting inside the nested folder rather than the expected top-level `backups/`:

```bash
cat backups/pass
```