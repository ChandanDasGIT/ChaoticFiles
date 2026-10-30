#include "contentinfo/contentinfo.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <elf.h>
#include <fstream>

namespace {

constexpr std::size_t kMaxFile = 1024ull * 1024 * 1024;

const char* machineName(int machine)
{
    switch (machine) {
    case EM_X86_64: return "x86-64";
    case EM_386: return "x86 (32-bit)";
    case EM_AARCH64: return "ARM64";
    case EM_ARM: return "ARM";
    case EM_RISCV: return "RISC-V";
    case EM_PPC64: return "PowerPC 64";
    case EM_S390: return "IBM S/390";
    case EM_MIPS: return "MIPS";
    default: return "Other";
    }
}

template <typename Ehdr, typename Phdr, typename Shdr, typename Dyn>
void readElf(const std::string& d, std::vector<PropertyRow>& rows)
{
    if (d.size() < sizeof(Ehdr)) return;
    Ehdr header;
    std::memcpy(&header, d.data(), sizeof header);

    std::vector<Phdr> segments;
    for (int i = 0; i < header.e_phnum; ++i) {
        const std::size_t at = header.e_phoff + static_cast<std::size_t>(i) * header.e_phentsize;
        if (at + sizeof(Phdr) > d.size()) break;
        Phdr segment;
        std::memcpy(&segment, d.data() + at, sizeof segment);
        segments.push_back(segment);
    }

    const auto fileOffset = [&segments](std::uint64_t address) -> std::uint64_t {
        for (const Phdr& s : segments)
            if (s.p_type == PT_LOAD && address >= s.p_vaddr && address < s.p_vaddr + s.p_filesz)
                return address - s.p_vaddr + s.p_offset;
        return 0;
    };

    bool isDynamic = false, pie = false;
    std::string interpreter, soname, runpath;
    std::vector<std::string> needed;

    for (const Phdr& s : segments) {
        if (s.p_type == PT_INTERP && s.p_offset + s.p_filesz <= d.size()) {
            interpreter = d.c_str() + s.p_offset;
        } else if (s.p_type == PT_DYNAMIC && s.p_offset + s.p_filesz <= d.size()) {
            isDynamic = true;
            std::vector<Dyn> entries(s.p_filesz / sizeof(Dyn));
            std::memcpy(entries.data(), d.data() + s.p_offset, entries.size() * sizeof(Dyn));

            std::uint64_t strings = 0;
            for (const Dyn& e : entries)
                if (e.d_tag == DT_STRTAB) strings = fileOffset(e.d_un.d_ptr);
            const auto textAt = [&](std::uint64_t index) -> std::string {
                return strings && strings + index < d.size() ? d.c_str() + strings + index : "";
            };
            for (const Dyn& e : entries) {
                if (e.d_tag == DT_NEEDED) needed.push_back(textAt(e.d_un.d_val));
                else if (e.d_tag == DT_SONAME) soname = textAt(e.d_un.d_val);
                else if (e.d_tag == DT_RUNPATH || e.d_tag == DT_RPATH) runpath = textAt(e.d_un.d_val);
                else if (e.d_tag == DT_FLAGS_1 && (e.d_un.d_val & DF_1_PIE)) pie = true;
            }
        }
    }

    bool hasSymbols = false, hasDebug = false;
    if (header.e_shoff && header.e_shstrndx < header.e_shnum) {
        const auto sectionAt = [&](int i, Shdr& s) {
            const std::size_t at = header.e_shoff + static_cast<std::size_t>(i) * header.e_shentsize;
            if (at + sizeof(Shdr) > d.size()) return false;
            std::memcpy(&s, d.data() + at, sizeof s);
            return true;
        };
        Shdr names;
        if (sectionAt(header.e_shstrndx, names)) {
            for (int i = 0; i < header.e_shnum; ++i) {
                Shdr s;
                if (!sectionAt(i, s)) break;
                if (s.sh_type == SHT_SYMTAB) hasSymbols = true;
                const std::size_t nameAt = names.sh_offset + s.sh_name;
                if (nameAt < d.size() && !std::strcmp(d.c_str() + nameAt, ".debug_info")) hasDebug = true;
            }
        }
    }

    const char* kind = "Other";
    if (header.e_type == ET_EXEC) kind = "Program";
    else if (header.e_type == ET_REL) kind = "Object file";
    else if (header.e_type == ET_CORE) kind = "Core dump";
    else if (header.e_type == ET_DYN) kind = pie || !interpreter.empty() ? "Program (position independent)" : "Shared library";

    char entry[32];
    std::snprintf(entry, sizeof entry, "0x%llx", static_cast<unsigned long long>(header.e_entry));

    rows.push_back({"Binary kind", kind});
    rows.push_back({"Architecture", machineName(header.e_machine)});
    rows.push_back({"Linking", isDynamic ? "Dynamic" : "Static"});
    rows.push_back({"Entry point", entry});
    if (!interpreter.empty()) rows.push_back({"Interpreter", interpreter});
    if (!soname.empty()) rows.push_back({"Library name", soname});
    if (!runpath.empty()) rows.push_back({"Library search path", runpath});
    rows.push_back({"Stripped", hasSymbols ? "No" : "Yes"});
    rows.push_back({"Debug information", hasDebug ? "Yes" : "No"});
    std::string libraries;
    for (const std::string& name : needed) libraries += (libraries.empty() ? "" : "\n") + name;
    if (!libraries.empty()) rows.push_back({"Linked libraries (" + std::to_string(needed.size()) + ")", libraries});
}

} // namespace

namespace ContentInfo {

std::vector<PropertyRow> elf(const std::string& path)
{
    std::vector<PropertyRow> rows;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return rows;
    const auto size = static_cast<std::size_t>(file.tellg());
    if (size > kMaxFile || size < EI_NIDENT) return rows;

    std::string data(size, '\0');
    file.seekg(0);
    file.read(data.data(), static_cast<std::streamsize>(size));
    if (data.compare(0, 4, ELFMAG) != 0) return rows;

    if (data[EI_CLASS] == ELFCLASS64) readElf<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Dyn>(data, rows);
    else if (data[EI_CLASS] == ELFCLASS32) readElf<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Dyn>(data, rows);
    return rows;
}

} // namespace ContentInfo
