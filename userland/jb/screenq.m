/*
 * screenq: what UIKit says about the main screen's colour (WebKit's ScreenProperties take
 * screenSupportsExtendedColor from UIScreen in the UI process). MIT, as userland/.
 */
#import <Foundation/Foundation.h>
#import <objc/message.h>
#import <objc/runtime.h>
#include <dlfcn.h>

int
main(void)
{
	@autoreleasepool {
		dlopen("/System/Library/Frameworks/UIKit.framework/UIKit", RTLD_NOW);
		Class UIScreen = NSClassFromString(@"UIScreen");
		id s = ((id (*)(id, SEL))objc_msgSend)(UIScreen, sel_registerName("mainScreen"));
		printf("screenq: mainScreen %s\n", s ? "yes" : "no");
		const char *sels[] = { "_supportsDeepColor", "_displayGamut", "_isExtendedColor", "_supportsExtendedColor" };
		for (unsigned i = 0; s && i < sizeof(sels) / sizeof(sels[0]); i++) {
			SEL sel = sel_registerName(sels[i]);
			if ([s respondsToSelector:sel])
				printf("screenq: %s = %ld\n", sels[i], ((long (*)(id, SEL))objc_msgSend)(s, sel));
			else
				printf("screenq: %s: no such method\n", sels[i]);
		}
		id tc = s ? ((id (*)(id, SEL))objc_msgSend)(s, sel_registerName("traitCollection")) : nil;
		if (tc)
			printf("screenq: traitCollection.displayGamut = %ld\n", ((long (*)(id, SEL))objc_msgSend)(tc, sel_registerName("displayGamut")));
	}
	return 0;
}
