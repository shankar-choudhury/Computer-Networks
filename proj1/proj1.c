/*
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: proj1.c
 * Date created: 2025-09-15
 * Brief description: This code reads a .list binary file of IPv4 addresses and supports:
 *                    - printing addresses in dotted-quad notation (-p)
 *                    - showing a summary of total IPs and private IPs (-s)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

typedef struct
{
    int print_mode;
    int summary_mode;
    char *filename;
} CliArgs;

void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [-p|-s] -r <file>\n", progname);
    fprintf(stderr, "  -p    print IPv4 addresses in dotted-quad format\n");
    fprintf(stderr, "  -s    show summary: total IPs and private IPs\n");
    fprintf(stderr, "  -r    specify input binary file (.list)\n");
    exit(1);
}

void parseargs(int argc, char *argv[], CliArgs *args)
{
    int opt;
    args->print_mode = 0;
    args->summary_mode = 0;
    args->filename = NULL;

    while ((opt = getopt(argc, argv, "psr:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            args->print_mode = 1;
            break;
        case 's':
            args->summary_mode = 1;
            break;
        case 'r':
            args->filename = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    if ((!args->print_mode && !args->summary_mode) ||
        (args->print_mode && args->summary_mode))
    {
        fprintf(stderr, "Error: Specify exactly one mode (-p or -s)\n");
        usage(argv[0]);
    }
    if (!args->filename)
    {
        fprintf(stderr, "Error: No file specified\n");
        usage(argv[0]);
    }
}

FILE *open_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    if (filesize == 0)
    {
        fprintf(stderr, "Error: File is empty\n");
        fclose(fp);
        exit(1);
    }

    if (filesize % 4 != 0)
    {
        fprintf(stderr, "Error: File size not multiple of 4 bytes (invalid .list file)\n");
        fclose(fp);
        exit(1);
    }

    return fp;
}

int is_private(uint8_t a, uint8_t b)
{
    if (a == 10)
        return 1; // 10.0.0.0/8
    return 0;
}

int main(int argc, char *argv[])
{
    CliArgs args;
    parseargs(argc, argv, &args);

    FILE *fp = open_file(args.filename);

    uint32_t addr_raw;
    size_t total_ips = 0;
    size_t private_ips = 0;

    while (fread(&addr_raw, sizeof(addr_raw), 1, fp) == 1)
    {
        total_ips++;

        // Convert from network byte order
        uint32_t addr = ntohl(addr_raw);

        // Extract bytes
        uint8_t a = (addr >> 24) & 0xFF;
        uint8_t b = (addr >> 16) & 0xFF;
        uint8_t c = (addr >> 8) & 0xFF;
        uint8_t d = addr & 0xFF;

        if (is_private(a, b))
            private_ips++;

        if (args.print_mode)
            printf("%u.%u.%u.%u\n", a, b, c, d);
    }

    if (args.summary_mode)
    {
        printf("total IPs: %zu\n", total_ips);
        printf("private IPs: %zu\n", private_ips);
    }

    fclose(fp);
    return 0;
}
