#include "convert.h"

#include <vector>
#include <tuple>
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

std::vector<MemoryRange> memoryRanges;

struct CodeLine {
    uint16_t op;
    uint32_t val1;
    uint32_t val2;
};
static int sectype = -1;
static std::string secname;
std::vector<CodeLine> rLines;
static bool pcJumping = false;
std::vector<std::tuple<std::string, uint32_t, uint32_t>> lines;

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

static inline void addLine(uint32_t val1, uint32_t val2, const std::string &op = "L") {
    lines.push_back(std::make_tuple("_" + op, val1, val2));
}

static void processLine(size_t &index) {
    if (sectype < 0) return;
    uint32_t op = rLines[index].op;
    uint32_t skips = op & 0xFF;
    uint32_t bits = (op >> 8) & 0xF;
    op >>= 12;
    uint32_t val1 = rLines[index].val1;
    uint32_t val2 = rLines[index++].val2;
    switch (op) {
        case 0x0:
            addLine(convertMemoryToRange(val1) | (bits << 28), val2);
            break;
        case 0x3: {
            addLine(convertMemoryToRange(val1) | 0x90000000U, (bits << 28) | skips);
            for (uint32_t i = 0; i < skips - 1; ++i, ++index) {
                addLine(rLines[index - 1].val2, 0U);
            }
            addLine(rLines[index - 1].val2, rLines[index].val2);
            ++index;
            break;
        }
        case 0x4: {
            uint32_t steps = (uint32_t)rLines[index].val1;
            while (bits > 0 && ((steps >> bits) << bits) != steps) --bits;
            steps >>= bits;
            if (bits < 2) {
                addLine(convertMemoryToRange(val1) | 0x80000000U, ((uint32_t)rLines[index].op << 16) | (steps & 0xFFFFU));
                addLine((bits << 28) | (val2 & 0xFFFFU), rLines[index++].val2 & 0xFFFFU);
            } else {
                addLine(convertMemoryToRange(val1) | 0x40000000U, ((uint32_t)rLines[index].op << 16) | (steps & 0xFFFFU));
                addLine(val2, rLines[index++].val2);
            }
            break;
        }
        case 0x5:
            addLine(convertMemoryToRange(val1) | 0x50000000U, 1U << bits);
            addLine(convertMemoryToRange(val2), 0);
            return;
        case 0x7: {
            addLine(convertMemoryToRange(val1) | 0x90000000U, (bits << 28) | 0x2000000U | (rLines[index + skips].op << 8) | (skips + 1));
            for (uint32_t i = 0; i < skips - 1; ++i, ++index) {
                addLine(rLines[index - 1].val2, 0U);
            }
            addLine(rLines[index - 1].val2, rLines[index].val2);
            ++index;
            addLine(((rLines[index - 1].op & 0xFFU) << 24) | (rLines[index].val1 & 0xFFFFFFU), rLines[index].val2);
            ++index;
            break;
        }
        case 0x8: {
            uint32_t skips2 = rLines[index + skips].op & 0xFFU;
            if (skips2 != skips) break;
            if (lines.size() > 0) {
                for (int i = (int)lines.size() - 1; i >= 0; --i) {
                    auto &ln = lines[i];
                    uint32_t op = std::get<1>(ln) >> 24, sub = std::get<2>(ln) >> 28;
                    if (op == 0xD0 && (sub == 1 || sub == 3)) {
                        uint32_t bskips = std::get<1>(ln) & 0xFFU;
                        if (i + bskips + 2 >= lines.size() + skips * 2 + 2)
                            std::get<1>(ln) -= skips;
                    }
                    if (op == 0xE0 || op == 0xE1) {
                        uint32_t bskips = (std::get<1>(ln) >> 16) & 0xFFU;
                        if (i + bskips + 1 >= lines.size() + skips * 2 + 2)
                            std::get<1>(ln) -= (skips << 16);
                    }
                    if (op == 0xE2) {
                        uint32_t bskips = std::get<1>(lines[i + 1]) & 0xFFU;
                        if (i + bskips + 2 >= lines.size() + skips * 2 + 2)
                            std::get<1>(lines[i + 1]) -= skips;
                    }
                }
            }
            addLine(convertMemoryToRange(val1) | 0x90000000U, (bits << 28) | 0x1000000U | (skips + 1));
            addLine(convertMemoryToRange(rLines[index + skips].val1), 0);
            for (uint32_t i = 0; i < skips; ++i, ++index) {
                addLine(rLines[index - 1].val2, rLines[index + skips].val2);
            }
            index += skips + 1;
            break;
        }
        case 0xA:
            sectype |= 2;
            if (bits < 2)
                pcJumping = false;
            else {
                if (pcJumping) {
                    if (val2 >= 80000000U && val2 < 0x90000000U) {
                        addLine(convertMemoryToRange(val1), convertMemoryToRange(val2), "E");
                        break;
                    } else {
                        pcJumping = false;
                    }
                } else {
                    if (val2 == 0xE51FF004) { // PC Jumping
                        pcJumping = true;
                    }
                }
            }
            addLine(convertMemoryToRange(val1) | (bits << 28), val2);
            break;
        case 0xC: {
            if (skips == 0) break;
            switch (val1) {
                case 1: // Convert LTRIGGER/RTRIGGER to L1/R1
                    if (val2 & 0x100U)
                        val2 = (val2 & ~0x100U) | 0x400U;
                    if (val2 & 0x200U)
                        val2 = (val2 & ~0x200U) | 0x800U;
                case 2:
                case 3:
                case 4:
                    addLine(0xD0000000U | (skips - 1), val2 | 0x10000000U);
                    break;
                default:
                    break;
            }
            break;
        }
        case 0xD: {
            uint32_t comp = bits / 3;
            bits %= 3;
            if (comp == 2 || comp == 3) comp = 5 - comp;
            if (bits < 2)
                addLine(0xE0000000U | ((1U - bits) << 24) | (skips << 16) | (val2 & 0xFFFFU), (comp << 28) | (convertMemoryToRange(val1)));
            else {
                addLine(0xE2000000U, (comp << 28) | (convertMemoryToRange(val1)));
                addLine(skips, val2);
            }
            break;
        }
        default:
            break;
    }
}

static void writeSection(std::ofstream &outfile) {
    const char *NEWLINE =
#ifdef _WIN32
        "\r\n";
#else
        "\n";
#endif
    size_t sz = rLines.size();
    for (size_t i = 0; i < rLines.size();) {
        processLine(i);
    }
    pcJumping = false;
    rLines.clear();
    if (lines.empty()) return;
#ifdef _WIN32
    const char* GBK_LOCALE_NAME = ".936";
#else
    const char* GBK_LOCALE_NAME = "zh_CN.GBK";
#endif
    static std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> cv1(new std::codecvt_byname<wchar_t, char, mbstate_t>(GBK_LOCALE_NAME));
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> cv2;

    outfile << NEWLINE << "_C" << sectype << " " << cv2.to_bytes(cv1.from_bytes(secname)) << NEWLINE;
    for (auto &p: lines) {
        outfile << std::get<0>(p) << " 0x" << std::hex << std::uppercase
            << std::setfill('0') << std::setw(8) << std::get<1>(p)
            << " 0x"
            << std::setfill('0') << std::setw(8) << std::get<2>(p)
            << NEWLINE;
    }
    lines.clear();
}

void convertStart(const MemoryRange *mr, int count) {
    memoryRanges.assign(mr, mr+count);
    for (auto &p: memoryRanges) {
        printf("Memory block %3d: %X %X %d\n", p.index, p.start, p.size, p.flag);
    }
    std::ifstream infile(srcFilename, std::ios::in);
    if (!infile.is_open()) {
        std::cerr << "ERROR: Unable to open " << srcFilename << std::endl;
        return;
    }
    std::ofstream outfile(dstFilename, std::ios::out | std::ios::binary);
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
                rLines.push_back(CodeLine {op, val1, val2});
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
                            secname = line.substr(pos);
                            break;
                        }
                        if (line[epos] == '#') {
                            secname = line.substr(pos, epos - pos);
                            break;
                        }
                        if (line[epos] == '/' && epos + 1 <= line.length() && line[epos + 1] == '/') {
                            secname = line.substr(pos, epos - pos);
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
