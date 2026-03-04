# 🔐 C++ SHA256 Duplicate File Cleaner

A lightweight C++17 tool that recursively scans directories, identifies duplicate files using SHA256 hashing, and safely removes copies while preserving the original (oldest) file.

## ✨ Features

- **Recursive Directory Scanning** - Finds all files in nested directories
- **SHA256 Hashing** - Accurate duplicate detection via OpenSSL
- **Smart Preservation** - Keeps the oldest file, deletes newer copies
- **Clean Output** - Easy-to-read summary with timestamps
- **Cross-Platform** - Works on Windows, Linux, and macOS
- **Transparent Deletion** - Clear feedback on what is removed

## 📋 Requirements

- **C++17** compatible compiler (`g++`, `clang++`, `msvc`, etc.)
- **OpenSSL** development libraries

### Installing OpenSSL

**Windows:**
- Download from https://slproweb.com/products/Win32OpenSSL.html
- Install to default location (`C:\OpenSSL-Win64`)

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install libssl-dev
```

**macOS:**
```bash
brew install openssl
```

## 🛠️ Build

### Windows
```bash
g++ -std=c++17 -o main main.cpp -lssl -lcrypto -IC:\OpenSSL-Win64\include -LC:\OpenSSL-Win64\lib
```

### Linux/macOS
```bash
g++ -std=c++17 -o main main.cpp -lssl -lcrypto
```

## 🚀 Usage

### Linux/macOS
```bash
./main <directory>
```

### Windows
```bash
.\main.exe <directory>
```

### Examples

Scan current directory:
```bash
# Linux/macOS
./main . -d <exception_directory>

# Windows
.\main.exe . -d <exception_directory>
```

Scan a specific folder:
```bash
# Linux/macOS
./main /path/to/documents -d <exception_directory>

# Windows
.\main.exe "C:\Users\YourName\Downloads" -d <exception_directory>
```

## 🧪 Quick Test

### Windows
```bash
g++ -o create_files create_files.cpp && .\create_files.exe && g++ -std=c++17 -o main main.cpp -lssl -lcrypto -IC:\OpenSSL-Win64\include -LC:\OpenSSL-Win64\lib && .\main.exe . -d .git -d folder
```

### Linux/macOS
```bash
./create_files && g++ -std=c++17 -o main main.cpp -lssl -lcrypto && ./main . -d .git -d folder
```

## 📊 How It Works

1. **Scan** - Recursively traverses all directories and files
2. **Group by Size** - Groups files of identical size (optimization)
3. **Compute Hashes** - Calculates SHA256 for each file
4. **Identify Duplicates** - Groups files with matching hashes
5. **Preserve Original** - Marks the oldest file as `<- ORIGINAL`
6. **Delete Copies** - Removes all newer duplicate files
7. **Report** - Displays summary of groups and deletions

## 📝 Example Output
```
Scanning: "."

Duplicate group (SHA256: 7e441520c0ef3f71...):
  ".\folder\data1.csv" [2026-03-02 15:21:10] <- ORIGINAL
  ".\folder\data2.csv" [2026-03-02 15:22:10]
  Deleted: 1

Duplicate group (SHA256: 9b42bfdacee8463b...):
  ".\config1.json" [2026-03-02 18:11:10] <- ORIGINAL
  ".\config2.json" [2026-03-02 19:11:10]
  Deleted: 1

=== SUMMARY ===
Groups found: 2
Files deleted: 2
```

## ⚙️ How Duplicates Are Determined

Files are considered duplicates if:
- ✅ They have **identical content** (same SHA256 hash)
- ✅ File size matches
- ❌ Filename or path does **not** matter
- ❌ Creation/modification time does **not** affect detection

The **oldest file is kept**, newer copies are deleted.

## ⚠️ Important Notes

- **⚠️ Files are permanently deleted** — Use on test data first
- **Backup important data** before running on production directories
- **Test in a controlled directory** with known duplicates
- **No undo operation** — Deleted files cannot be recovered

## 📄 License

Free to use and modify. No warranty provided.

## 🤝 Contributing

Found a bug or have an improvement? Feel free to submit a pull request or open an issue.
