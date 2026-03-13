# combine_chunklengths

Fast, memory-efficient combiner for **ChromoPainter**, **PBWT**, and **SparsePainter** chunklength output files.

This tool reads multiple chromosome-specific `.gz` chunklength files and combines them into a single summed matrix.

Designed for large-scale datasets with:

* Streaming gzipped input
* Robust header parsing
* Whitespace-safe tokenization
* Timestamped progress logging
* Memory allocation checks
* Gzipped output

---

## Requirements

* C++17 compatible compiler (GCC ≥ 7, Clang ≥ 6 recommended)
* CMake ≥ 3.10
* zlib development library

### Install zlib

**Ubuntu / Debian**

```bash
sudo apt-get install zlib1g-dev
```

**CentOS / RHEL**

```bash
sudo yum install zlib-devel
```

**macOS (Homebrew)**

```bash
brew install zlib
```

---

## Build Instructions (CMake)

Since the repository already includes a `CMakeLists.txt`, you can build as follows:

```bash
git clone https://github.com/<your-username>/combinePBWT.git
cd combinePBWT

mkdir build
cd build

cmake ..
make
```

The executable will be created in:

```
build/combine_chunklengths
```

For optimized builds:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
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

| Option             | Description                                      |
| ------------------ | ------------------------------------------------ |
| `-p`, `--pre_chr`  | Prefix before chromosome number                  |
| `-a`, `--post_chr` | Suffix after chromosome number                   |
| `-c`, `--chrs`     | Comma-separated chromosome list (e.g. `1,2,3,4`) |
| `-o`, `--output`   | Output gzipped file                              |
| `-t`, `--type`     | `pbwt`, `chromopainter`, or `SparsePainter`      |

---

## Example

If your files are:

```
chunk_chr1.out.gz
chunk_chr2.out.gz
chunk_chr3.out.gz
```

Run:

```bash
./combine_chunklengths \
  -p chunk_chr \
  -a .out.gz \
  -c 1,2,3 \
  -o combined.out.gz \
  -t chromopainter
```

---

## Supported Input Types

The tool automatically detects the ID column based on `--type`:

| Type          | Required Header Column |
| ------------- | ---------------------- |
| pbwt          | `RECIPIENT`            |
| chromopainter | `Recipient`            |
| SparsePainter | `indnames`             |

The ID column is removed during summation and restored in the output.

---

## Output

* Gzipped matrix file
* Summed values across chromosomes
* Float precision: 6 decimal places
* Preserves row and column labels

---

## Logging

The program prints timestamped progress messages:

```
2026-03-13 12:01:22  Processing chunk_chr1.out.gz
2026-03-13 12:01:45  Finished chunk_chr1.out.gz  rows=5000
2026-03-13 12:02:10  Done  (5000×5000)
```

Stdout is unbuffered for real-time monitoring (useful on clusters).

---

## Memory Usage

The full output matrix is stored in memory:

```
memory ≈ nrows × ncols × 4 bytes
```

Example:

* 10,000 × 10,000 matrix ≈ 400 MB RAM

Ensure sufficient memory for very large datasets.

---

## Notes

* Input files must have consistent dimensions.
* Row count mismatches will trigger warnings.
* Designed for high-performance genomic pipelines.

---

## License

Add your preferred license here.

---

## Author

Add author / contact information here.
