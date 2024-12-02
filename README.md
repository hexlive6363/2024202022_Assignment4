# MyGit - A Simple Git Clone Implementation

MyGit is a simple implementation of some basic Git commands in C++. It provides functionality to initialize a repository, add files, commit changes, and more, all within a custom version control system.

## Features

This project implements the following commands:

### 1. `init` - Initialize Repository
- **Command**: `./mygit init`
- **Description**: Initializes a new repository in the current directory by creating a `.mygit` folder.


### 2. `hash-object` - Hash a File
- **Command**: `./mygit hash-object [-w] <file>`
- **Description**: Calculates the SHA-1 hash of a file. The optional `-w` flag writes the file to the `.mygit/objects` directory.

### 3. `cat-file` - Display File Contents
- **Command**: `./mygit cat-file <flag> <file_sha>`
- **Flags**:
  - `-p`: Print file contents.
  - `-s`: Display the file size.
  - `-t`: Display the type (always `blob`).


### 4. `write-tree` - Write Directory Structure
- **Command**: `./mygit write-tree`
- **Description**: Writes the current directory structure (except `.mygit`) as a tree object.


### 5. `ls-tree` - List Tree Contents
- **Command**: `./mygit ls-tree <tree_sha>`
- **Description**: Lists the contents of a tree object. Optionally, use `--name-only` to list only filenames.


### 6. `add` - Add Files to Staging Area
- **Command**: `./mygit add <file(s)>`
- **Description**: Adds files to the staging area by saving their hashes in `.mygit/index`.


### 7. `commit` - Commit Changes
- **Command**: `./mygit commit "<message>"`
- **Description**: Commits changes to the repository with a given message. Creates a new commit object in `.mygit/objects` and updates `HEAD`.

### 8. `log` - Display Commit History
- **Command**: `./mygit log`
- **Description**: Displays commit history from latest to oldest, including commit SHA, parent SHA (if applicable), message, and committer information.



### 9. `checkout` - Restore Directory to Specific Commit
- **Command**: `./mygit checkout <commit_sha>`
- **Description**: Restores the state of the working directory to a specific commit.

## Requirements
- **C++17 or newer**: The code uses modern C++ features and the `<filesystem>` library.
- **OpenSSL**: Used for calculating SHA-1 hashes.

## Compilation Instructions
Use `g++` to compile the code. Make sure you link with the OpenSSL library (`-lssl -lcrypto`).
```sh
g++ -std=c++17 -o mygit mygit.cpp -lssl -lcrypto
```

## Error Handling
- If the repository is not initialized and a command requiring `.mygit` is run, an appropriate error message is displayed.
- Commands that require specific arguments will print usage instructions if the arguments are missing or incorrect.



