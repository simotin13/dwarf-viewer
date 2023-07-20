#pragma once
#include <string>
#include <vector>
#include <map>
#include <elf.h>

/* Special section indices.  */

#ifndef SHN_UNDEF
#define SHN_UNDEF       0               /* Undefined section */
#endif

#ifndef SHN_LORESERVE
#define SHN_LORESERVE   0xff00          /* Start of reserved indices */
#endif

#ifndef SHN_LOPROC
#define SHN_LOPROC      0xff00          /* Start of processor-specific */
#endif

#ifndef SHN_BEFORE
#define SHN_BEFORE      0xff00          /* Order section before all others (Solaris).  */
#endif

#ifndef SHN_AFTER
#define SHN_AFTER       0xff01          /* Order section after all others (Solaris).  */
#endif

#ifndef SHN_HIPROC
#define SHN_HIPROC      0xff1f          /* End of processor-specific */
#endif

#ifndef SHN_LOOS
#define SHN_LOOS        0xff20          /* Start of OS-specific */
#endif

#ifndef SHN_HIOS
#define SHN_HIOS        0xff3f          /* End of OS-specific */
#endif

#ifndef SHN_ABS
#define SHN_ABS         0xfff1          /* Associated symbol is absolute */
#endif

#ifndef SHN_COMMON
#define SHN_COMMON      0xfff2          /* Associated symbol is common */
#endif

#ifndef SHN_XINDEX
#define SHN_XINDEX      0xffff          /* Index is in extra table.  */
#endif

#ifndef SHN_HIRESERVE
#define SHN_HIRESERVE   0xffff          /* End of reserved indices */
#endif

typedef struct {
    uint64_t Line;
    uint64_t Addr;
    bool IsStmt;
    std::string SrcDirName;
    std::string SrcFileName;
} LineAddrInfo;

typedef struct {
    std::string Name;
    std::string SrcDirName;
    std::string SrcFileName;
    uint64_t Addr;
    uint64_t Size;
    std::string SecName;
    std::map<uint64_t, LineAddrInfo> LineAddrs;
} ElfFunctionInfo;

// ElfFunctionInfo array and Map
typedef struct
{
    std::string Path;                               // elf file path
    std::vector<ElfFunctionInfo> ElfFuncInfos;      // elf function infos
    std::map<uint64_t, uint32_t> AddrFuncIdxMap;    // key: function address, value: Index of ElfFuncInfos
} ElfFunctionTable;

class Elf
{
public:
    static bool IsElf(const uint8_t *bin, const uint64_t size);
    static bool IsElf32(const uint8_t *bin, const uint64_t size);
    static bool IsElf64(const uint8_t *bin, const uint64_t size);
    static bool IsLittleEndian(const uint8_t *bin, const uint64_t size);
    static bool IsCurrentVersion(const uint8_t *bin, const uint64_t size);
};

class Elf64 : Elf
{
public:
    static bool ReadEhdr(const uint8_t *bin, const uint64_t size, Elf64_Ehdr &ehdr);
    static bool ReadShdr(const uint8_t *bin, const uint64_t size, uint64_t offset, Elf64_Shdr &shdr);
    static bool ReadPhdr(const uint8_t *bin, const uint64_t size, uint64_t offset, Elf64_Phdr &phdr);
    static std::string GetSectionName(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &strShdr, uint64_t sh_name);
    static bool GetSymbolTbl(const uint8_t *bin, const uint64_t size, const Elf64_Shdr &symTabShdr, std::vector<Elf64_Sym> &symTbl);
    static bool GetElfFuncInfos(const uint8_t *bin, const uint64_t size, const std::vector<Elf64_Shdr> &shdrs, const std::vector<Elf64_Sym> &symTbl, const Elf64_Shdr &secStrShdr, const Elf64_Shdr &strTabShdr, std::vector<ElfFunctionInfo> &elfFuncInfos);
    static std::string GetStrFromStrTbl(const uint8_t *strTab, const uint64_t strTabSize, const uint64_t offset);
    static std::string GetClassStr(const Elf64_Ehdr &ehdr);
    static void ShowElf64Ehdr(const Elf64_Ehdr &ehdr);
    static std::string GetDataStr(const Elf64_Ehdr &ehdr);
    static std::string GetElfVersionStr(const Elf64_Ehdr &ehdr);
    static std::string GetOsAbiStr(const Elf64_Ehdr &ehdr);
    static std::string GetTypeStr(const uint16_t e_type);
    static std::string GetMachineStr(const uint16_t e_machine);
protected:
    static bool isSpecialShndx(uint16_t shndx);
protected:
    Elf64_Ehdr _elf64Ehdr;
    Elf64_Shdr _elf64Shdr;
    Elf64_Phdr _elf64Phdr;
};
