#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BUFFERSIZE 512

void print_help()
{
    printf("usage: copy [OPTION] <source file> <destination file>\n"
           "Copies contents from source file to destination file. If file doesn't exist, it creates it.\n"
           "If no options are provided, copying is done using read() and write().\n\n"
           "Options:\n"
           "   -m      copies contents using mmap and memcpy\n"
           "   -h      shows this help\n");
}

void copy_read_write(int fd_from, int fd_to)
{
    char buf[BUFFERSIZE];
    size_t nbytes;
    nbytes = sizeof(buf);
    while ((nbytes = read(fd_from, buf, nbytes)) > 0)
    {
        if (write(fd_to, buf, nbytes) == -1)
        {
            perror("write");
            close(fd_from);
            close(fd_to);
            exit(EXIT_FAILURE);
        }
    }
}

void copy_mmap(int fd_from, int fd_to)
{
    void *input, *output;
    struct stat sb;
    if (fstat(fd_from, &sb) == -1)
    {
        perror("fstat");
        close(fd_from);
        close(fd_to);
        exit(EXIT_FAILURE);
    }
    int filesize = sb.st_size;

    if ((input = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd_from, 0)) == (void *)-1)
    {
        perror("Input mapping");
        close(fd_from);
        close(fd_to);
        exit(EXIT_FAILURE);
    }
    ftruncate(fd_to, filesize);
    if ((output = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_to, 0)) == (void *)-1)
    {
        perror("Output mapping");
        close(fd_from);
        close(fd_to);
        exit(EXIT_FAILURE);
    }
    memcpy(output, input, filesize);
    munmap(input, filesize);
    munmap(output, filesize);
}

int main(int argc, char *argv[])
{
    bool m_flag = false;
    // check options
    if (argc == 1)
    {
        print_help();
        return 1;
    }
    int option = getopt(argc, argv, "mh");
    switch (option)
    {
    case 'h':
        print_help();
        return 1;
    case 'm':
        m_flag = true;
        break;
    default:
        break;
    }
    if ((argc - optind) != 2)
    {
        fprintf(stderr, "Wrong number of arguments");
        exit(EXIT_FAILURE);
    }

    // open files
    int fd_in = open(argv[optind++], O_RDONLY);
    if (fd_in == -1)
    {
        perror("Input file");
        // printf("%s", argv[optind-1]);
        exit(EXIT_FAILURE);
    }
    int fd_out = open(argv[optind], O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_out == -1)
    {
        perror("Output file");
        close(fd_in);
        exit(EXIT_FAILURE);
    }

    // copy using read/write
    if (m_flag == false)
    {
        copy_read_write(fd_in, fd_out);
    }
    // copy using memcpy
    else
    {
        copy_mmap(fd_in, fd_out);
    }
    close(fd_in);
    close(fd_out);

    puts("File contents copied successfully.");
}
