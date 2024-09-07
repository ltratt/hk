#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// How many nanoseconds to wait when polling to see if all keys are released.
#define WAIT_RELEASE 10000000

// How many seconds (as a double) between printing out which keys are pressed
// in verbose mode.
#define VERBOSE_INTERVAL 1.0

// A KeyCode representing no "real" KeyCode. The X11 spec defines the valid
// `KeyCode` values to be 8..255 (both ends inclusive), but GrabKey uses `0` as
// `AnyKey`, so we use `1` as a safe bet.
#define NO_KEYCODE 1

// The X11 modifiers which we want to ignore when listening for a hotkey. Put
// another way, we don't care whether these modifiers are set or unset when
// listening for a hotkey; we also don't care if these are set when waiting
// for "all" keys to be released with `-w`.
static const KeySym IGNORABLE_MODIFIER_KEYSYMS[] = {
    XK_Caps_Lock, XK_Num_Lock, XK_Scroll_Lock, XK_Mode_switch};

// Map the output from XGetModifierMapping to `XGrabKey` modifier masks. It's
// unclear whether we're actually required to do this or not (on my machine,
// these masks are defined as 1<<i for each i in the array), but it doesn't
// hurt to do so.
static const unsigned int MODIFIER_MASKS[] = {
    ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
};

extern char* __progname;

// Return an XGrabKey `modifiers` bitmask of all the ignorable modifiers.
unsigned int ignorable_modifiers(Display *dpy) {
    size_t num_ignorable = sizeof(IGNORABLE_MODIFIER_KEYSYMS) / sizeof(KeySym);
    KeyCode ignorable_keycodes[num_ignorable];
    for (size_t i = 0; i < num_ignorable; i++) {
        ignorable_keycodes[i] = XKeysymToKeycode(dpy, IGNORABLE_MODIFIER_KEYSYMS[i]);
    }

    XModifierKeymap *mm = XGetModifierMapping(dpy);
    unsigned int mask = 0;
    for (int i = 0; i < 8 * mm->max_keypermod; i++) {
        for (size_t j = 0; j < num_ignorable; j++) {
            if (ignorable_keycodes[j] != NoSymbol && mm->modifiermap[i] == ignorable_keycodes[j]) {
                mask |= MODIFIER_MASKS[i / mm->max_keypermod];
            }
        }
    }
    XFreeModifiermap(mm);

    return mask;
}

// Is the string `s` of `len` bytes case-insensitive-equal to the
// NULL-terminated string `cmp`?
bool streq(char *s, size_t len, char *cmp) {
    if (len != strlen(cmp))
        return false;
    for (size_t i = 0; i < len; i++) {
        if (tolower(s[i]) != tolower(cmp[i]))
            return false;
    }
    return true;
}

// If the string `s`, of `len` bytes, is a valid modifier, update
// `modifier_mask` and return `true`; else return `false`.
bool parse_modifier(Display *dpy, char *s, size_t len, unsigned int *modifier_mask) {
    char *map_name = NULL;
    // We have to map the "generic" modifier name the user gave us to a
    // physical key, and then see if that key exists on this keyboard.
    // Presumably this could fail in theory if, for example, a user has a
    // Control_R but not a Control_L key.
    if (streq(s, len, "alt")) {
        map_name = "Alt_L";
    } else if (streq(s, len, "ctrl") || streq(s, len, "control")) {
        map_name = "Control_L";
    } else if (streq(s, len, "shift")) {
        map_name = "Shift_L";
    } else if (streq(s, len, "meta")) {
        map_name = "Meta_L";
    } else if (streq(s, len, "super")) {
        map_name = "Super_L";
    } else {
        return false;
    }

    KeySym ks = XStringToKeysym(map_name);
    if (ks == NoSymbol)
        return false;
    KeyCode kc = XKeysymToKeycode(dpy, ks);
    XModifierKeymap *mm = XGetModifierMapping(dpy);
    bool found = false;
    for (int i = 0; i < 8 * mm->max_keypermod; i++) {
        if (mm->modifiermap[i] == kc) {
            if ((*modifier_mask & MODIFIER_MASKS[i / mm->max_keypermod]) != 0)
                errx(EXIT_FAILURE, "Repeated modifier '%.*s'", (int) len, s);
            *modifier_mask |= MODIFIER_MASKS[i / mm->max_keypermod];
            found = true;
            break;
        }
    }
    XFreeModifiermap(mm);
    return found;
}

// If the string `s`, of `len` bytes, is a valid keycode, update `keycode` and
// return `true`; else return `false`. If no keycode has been set, `*keycode`
// must be equal to `NO_KEYCODE`.
bool parse_key(Display *dpy, char *s, size_t len, KeyCode *keycode) {
    char name[len + 1];
    strncpy(name, s, len);
    name[len] = 0;
    KeySym ks = XStringToKeysym(name);
    if (ks != NoSymbol) {
        if (*keycode != NO_KEYCODE)
            errx(EXIT_FAILURE, "Repeated key '%.*s'", (int) len, s);
        *keycode = XKeysymToKeycode(dpy, ks);
        return true;
    }
    return false;
}

// Parse a string of the form 'A+B+C' into a modifier mask and keycode.
void parse(Display *dpy, char *s, unsigned int *modifier_mask, KeyCode *keycode) {
    size_t i = 0;
    *modifier_mask = 0;
    *keycode = NO_KEYCODE;
    while (i < strlen(s)) {
        char *j = strchr(s + i, '+');
        size_t k;
        if (j == s + i) {
            // String of the form 'Ctrl++F6': we can't guarantee that '+' isn't
            // a valid keyname, so we try to parse it as normal. This also
            // neatly deals with the case where the user tries to start the
            // string with `+`.
            k = 1;
        }
        else if (j == NULL)
            k = strlen(s);
        else
            k = j - (s + i);
        if (parse_modifier(dpy, s + i, k, modifier_mask) || parse_key(dpy, s + i, k, keycode)) {
            i += k + 1;
            continue;
        }
        errx(EXIT_FAILURE, "Illegal modifier or key '%.*s'", (int) k, s + i);
    }
}

// Return `(time1 - time0)` in seconds as a double. Note that it is undefined
// behaviour for `time0 < time1`.
double timespec_delta(const struct timespec *time1, const struct timespec *time0) {
    return (time1->tv_sec - time0->tv_sec) + (time1->tv_nsec - time0->tv_nsec) / 1e9;
}

static void usage(int rtn_code) {
    fprintf(stderr, "Usage: %s [-hw] <hotkey> <cmd> [<cmdarg1> ... <cmdargn>]\n", __progname);
    exit(rtn_code);
}

int main(int argc, char** argv) {
    bool verbose = false, wait = false;
    while (true) {
        int ch = getopt(argc, argv, "hvw");
        if (ch == -1)
            break;
        switch (ch) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 'v':
            verbose = true;
            break;
        case 'w':
            wait = true;
            break;
        default:
            usage(EXIT_FAILURE);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc < 2)
        usage(1);

    Display* dpy = XOpenDisplay(NULL);
    if (dpy == NULL)
        errx(EXIT_FAILURE, "Cannot open display");
    Window root = DefaultRootWindow(dpy);

    unsigned int ignore_mask = ignorable_modifiers(dpy);
    unsigned int mask;
    KeyCode keycode;
    parse(dpy, argv[0], &mask, &keycode);
    XGrabKey(dpy, keycode, mask, root, False, GrabModeAsync, GrabModeAsync);
    for (unsigned int i = 0; i <= ignore_mask; i++) {
        if ((ignore_mask & i) == i) {
            XGrabKey(dpy, keycode, mask | i, root, False, GrabModeAsync, GrabModeAsync);
        }
    }

    XSelectInput(dpy, root, KeyPressMask);
    while (true) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        // FIXME: Do we need to check that we get the KeyPress event we expected?
        if (ev.type == KeyPress)
            break;
    }

    if (wait) {
        struct timespec verbose_last;
        clock_gettime(CLOCK_MONOTONIC, &verbose_last);
        while (true) {
            char kr[32];
            XQueryKeymap(dpy, kr);
            bool pressed = false;
            uint64_t verbose_show = 0;
            if (verbose) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                if (timespec_delta(&now, &verbose_last) > VERBOSE_INTERVAL) {
                    verbose_last = now;
                    verbose_show = 1;
                }
            }
            for (int i = 0; i < 32; i++) {
                for (int j = 0; j < 8; j++) {
                    if (kr[i] & (1 << j)) {
                        bool ignorable = false;
                        KeySym ks = XkbKeycodeToKeysym(dpy, i * 8 + j, 0, 0);
                        for (size_t k = 0; k < sizeof(IGNORABLE_MODIFIER_KEYSYMS) / sizeof(KeySym); k++) {
                            if (ks == IGNORABLE_MODIFIER_KEYSYMS[k]) {
                                ignorable = true;
                            }
                        }
                        if (!ignorable) {
                            if (verbose_show > 0) {
                                if (verbose_show == 1) {
                                    printf("Key(s) pressed:");
                                }
                                char *s = XKeysymToString(ks);
                                if (s)
                                    printf(" %s", s);
                                else
                                    printf(" <unknown Keysym>");
                                verbose_show++;
                            }
                            pressed = true;
                        }
                    }
                }
            }
            if (!pressed)
                break;
            if (verbose_show > 1)
                printf("\n");
            struct timespec tm;
            tm.tv_sec = 0;
            tm.tv_nsec = WAIT_RELEASE;
            nanosleep(&tm, NULL);
        }
    }
    XCloseDisplay(dpy);
    execvp(argv[1], argv + 1);
    err(EXIT_FAILURE, "Couldn't run command");
}
