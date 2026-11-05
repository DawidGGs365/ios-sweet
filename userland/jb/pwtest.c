/*
 * pwtest: why the bootstrap's ssh-keygen says "No user exists for uid 0" (jb1): the user lookups
 * libinfo answers, and the files they would come from. MIT, as userland/.
 */
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void
show(const char *what, struct passwd *pw, int err)
{
	if (pw != NULL)
		printf("pwtest: %s -> %s uid %d gid %d home %s shell %s\n", what, pw->pw_name, pw->pw_uid, pw->pw_gid,
		    pw->pw_dir, pw->pw_shell);
	else
		printf("pwtest: %s -> none (errno %d %s)\n", what, err, strerror(err));
}

int
main(void)
{
	printf("pwtest: uid %d euid %d gid %d\n", getuid(), geteuid(), getgid());
	errno = 0;
	struct passwd *pw = getpwuid(0);
	show("getpwuid(0)", pw, errno);
	const char *names[] = { "root", "mobile", "_sshd", "sshd", "nobody" };
	for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		char what[64];
		snprintf(what, sizeof(what), "getpwnam(%s)", names[i]);
		errno = 0;
		pw = getpwnam(names[i]);
		show(what, pw, errno);
	}
	setpwent();
	int n = 0;
	while ((pw = getpwent()) != NULL)
		n++;
	endpwent();
	printf("pwtest: getpwent: %d entries\n", n);
	const char *files[] = { "/etc/passwd", "/etc/master.passwd", "/private/etc/passwd", "/var/jb/etc/passwd" };
	for (unsigned i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
		FILE *f = fopen(files[i], "r");
		char line[256] = "";
		if (f != NULL) {
			while (fgets(line, sizeof(line), f) != NULL && line[0] == '#')
				;
			fclose(f);
			line[strcspn(line, "\n")] = 0;
			printf("pwtest: %s: first entry %.20s...\n", files[i], line);
		} else {
			printf("pwtest: %s: %s\n", files[i], strerror(errno));
		}
	}
	return 0;
}
