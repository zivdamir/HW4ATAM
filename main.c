#include <stdio.h>
#include <stdlib.h>
#include "elf64.h"
#include <string.h>

#define MAX_NAME 256


void part1_function(Elf64_Ehdr *Ehdr, FILE* fd, char* filename);

void part2_4_function(Elf64_Ehdr *Ehdr, FILE* fd, char* function_name, char** argv);//part 2+3+4

void part5_function(FILE* fd, Elf64_Ehdr *Ehdr, char** argv, unsigned long strtab_offset);

void part6_function(FILE* fd, Elf64_Ehdr *Ehdr,unsigned long address, char** argv, int dyn_flag);

int main(int argc, char **argv)
{

    int size = 0; //open file
    char* val;
    FILE* fd = fopen(argv[2], "r");
    if(fd == NULL) exit(1);
    Elf64_Ehdr *Ehdr = (Elf64_Ehdr*)malloc(sizeof(*Ehdr));
    if(Ehdr==NULL)
    {
        fclose(fd);
        exit(1);
    }
    if(fread(Ehdr,  sizeof(*Ehdr),1, fd) != 1)
    {
        free(Ehdr);
        fclose(fd);
        exit(1);
    }
    part1_function(Ehdr, fd,argv[2]);
    part2_4_function(Ehdr, fd,argv[1], argv);



    free(Ehdr);
    fclose(fd);
    return 0;
}


void part1_function(Elf64_Ehdr *Ehdr, FILE* fd, char* filename)
{
    if(Ehdr->e_type != 2) //TODO, maybe char or int casting
    {
        printf("PRF:: %s not an executable! :(\n", filename);
        free(Ehdr);
        fclose(fd);
        exit(0); //TODO
    }
}


void part2_4_function(Elf64_Ehdr *Ehdr, FILE* fd, char* function_name, char** argv)
{
    // todo dont forget to free!!
    int error_flag = 0;
    Elf64_Shdr *Shdr = (Elf64_Shdr *) malloc(sizeof(*Shdr));
    if (Shdr == NULL)
    {
        free(Ehdr);
        fclose(fd);
        exit(1);
    }
    //section header entry
    unsigned long strtab_section_index = Ehdr->e_shstrndx;
    long strtab_offset = Ehdr->e_shoff + (strtab_section_index * Ehdr->e_shentsize);
    if (fseek(fd, strtab_offset, SEEK_SET) != 0)
    {
        error_flag++;
    }
    Elf64_Shdr *Shdr_sym = (Elf64_Shdr *) malloc(sizeof(*Shdr));
    if (Shdr_sym== NULL)
    {
        error_flag++;
    }
    if(error_flag > 0)
    {
        free(Shdr);
        fclose(fd);
        free(Ehdr);
        exit(1);

    }
    //problem here
    if(fread(Shdr_sym, Ehdr->e_shentsize, 1, fd) != 1)
    {
        free(Shdr);
        fclose(fd);
        free(Ehdr);
        exit(1);
    }
    unsigned long sym_offset = Shdr_sym->sh_offset;
    free(Shdr_sym);
    //Sym_tab
//    Elf64_Sym* sym_tab=(Elf64_Sym*)malloc(sizeof(*sym_tab));
//    if(fseek(fd, strtab_offset, SEEK_CUR)!=0) exit(1) ;
//    if(fread(fd,sizeof(Elf64_Shdr),1,sym_tab)!=1) exit(1);
//    //str_tab
    int i = 0;
    char* section_name = (char*)malloc(MAX_NAME);
    if(section_name == NULL)
    {
        error_flag++;
    }
    Elf64_Shdr *Shdr_tab = (Elf64_Shdr *) malloc(sizeof(*Shdr));//this is str tab
    if (Shdr_tab == NULL)
    {
        error_flag++;
        free(section_name);
    }
    Elf64_Shdr *Shdr_symtab = (Elf64_Shdr *) malloc(sizeof(*Shdr));//this is str tab
    if (Shdr_symtab== NULL)
    {
        error_flag++;
        free(section_name);
        free(Shdr_tab);
    }
    if(error_flag > 0)
    {
        fclose(fd);
        free(Shdr);
        free(Ehdr);
        exit(1);
    }
    int flag = 0;
    while (1)
    {
        if (fseek(fd, Ehdr->e_shoff + i * Ehdr->e_shentsize, SEEK_SET) != 0)
        {
            fclose(fd);
            free(Shdr);
            free(Ehdr);
            free(section_name);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }
        if (fread(Shdr, sizeof(*Shdr), 1, fd) != 1)
        {
            fclose(fd);
            free(Shdr);
            free(section_name);
            free(Ehdr);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }
        if (fseek(fd, sym_offset + Shdr->sh_name+1, SEEK_SET) != 0) //move to strtab + offset
        {
            fclose(fd);
            free(section_name);
            free(Shdr);
            free(Ehdr);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }

        for(int j=0; ; j++)
        {
            if (fread(&section_name[j], 1, 1, fd) != 1)
            {
                fclose(fd);
                free(Shdr);
                free(section_name);
                free(Ehdr);
                free(Shdr_tab);
                free(Shdr_symtab);
                exit(1);
            }
            if(*(section_name+j) == '\0') break;
        }
        if (strcmp("symtab", section_name) == 0)
        {
            flag ++;
            *Shdr_symtab = *Shdr;
            if(flag == 2) break;
        }
        if (strcmp("strtab", section_name) == 0)
        {
            flag ++;
            *Shdr_tab = *Shdr;
            if(flag == 2) break;
        }
        i++;
    }
    free(section_name);

    unsigned long symtab_offset = Shdr_symtab->sh_offset;
    Elf64_Sym *Sym = (Elf64_Sym *) malloc(sizeof(*Sym));
    if (Sym == NULL)
    {
        fclose(fd);
        free(Shdr);
        free(Ehdr);
        free(Shdr_tab);
        free(Shdr_symtab);
        exit(1);
    }


    unsigned long num_syms = Shdr_symtab->sh_size / Shdr_symtab->sh_entsize;
    char* sym_name = (char*)malloc(MAX_NAME);
    if(sym_name==NULL)
    {
        fclose(fd);
        free(Shdr);
        free(Ehdr);
        free(Shdr_tab);
        free(Shdr_symtab);
        exit(1);
    }
    for (i = 0; i < num_syms; i++)
    {
        if (fseek(fd, symtab_offset + i * sizeof(*Sym), SEEK_SET) != 0)
        {
            free(sym_name);
            fclose(fd);
            free(Shdr);
            free(Ehdr);
            free(Sym);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }
        if (fread(Sym, sizeof(*Sym), 1, fd) != 1)
        {
            free(sym_name);
            fclose(fd);
            free(Shdr);
            free(Ehdr);
            free(Sym);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }
        if (fseek(fd, Shdr_tab->sh_offset + Sym->st_name, SEEK_SET) != 0) //move to strtab + offset
        {
            free(sym_name);
            fclose(fd);
            free(Shdr);
            free(Ehdr);
            free(Sym);
            free(Shdr_tab);
            free(Shdr_symtab);
            exit(1);
        }
        for(int j=0; ; j++)
        {
            if (fread(&sym_name[j], 1, 1, fd) != 1)
            {
                fclose(fd);
                free(Shdr);
                free(sym_name);
                free(Ehdr);
                free(Shdr_tab);
                free(Shdr_symtab);
                exit(1);
            }
            if(*(sym_name+j) == '\0') break;
        }
        if (strcmp(function_name, sym_name) == 0)
            break; //found the function!
    }
    if(i==num_syms)
    {
        printf("PRF:: %s not found!\n" , function_name);
        fclose(fd);
        free(Shdr);
        free(sym_name);
        free(Ehdr);
        free(Sym);
        free(Shdr_tab);
        free(Shdr_symtab);
        exit(0);
    }
    if(ELF64_ST_BIND(Sym->st_info) != 1)
    {
        printf("PRF:: %s is not a global symbol! :(\n", function_name);
        fclose(fd);
        free(sym_name);
        free(Shdr);
        free(Ehdr);
        free(Sym);
        free(Shdr_tab);
        free(Shdr_symtab);
        exit(0);
    }
    unsigned long address;
    free(Shdr);
    free(Shdr_tab);
    free(Shdr_symtab);
    free(sym_name);
    if(Sym->st_shndx==SHN_UNDEF)
    {
        free(Sym);
        part5_function(fd, Ehdr, argv, sym_offset);
    }
    else
    {
        address = Sym->st_value;
        free(Sym);
        part6_function(fd, Ehdr, address, argv, 0);
    }
}

void part5_function(FILE* fd, Elf64_Ehdr *Ehdr, char** argv, unsigned long strtab_offset)
{
 //Here the function is global and undef in the symtab. now we need to find the address in the plt which refferences to foo
//find rela.plt
//find entry in the rela.plt with sym. name = function
//find offset of this entry. the offset is the wanted address
    Elf64_Shdr *Shdr_rela_plt = (Elf64_Shdr *) malloc(sizeof(*Shdr_rela_plt));//this is str tab
    if (Shdr_rela_plt== NULL)
    {
        fclose(fd);
        free(Ehdr);
        exit(1);
    }

    Elf64_Shdr *Shdr_dynsym = (Elf64_Shdr *) malloc(sizeof(*Shdr_dynsym));//this is str tab
    if (Shdr_dynsym== NULL)
    {
        fclose(fd);
        free(Shdr_rela_plt);
        free(Ehdr);
        exit(1);
    }
    //THIS IS TEMPORARY
    Elf64_Shdr *Temp_shdr_entry = (Elf64_Shdr *) malloc(sizeof(*Temp_shdr_entry));//this is str tab
    if (Temp_shdr_entry== NULL)
    {
        free(Shdr_rela_plt);
        free(Shdr_dynsym);
        fclose(fd);
        free(Ehdr);
        exit(1);
    }
    char* section_name = (char*)malloc(MAX_NAME);
    if(section_name == NULL)
    {
        free(Temp_shdr_entry);
        fclose(fd);
        free(Shdr_dynsym);
        free(Shdr_rela_plt);
        free(Ehdr);
        exit(1);
    }
    int i = 0;
    int flag = 0;
    while (1)
    {
        if (fseek(fd, Ehdr->e_shoff + i * Ehdr->e_shentsize, SEEK_SET) != 0)
        {
            free(Temp_shdr_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(Ehdr);
            free(section_name);
            exit(1);
        }
        if (fread(Temp_shdr_entry, sizeof(*Temp_shdr_entry), 1, fd) != 1)
        {
            free(Temp_shdr_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(section_name);
            free(Ehdr);
            exit(1);
        }
        if (fseek(fd, strtab_offset + Temp_shdr_entry->sh_name + 1, SEEK_SET) != 0) //move to strtab + offset
        {
            free(Temp_shdr_entry);
            fclose(fd);
            free(section_name);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(Ehdr);
            exit(1);
        }

        for (int j = 0;; j++)
        {
            if (fread(&section_name[j], 1, 1, fd) != 1)
            {
                free(Temp_shdr_entry);
                fclose(fd);
                free(Shdr_rela_plt);
                free(section_name);
                free(Shdr_dynsym);
                free(Ehdr);
                exit(1);
            }
            if (*(section_name + j) == '\0') break;
        }
        if (strcmp("rela.plt", section_name) == 0)
        {
            flag++;
            *Shdr_rela_plt = *Temp_shdr_entry;
            if(flag == 2) break; //if break, rela_plt and dynsym is loaded
        }
        if (strcmp("dynsym", section_name) == 0)
        {
            flag++;
            *Shdr_dynsym = *Temp_shdr_entry;
            if(flag == 2) break; //if break, rela_plt and dynsym is loaded
        }
        i++;
    }
    free(Temp_shdr_entry);
    Elf64_Rela* func_rela_entry = (Elf64_Rela*)malloc(sizeof(*func_rela_entry));
    if(func_rela_entry == NULL)
    {
     fclose(fd);
     free(Shdr_rela_plt);
     free(Shdr_dynsym);
     free(Ehdr);
     free(section_name);
     exit(1);
    }
    Elf64_Sym * Dymsym_entry = (Elf64_Sym*)malloc(sizeof(*Dymsym_entry));
    if(Dymsym_entry == NULL)
    {
        free(func_rela_entry);
        fclose(fd);
        free(Shdr_rela_plt);
        free(Shdr_dynsym);
        free(Ehdr);
        free(section_name);
        exit(1);
    }
    Elf64_Shdr * Shdr_strtab = (Elf64_Shdr*)malloc(sizeof(*Shdr_strtab)); //todo*
    if(Shdr_strtab == NULL)
    {
        free(func_rela_entry);
        fclose(fd);
        free(Shdr_rela_plt);
        free(Shdr_dynsym);
        free(Ehdr);
        free(section_name);
        exit(1);
    }
    i = 0;
    while (1)
    {
        if (fseek(fd, Shdr_rela_plt->sh_offset + i * sizeof(*func_rela_entry), SEEK_SET) != 0)
        {
            free(Dymsym_entry);
            free(func_rela_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(Ehdr);
            free(section_name);
            exit(1);
        }
        if (fread(func_rela_entry, sizeof(*func_rela_entry), 1, fd) != 1)
        {
            free(Dymsym_entry);
            free(func_rela_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(section_name);
            free(Ehdr);
            exit(1);
        }
        if (fseek(fd, Shdr_dynsym->sh_offset + Shdr_dynsym->sh_entsize*ELF64_R_SYM(func_rela_entry->r_info), SEEK_SET) != 0)
        {
            free(func_rela_entry);
            free(Dymsym_entry);
            fclose(fd);
            free(section_name);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(Ehdr);
            exit(1);
        }
        if(fread(Dymsym_entry, sizeof(*Dymsym_entry), 1, fd) != 1)
        {
            free(Dymsym_entry);
            free(func_rela_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(section_name);
            free(Ehdr);
            exit(1);
        }
        if (fseek(fd, Ehdr->e_shoff+(Ehdr->e_shentsize)*(Shdr_dynsym->sh_link), SEEK_SET) != 0) //move to strtab
        {
            free(func_rela_entry);
            free(Dymsym_entry);
            fclose(fd);
            free(section_name);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(Ehdr);
            exit(1);
        }
        if(fread(Shdr_strtab, sizeof(*Shdr_strtab), 1, fd) != 1)
        {
            free(Dymsym_entry);
            free(func_rela_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(section_name);
            free(Ehdr);
            exit(1);
        }// not good
        if (fseek(fd, Shdr_strtab->sh_offset + Dymsym_entry->st_name, SEEK_SET) != 0)
        {
            free(Dymsym_entry);
            free(func_rela_entry);
            fclose(fd);
            free(Shdr_rela_plt);
            free(Shdr_dynsym);
            free(section_name);
            free(Ehdr);
            exit(1);
        }//move to strdym
        for (int j = 0;; j++)
        {
            if (fread(section_name+j, 1, 1, fd) != 1)
            {
                free(Dymsym_entry);
                free(func_rela_entry);
                fclose(fd);
                free(Shdr_rela_plt);
                free(section_name);
                free(Shdr_dynsym);
                free(Ehdr);
                exit(1);
            }
            //printf("next char: %c\n", *(section_name+j));
            if (*(section_name + j) == '\0')
            {
                break;
            }
        }
        if (strcmp(argv[1], section_name) == 0)
        {
            break; //if break, rela_plt and dynsym is loaded
        }
        i++;
    }
    //find strtab header and find offset according to:
    //rela entry
    //dynsym enrty in according to rela entry offset - using macro end dynsym_entry size
    //strtab entry according to offset from file and dynsym_entry.name
    unsigned long address = func_rela_entry->r_offset;
    free(Dymsym_entry);
    free(Shdr_dynsym);
    free(section_name);
    free(Shdr_rela_plt);
    free(func_rela_entry);
    part6_function(fd,Ehdr,address,argv, 1);

}

void part6_function(FILE* fd, Elf64_Ehdr *Ehdr, unsigned long address, char** argv, int dyn_flag)
{
    deb(argv+2, address, dyn_flag);
}