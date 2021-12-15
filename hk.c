#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// How many nanoseconds to wait when polling to see if all keys are released.
#define WAIT_RELEASE 10000000

// The X11 modifiers which we want to ignore when listening for a hotkey. Put
// another way, we don't care whether these modifiers are set or unset when
// listening for a hotkey.
static const KeySym IGNORABLE_MODIFIER_KEYSYMS[] = {XK_Caps_Lock, XK_Num_Lock, XK_Scroll_Lock};

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
    KeyCode ignorable_keycodes[sizeof(IGNORABLE_MODIFIER_KEYSYMS) / sizeof(KeySym)];
    for (size_t i = 0; i < sizeof(IGNORABLE_MODIFIER_KEYSYMS) / sizeof(KeySym); i++) {
        ignorable_keycodes[i] = XKeysymToKeycode(dpy, IGNORABLE_MODIFIER_KEYSYMS[i]);
    }

    XModifierKeymap *mm = XGetModifierMapping(dpy);
    unsigned int mask = 0;
    for (int i = 0; i < 8 * mm->max_keypermod; i++) {
        for (size_t j = 0; j < sizeof(ignorable_keycodes) / sizeof(KeyCode); j++) {
            if (ignorable_keycodes[j] != NoSymbol && mm->modifiermap[i] == ignorable_keycodes[j]) {
                mask |= MODIFIER_MASKS[i / mm->max_keypermod];
            }
        }
    }
    XFreeModifiermap(mm);

    return mask;
}

// Is the string `s` of `end` bytes case-insensitive-equal to the string `cmp`?
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
    }

    if (map_name == NULL)
        return false;
    KeySym ks = XStringToKeysym(map_name);
    if (ks == NoSymbol)
        return false;
    KeyCode kc = XKeysymToKeycode(dpy, ks);
    XModifierKeymap *mm = XGetModifierMapping(dpy);
    bool found = false;
    for (int i = 0; i < 8 * mm->max_keypermod; i++) {
        if (mm->modifiermap[i] == kc) {
            *modifier_mask |= MODIFIER_MASKS[i / mm->max_keypermod];
            found = true;
            break;
        }
    }
    XFreeModifiermap(mm);
    return found;
}

// If the string `s`, of `len` bytes, is a valid keycode, update `keycode` and
// return `true`; else return `false`.
bool parse_key(Display *dpy, char *s, size_t len, KeyCode *keycode) {
    char name[len + 1];
    strncpy(name, s, len);
    name[len] = 0;
    KeySym ks = XStringToKeysym(name);
    if (ks != NoSymbol) {
        if (*keycode != NoSymbol)
            errx(EXIT_FAILURE, "Can't bind a second key '%.*s'", (int) len, s);
        *keycode = XKeysymToKeycode(dpy, ks);
        return true;
    }
    return false;
}

// Parse a string of the form 'A+B+C' into a modifier mask and keycode.
void parse(Display *dpy, char *s, unsigned int *modifier_mask, KeyCode *keycode) {
    size_t i = 0;
    *modifier_mask = 0;
    *keycode = NoSymbol;
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

static void usage(int rtn_code)
{
    fprintf(stderr, "Usage: %s [-hw] <hotkey> <cmd> [<cmdarg1> ... <cmdargn>]\n", __progname);
    exit(rtn_code);
}

int main(int argc, char** argv) {
    bool wait = false;
    while (true) {
        int ch = getopt(argc, argv, "hw");
        if (ch == -1)
            break;
        switch (ch) {
        case 'h':
            usage(EXIT_SUCCESS);
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
        while (true) {
            char kr[32];
            XQueryKeymap(dpy, kr);
            bool pressed = false;
            for (int i = 0; i < 32; i++) {
                if (kr[i] != 0) {
                    pressed = true;
                    break;
                }
            }
            if (!pressed)
                break;
            struct timespec tm;
            tm.tv_sec = 0;
            tm.tv_nsec = WAIT_RELEASE;
            nanosleep(&tm, NULL);
        }
    }
    XCloseDisplay(dpy);
    if (execvp(argv[1], argv + 1) != 0) {
        err(EXIT_FAILURE, "Couldn't run command");
    }

    return EXIT_SUCCESS;
}
