#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct _size {
    uint32_t offset;
    uint32_t length;
    char name[64];
} size;

int main(int argc, char ** argv) {
    size sizes[100];
    int sizeCount = 0;
    if(argc < 2) {
        printf("Usage: %s <path to mach-o file>\n", argv[0]);
        return 1;
    }
    FILE * file = fopen(argv[1], "rb");
    if(file == NULL) {
        printf("Failed to open file: %s\n", argv[1]);
        return 1;
    }
    struct mach_header_64 header;
    sizes[sizeCount++] = (size){0, sizeof(header), "header"};
    fread(&header, sizeof(header), 1, file);
    struct load_command command;
    sizes[sizeCount++] = (size){sizeof(header), header.sizeofcmds, "load commands"};
    uint32_t offset = sizeof(header) + header.sizeofcmds;
    for(uint32_t i = 0; i < header.ncmds; i++) {
        fread(&command, sizeof(command), 1, file);
        fseek(file, -sizeof(command), SEEK_CUR);
        switch(command.cmd) {
            case LC_SEGMENT_64: {
                struct segment_command_64 segment;
                fread(&segment, sizeof(segment), 1, file);
                fseek(file, -sizeof(segment), SEEK_CUR);
                printf("Segment: %s, %llu, %llu\n", segment.segname, segment.fileoff, segment.filesize);
                sizes[sizeCount++] = (size){offset + segment.fileoff, segment.filesize, ""};
                strncpy(sizes[sizeCount - 1].name, segment.segname, 64);
                break;
            }
            case LC_SYMTAB: {
                struct symtab_command symtab;
                fread(&symtab, sizeof(symtab), 1, file);
                fseek(file, -sizeof(symtab), SEEK_CUR);
                printf("Symtab: %u, %u, %u, %u\n", symtab.symoff, symtab.nsyms, symtab.stroff, symtab.strsize);
                sizes[sizeCount++] = (size){offset + symtab.symoff, symtab.nsyms * sizeof(struct nlist_64), "symtab"};
                sizes[sizeCount++] = (size){offset + symtab.stroff, symtab.strsize, "strtab"};
                break;
            }
            case LC_DYSYMTAB: {
                struct dysymtab_command dysymtab;
                fread(&dysymtab, sizeof(dysymtab), 1, file);
                fseek(file, -sizeof(dysymtab), SEEK_CUR);
                printf("Dysymtab: %u, %u, %u, %u, %u, %u, %u, %u\n", dysymtab.ilocalsym, dysymtab.nlocalsym, dysymtab.iextdefsym, dysymtab.nextdefsym, dysymtab.iundefsym, dysymtab.nundefsym, dysymtab.tocoff, dysymtab.ntoc);
                sizes[sizeCount++] = (size){offset + dysymtab.indirectsymoff, dysymtab.nindirectsyms * sizeof(uint32_t), "indirectsym"};
                break;
            }
            case LC_DYLD_INFO_ONLY: {
                struct dyld_info_command dyld;
                fread(&dyld, sizeof(dyld), 1, file);
                fseek(file, -sizeof(dyld), SEEK_CUR);
                printf("Dyld: %u, %u, %u, %u, %u, %u\n", dyld.rebase_off, dyld.rebase_size, dyld.bind_off, dyld.bind_size, dyld.weak_bind_off, dyld.weak_bind_size);
                sizes[sizeCount++] = (size){offset + dyld.rebase_off, dyld.rebase_size, "rebase"};
                sizes[sizeCount++] = (size){offset + dyld.bind_off, dyld.bind_size, "bind"};
                sizes[sizeCount++] = (size){offset + dyld.weak_bind_off, dyld.weak_bind_size, "weak_bind"};
                break;
            }
            case LC_CODE_SIGNATURE: {
                struct linkedit_data_command linkedit;
                fread(&linkedit, sizeof(linkedit), 1, file);
                fseek(file, -sizeof(linkedit), SEEK_CUR);
                printf("Linkedit: %u, %u\n", linkedit.dataoff, linkedit.datasize);
                sizes[sizeCount++] = (size){offset + linkedit.dataoff, linkedit.datasize, "codesig"};
                break;
            }
            default:
                printf("Command: %02x\n", command.cmd & 0xffff);
        }
        fseek(file, command.cmdsize, SEEK_CUR);
    }
    for(int i = 0; i < sizeCount; i++) {
        if(sizes[i].length > 0)
            printf("%20s: %08x, %08x, %08x\n", sizes[i].name, sizes[i].offset, sizes[i].length, sizes[i].offset + sizes[i].length);
    }
    return 0;
}