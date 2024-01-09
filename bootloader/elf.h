// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12]; //Executable and Linkable Format
  ushort type; //type of the ELF file...ET_EXEC(executable),ET_DYN,ET_REL
  ushort machine;//Identifies the target architecture for which the binary is intended. 
  uint version;//Indicates the version of the ELF format being used.
  uint64 entry; 
  uint64 phoff;//Program Header Table
  uint64 shoff;//Section Header Table 
  uint flags;//Contains machine-specific flags.
  ushort ehsize;
  ushort phentsize; //Specifies the size of each entry in the program header table.
  ushort phnum;//Specifies the size of each entry in the section header table.
  ushort shentsize;//Specifies the size of each entry in the section header table.
  ushort shnum;// Indicates the number of entries in the section header table.
  ushort shstrndx;//The index of the section header table entry that contains the section names.
};

// Program section header
struct proghdr {
  uint32 type; //Describes the type of the segment
  uint32 flags;//Describes the permissions and attributes of the segment. Common flags include
  uint64 off;//Describes the permissions and attributes of the segment. Common flags include
  uint64 vaddr;//Specifies the virtual address where the segment should be loaded into memory.
  uint64 paddr;//Specifies the virtual address where the segment should be loaded into memory.
  uint64 filesz;//Specifies the virtual address where the segment should be loaded into memory.
  uint64 memsz;//Specifies the virtual address where the segment should be loaded into memory.
  uint64 align;//Describes the alignment requirements for the segment's data in memory and in the file

};
// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
