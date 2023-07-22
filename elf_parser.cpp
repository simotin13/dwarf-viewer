#include <stdint.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <elf.h>
#include "binutil.h"
#include "elf_parser.h"
#include "common.h"
#include "logger.h"

bool Elf::IsElf(const uint8_t *bin, const uint64_t size)
{
    // size should be atleast sizeof Elf32_Ehdr
    if (size < sizeof(Elf32_Ehdr))
    {
        return false;
    }

    // check <0x7F>ELF
    if (bin[0] != 0x7F)
    {
        return false;
    }
    if (bin[1] != 'E')
    {
        return false;
    }
    if (bin[2] != 'L')
    {
        return false;
    }
    if (bin[3] != 'F')
    {
        return false;
    }
    return true;
}

bool Elf::IsElf32(const uint8_t *bin, const uint64_t size)
{
    return bin[4] == ELFCLASS32;
}
bool Elf::IsElf64(const uint8_t *bin, const uint64_t size)
{
    return bin[4] == ELFCLASS64;
}

bool Elf::IsLittleEndian(const uint8_t *bin, const uint64_t size)
{
    return bin[5] == ELFDATA2LSB;
}
bool Elf::IsCurrentVersion(const uint8_t *bin, const uint64_t size)
{
    return bin[6] == EV_CURRENT;
}

bool Elf64::ReadEhdr(const uint8_t *bin, const uint64_t size, Elf64_Ehdr &ehdr)
{
    Logger::TLog("ReadEhdr In...");
    uint64_t offset = 0;
    std::memcpy(ehdr.e_ident, bin, EI_NIDENT);
    offset += EI_NIDENT;

    ehdr.e_type = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_machine = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_version = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    ehdr.e_entry = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    ehdr.e_phoff = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    ehdr.e_shoff = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    ehdr.e_flags = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    ehdr.e_ehsize = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_phentsize = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_phnum = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_shentsize = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_shnum = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    ehdr.e_shstrndx = BinUtil::FromLeToUInt16(&bin[offset]);
    offset += 2;

    Logger::TLog("ReadEhdr Out...");
    return true;
}

bool Elf64::ReadShdr(const uint8_t *bin, const uint64_t size, uint64_t offset, Elf64_Shdr &shdr)
{
    shdr.sh_name = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    shdr.sh_type = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    shdr.sh_flags = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    shdr.sh_addr = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    shdr.sh_offset = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    shdr.sh_size = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    shdr.sh_link = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    shdr.sh_info = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    shdr.sh_addralign = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    shdr.sh_entsize = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    return true;
}

bool Elf64::ReadPhdr(const uint8_t *bin, const uint64_t size, uint64_t offset, Elf64_Phdr &phdr)
{
    phdr.p_type = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    phdr.p_flags = BinUtil::FromLeToUInt32(&bin[offset]);
    offset += 4;

    phdr.p_offset = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    phdr.p_vaddr = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    phdr.p_paddr = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    phdr.p_filesz = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    phdr.p_memsz = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;

    phdr.p_align = BinUtil::FromLeToUInt64(&bin[offset]);
    offset += 8;
    return true;
}

std::string Elf64::GetSectionName(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &strShdr, uint64_t sh_name)
{
    uint64_t offset = strShdr.sh_offset + sh_name;
    uint64_t shEnd = strShdr.sh_offset + strShdr.sh_size;
    std::string secName = "";
    while(offset < shEnd)
    {
        if (bin[offset] == 0)
        {
            break;
        }

        secName += bin[offset];
        offset++;
    }

    return secName;
}

bool Elf64::GetSymbolTbl(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &symTabShdr, std::vector<Elf64_Sym> &symTbl)
{
    Logger::TLog("GetSymbolTbl In...");
    uint64_t offset = symTabShdr.sh_offset;
    uint64_t sectionEnd = symTabShdr.sh_offset + symTabShdr.sh_size;
    while(offset < sectionEnd)
    {
        Elf64_Sym sym;
        sym.st_name     = BinUtil::FromLeToUInt32(&bin[offset]);
        offset += 4;

        sym.st_info     = bin[offset];
        offset++;

        sym.st_other    = bin[offset];
        offset++;

        sym.st_shndx    = BinUtil::FromLeToUInt16(&bin[offset]);
        offset += 2;

        sym.st_value    = BinUtil::FromLeToUInt64(&bin[offset]);
        offset += 8;

        sym.st_size     = BinUtil::FromLeToUInt64(&bin[offset]);
        offset += 8;

        symTbl.push_back(sym);
    }

    Logger::TLog("GetSymbolTbl Out...");
    return true;
}

bool Elf64::GetElfFuncInfos(const uint8_t *bin, const uint64_t size, const std::vector<Elf64_Shdr> &shdrs, const std::vector<Elf64_Sym> &symTbl, const Elf64_Shdr &secStrShdr, const Elf64_Shdr &strTabShdr, std::vector<ElfFunctionInfo> &elfFuncInfos)
{
    Logger::TLog("GetElfFuncInfos In...");

    // read string section
    elfFuncInfos.clear();
    for(auto it = symTbl.begin(); it != symTbl.end(); it++)
    {
        Elf64_Sym sym = *it;
        if ((sym.st_info & 0x0F) == STT_FUNC)
        {
            if (isSpecialShndx(sym.st_shndx))
            {
                continue;
            }

            ElfFunctionInfo f;
            Logger::DLog("strTabShdr.sh_offset:[%ld], sym.st_name:[%ld], sym.st_value:[%ld], sym.st_size:[%ld], sym.st_shndx:[%d]", strTabShdr.sh_offset, sym.st_name, sym.st_value, sym.st_size, sym.st_shndx);
            f.Name = GetStrFromStrTbl(&bin[strTabShdr.sh_offset], strTabShdr.sh_size, sym.st_name);
            f.Addr = sym.st_value;
            f.Size = sym.st_size;

            Elf64_Shdr symShdr = shdrs[sym.st_shndx];
            f.SecName = GetSectionName(bin, size, secStrShdr, symShdr.sh_name);
            f.LineAddrs = std::map<uint64_t, LineAddrInfo>();
            elfFuncInfos.push_back(f);
        }
    }

    Logger::TLog("GetElfFuncInfos Out...");
    return true;
}

std::string Elf64::GetStrFromStrTbl(const uint8_t *strTab, const uint64_t strTabSize, const uint64_t offset)
{
    std::string str = "";
    uint64_t pos = offset;
    Logger::DLog("GetStrFromStrTbl In offset=[%ld], strTabSize:[%ld]", offset, strTabSize);
    while(pos < strTabSize)
    {
        if (pos == 0)
        {
            break;
        }

        str += strTab[pos];
        pos++;
    }
    return str;
}
bool Elf64::isSpecialShndx(uint16_t shndx)
{
    uint16_t specialShNdx[9] = 
    {
        SHN_UNDEF,
        SHN_LORESERVE,
        SHN_LOPROC,
        SHN_BEFORE,
        SHN_AFTER,
        SHN_HIPROC,
        SHN_ABS,
        SHN_COMMON,
        SHN_HIRESERVE
    };
    for(uint32_t i = 0; i < 9; i++)
    {
        if (specialShNdx[i] == shndx)
        {
            return true;
        }
    }
    return false;
}

std::string Elf64::GetClassStr(const Elf64_Ehdr &ehdr)
{
    std::string s = "";
    if (ehdr.e_ident[4] == ELFCLASS32)
    {
        s = "ELF32";
    }
    else if (ehdr.e_ident[4] == ELFCLASS64)
    {
        s = "ELF64";
    }

    return s;
}

std::string Elf64::GetDataStr(const Elf64_Ehdr &ehdr)
{
    std::string s = "";
    if (ehdr.e_ident[5] == ELFDATA2LSB)
    {
        s = "2's complement, little endian";
    }

    return s;
}

std::string Elf64::GetElfVersionStr(const Elf64_Ehdr &ehdr)
{
    std::string s = "";
    if (ehdr.e_ident[6] == EV_CURRENT)
    {
        s = "1 (current)";
    }

    return s;
}

std::string Elf64::GetOsAbiStr(const Elf64_Ehdr &ehdr)
{
    std::string s = "";
    switch(ehdr.e_ident[7])
    {
    case ELFOSABI_NONE:
        s = "UNIX - System V";
        break;
    }

    return s;
}

std::string Elf64::GetTypeStr(const uint16_t e_type)
{
    std::string s = "";
    switch(e_type)
    {
    case ET_DYN:
        s = "DYN Position-Independent Executable file";
        break;
    }

    return s;
}

std::string Elf64::GetMachineStr(const uint16_t e_machine)
{
    std::string s = "";
    switch(e_machine)
    {
        case EM_X86_64:
        s = "Advanced Micro Devices X86-64";
        break;
    }

    return s;
}

void Elf64::ShowElf64Ehdr(const Elf64_Ehdr &ehdr)
{
    std::cout << "ELF Header: " << std::endl;
    std::cout << "Magic:    ";
    for (int i = 0; i < EI_NIDENT; i++)
    {
        std::cout << StringHelper::strprintf("%02x ", ehdr.e_ident[i]);
    }
    std::cout << std::endl;
    std::cout << "Class:    " << GetClassStr(ehdr) << std::endl;
    std::cout << "Data:    " << GetDataStr(ehdr) << std::endl;
    std::cout << "Version:    " << GetElfVersionStr(ehdr) << std::endl;
    std::cout << "OS/ABI:    " << GetOsAbiStr(ehdr) << std::endl;
    std::cout << "Type:    "    << GetTypeStr(ehdr.e_type) << std::endl;
    std::cout << "Machine:    " << GetMachineStr(ehdr.e_machine) << std::endl;
    #if 0
    std::cout << "Version:    " <<  std::endl;
    std::cout << "Entry point address::    " << std::endl;
    std::cout << "Start of program headers:    " << std::endl;
    std::cout << "Start of section headers:    " << std::endl;
    std::cout << "Flags:    " << std::endl;
    std::cout << "Size of this header:    " << std::endl;
    std::cout << "Size of program headers:    " << std::endl;
    std::cout << "Number of section headers:    " << std::endl;
    std::cout << "Section header string table index:    " << std::endl;
    #endif
    printf("0x%02x\n", ehdr.e_version);
    printf("0x%02lx\n", ehdr.e_entry);
    printf("%ld\n", ehdr.e_phoff);
    printf("%ld\n", ehdr.e_shoff);
    printf("%02x\n", ehdr.e_flags);
    printf("%d\n", ehdr.e_ehsize);
    printf("%d\n", ehdr.e_phentsize);
    printf("%d\n", ehdr.e_phnum);
    printf("%d\n", ehdr.e_shentsize);
    printf("%d\n", ehdr.e_shnum);
    printf("%d\n", ehdr.e_shstrndx);
}

