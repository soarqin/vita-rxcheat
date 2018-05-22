#include "convert.h"

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <codecvt>

static std::string srcFilename;
static std::string dstFilename;

void convertSetSource(const std::string &filename) {
    srcFilename = filename;
    dstFilename = srcFilename;
    size_t off = dstFilename.find_last_of('.');
    if (off == std::string::npos)
        dstFilename.append(".ini");
    else
        dstFilename = dstFilename.substr(0, off) + ".ini";
}

static int sectype = -1;
static std::string secname;
static bool pcJumping = false;
std::vector<MemoryRange> memoryRanges;
std::vector<std::string> lines;

static uint32_t convertMemoryToRange(uint32_t addr) {
    for (auto &p: memoryRanges) {
        if (p.start <= addr && addr < p.start + p.size) {
            return (p.index << 24) | (addr - p.start);
        }
    }
    return 0U;
}

static void processReset() {
    sectype = -1;
    secname.clear();
    pcJumping = false;
    lines.clear();
}

static void processLine(uint16_t op, uint32_t val1, uint32_t val2) {
    if (sectype < 0) return;
    std::string prefix = "_L ";
    switch (op >> 8) {
        case 0xA0:
            sectype |= 2;
            pcJumping = false;
            val1 = convertMemoryToRange(val1);
            break;
        case 0xA1:
            sectype |= 2;
            pcJumping = false;
            val1 = convertMemoryToRange(val1) | 0x10000000U;
            break;
        case 0xA2:
            sectype |= 2;
            val1 = convertMemoryToRange(val1) | 0x20000000U;
            if (pcJumping) {
                if (val2 >= 80000000U && val2 < 0x90000000U) {
                    prefix = "_E ";
                    val1 &= 0x0FFFFFFFU;
                    val2 = convertMemoryToRange(val2);
                } else {
                    pcJumping = false;
                }
            } else {
                if (val2 == 0xE51FF004) { // PC Jumping
                    pcJumping = true;
                }
            }
            break;
        default:
            return;
    }
    std::ostringstream os;
    os << prefix << std::hex << std::uppercase
        << std::setfill('0') << std::setw(8) << val1
        << " "
        << std::setfill('0') << std::setw(8) << val2;
    lines.push_back(os.str());
}

static void writeSection(std::ostream &outfile) {
    pcJumping = false;
    if (lines.empty()) return;
    outfile << std::endl << "_C" << sectype << " " << secname << std::endl;
    for (auto &p: lines) {
        outfile << p << std::endl;
    }
    lines.clear();
}

void convertStart(const MemoryRange *mr, int count) {
    memoryRanges.assign(mr, mr+count);
    for (auto &p: memoryRanges) {
        printf("Memory block %3d: %X %X %d\n", p.index, p.start, p.size, p.flag);
    }
    std::ifstream infile(srcFilename);
    if (!infile.is_open()) {
        std::cerr << "ERROR: Unable to open " << srcFilename << std::endl;
        return;
    }
    std::ofstream outfile(dstFilename);
    if (!outfile.is_open()) {
        std::cerr << "ERROR: Unable to write to " << dstFilename << std::endl;
        return;
    }
    processReset();
    std::string line;
    int lineno = 0;
    while (std::getline(infile, line)) {
        ++lineno;
        size_t pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos) continue;
        if (pos > 0) line = line.substr(pos);
        switch (line[0]) {
            case '$':
            {
                if (line.length() < 23) break;
                uint16_t op = (uint16_t)strtoul(line.substr(1, 4).c_str(), NULL, 16);
                pos = line.find_first_of(" \t");
                if (pos == std::string::npos) {
                    std::cerr << "ERROR: invalid line " << lineno << ": " << line << std::endl;
                    return;
                }
                pos = line.find_first_not_of(" \t", pos);
                if (pos == std::string::npos) {
                    std::cerr << "ERROR: invalid line " << lineno << ": " << line << std::endl;
                    return;
                }
                uint32_t val1 = strtoul(line.substr(pos, 8).c_str(), NULL, 16);
                pos = line.find_first_of(" \t", pos);
                if (pos == std::string::npos) {
                    std::cerr << "ERROR: invalid line " << lineno << ": " << line << std::endl;
                    return;
                }
                pos = line.find_first_not_of(" \t", pos);
                if (pos == std::string::npos) {
                    std::cerr << "ERROR: invalid line " << lineno << ": " << line << std::endl;
                    return;
                }
                uint32_t val2 = strtoul(line.substr(pos, 8).c_str(), NULL, 16);
                std::cout << "Processing: " << std::hex << std::uppercase << op << " " << val1 << " " << val2 << std::endl;
                processLine(op, val1, val2);
                break;
            }
            case '_':
            {
                if (line.length() < 3 || line[1] != 'V') break;
                writeSection(outfile);
                sectype = line[2] == '0' ? 0 : 1;
                secname.clear();
                pos = line.find_first_of(" \t", 2);
                if (pos != std::string::npos) {
                    pos = line.find_first_not_of(" \t", pos);
                    size_t spos = pos;
                    while (1) {
                        size_t epos = line.find_first_of("/#", spos);
                        if (epos == std::string::npos) {
                            secname = line.substr(spos);
                            break;
                        }
                        if (line[epos] == '#') {
                            secname = line.substr(spos, epos - pos);
                            break;
                        }
                        if (line[epos] == '/' && epos + 1 <= line.length() && line[epos + 1] == '/') {
                            secname = line.substr(spos, epos - pos);
                            break;
                        }
                        spos = epos + 1;
                    }
                }
                pos = secname.find_last_not_of(" \t");
                if (pos != std::string::npos) {
                    secname = secname.substr(0, pos + 1);
                }
                std::cout << "Section: " << secname << std::endl;
                break;
            }
        }
    }
    writeSection(outfile);
    outfile.close();
    infile.close();
}
