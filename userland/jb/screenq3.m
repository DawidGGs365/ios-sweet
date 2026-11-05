/* screenq3: the values behind the screen's colour gamut in UIKit, FrontBoard and CoreAnimation. MIT. */
#import <Foundation/Foundation.h>
#import <objc/message.h>
#import <objc/runtime.h>
#include <dlfcn.h>
static long
call(id o, const char *s)
{
	SEL sel = sel_registerName(s);
	return (o && [o respondsToSelector:sel]) ? ((long (*)(id, SEL))objc_msgSend)(o, sel) : -99;
}
int
main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	@autoreleasepool {
		dlopen("/System/Library/Frameworks/UIKit.framework/UIKit", RTLD_NOW);
		id scr = ((id (*)(id, SEL))objc_msgSend)(objc_getClass("UIScreen"), sel_registerName("mainScreen"));
		id dev = ((id (*)(id, SEL))objc_msgSend)(objc_getClass("UIDevice"), sel_registerName("currentDevice"));
		printf("UIScreen.gamut = %ld\n", call(scr, "gamut"));
		printf("UIDevice._nativeScreenGamut = %ld\n", call(dev, "_nativeScreenGamut"));
		printf("UIDevice._supportsDeepColor = %ld\n", call(dev, "_supportsDeepColor"));
		id disp = ((id (*)(id, SEL))objc_msgSend)(objc_getClass("CADisplay"), sel_registerName("mainDisplay"));
		printf("CADisplay.mainDisplay.supportsExtendedColors = %ld\n", call(disp, "supportsExtendedColors"));
		id cfg = scr ? ((id (*)(id, SEL))objc_msgSend)(scr, sel_registerName("displayConfiguration")) : nil;
		printf("FBSDisplayConfiguration.colorGamut = %ld supportsExtendedColor = %ld\n", call(cfg, "colorGamut"), call(cfg, "supportsExtendedColor"));
	}
	return 0;
}
