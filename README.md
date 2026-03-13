# combine_chunklengths

combinePBWT is a high-performance combiner for **ChromoPainter**, **PBWT**, and **SparsePainter** chunklength output files.

This tool reads multiple gzipped chromosome-specific matrices and combines them into a single summed matrix, optimized for very large genomic datasets.

---

## Features

* Streaming `.gz` input (low per-file memory overhead)
* Full in-memory accumulation matrix
* Timestamped, unbuffered progress logging
* Automatic ID column detection
* Strict compiler warnings
* Aggressive `-O3 -march=native` optimization
* Optional:

  * Link-Time Optimization (LTO)
  * OpenMP support
  * libdeflate instead of zlib

---

## Requirements

* CMake ≥ 3.18
* C++17 compatible compiler (GCC ≥ 9 recommended)
* zlib development library
  *(or libdeflate if enabled)*

---

##  Dependencies

### Default (zlib)

**Ubuntu / Debian**

```bash
sudo apt install zlib1g-dev
```

**CentOS / RHEL**

```bash
sudo yum install zlib-devel
```

**macOS**

```bash
brew install zlib
```

---

### Optional: libdeflate (faster decompression)

```bash
sudo apt install libdeflate-dev
```

---

## Build Instructions

The repository already includes a complete `CMakeLists.txt`.

### Standard Optimized Build (Recommended)

```bash
git clone https://github.com/sahwa/combinePBWT.git
cd combinePBWT

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

Executable will be placed in:

```
bin/combine_chunklengths
```

*(Binaries are written to `<source>/bin` by design.)*

---

## Build Options

You can enable additional features at configure time:

### Enable OpenMP

```bash
cmake -DENABLE_OPENMP=ON ..
```

### Use libdeflate instead of zlib

```bash
cmake -DUSE_LIBDEFLATE=ON ..
```

### Disable LTO

```bash
cmake -DENABLE_LTO=OFF ..
```

### Full Example (maximum performance build)

```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_OPENMP=ON \
  -DUSE_LIBDEFLATE=ON \
  ..
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

| Option             | Description                                 |
| ------------------ | ------------------------------------------- |
| `-p`, `--pre_chr`  | Prefix before chromosome number             |
| `-a`, `--post_chr` | Suffix after chromosome number              |
| `-c`, `--chrs`     | Comma-separated chromosome list             |
| `-o`, `--output`   | Output gzipped file                         |
| `-t`, `--type`     | `pbwt`, `chromopainter`, or `SparsePainter` |

---

## Example

If files are:

```
chunk_chr1.out.gz
chunk_chr2.out.gz
chunk_chr3.out.gz
```

Run:

```bash
bin/combine_chunklengths \
  -p chunk_chr \
  -a .out.gz \
  -c 1,2,3 \
  -o combined.out.gz \
  -t chromopainter
```

---

## Supported Input Types

| Type          | Required Header Column |
| ------------- | ---------------------- |
| pbwt          | `RECIPIENT`            |
| chromopainter | `Recipient`            |
| SparsePainter | `indnames`             |

The ID column is removed during summation and restored in output.

---

## Memory Usage

The full matrix is stored in memory:

```
RAM ≈ nrows × ncols × 4 bytes
```

Example:

* 10,000 × 10,000 → ~400 MB
* 20,000 × 20,000 → ~1.6 GB

Ensure sufficient RAM for large cohorts.

---

## Logging

The program prints timestamped progress:

```
2026-03-13 12:01:22  Processing chunk_chr1.out.gz
2026-03-13 12:01:45  Finished chunk_chr1.out.gz  rows=5000
2026-03-13 12:02:10  Done  (5000×5000)
```

Stdout is unbuffered — suitable for cluster monitoring.

---

## Compiler Details

Release builds automatically enable:

* `-O3`
* `-march=native`
* `-fstrict-aliasing`
* `-fexceptions`
* `-fno-rtti`
* Link-Time Optimization (if supported)

Warnings:

```
-Wall -Wextra -Wpedantic -Wconversion
```

---

## Notes

* Input files must have identical dimensions.
* Row mismatches trigger warnings.
* Very large matrices may require high-memory nodes.

---

## License

Add license here.

---

## 👤 Author

sam morris - sam.morris@ndph.ox.ac.uk
