# deccoder v2.2.5

[中文文档](README.md) | English

A lightweight, single-binary, cross-platform command-line encryption tool written in C++.
It produces `.dec` files whose header carries readable metadata (remarks, source IP, original
filename, creation time, etc.), while the body is streamed through ChaCha20 and protected
against tampering.

## Features

- **Interactive wizard**: the `-W` mode walks you through everything — path, password, remarks — so you never have to remember the argument order.
- **One file in, one file out**: each source file maps to a single `.dec`. Directories are packed and encrypted as a whole.
- **Streaming**: both encryption and decryption process 64 KB chunks, so memory usage stays flat no matter how large the file is.
- **Live progress**: percentage bars for all five phases (pack, encrypt, verify, decrypt, unpack). When stdout is redirected to a file or pipe, output degrades to one plain line per phase instead of flooding logs with `\r`.
- **Integrity authentication**: HMAC-SHA256 (encrypt-then-MAC). Any modification or truncation of the ciphertext or header is detected at decryption time and rejected.
- **Encrypted header**: the header is ChaCha20-encrypted. With an empty password the header is left in the clear, though the integrity tag is still present.
- **Readable metadata**: `-R` shows remarks, source IP, original filename and timestamp without touching the body.
- **Real public IP**: at encryption time the tool asks the system `curl` for your public IP and records it in the header. Offline, it silently falls back to the LAN address and never blocks the operation.
- **Fast header rewrite**: `-M` rewrites only the encrypted header, keeps the ciphertext untouched, and recomputes the whole-file MAC — seconds even on large files.
- **No third-party dependencies**: SHA-256, ChaCha20, HMAC-SHA256, Base64 and a JSON parser are all built in. No OpenSSL, no external libraries.
- **Cross-platform**: builds on Linux, macOS and Windows with nothing but a C++17 compiler.

## Security design

| Stage | Algorithm / approach |
|-------|----------------------|
| Key derivation | SHA-256, 100 000 iterations (`password + salt + counter`) |
| Header encryption | ChaCha20, 20 rounds, independent nonce |
| Body encryption | ChaCha20, 20 rounds, key derived again from the header key, independent nonce |
| File authentication | HMAC-SHA256 (encrypt-then-MAC), covering the preamble, encrypted header and encrypted body |
| Password check | A `check` field in the header (SHA-256 prefix of the derived key) distinguishes a wrong password from a corrupted file |
| Salt / nonce | Randomly generated on every encryption (16-byte salt, 12-byte nonce) |

> ⚠️ **Note**: format v2 adds whole-file integrity (MAC) verification. If a file is corrupted or truncated in transit, decryption fails outright rather than emitting damaged data. `-M` re-encrypts the header *and* recomputes the file-wide MAC, so the edited file stays valid. This tool is aimed at personal file protection, not at anti-forgery scenarios such as untrusted cloud storage.

## File format

`.dec` layout (little-endian, current version `v2`):

```
┌──────────────────────────────────────────┐
│ Magic "DECF"                 (4 bytes)   │
│ Format version (v2)          (2 bytes)   │
│ Flags                        (1 byte)    │
│    bit0: header encrypted                │
│    bit1: password required               │
│ Reserved                     (1 byte)    │
│ Header length                (4 bytes)   │
│ Reserved                     (4 bytes)   │
├──────────────────────────────────────────┤
│ Salt                         (16 bytes)  │
│ Header nonce                 (12 bytes)  │
│ KDF iterations               (4 bytes)   │
│ Encrypted header JSON        (variable)  │
├──────────────────────────────────────────┤
│ Encrypted body               (variable)  │
├──────────────────────────────────────────┤
│ HMAC-SHA256 integrity tag    (32 bytes)  │
└──────────────────────────────────────────┘
```

The header JSON holds `format`, `remarks`, `source_ip`, `original_name`, `original_size`,
`original_mtime`, `encrypted_at`, `modified_at`, `cipher`, `kdf`, `body_nonce` and `check`.

## Building

### Requirements
- A C++17 compiler (GCC ≥ 9, Clang ≥ 7, or MSVC ≥ 2019)
- CMake ≥ 3.16 (optional)

### With CMake (recommended)

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

The binary lands in `build/deccoder` (`build/deccoder.exe` on Windows).

### Compiling directly

```bash
g++ -std=c++17 -O2 -Wall -Wextra \
    src/main.cpp src/crypto.cpp src/json.cpp src/decfile.cpp src/util.cpp \
    -o deccoder
```

On Windows with MinGW you also need to link Winsock:

```bash
g++ -std=c++17 -O2 -Wall -Wextra src/*.cpp -o deccoder.exe -lws2_32
```

### Installing to PATH

```bash
sudo cp build/deccoder /usr/local/bin/
```

## Usage

```
deccoder <operation> [...]
```

| Operation | Long option | Description |
|-----------|-------------|-------------|
| `-W` | `--wizard`      | **Interactive encryption wizard** |
| `-E` | `--encryption`  | Encrypt a file or directory |
| `-U` | `--decryption`  | Decrypt a `.dec` file |
| `-R` | `--readinghead` | Read and print the encrypted header only |
| `-M` | `--modifyhead`  | Modify the encrypted header (body untouched) |
| `-h` | `--help`        | Help |
| `-V` | `--version`     | Version |

Every operation has its own sub-help, e.g. `deccoder -E -h`. The interface follows `$LANG` / `$LC_ALL`: Chinese in a `zh` locale, English otherwise.

### Encryption wizard `-W` (recommended for newcomers)

```bash
deccoder -W
```

It asks one question at a time so you cannot get the arguments wrong:

1. Path to encrypt (dragging a file from your file manager works — surrounding double quotes are stripped automatically).
2. Whether to set a password (answering `y` asks for it twice, to catch typos).
3. Remarks for the file (press Enter to leave empty).
4. Whether to force-overwrite an existing target.
5. Whether to delete the source after a successful encryption.
6. A summary for confirmation before it starts.

### Encrypting `-E`

```bash
deccoder -E <path> [password] [remarks] [-f] [-r]
```

- `<path>` may be a file or a directory; a directory is packed recursively and encrypted as a single archive.
- Existing `.dec` files are skipped so nothing gets encrypted twice.
- Both `<password>` and `<remarks>` are optional: no password means no password is needed to decrypt, no remarks means an empty field.
- `-f, --force` overwrites an existing `.dec`.
- `-r, --remove` deletes the source file or directory after a successful encryption.

```bash
# Encrypt a single file
deccoder -E ./report.docx "mypass" "Q3 report"

# Encrypt a whole directory without a password
deccoder -E ./photos "" "Family album"

# Force overwrite
deccoder -E ./a.txt "pw" "example" -f

# Encrypt and then delete the source
deccoder -E ./secret/ "" "archive" -r
```

### Decrypting `-U`

```bash
deccoder -U <dec-file> [password] [-o <output>] [-f]
```

- By default the file is restored under the `original_name` recorded in the header, next to the `.dec` file.
- `-o, --output` names a target file or directory. For an archive, the contents are unpacked into it.
- `-f, --force` overwrites an existing target.
- If the MAC check fails (tampered or corrupted file), decryption is refused.

```bash
# Restore next to the .dec file
deccoder -U ./report.docx.dec "mypass"

# Restore into a specific directory
deccoder -U ./report.docx.dec "mypass" -o ./restored/

# Name the output file and overwrite
deccoder -U ./report.docx.dec "mypass" -o ./x.docx -f
```

On success it prints:

```
---------------Decryption complete---------------

Decryption file: ./report.docx.dec decrypted to target file ./report.docx
Encryption Header   : "Q3 report"
Encryption Source IP: "112.65.12.34"

------------------------------------------------------
```

### Reading the header `-R`

```bash
deccoder -R <dec-file> [password] [--json]
```

- Prints a human-readable summary by default.
- `--json` emits the full JSON for scripting.
- Only the header is decrypted; the body is never touched, so it is fast.

```bash
deccoder -R ./report.docx.dec "mypass"
deccoder -R ./report.docx.dec "mypass" --json
```

### Modifying the header `-M`

```bash
deccoder -M <dec-file> [password] [--remarks <text>] [-i <ip>] [-e]
```

| Option | Description |
|--------|-------------|
| `--remarks <text>` | Set the remarks directly |
| `-i, --ip <addr>`  | Set the source IP directly |
| `-e, --edit`       | Open the full header JSON in `$VISUAL` / `$EDITOR` (default `vi`) |
| no option          | Prompt interactively for new remarks |

Only the header is rewritten; the ciphertext is copied verbatim, and the whole-file MAC is
recomputed automatically — a fast operation even for large files.

```bash
deccoder -M ./report.docx.dec "mypass" --remarks "archived"
deccoder -M ./report.docx.dec "mypass" -i 10.0.0.1
deccoder -M ./report.docx.dec "mypass" -e
deccoder -M ./report.docx.dec "mypass"       # interactive
```

> `format`, `original_name`, `original_size` and `body_nonce` are protected fields: even if you
> change them in the editor they are forced back, so the body never becomes undecryptable.

## Error messages

| Message | Meaning |
|---------|---------|
| `This is not a DEC encrypted file` | The magic at the start is not `DECF` |
| `The .dec version is newer than this deccoder: this version cannot open it` | The file format is newer than the program |
| `This file requires a password` | The header is flagged as password-protected, none was supplied |
| `Wrong password` | The password is incorrect (checked via the header `check` field) |
| `File has been tampered with or is corrupted (MAC verification failed)` | Truncated, modified, or forged ciphertext |
| `Target file already exists` | Add `-f` to overwrite |
| `No operation specified (use -h for help)` | No operation was given |
| `Error: path does not exist: ...` | The file or directory cannot be found |
| `Permission denied` | Insufficient rights (e.g. no write access to the target directory, or a restricted source file) |

## Project layout

```
deccoder/
├── CMakeLists.txt
├── README.md
├── README.en.md
└── src/
    ├── main.cpp        entry point and subcommands
    ├── crypto.h/.cpp   SHA-256, ChaCha20, HMAC-SHA256, Base64, KDF
    ├── json.h/.cpp     minimal JSON serializer/parser
    ├── decfile.h/.cpp  .dec format, encryption/decryption, integrity checks
    └── util.h/.cpp     time formatting, public/LAN IP detection
```

## Quick self-test

```bash
# Interactive wizard
./build/deccoder -W

# Short command-line round trip
echo "hello deccoder" > t.txt
./build/deccoder -E t.txt "pw" "test"          # produces t.txt.dec
./build/deccoder -R t.txt.dec "pw"             # inspect the header
./build/deccoder -M t.txt.dec "pw" --remarks "edited"
./build/deccoder -U t.txt.dec "pw" -f          # restore t.txt
```

## License

GNU GPL v3
