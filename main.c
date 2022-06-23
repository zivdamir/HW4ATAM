#include <stdio.h>
//#include <sys/mman.h>
#include <stdlib.h>
//#include <fcntl.h>
//#include <unistd.h>
#include "elf64.h"
#include <string.h>
#include <errno.h>

#define MAX_NAME 256


void part1_function(Elf64_Ehdr *Ehdr, FILE* fd, char* filename);

void part2_4_function(Elf64_Ehdr *Ehdr, FILE* fd, char* function_name);//part 2+3+4

void part5_function(FILE* fd, Elf64_Ehdr *Ehdr);

void part6_function(FILE* fd, Elf64_Ehdr *Ehdr,unsigned long address);

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
//    part1_function(Ehdr, fd,argv[2]);
    part2_4_function(Ehdr, fd,argv[1]);



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


void part2_4_function(Elf64_Ehdr *Ehdr, FILE* fd, char* function_name)
{
    // todo dont forget to free!!
    if (fseek(fd, 0, SEEK_SET) != 0)
    {
        fclose(fd);
        free(Ehdr);
        exit(1);
    }
    Elf64_Shdr *Shdr = (Elf64_Shdr *) malloc(sizeof(*Shdr));
    if (Shdr == NULL)
    {
        fclose(fd);
        exit(1);
    }
    //section header entry
    unsigned long strtab_section_index = Ehdr->e_shstrndx;
    long strtab_offset = Ehdr->e_shoff + (strtab_section_index * Ehdr->e_shentsize);
    if (fseek(fd, strtab_offset, SEEK_CUR) != 0)
    {
        fclose(fd);
        free(Shdr);
        free(Ehdr);
        exit(1);
    }
    Elf64_Shdr *Shdr_sym = (Elf64_Shdr *) malloc(sizeof(*Shdr));
    if (Shdr_sym== NULL)
    {
        free(Shdr);
        fclose(fd);
        free(Ehdr);
        exit(1);
    }
    //problem here
    if(fread(Shdr_sym, Ehdr->e_shentsize, 1, fd) != 1)
    {//TODO

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
    if(section_name==NULL)
    {
        fclose(fd);
        free(Shdr);
        free(Ehdr);
        free(section_name);
        exit(1);
    }
    Elf64_Shdr *Shdr_tab = (Elf64_Shdr *) malloc(sizeof(*Shdr));//this is str tab
    if (Shdr_tab== NULL)
    {
        free(Shdr);
        fclose(fd);
        free(Ehdr);
        exit(1);
    }
    Elf64_Shdr *Shdr_symtab = (Elf64_Shdr *) malloc(sizeof(*Shdr));//this is str tab
    if (Shdr_symtab== NULL)
    {
        free(Shdr);
        fclose(fd);
        free(Ehdr);
        free(Shdr_tab);
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
        part5_function(fd, Ehdr);
    }
    else
    {
        unsigned long mid_address = (Sym->st_shndx)*(Ehdr->e_shentsize)+Ehdr->e_shoff;
        Elf64_Shdr *Shdr_txt = (Elf64_Shdr*) malloc(sizeof(*Shdr_txt));
        if (Shdr_txt == NULL)
        {
            fclose(fd);
            free(Ehdr);
            free(Sym);
            exit(1);
        }
        if (fseek(fd, mid_address, SEEK_SET) != 0)
        {
            fclose(fd);
            free(Ehdr);
            free(Sym);
            free(Shdr_txt);
            exit(1);
        }
        if (fread(Shdr_txt, sizeof(*Shdr_txt), 1, fd) != 1)
        {
            fclose(fd);
            free(Ehdr);
            free(Sym);
            free(Shdr_txt);
            exit(1);
        }
        address = Shdr_txt->sh_offset + Sym->st_value;
        free(Sym);
        part6_function(fd, Ehdr, address);
    }
}

void part5_function(FILE* fd, Elf64_Ehdr *Ehdr)
{

}

void part6_function(FILE* fd, Elf64_Ehdr *Ehdr, unsigned long address)
{
    printf("the andress is: %lu", address);

}