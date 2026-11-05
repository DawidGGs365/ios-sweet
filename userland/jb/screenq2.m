/* screenq2: UIScreen / UITraitCollection methods about gamut and colour (names only). MIT. */
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#include <dlfcn.h>
#include <string.h>
static void
dump(Class c, int meta)
{
	unsigned n = 0;
	Method *m = class_copyMethodList(meta ? object_getClass(c) : c, &n);
	for (unsigned i = 0; i < n; i++) {
		const char *s = sel_getName(method_getName(m[i]));
		if (strcasestr(s, "gamut") || strcasestr(s, "deepcolor") || strcasestr(s, "extendedcolor") || strcasestr(s, "wideColor"))
			printf("%s %c%s\n", class_getName(c), meta ? '+' : '-', s);
	}
	free(m);
}
int
main(void)
{
	dlopen("/System/Library/Frameworks/UIKit.framework/UIKit", RTLD_NOW);
	const char *cls[] = { "UIScreen", "UITraitCollection", "_UIScreenDisplayInfo", "UIDevice", "_UIDisplayInfoProvider", "FBSDisplayConfiguration", "CADisplay" };
	for (unsigned i = 0; i < sizeof(cls) / sizeof(cls[0]); i++) {
		Class c = objc_getClass(cls[i]);
		if (c) { dump(c, 0); dump(c, 1); } else printf("%s: none\n", cls[i]);
	}
	return 0;
}
