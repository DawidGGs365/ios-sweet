/*
 * jbstrap: unpacks a rootless Procursus bootstrap (tar, gzip or plain) into /private/var/jb with the
 * system's libarchive. Entries must be under ./var/jb/; ./ and ./var/ themselves are skipped (/var
 * is the system's symlink to /private/var). Owners, modes, times and links as in the archive.
 *
 *   jbstrap ARCHIVE            does nothing when /private/var/jb/.procursus_strapped exists
 *
 * libarchive is in the iOS SDK as libarchive.tbd but without its header: the few calls are
 * declared here (archive.h, archive_entry.h). MIT, as userland/.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct archive;
struct archive_entry;
#define ARCHIVE_OK	0
#define ARCHIVE_EOF	1
#define ARCHIVE_WARN	(-20)
#define ARCHIVE_EXTRACT_OWNER	0x0001
#define ARCHIVE_EXTRACT_PERM	0x0002
#define ARCHIVE_EXTRACT_TIME	0x0004
#define ARCHIVE_EXTRACT_UNLINK	0x0010
extern struct archive *archive_read_new(void);
extern int archive_read_support_filter_all(struct archive *);
extern int archive_read_support_format_all(struct archive *);
extern int archive_read_open_filename(struct archive *, const char *, size_t);
extern int archive_read_next_header(struct archive *, struct archive_entry **);
extern int archive_read_extract(struct archive *, struct archive_entry *, int);
extern int archive_read_free(struct archive *);
extern const char *archive_error_string(struct archive *);
extern const char *archive_entry_pathname(struct archive_entry *);
extern void archive_entry_set_pathname(struct archive_entry *, const char *);
extern const char *archive_entry_hardlink(struct archive_entry *);
extern void archive_entry_set_hardlink(struct archive_entry *, const char *);

#define JB	"/private/var/jb"

/* ./var/jb/x -> /private/var/jb/x; anything else is not ours (NULL) */
static const char *
target(const char *p, char *buf, size_t len)
{
	if (strncmp(p, "./", 2) == 0)
		p += 2;
	if (strncmp(p, "var/jb", 6) != 0 || (p[6] != '/' && p[6] != 0))
		return NULL;
	snprintf(buf, len, "/private/%s", p);
	return buf;
}

int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: jbstrap ARCHIVE\n");
		return 2;
	}
	if (access(JB "/.procursus_strapped", F_OK) == 0) {
		printf("jbstrap: " JB " is strapped already\n");
		return 0;
	}
	struct archive *a = archive_read_new();
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	if (archive_read_open_filename(a, argv[1], 1 << 16) != ARCHIVE_OK) {
		printf("jbstrap: %s: %s\n", argv[1], archive_error_string(a));
		return 1;
	}
	unsigned files = 0, skipped = 0, failed = 0;
	struct archive_entry *e;
	int r;
	umask(0);
	while ((r = archive_read_next_header(a, &e)) == ARCHIVE_OK || r == ARCHIVE_WARN) {
		char path[1024], link[1024];
		if (target(archive_entry_pathname(e), path, sizeof(path)) == NULL) {
			skipped++;
			continue;
		}
		archive_entry_set_pathname(e, path);
		const char *h = archive_entry_hardlink(e);
		if (h != NULL && target(h, link, sizeof(link)) != NULL)
			archive_entry_set_hardlink(e, link);
		r = archive_read_extract(a, e, ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME |
		    ARCHIVE_EXTRACT_UNLINK);
		if (r != ARCHIVE_OK && r != ARCHIVE_WARN) {
			if (failed++ < 20)
				printf("jbstrap: %s: %s\n", path, archive_error_string(a));
		} else {
			files++;
		}
	}
	if (r != ARCHIVE_EOF)
		printf("jbstrap: reading: %s\n", archive_error_string(a));
	archive_read_free(a);
	printf("jbstrap: %u entries into " JB ", %u outside it skipped, %u failed\n", files, skipped, failed);
	return failed || r != ARCHIVE_EOF;
}
