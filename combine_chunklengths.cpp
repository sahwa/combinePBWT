#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    token.erase(std::remove_if(token.begin(), token.end(), ::isspace),
                token.end());
    if (!token.empty())
      tokens.push_back(token);
  }
  return tokens;
}

void usage(const char *progName) {
  std::cerr << "Usage: " << progName
            << " -p <pre_chr> -a <post_chr> -c <chrs> -o <output> -t <type>\n";
}

int main(int argc, char *argv[]) {
  std::string pre_chr, post_chr, chrsStr, output, prog;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if ((arg == "-p") || (arg == "--pre_chr"))
      pre_chr = argv[++i];
    else if ((arg == "-a") || (arg == "--post_chr"))
      post_chr = argv[++i];
    else if ((arg == "-c") || (arg == "--chrs"))
      chrsStr = argv[++i];
    else if ((arg == "-o") || (arg == "--output"))
      output = argv[++i];
    else if ((arg == "-t") || (arg == "--type"))
      prog = argv[++i];
    else {
      usage(argv[0]);
      return 1;
    }
  }

  std::cout << "Parsed arguments:\n"
            << "   Pre-chr path: " << pre_chr << "\n"
            << "   Post-chr path: " << post_chr << "\n"
            << "   Chromosomes: " << chrsStr << "\n"
            << "   Output: " << output << "\n"
            << "   Program type: " << prog << "\n";

  if (prog != "pbwt" && prog != "chromopainter") {
    std::cerr << "Invalid --type. Must be 'pbwt' or 'chromopainter'.\n";
    return 1;
  }

  std::vector<std::string> chrs = split(chrsStr, ',');
  if (chrs.empty()) {
    std::cerr << "No chromosomes provided.\n";
    return 1;
  }

  const int bufferSize = 64 * 1024 * 1024;
  char *buffer = new char[bufferSize];

  std::string firstFile = pre_chr + chrs[0] + post_chr;
  gzFile gzfirst = gzopen(firstFile.c_str(), "rb");
  if (!gzfirst) {
    std::cerr << "Error opening file: " << firstFile << "\n";
    return 1;
  }

  if (gzgets(gzfirst, buffer, bufferSize) == nullptr) {
    std::cerr << "Error reading header from file.\n";
    return 1;
  }
  std::string headerLine(buffer);
  headerLine.erase(std::remove(headerLine.begin(), headerLine.end(), '\n'),
                   headerLine.end());

  std::vector<std::string> headers = split(headerLine, ' ');

  int removeIndex = -1;
  for (std::size_t i = 0; i < headers.size(); i++) {
    if ((prog == "pbwt" && headers[i] == "RECIPIENT") ||
        (prog == "chromopainter" && headers[i] == "Recipient")) {
      removeIndex = static_cast<int>(i);
      break;
    }
  }

  if (removeIndex == -1) {
    std::cerr << "Could not find RECIPIENT or Recipient column in header.\n";
    return 1;
  }

  std::vector<std::string> sampleNames;
  std::vector<int> colIndices;
  for (std::size_t i = 0; i < headers.size(); i++) {
    if (static_cast<int>(i) != removeIndex) {
      colIndices.push_back(i);
      sampleNames.push_back(headers[i]);
    }
  }

  std::size_t nrows = sampleNames.size();
  std::size_t ncols = sampleNames.size();
  std::cout << "Header contains " << headers.size() << " columns.\n";
  std::cout << "Dropping column at index  " << removeIndex << ". Keeping "
            << ncols << " columns.\n";
  std::cout << "Allocating matrix of size " << nrows << " x " << ncols << "\n";

  std::vector<float> total(nrows * ncols, 0.0f);
  gzclose(gzfirst);

  auto processFile = [&](const std::string &filename) {
    std::cout << "Reading file: " << filename << "\n";
    gzFile gzfile = gzopen(filename.c_str(), "rb");
    if (!gzfile) {
      std::cerr << "Error opening file: " << filename << "\n";
      std::exit(1);
    }

    gzgets(gzfile, buffer, bufferSize); // discard header
    std::size_t row = 0;

    while (gzgets(gzfile, buffer, bufferSize) != nullptr) {
      std::string line(buffer);
      std::istringstream ss(line);
      std::string token;
      int col = 0, outCol = 0;

      while (ss >> token) {
        if (col == removeIndex) {
          col++;
          continue;
        }
        try {
          float val = std::stof(token);
          total[row * ncols + outCol] += val;
        } catch (const std::exception &e) {
          std::cerr << "Invalid float at row " << row << ", col " << col
                    << ": '" << token << "'\n";
        }
        col++;
        outCol++;
      }
      row++;
      if (row % 10000 == 0)
        std::cout << "  Processed " << row << " rows...\r" << std::flush;
    }
    std::cout << "Done processing " << filename << ".\n";
    gzclose(gzfile);
  };

  for (const auto &chr : chrs) {
    std::string filename = pre_chr + chr + post_chr;
    processFile(filename);
  }

  std::cout << "Merging complete. Writing output...\n";

  gzFile outFile = gzopen(output.c_str(), "wb");
  if (!outFile) {
    std::cerr << "Error opening gzipped output file: " << output << "\n";
    return 1;
  }

  gzprintf(outFile, "%s", (prog == "pbwt" ? "RECIPIENT" : "Recipient"));
  for (const auto &name : sampleNames) {
    gzprintf(outFile, " %s", name.c_str());
  }
  gzprintf(outFile, "\n");

  for (std::size_t i = 0; i < nrows; i++) {
    gzprintf(outFile, "%s", sampleNames[i].c_str());
    for (std::size_t j = 0; j < ncols; j++) {
      gzprintf(outFile, " %.6f", total[i * ncols + j]);
    }
    gzprintf(outFile, "\n");
  }

  gzclose(outFile);
}
