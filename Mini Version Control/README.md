## mgit: A Mini Version Control System in C

### Architecture Overview

mgit is a small, educational reimplementation of a subset of Git’s behavior. It is structured into ten C source files plus a shared header:

- **`mgit.h`**: Shared data structures (`Commit`, `FileEntry`, `Index`, `HashObject`), constants (`MGIT_DIR`, `HASH_SIZE`, etc.), and function declarations.
- **`main.c`**: CLI entry point that dispatches to subcommands (`init`, `add`, `commit`, `log`, `status`, `diff`).
- **`init.c`**: Repository initialization; creates the `.mgit` directory layout and core files.
- **`hash.c`**: Standalone SHA‑1 implementation and helpers for binary/hex conversion and file hashing.
- **`objects.c`**: Content‑addressed object store under `.mgit/objects/` using the same two‑character fan‑out strategy as Git.
- **`index.c`**: In‑memory `Index` representation plus disk persistence and the `mgit add` implementation.
- **`commit.c`**: Commit creation logic, including HEAD/refs handling and a simple commit text format.
- **`log.c`**: Commit history traversal and pretty printing from `HEAD` backwards.
- **`status.c`**: Basic status view showing staged and untracked files.
- **`diff.c`**: Simple line‑based diff of a working file against its last committed version (simplified for pedagogy).

A high‑level architecture diagram:

```text
             +--------------------+
             |      main.c        |
             |  CLI dispatcher    |
             +----------+---------+
                        |
   ---------------------------------------------------------
   |        |          |          |         |             |
 init.c  index.c   commit.c    log.c    status.c      diff.c
   |        |          |          |         |             |
   +--------+----------+----------+---------+-------------+
                        |
                   mgit.h (API)
                        |
          +-------------+---------------+
          |                             |
       hash.c                       objects.c
 (SHA-1, hash helpers)    (read/write .mgit/objects/*)
```

### `.mgit/` Folder Structure

mgit mirrors Git’s on‑disk layout in a reduced form:

- **`.mgit/`**: Repository metadata root.
  - **`objects/`**: Content‑addressed storage of all blobs/trees/commits.
    - **`aa/bb...`**: Objects are stored as `objects/<first2>/<remaining38>`, just like Git.
  - **`refs/`**: References to commit hashes.
    - **`heads/`**: Branch heads.
      - **`main`**: Default branch pointer storing the current `main` commit hash.
  - **`HEAD`**: Symbolic reference pointing at `refs/heads/main`.
  - **`index`**: Binary file storing the staging area (`Index` of `FileEntry` records).

### Building mgit

mgit is designed to build cleanly with a standard C toolchain on macOS:

- **Compiler flags**: `-Wall -Wextra -std=c99`
- **No external libraries**: only standard headers (`stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `dirent.h`, `sys/stat.h`, `stdint.h`, `time.h`) are used.

To build:

```sh
cd "Mini Version Control"
make
```

This produces a single binary named `mgit` in the project root.

### Using mgit: End‑to‑End Example

Below is a full example of creating a repository, adding a file, committing, viewing history, and checking status. All commands are intended to be run from the project directory that contains the `mgit` binary.

1. **Run `make`**

```sh
cd "Mini Version Control"
make
```

2. **Initialize a new repository**

```sh
./mgit init
```

This creates the `.mgit/` directory with `objects/`, `refs/heads/`, `HEAD`, and an empty `index`.

3. **Create a test file and stage it**

```sh
echo "hello mgit" > testfile.txt
./mgit add testfile.txt
```

4. **Create the first commit**

```sh
./mgit commit -m "first commit"
```

5. **View the commit log**

```sh
./mgit log
```

You should see the latest commit printed with:

- Short hash (7 characters)
- Commit date/time
- Commit message

6. **Check repository status**

```sh
./mgit status
```

You should see:

- `Staged files:` empty (the index is cleared after commit).
- `Untracked files:` listing `testfile.txt` only if you created new files after the last commit.

### How mgit Mirrors Real Git Internals

- **Content‑addressable storage**: Like Git, mgit uses SHA‑1 hashes of object contents to name files under `.mgit/objects/aa/bb...`. While the object *formats* are simplified, the addressing model is the same.
- **Index/staging area**: mgit maintains a binary `index` file containing `FileEntry` records with path, content hash, and modification time. This mirrors Git’s index concept, though with a much smaller schema.
- **Commits and history**: Commits store a tree hash, optional parent hash, author, timestamp, and message. mgit follows Git’s parent‑linked commit graph, and the log walks parent pointers backward from `HEAD`.
- **Refs and HEAD**: `HEAD` is a symbolic ref to `refs/heads/main`, and branch refs contain raw commit hashes, just as in Git. Updating a branch ref atomically advances history.
- **Commands**:
  - `mgit init` ≈ `git init` (create `.mgit/` structure, HEAD, refs, index).
  - `mgit add` ≈ `git add` (stage working files into the index).
  - `mgit commit` ≈ `git commit` (create a new commit object from the index and update the current branch).
  - `mgit log` ≈ `git log --oneline` (linear history view).
  - `mgit status` ≈ `git status --short` (simplified staged/untracked view).
  - `mgit diff` ≈ `git diff` (simplified, line‑oriented diff).

The result is a compact, readable codebase that illustrates how Git’s core ideas—content‑addressed objects, an index, commits, and refs—fit together, while remaining small enough to study in a single sitting.

