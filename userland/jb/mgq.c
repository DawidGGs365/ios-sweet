/*
 * mgq: asks MobileGestalt itself (MGCopyAnswer) for answers by name, as UIKit and WebKit do.
 *   mgq NAME...      MIT, as userland/.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>

extern CFTypeRef MGCopyAnswer(CFStringRef question, CFDictionaryRef options);

int
main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		CFStringRef q = CFStringCreateWithCString(NULL, argv[i], kCFStringEncodingUTF8);
		CFTypeRef a = MGCopyAnswer(q, NULL);
		char buf[512] = "(null)";
		if (a != NULL) {
			CFStringRef d = CFCopyDescription(a);
			CFStringGetCString(d, buf, sizeof(buf), kCFStringEncodingUTF8);
		}
		printf("%s = %s\n", argv[i], buf);
	}
	return 0;
}
