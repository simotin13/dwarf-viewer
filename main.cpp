#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdlib>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include "elf_parser.h"
#include "dwarf.h"
#include "logger.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage) ./dwarf-viewer <target path>" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    struct stat st;
    int ret = stat(argv[1], &st);
    if (ret < 0)
    {
        std::string msg = StringHelper::strprintf("%s not exitst", argv[1]);
        std::cout << msg << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Logger::DLog("target:[%s]", argv[1]);
    const uint64_t binSize = st.st_size;
    int fd = open(argv[1], O_RDONLY);
    if(fd < 0)
    {
        std::exit(EXIT_FAILURE);
        return -1;
    }

    const uint8_t* pBin = (uint8_t *)mmap(NULL, binSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!Elf::IsElf(pBin, binSize))
    {
        Logger::ELog("IsElf failed...");
        std::exit(EXIT_FAILURE);
    }

    if (!Elf::IsElf64(pBin, binSize))
    {
        Logger::ELog("IsElf64 failed...");
        std::exit(EXIT_FAILURE);
    }

    if (!Elf::IsLittleEndian(pBin, binSize))
    {
        Logger::ELog("IsLittleEndian failed...");
        std::exit(EXIT_FAILURE);
    }

    if (!Elf::IsCurrentVersion(pBin, binSize))
    {
        Logger::ELog("IsCurrentVersion failed...");
        std::exit(EXIT_FAILURE);
    }

    Elf64_Ehdr ehdr;
    std::vector<Elf64_Shdr> shdrs;
    std::vector<Elf64_Phdr> phdrs;
    Elf64::ReadEhdr(pBin, binSize, ehdr);
    uint64_t offset = ehdr.e_shoff;
    for (uint32_t i = 0; i < ehdr.e_shnum; i++)
    {
        Elf64_Shdr shdr;
        Elf64::ReadShdr(pBin, binSize, offset, shdr);
        shdrs.push_back(shdr);
        offset += ehdr.e_shentsize;
    }

    offset = ehdr.e_phoff;
    for (uint32_t i = 0; i < ehdr.e_phnum; i++)
    {
        Elf64_Phdr phdr;
        Elf64::ReadPhdr(pBin, binSize, offset, phdr);
        phdrs.push_back(phdr);
        offset += ehdr.e_phentsize;
    }

    std::map<std::string, uint32_t> sectionNameShdrIdxMap; // key: section name, value: section Idx
    Elf64_Shdr secStrSh = shdrs[ehdr.e_shstrndx];
    for (uint32_t i = 0; i < shdrs.size(); i++)
    {
        Elf64_Shdr &sh = shdrs.at(i);
        std::string secName = Elf64::GetSectionName(pBin, binSize, secStrSh, sh.sh_name);
        sectionNameShdrIdxMap[secName] = i;
    }

    // read .symtab
    uint32_t shIdx = sectionNameShdrIdxMap[".symtab"];
    Elf64_Shdr &symTabShdr = shdrs[shIdx];
    std::vector<Elf64_Sym> symTbl;
    Elf64::GetSymbolTbl(pBin, binSize, symTabShdr, symTbl);

    ElfFunctionTable elfFuncTable;
    // read .strtab
    shIdx = sectionNameShdrIdxMap[".strtab"];
    Elf64_Shdr &strTabShdr = shdrs[shIdx];
    Elf64::GetElfFuncInfos(pBin, binSize, shdrs, symTbl, secStrSh, strTabShdr, elfFuncTable.ElfFuncInfos);

    // make function addr <-> function Idx Map
    std::map<uint64_t, int> addrFuncIdxMap;
    for (uint32_t fIdx = 0; fIdx < elfFuncTable.ElfFuncInfos.size(); fIdx++)
    {
        ElfFunctionInfo &elfFuncInfo = elfFuncTable.ElfFuncInfos[fIdx];
        uint64_t offset = 0;
        while (offset < elfFuncInfo.Size)
        {
            uint64_t funcAddr = elfFuncInfo.Addr + offset;
            offset++;
            elfFuncTable.AddrFuncIdxMap[funcAddr] = fIdx;
        }
    }

    if (sectionNameShdrIdxMap.find(".debug_aranges") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_aranges section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    if (sectionNameShdrIdxMap.find(".debug_line") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_line section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    if (sectionNameShdrIdxMap.find(".debug_line_str") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_line_str section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    if (sectionNameShdrIdxMap.find(".debug_abbrev") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_abbrev section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    if (sectionNameShdrIdxMap.find(".debug_info") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_info section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    if (sectionNameShdrIdxMap.find(".debug_str") == sectionNameShdrIdxMap.end())
    {
        std::string msg = ".debug_str section not found. You need to set -g option for build.";
        std::cerr << msg << std::endl;;
        std::exit(EXIT_FAILURE);
    }

    shIdx = sectionNameShdrIdxMap[".debug_aranges"];
    Elf64_Shdr &debugArangesShdr = shdrs[shIdx];
    std::map<uint64_t, DwarfArangeInfo> arrangesMap = Dwarf::ReadAranges(pBin, binSize, debugArangesShdr);

    shIdx = sectionNameShdrIdxMap[".debug_line"];
    Elf64_Shdr &dbgLineShdr = shdrs[shIdx];

    shIdx = sectionNameShdrIdxMap[".debug_line_str"];
    Elf64_Shdr &dbgLineStrShdr = shdrs[shIdx];

    std::map<uint64_t, DwarfLineInfoHdr> offsetLineInfoMap = Dwarf::ReadLineInfo(pBin, binSize, dbgLineShdr, dbgLineStrShdr, elfFuncTable);

    shIdx = sectionNameShdrIdxMap[".debug_abbrev"];
    Elf64_Shdr &dbgAbbrevShdr = shdrs[shIdx];

    shIdx = sectionNameShdrIdxMap[".debug_info"];
    Elf64_Shdr &dbgInfoShdr = shdrs[shIdx];

    shIdx = sectionNameShdrIdxMap[".debug_str"];
    Elf64_Shdr &dbgStrShdr = shdrs[shIdx];

    std::vector<DwarfCuDebugInfo> dbgInfos = Dwarf::ReadDebugInfo(pBin, binSize, dbgInfoShdr, dbgStrShdr, dbgLineStrShdr, dbgAbbrevShdr, arrangesMap, offsetLineInfoMap);

    std::cout << "dwarf-viewer end..." << std::endl;
    std::exit(EXIT_SUCCESS);
}
