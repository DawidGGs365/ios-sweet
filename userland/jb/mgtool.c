/*
 * mgtool: reads and edits MobileGestalt's answer cache (CacheExtra in com.apple.MobileGestalt.plist),
 * whose keys are base64(MD5("MGCopyAnswer" + name)) cut to 22 characters.
 *
 *   mgtool get NAME...          print each answer the cache has
 *   mgtool set NAME INTEGER     replace one integer answer (a copy of the file is kept as .sweet-orig)
 *   mgtool setbool NAME true|false   add or replace a boolean answer (same copy)
 *
 * Safari (jb2): WebKit asks for 10-bit 'w30r' / 'b3a8' tiles on a P3 display, and only its sRGB
 * (BGRA) surfaces get drawn here; an sRGB answer for the display gamut keeps it on BGRA. MIT, as
 * userland/.
 */
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CACHE "/private/var/containers/Shared/SystemGroup/systemgroup.com.apple.mobilegestaltcache/Library/Caches/com.apple.MobileGestalt.plist"

static CFStringRef
obfuscated(const char *name)
{
	char in[256];
	unsigned char md[CC_MD5_DIGEST_LENGTH];
	snprintf(in, sizeof(in), "MGCopyAnswer%s", name);
	CC_MD5(in, (CC_LONG)strlen(in), md);
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char out[25];
	int o = 0;
	for (int i = 0; i < 16; i += 3) {
		unsigned v = (unsigned)md[i] << 16 | (i + 1 < 16 ? (unsigned)md[i + 1] << 8 : 0) | (i + 2 < 16 ? md[i + 2] : 0);
		out[o++] = b64[(v >> 18) & 63];
		out[o++] = b64[(v >> 12) & 63];
		out[o++] = b64[(v >> 6) & 63];
		out[o++] = b64[v & 63];
	}
	out[22] = 0;
	return CFStringCreateWithCString(NULL, out, kCFStringEncodingASCII);
}

static void
print_value(const char *name, CFTypeRef v)
{
	if (v == NULL) {
		printf("mgtool: %s: not in the cache\n", name);
		return;
	}
	if (CFGetTypeID(v) == CFNumberGetTypeID()) {
		long long n = 0;
		CFNumberGetValue(v, kCFNumberLongLongType, &n);
		printf("mgtool: %s = %lld\n", name, n);
	} else if (CFGetTypeID(v) == CFBooleanGetTypeID()) {
		printf("mgtool: %s = %s\n", name, CFBooleanGetValue(v) ? "true" : "false");
	} else if (CFGetTypeID(v) == CFStringGetTypeID()) {
		char buf[256] = "";
		CFStringGetCString(v, buf, sizeof(buf), kCFStringEncodingUTF8);
		printf("mgtool: %s = \"%s\"\n", name, buf);
	} else {
		printf("mgtool: %s = (type %lu)\n", name, (unsigned long)CFGetTypeID(v));
	}
}

int
main(int argc, char **argv)
{
	int setbool = strcmp(argv[1], "setbool") == 0;
	if (argc < 3 || (strcmp(argv[1], "get") != 0 && strcmp(argv[1], "set") != 0 && !setbool) ||
	    (strcmp(argv[1], "get") != 0 && argc != 4)) {
		fprintf(stderr, "usage: mgtool get NAME... | mgtool set NAME INTEGER | mgtool setbool NAME true|false\n");
		return 2;
	}
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)CACHE, strlen(CACHE), false);
	CFReadStreamRef rs = CFReadStreamCreateWithFile(NULL, url);
	if (rs == NULL || !CFReadStreamOpen(rs)) {
		printf("mgtool: cannot open " CACHE "\n");
		return 1;
	}
	CFPropertyListFormat fmt = 0;
	CFErrorRef err = NULL;
	CFPropertyListRef pl = CFPropertyListCreateWithStream(NULL, rs, 0, kCFPropertyListMutableContainersAndLeaves, &fmt, &err);
	CFReadStreamClose(rs);
	if (pl == NULL || CFGetTypeID(pl) != CFDictionaryGetTypeID()) {
		printf("mgtool: the cache is not a dictionary\n");
		return 1;
	}
	CFMutableDictionaryRef extra = (CFMutableDictionaryRef)CFDictionaryGetValue(pl, CFSTR("CacheExtra"));
	if (extra == NULL || CFGetTypeID(extra) != CFDictionaryGetTypeID()) {
		printf("mgtool: no CacheExtra\n");
		return 1;
	}
	printf("mgtool: %ld answers in CacheExtra\n", (long)CFDictionaryGetCount(extra));
	if (strcmp(argv[1], "get") == 0) {
		for (int i = 2; i < argc; i++)
			print_value(argv[i], CFDictionaryGetValue(extra, obfuscated(argv[i])));
		return 0;
	}
	CFStringRef key = obfuscated(argv[2]);
	CFTypeRef old = CFDictionaryGetValue(extra, key);
	print_value(argv[2], old);
	CFTypeRef nv;
	if (setbool) {
		/*
		 * WebKit's screenSupportsExtendedColor() is MGGetBoolAnswer(HasExtendedColorDisplay), which
		 * MobileGestalt derives from the model (iPhone10,3: true), not from the device tree; an
		 * answer in CacheExtra is what MobileGestalt returns first.
		 */
		nv = strcmp(argv[3], "true") == 0 ? kCFBooleanTrue : kCFBooleanFalse;
	} else {
		if (old == NULL || CFGetTypeID(old) != CFNumberGetTypeID()) {
			printf("mgtool: %s is not an integer answer in the cache: left alone\n", argv[2]);
			return 1;
		}
		long long n = strtoll(argv[3], NULL, 0);
		nv = CFNumberCreate(NULL, kCFNumberLongLongType, &n);
	}
	CFDictionarySetValue(extra, key, nv);
	if (access(CACHE ".sweet-orig", F_OK) != 0) {
		FILE *in = fopen(CACHE, "rb"), *out = fopen(CACHE ".sweet-orig", "wb");
		char buf[65536];
		size_t k;
		while (in != NULL && out != NULL && (k = fread(buf, 1, sizeof(buf), in)) > 0)
			fwrite(buf, 1, k, out);
		if (in != NULL)
			fclose(in);
		if (out != NULL)
			fclose(out);
	}
	CFWriteStreamRef ws = CFWriteStreamCreateWithFile(NULL, url);
	if (ws == NULL || !CFWriteStreamOpen(ws) ||
	    CFPropertyListWrite(pl, ws, fmt ? fmt : kCFPropertyListBinaryFormat_v1_0, 0, &err) <= 0) {
		printf("mgtool: writing the cache failed\n");
		return 1;
	}
	CFWriteStreamClose(ws);
	print_value(argv[2], nv);
	printf("mgtool: written (original in " CACHE ".sweet-orig)\n");
	return 0;
}
