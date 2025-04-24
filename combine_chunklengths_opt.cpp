#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

constexpr std::size_t BUFFER_SIZE = 1 << 20; // 1 MiB

// Split on arbitrary whitespace
std::vector<std::string> split_ws(const std::string &s) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string tok;
    while (ss >> tok) out.push_back(tok);
    return out;
}

// Split on a single character delimiter
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out;
    std::string tok;
    std::istringstream ss(s);
    while (std::getline(ss, tok, delim)) {
        if (!tok.empty()) out.push_back(tok);
    }
    return out;
}

// Simple RAII wrapper around gzFile
struct GzFile {
    gzFile f;
    GzFile(const std::string &path, const char *mode) : f(gzopen(path.c_str(), mode)) {
        if (!f) throw std::runtime_error("Cannot open " + path);
    }
    ~GzFile() { if (f) gzclose(f); }
    operator gzFile() const { return f; }
};

// Accumulate the numeric matrix of one chromosome into a local buffer
void accumulate_matrix(const std::string &filename,
                       int remove_idx,
                       std::size_t ncols,
                       std::vector<float> &local) {

    GzFile in(filename, "rb");
    std::unique_ptr<char[]> buf(new char[BUFFER_SIZE]);

    // Skip header line
    if (!gzgets(in, buf.get(), BUFFER_SIZE)) return;

    std::size_t row = 0;
    while (gzgets(in, buf.get(), BUFFER_SIZE)) {
        char *p = buf.get();
        int col = 0, out_col = 0;
        float *row_ptr = local.data() + row * ncols;

        while (true) {
            // Skip leading whitespace between tokens
            while (*p == ' ' || *p == '\t') ++p;
            if (*p == '\0' || *p == '\n') break;

            char *end;
            float v = std::strtof(p, &end);
            if (col != remove_idx) row_ptr[out_col++] += v;
            ++col;
            p = end;
        }
        ++row;
    }
}

} // namespace

int main(int argc, char *argv[]) try {
    std::string pre_chr, post_chr, chrs_str, output, prog;

    // -----------------------------
    // Lightweight CLI parsing
    // -----------------------------
    auto next_arg = [&](int &i) -> std::string {
        if (++i >= argc) throw std::runtime_error("Missing value for option");
        return argv[i];
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-p" || arg == "--pre_chr") pre_chr = next_arg(i);
        else if (arg == "-a" || arg == "--post_chr") post_chr = next_arg(i);
        else if (arg == "-c" || arg == "--chrs") chrs_str = next_arg(i);
        else if (arg == "-o" || arg == "--output") output = next_arg(i);
        else if (arg == "-t" || arg == "--type") prog = next_arg(i);
        else throw std::runtime_error("Unknown option " + arg);
    }

    if (prog != "pbwt" && prog != "chromopainter")
        throw std::runtime_error("--type must be 'pbwt' or 'chromopainter'");

    const auto chromosomes = split(chrs_str, ',');
    if (chromosomes.empty()) throw std::runtime_error("No chromosomes provided");

    // ----------------------------------
    // Read header of the first chromosome
    // ----------------------------------
    const std::string first_path = pre_chr + chromosomes.front() + post_chr;
    GzFile first(first_path, "rb");

    std::unique_ptr<char[]> buf(new char[BUFFER_SIZE]);
    if (!gzgets(first, buf.get(), BUFFER_SIZE))
        throw std::runtime_error("Cannot read header from " + first_path);

    const auto header = split_ws(buf.get());
    int remove_idx = -1;
    for (std::size_t i = 0; i < header.size(); ++i) {
        if ((prog == "pbwt" && header[i] == "RECIPIENT") ||
            (prog == "chromopainter" && header[i] == "Recipient")) {
            remove_idx = static_cast<int>(i);
            break;
        }
    }
    if (remove_idx == -1) throw std::runtime_error("Recipient column not found");

    std::vector<std::string> samples;
    for (std::size_t i = 0; i < header.size(); ++i)
        if (static_cast<int>(i) != remove_idx) samples.push_back(header[i]);

    const std::size_t nrows = samples.size();
    const std::size_t ncols = samples.size();
    std::vector<float> total(nrows * ncols, 0.0f);

    std::cerr << "Processing " << chromosomes.size() << " chromosomes...\n";

    // ---------------------------------
    // Parallel over input chromosomes
    // ---------------------------------
    #pragma omp parallel for default(none) shared(chromosomes, pre_chr, post_chr, remove_idx, ncols, total)
    for (std::size_t idx = 0; idx < chromosomes.size(); ++idx) {
        const std::string file = pre_chr + chromosomes[idx] + post_chr;
        std::vector<float> local(total.size(), 0.0f);

        accumulate_matrix(file, remove_idx, ncols, local);

        // Reduce into global matrix
        #pragma omp critical
        for (std::size_t i = 0; i < total.size(); ++i) total[i] += local[i];
    }

    // -----------------
    // Write gzipped output
    // -----------------
    GzFile out(output, "wb");

    gzprintf(out, "%s", (prog == "pbwt" ? "RECIPIENT" : "Recipient"));
    for (const auto &s : samples) gzprintf(out, " %s", s.c_str());
    gzprintf(out, "\n");

    for (std::size_t r = 0; r < nrows; ++r) {
        gzprintf(out, "%s", samples[r].c_str());
        const float *rowptr = total.data() + r * ncols;
        for (std::size_t c = 0; c < ncols; ++c)
            gzprintf(out, " %.6f", rowptr[c]);
        gzprintf(out, "\n");
    }

    std::cerr << "Done.\n";
    return 0;

} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
}

