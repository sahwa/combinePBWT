#include <algorithm>
#include <chrono>      // timestamps for logging
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <cstring>     // std::memmove, std::strlen
#include <cfloat>      // FLT_MAX
#include <cerrno>
#include <cctype>      // std::isspace

/*
------------------------------------------------------------------------------
 Fast, memory-efficient combiner for ChromoPainter / pbwt / SparsePainter
 Now with timestamped progress logging and unbuffered stdout.
 - Reads arbitrarily long header lines safely (gzgets loop)
 - Splits on ANY whitespace (not just spaces)
 - Safer tokenization for streaming row parsing
------------------------------------------------------------------------------
*/

#define LOG(msg)                                                             \
  do {                                                                       \
    using clk = std::chrono::system_clock;                                   \
    auto  now = clk::to_time_t(clk::now());                                  \
    std::cout << std::put_time(std::localtime(&now), "%F %T") << "  " << msg \
              << std::endl;                                                  \
  } while (0)

/* --------------------------------------------------------------------- */
// simple CSV splitter for small strings like "1,2,3"
static std::vector<std::string> split_csv(const std::string& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream ss(s);
  while (std::getline(ss, token, delimiter)) {
    // trim leading/trailing whitespace
    auto start = token.find_first_not_of(" \t\r\n");
    auto end   = token.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) tokens.emplace_back(token.substr(start, end - start + 1));
  }
  return tokens;
}

// split on any whitespace
static std::vector<std::string> split_ws(const std::string& s)
{
  std::istringstream iss(s);
  std::vector<std::string> out;
  for (std::string tok; iss >> tok; ) out.push_back(tok);
  return out;
}

void usage(const char* prog)
{
  std::cerr << "Usage: " << prog
            << " -p <pre_chr> -a <post_chr> -c <chrs> -o <output> -t <type>\n";
}

/* -------------------------------------------------------------------------
   SparsePainter row discovery (rectangular matrices)
   --------------------------------------------------------------------- */
static std::size_t collect_row_names_sparsepainter(
    const std::string        &filename,
    int                       removeIndex,
    std::vector<std::string> &rowNames,
    char                     *lineBuf,
    const std::size_t         lineBufSz)
{
  gzFile fh = gzopen(filename.c_str(), "rb");
  if (!fh) {
    std::cerr << "[collect] could not open " << filename << '\n';
    std::exit(1);
  }

  // read + discard header (could be very long)
  std::string header;
  while (true) {
    char* got = gzgets(fh, lineBuf, lineBufSz);
    if (!got) { std::cerr << "[collect] empty file " << filename << '\n'; std::exit(1); }
    header.append(got);
    size_t len = std::strlen(got);
    if (len && got[len - 1] == '\n') break;
  }

  rowNames.clear();
  rowNames.reserve(4'000'000);   // heuristic

  // read rows; extract the token at removeIndex (i.e., the ID column)
  std::string spill; spill.reserve(1024);
  constexpr std::size_t CHUNK = 32 * 1024 * 1024;
  std::vector<char> chunk(CHUNK);

  auto skip_ws = [](const char*& p, const char* end) {
    while (p < end && std::isspace(static_cast<unsigned char>(*p))) ++p;
  };

  while (true) {
    int got = gzread(fh, chunk.data(), CHUNK);
    if (got <= 0) break;
    const char* data      = chunk.data();
    const char* endChunk  = data + got;
    const char* lineStart = data;

    for (const char* p = data; p < endChunk; ++p) {
      if (*p == '\n') {
        spill.append(lineStart, p - lineStart);

        const char* cur = spill.data();
        const char* lineEnd = cur + spill.size();
        int col = 0;

        // walk tokens by whitespace
        while (true) {
          skip_ws(cur, lineEnd);
          if (cur >= lineEnd) break;
          const char* tokBeg = cur;
          while (cur < lineEnd && !std::isspace(static_cast<unsigned char>(*cur))) ++cur;

          if (col == removeIndex) {
            rowNames.emplace_back(tokBeg, static_cast<std::size_t>(cur - tokBeg));
            break; // we only needed the ID column
          }
          ++col;
        }

        spill.clear();
        lineStart = p + 1;
      }
    }
    if (lineStart < endChunk) spill.append(lineStart, endChunk - lineStart);
  }

  gzclose(fh);
  return rowNames.size();
}

/* --------------------------------------------------------------------- */
inline bool next_token(const char *&p, const char *end)
{
  while (p < end && std::isspace(static_cast<unsigned char>(*p))) ++p;
  return p < end;
}

/* --------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
  /* ---- unbuffered stdout so every log line is immediate --------------- */
  std::cout.setf(std::ios::unitbuf);

  LOG("starting combine_chunklengths");

  /* ---- command-line parsing ------------------------------------------ */
  std::string pre_chr, post_chr, chrsStr, output, prog;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-p") || (arg == "--pre_chr"))        pre_chr = argv[++i];
    else if ((arg == "-a") || (arg == "--post_chr"))  post_chr = argv[++i];
    else if ((arg == "-c") || (arg == "--chrs"))      chrsStr = argv[++i];
    else if ((arg == "-o") || (arg == "--output"))    output  = argv[++i];
    else if ((arg == "-t") || (arg == "--type"))      prog    = argv[++i];
    else { usage(argv[0]); return 1; }
  }

  if (prog != "pbwt" && prog != "chromopainter" && prog != "SparsePainter") {
    std::cerr << "--type must be pbwt, chromopainter, or SparsePainter\n";
    return 1;
  }

  std::vector<std::string> chrs = split_csv(chrsStr, ',');
  if (chrs.empty()) {
    std::cerr << "No chromosomes specified\n";
    return 1;
  }

  LOG("pre_chr=" << pre_chr << "  post_chr=" << post_chr
      << "  chrs=" << chrsStr << "  output=" << output
      << "  type=" << prog);

  constexpr std::size_t LINE_BUF = 1 << 20;   // 1 MiB per gzgets chunk
  char* lineBuf = new char[LINE_BUF];

  /* ---- read header of first chromosome (robustly) --------------------- */
  std::string firstFile = pre_chr + chrs[0] + post_chr;
  gzFile gzfirst = gzopen(firstFile.c_str(), "rb");
  if (!gzfirst) { std::cerr << "Cannot open " << firstFile << '\n'; return 1; }

  std::string headerLine;
  headerLine.reserve(8 << 20); // 8 MiB initial guess
  while (true) {
    char* got = gzgets(gzfirst, lineBuf, LINE_BUF);
    if (!got) { std::cerr << "Header read error\n"; return 1; }
    headerLine.append(got);
    size_t len = std::strlen(got);
    if (len && got[len - 1] == '\n') {
      // strip trailing newline
      if (!headerLine.empty() && headerLine.back() == '\n') headerLine.pop_back();
      break;
    }
  }

  const auto headers = split_ws(headerLine);

  int removeIndex = -1;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    if ((prog == "pbwt"          && headers[i] == "RECIPIENT") ||
        (prog == "chromopainter" && headers[i] == "Recipient") ||
        (prog == "SparsePainter" && headers[i] == "indnames")) {
      removeIndex = static_cast<int>(i);
      break;
    }
  }
  if (removeIndex == -1) {
    std::cerr << "Could not locate ID column in header\n";
    return 1;
  }

  std::vector<std::string> colNames;
  std::vector<int>         colIndices;
  colNames.reserve(headers.size() ? headers.size() - 1 : 0);
  for (std::size_t i = 0; i < headers.size(); ++i) {
    if (static_cast<int>(i) != removeIndex) {
      colNames  .push_back(headers[i]);
      colIndices.push_back(static_cast<int>(i));
    }
  }
  const std::size_t ncols = colNames.size();

  /* ---- row discovery -------------------------------------------------- */
  std::vector<std::string> rowNames;
  std::size_t nrows = 0;
  if (prog == "SparsePainter") {
    gzclose(gzfirst);
    nrows = collect_row_names_sparsepainter(firstFile, removeIndex,
                                            rowNames, lineBuf, LINE_BUF);
  } else {
    rowNames = colNames;  // square matrix for pbwt / chromopainter
    nrows    = ncols;
    gzrewind(gzfirst);
  }

  LOG("matrix size will be " << nrows << " rows × " << ncols << " cols");

  // WARNING: nrows * ncols could be huge. Consider tiling if needed.
  std::vector<float> total;
  try {
    total.assign(nrows * ncols, 0.0f);
  } catch (const std::bad_alloc&) {
    std::cerr << "Memory allocation failed for matrix of size "
              << nrows << " x " << ncols << '\n';
    return 1;
  }

  /* ---- streaming decode helper --------------------------------------- */
  constexpr std::size_t CHUNK = 32 * 1024 * 1024;   // 32 MiB
  std::vector<char> chunk(CHUNK);

  auto processFile = [&](const std::string& fname)
  {
    LOG("Processing " << fname);
    gzFile gzf = gzopen(fname.c_str(), "rb");
    if (!gzf) { std::cerr << "Cannot open " << fname << '\n'; std::exit(1); }

    // skip header line (robustly)
    std::string dummyHeader;
    while (true) {
      char* got = gzgets(gzf, lineBuf, LINE_BUF);
      if (!got) { std::cerr << "Header read error in " << fname << '\n'; std::exit(1); }
      dummyHeader.append(got);
      size_t len = std::strlen(got);
      if (len && got[len - 1] == '\n') break;
    }

    std::string spill; spill.reserve(1024);
    std::size_t row = 0;

    while (true) {
      int got = gzread(gzf, chunk.data(), CHUNK);
      if (got <= 0) break;
      const char* data      = chunk.data();
      const char* endChunk  = data + got;
      const char* lineStart = data;

      for (const char* p = data; p < endChunk; ++p) {
        if (*p == '\n') {
          spill.append(lineStart, p - lineStart);

          const char* cur = spill.data();
          const char* lineEnd = cur + spill.size();
          int col = 0, outCol = 0;

          while (next_token(cur, lineEnd)) {
            const char* tokBeg = cur;
            while (cur < lineEnd && !std::isspace(static_cast<unsigned char>(*cur))) ++cur;

            if (col != removeIndex) {
              errno = 0;
              float v = strtof(tokBeg, nullptr);
              if (errno == ERANGE) v = (v < 0 ? -FLT_MAX : FLT_MAX);
              if (row < nrows && static_cast<std::size_t>(outCol) < ncols) {
                total[row * ncols + outCol] += v;
              }
              ++outCol;
            }
            ++col;
          }
          ++row;
          spill.clear();
          lineStart = p + 1;
        }
      }
      if (lineStart < endChunk) spill.append(lineStart, endChunk - lineStart);
    }

    if (row != nrows) {
      std::cerr << "Warning: " << fname << " has " << row
                << " rows (expected " << nrows << ")\n";
    }
    gzclose(gzf);
    LOG("Finished " << fname << "  rows=" << row);
  };

  /* ---- pass over all chromosomes ------------------------------------- */
  if (prog != "SparsePainter") {
    // resume at beginning for first file
    processFile(firstFile);
  } else {
    // we closed gzfirst already in SparsePainter branch; reopen
    processFile(firstFile);
  }
  for (std::size_t i = 1; i < chrs.size(); ++i)
    processFile(pre_chr + chrs[i] + post_chr);

  LOG("All chromosomes processed");

  /* ---- write result --------------------------------------------------- */
  LOG("Writing gzipped output to " << output);

  gzFile out = gzopen(output.c_str(), "wb");
  if (!out) { std::cerr << "Cannot create output " << output << '\n'; return 1; }

  const char* idLabel =
      (prog == "pbwt") ? "RECIPIENT" :
      (prog == "chromopainter") ? "Recipient" :
      "indnames";

  gzprintf(out, "%s", idLabel);
  for (const auto& c : colNames) gzprintf(out, " %s", c.c_str());
  gzprintf(out, "\n");

  for (std::size_t r = 0; r < nrows; ++r) {
    gzprintf(out, "%s", rowNames[r].c_str());
    for (std::size_t c = 0; c < ncols; ++c)
      gzprintf(out, " %.6f", total[r * ncols + c]);
    gzprintf(out, "\n");
  }
  gzclose(out);

  LOG("Done  (" << nrows << "×" << ncols << ")");
  delete[] lineBuf;
  return 0;
}

