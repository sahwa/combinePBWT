# combine_chunklengths

combinePBWT is a fast and memory-efficient combiner for **ChromoPainter**, **pbwt**, and **SparsePainter** chunklength output files. It tool reads multiple gzipped chromosome-specific output files and combines them into a single summed matrix. It is designed for very large datasets and emphasizes:

* ✅ Streaming processing (meaning low memory overhead per file)
* ✅ Robust parsing of long headers
* ✅ Whitespace-safe tokenization
* ✅ Timestamped progress logging
* ✅ Gzipped input and output support
* ✅ Large matrix support (with allocation checks)

---

## Overview

`combine_chunklengths`:

* Reads multiple `.gz` chunklength files (one per chromosome)
* Identifies the ID column automatically (based on software type)
* Sums chunklength matrices across chromosomes
* Writes a gzipped combined matrix

Supported types:

* `pbwt`
* `chromopainter`
* `SparsePainter`

---

## Requirements

* C++17 compatible compiler
* CMake ≥ 3.10
* zlib development library

### Install zlib

**Ubuntu/Debian**

```bash
sudo apt-get install zlib1g-dev
```

**CentOS/RHEL**

```bash
sudo yum install zlib-devel
```

**macOS (Homebrew)**

```bash
brew install zlib
```

---

## Building with CMake

### 1️⃣ Directory Structure

```
combine_chunklengths/
├── CMakeLists.txt
└── main.cpp
```

Save the provided source code as `main.cpp`.

---

---

### 2️⃣ Compile

```bash
mkdir build
cd build
cmake ..
make
```

The executable will be created as:

```
build/combine_chunklengths
```

---

## Usage

```bash
combine_chunklengths \
  -p <pre_chr> \
  -a <post_chr> \
  -c <chrs> \
  -o <output> \
  -t <type>
```

### Arguments

| Option            | Description                     |
| ----------------- | ------------------------------- |
| `-p`, `--pre_chr` | Prefix before chromosome number |
| `-a`,             |                                 |
