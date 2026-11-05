/*
 * wifisvc: a network service for en0 in the SystemConfiguration preferences, as iOS makes one at
 * first boot for its built-in Wi-Fi. The phone's preferences.plist dates from before en0 existed
 * (0139), so configd named en0 and saw its link go up, but IPConfiguration never ran DHCP on it
 * (wifi29: no service, no IPConfiguration line).
 *
 *   wifisvc [IFNAME]      IFNAME defaults to en0; does nothing when a service for it exists
 *
 * The SCPreferences / SCNetworkConfiguration calls are in the iOS SystemConfiguration.framework,
 * but its SDK headers mark them unavailable on iOS: declared here as macOS's headers have them.
 * MIT, as userland/.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>

typedef const struct __SCPreferences *SCPreferencesRef;
typedef const struct __SCNetworkInterface *SCNetworkInterfaceRef;
typedef const struct __SCNetworkService *SCNetworkServiceRef;
typedef const struct __SCNetworkSet *SCNetworkSetRef;

extern SCPreferencesRef SCPreferencesCreate(CFAllocatorRef, CFStringRef, CFStringRef);
extern Boolean SCPreferencesLock(SCPreferencesRef, Boolean);
extern Boolean SCPreferencesUnlock(SCPreferencesRef);
extern Boolean SCPreferencesCommitChanges(SCPreferencesRef);
extern Boolean SCPreferencesApplyChanges(SCPreferencesRef);
extern int SCError(void);
extern const char *SCErrorString(int);
extern CFArrayRef SCNetworkInterfaceCopyAll(void);
extern CFStringRef SCNetworkInterfaceGetBSDName(SCNetworkInterfaceRef);
extern CFArrayRef SCNetworkServiceCopyAll(SCPreferencesRef);
extern SCNetworkInterfaceRef SCNetworkServiceGetInterface(SCNetworkServiceRef);
extern SCNetworkServiceRef SCNetworkServiceCreate(SCPreferencesRef, SCNetworkInterfaceRef);
extern Boolean SCNetworkServiceEstablishDefaultConfiguration(SCNetworkServiceRef);
extern Boolean SCNetworkServiceSetName(SCNetworkServiceRef, CFStringRef);
extern CFStringRef SCNetworkServiceGetServiceID(SCNetworkServiceRef);
extern SCNetworkSetRef SCNetworkSetCopyCurrent(SCPreferencesRef);
extern CFArrayRef SCNetworkSetCopyAll(SCPreferencesRef);
extern SCNetworkSetRef SCNetworkSetCreate(SCPreferencesRef);
extern Boolean SCNetworkSetSetCurrent(SCNetworkSetRef);
extern Boolean SCNetworkSetSetName(SCNetworkSetRef, CFStringRef);
extern Boolean SCNetworkSetAddService(SCNetworkSetRef, SCNetworkServiceRef);

static int
is_if(SCNetworkInterfaceRef i, CFStringRef name)
{
	CFStringRef b = i != NULL ? SCNetworkInterfaceGetBSDName(i) : NULL;
	return b != NULL && CFEqual(b, name);
}

static void
cfprint(const char *what, CFStringRef s)
{
	char buf[128] = "";
	if (s != NULL)
		CFStringGetCString(s, buf, sizeof(buf), kCFStringEncodingUTF8);
	printf("wifisvc: %s %s\n", what, buf);
}

int
main(int argc, char **argv)
{
	CFStringRef name = CFStringCreateWithCString(NULL, argc > 1 ? argv[1] : "en0", kCFStringEncodingUTF8);
	SCPreferencesRef prefs = SCPreferencesCreate(NULL, CFSTR("wifisvc"), NULL);
	if (prefs == NULL || !SCPreferencesLock(prefs, true)) {
		printf("wifisvc: preferences: %s\n", SCErrorString(SCError()));
		return 1;
	}
	CFArrayRef services = SCNetworkServiceCopyAll(prefs);
	for (CFIndex k = 0; services != NULL && k < CFArrayGetCount(services); k++) {
		SCNetworkServiceRef s = CFArrayGetValueAtIndex(services, k);
		if (is_if(SCNetworkServiceGetInterface(s), name)) {
			cfprint("a service exists already:", SCNetworkServiceGetServiceID(s));
			SCPreferencesUnlock(prefs);
			return 0;
		}
	}
	printf("wifisvc: %ld services, none for the interface\n", services != NULL ? (long)CFArrayGetCount(services) : 0L);

	SCNetworkInterfaceRef iface = NULL;
	CFArrayRef all = SCNetworkInterfaceCopyAll();
	for (CFIndex k = 0; all != NULL && k < CFArrayGetCount(all); k++)
		if (is_if(CFArrayGetValueAtIndex(all, k), name))
			iface = CFArrayGetValueAtIndex(all, k);
	if (iface == NULL) {
		printf("wifisvc: no such interface (yet)\n");
		SCPreferencesUnlock(prefs);
		return 1;
	}

	SCNetworkSetRef set = SCNetworkSetCopyCurrent(prefs);
	if (set == NULL) {			/* no current set at all: make the default one */
		set = SCNetworkSetCreate(prefs);
		if (set != NULL) {
			SCNetworkSetSetName(set, CFSTR("Automatic"));
			SCNetworkSetSetCurrent(set);
			printf("wifisvc: made a current set\n");
		}
	}
	SCNetworkServiceRef svc = SCNetworkServiceCreate(prefs, iface);
	if (set == NULL || svc == NULL || !SCNetworkServiceEstablishDefaultConfiguration(svc) ||
	    !SCNetworkSetAddService(set, svc)) {
		printf("wifisvc: service: %s\n", SCErrorString(SCError()));
		SCPreferencesUnlock(prefs);
		return 1;
	}
	SCNetworkServiceSetName(svc, CFSTR("Wi-Fi"));
	cfprint("service made:", SCNetworkServiceGetServiceID(svc));
	if (!SCPreferencesCommitChanges(prefs) || !SCPreferencesApplyChanges(prefs)) {
		printf("wifisvc: commit/apply: %s\n", SCErrorString(SCError()));
		SCPreferencesUnlock(prefs);
		return 1;
	}
	SCPreferencesUnlock(prefs);
	printf("wifisvc: committed and applied\n");
	return 0;
}
