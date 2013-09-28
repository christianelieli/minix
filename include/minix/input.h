#ifndef _MINIX_INPUT_H
#define _MINIX_INPUT_H

#include <sys/types.h>

#ifdef _SYSTEM

/* Device type. */
#define INPUT_DEV_KBD		0x01	/* keyboard device */
#define INPUT_DEV_MOUSE		0x02	/* mouse device */

/* Known-invalid input device ID. */
#define INVALID_INPUT_ID	(-1)

#endif /* _SYSTEM */

/*
 * The input server heavily draws on the USB HID specification for events.
 * Every driver should convert its event codes into this format.  This also
 * implies that we use the U.S. keyboard layout for key events.  The mapping to
 * localized keyboard layouts will be done at a different place (TTY, Xserver).
 */

/* A single event, as read from one of the input devices. */
struct input_event {
	u16_t page;			/* event page (INPUT_PAGE_) */
	u16_t code;			/* page-specific event code */
	i32_t value;			/* event value */
	u16_t flags;			/* event flags (INPUT_FLAG_) */
	u16_t devid;			/* identifier of source device */
	u32_t rsvd[2];			/* reserved for a future timestamp */
};

/* Event pages. */
#define INPUT_PAGE_GD		0x0001	/* General Desktop page */
#define INPUT_PAGE_KEY		0x0007	/* Keyboard/Keypad page */
#define INPUT_PAGE_LED		0x0008	/* LED page */
#define INPUT_PAGE_BUTTON	0x0009	/* Button page */
#define INPUT_PAGE_CONS		0x000C	/* Consumer page */

/* Event values.  Not exhaustive. */
#define INPUT_RELEASE		0
#define INPUT_PRESS		1

/* Event flags. */
#define INPUT_FLAG_ABS		0x00	/* absolute value (the default) */
#define INPUT_FLAG_REL		0x04	/* relative value */

/* Page-specific event codes. */
enum {
	INPUT_GD_X = 0x0030,
	INPUT_GD_Y,

	INPUT_GD_SYSTEM_POWER_DOWN = 0x0081,
	INPUT_GD_SYSTEM_SLEEP,
	INPUT_GD_SYSTEM_WAKE_UP
};

enum {
	INPUT_KEY_A = 0x0004,
	INPUT_KEY_B,
	INPUT_KEY_C,
	INPUT_KEY_D,
	INPUT_KEY_E,
	INPUT_KEY_F,
	INPUT_KEY_G,
	INPUT_KEY_H,
	INPUT_KEY_I,
	INPUT_KEY_J,
	INPUT_KEY_K,
	INPUT_KEY_L,
	INPUT_KEY_M,
	INPUT_KEY_N,
	INPUT_KEY_O,
	INPUT_KEY_P,
	INPUT_KEY_Q,
	INPUT_KEY_R,
	INPUT_KEY_S,
	INPUT_KEY_T,
	INPUT_KEY_U,
	INPUT_KEY_V,
	INPUT_KEY_W,
	INPUT_KEY_X,
	INPUT_KEY_Y,
	INPUT_KEY_Z,
	INPUT_KEY_1,
	INPUT_KEY_2,
	INPUT_KEY_3,
	INPUT_KEY_4,
	INPUT_KEY_5,
	INPUT_KEY_6,
	INPUT_KEY_7,
	INPUT_KEY_8,
	INPUT_KEY_9,
	INPUT_KEY_0,

	INPUT_KEY_ENTER,
	INPUT_KEY_ESCAPE,
	INPUT_KEY_BACKSPACE,
	INPUT_KEY_TAB,
	INPUT_KEY_SPACEBAR,
	INPUT_KEY_DASH,
	INPUT_KEY_EQUAL,
	INPUT_KEY_OPEN_BRACKET,
	INPUT_KEY_CLOSE_BRACKET,
	INPUT_KEY_BACKSLASH,
	INPUT_KEY_EUROPE_1,
	INPUT_KEY_SEMICOLON,
	INPUT_KEY_APOSTROPH,
	INPUT_KEY_GRAVE_ACCENT,
	INPUT_KEY_COMMA,
	INPUT_KEY_PERIOD,
	INPUT_KEY_SLASH,
	INPUT_KEY_CAPS_LOCK,

	INPUT_KEY_F1,
	INPUT_KEY_F2,
	INPUT_KEY_F3,
	INPUT_KEY_F4,
	INPUT_KEY_F5,
	INPUT_KEY_F6,
	INPUT_KEY_F7,
	INPUT_KEY_F8,
	INPUT_KEY_F9,
	INPUT_KEY_F10,
	INPUT_KEY_F11,
	INPUT_KEY_F12,

	INPUT_KEY_PRINT_SCREEN,
	INPUT_KEY_SCROLL_LOCK,
	INPUT_KEY_PAUSE,
	INPUT_KEY_INSERT,
	INPUT_KEY_HOME,
	INPUT_KEY_PAGE_UP,
	INPUT_KEY_DELETE,
	INPUT_KEY_END,
	INPUT_KEY_PAGE_DOWN,
	INPUT_KEY_RIGHT_ARROW,
	INPUT_KEY_LEFT_ARROW,
	INPUT_KEY_DOWN_ARROW,
	INPUT_KEY_UP_ARROW,
	INPUT_KEY_NUM_LOCK,

	INPUT_KEY_KP_SLASH,
	INPUT_KEY_KP_STAR,
	INPUT_KEY_KP_DASH,
	INPUT_KEY_KP_PLUS,
	INPUT_KEY_KP_ENTER,
	INPUT_KEY_KP_1,
	INPUT_KEY_KP_2,
	INPUT_KEY_KP_3,
	INPUT_KEY_KP_4,
	INPUT_KEY_KP_5,
	INPUT_KEY_KP_6,
	INPUT_KEY_KP_7,
	INPUT_KEY_KP_8,
	INPUT_KEY_KP_9,
	INPUT_KEY_KP_0,
	INPUT_KEY_KP_PERIOD,

	INPUT_KEY_EUROPE_2,
	INPUT_KEY_APPLICATION,
	INPUT_KEY_POWER,
	INPUT_KEY_KP_EQUAL,

	INPUT_KEY_F13,
	INPUT_KEY_F14,
	INPUT_KEY_F15,
	INPUT_KEY_F16,
	INPUT_KEY_F17,
	INPUT_KEY_F18,
	INPUT_KEY_F19,
	INPUT_KEY_F20,
	INPUT_KEY_F21,
	INPUT_KEY_F22,
	INPUT_KEY_F23,
	INPUT_KEY_F24,

	INPUT_KEY_EXECUTE,
	INPUT_KEY_HELP,
	INPUT_KEY_MENU,
	INPUT_KEY_SELECT,
	INPUT_KEY_STOP,
	INPUT_KEY_AGAIN,
	INPUT_KEY_UNDO,
	INPUT_KEY_CUT,
	INPUT_KEY_COPY,
	INPUT_KEY_PASTE,
	INPUT_KEY_FIND,
	INPUT_KEY_MUTE,
	INPUT_KEY_VOLUME_UP,
	INPUT_KEY_VOLUME_DOWN,
	INPUT_KEY_LOCKING_CAPS_LOCK,
	INPUT_KEY_LOCKING_NUM_LOCK,
	INPUT_KEY_LOCKING_SCROLL_LOCK,
	INPUT_KEY_KP_COMMA,
	INPUT_KEY_EQUAL_SIGN,
	INPUT_KEY_I10L_1,
	INPUT_KEY_I10L_2,
	INPUT_KEY_I10L_3,
	INPUT_KEY_I10L_4,
	INPUT_KEY_I10L_5,
	INPUT_KEY_I10L_6,
	INPUT_KEY_I10L_7,
	INPUT_KEY_I10L_8,
	INPUT_KEY_I10L_9,
	INPUT_KEY_LANG_1,
	INPUT_KEY_LANG_2,
	INPUT_KEY_LANG_3,
	INPUT_KEY_LANG_4,
	INPUT_KEY_LANG_5,
	INPUT_KEY_LANG_6,
	INPUT_KEY_LANG_7,
	INPUT_KEY_LANG_8,
	INPUT_KEY_LANG_9,
	INPUT_KEY_ALT_ERASE,
	INPUT_KEY_SYSREQ,
	INPUT_KEY_CANCEL,
	INPUT_KEY_CLEAR,
	INPUT_KEY_PRIOR,
	INPUT_KEY_RETURN,
	INPUT_KEY_SEPARATOR,
	INPUT_KEY_OUT,
	INPUT_KEY_OPER,
	INPUT_KEY_CLEAR_AGAIN,
	INPUT_KEY_CR_SEL,
	INPUT_KEY_EX_SEL,

	/* 0x00A5 -- 0x00AF RESERVED */

	INPUT_KEY_KP_00 = 0x00B0,
	INPUT_KEY_KP_000,
	INPUT_KEY_THOUSANDS_SEP,
	INPUT_KEY_DECIMAL_SEP,
	INPUT_KEY_CURRENCY_UNIT,
	INPUT_KEY_CURRENCY_SUBUNIT,
	INPUT_KEY_KP_OPEN_PARENTHESIS,
	INPUT_KEY_KP_CLOSE_PARENTHESIS,
	INPUT_KEY_KP_OPEN_BRACE,
	INPUT_KEY_KP_CLOSE_BRACE,
	INPUT_KEY_KP_TAB,
	INPUT_KEY_KP_BACKSPACE,
	INPUT_KEY_KP_A,
	INPUT_KEY_KP_B,
	INPUT_KEY_KP_C,
	INPUT_KEY_KP_D,
	INPUT_KEY_KP_E,
	INPUT_KEY_KP_F,
	INPUT_KEY_KP_XOR,
	INPUT_KEY_KP_CARET,
	INPUT_KEY_KP_PERCENT,
	INPUT_KEY_KP_SMALLER_THEN,
	INPUT_KEY_KP_GREATER_THEN,
	INPUT_KEY_KP_AMP,
	INPUT_KEY_KP_DOUBLE_AMP,
	INPUT_KEY_KP_PIPE,
	INPUT_KEY_KP_DOUBLE_PIPE,
	INPUT_KEY_KP_COLON,
	INPUT_KEY_KP_NUMBER,
	INPUT_KEY_KP_SPACE,
	INPUT_KEY_KP_AT,
	INPUT_KEY_KP_EXCLAMATION_MARK,
	INPUT_KEY_KP_MEM_STORE,
	INPUT_KEY_KP_MEM_RECALL,
	INPUT_KEY_KP_MEM_CLEAR,
	INPUT_KEY_KP_MEM_ADD,
	INPUT_KEY_KP_MEM_SUBTRACT,
	INPUT_KEY_KP_MEM_MULTIPLY,
	INPUT_KEY_KP_MEM_DIVIDE,
	INPUT_KEY_KP_PLUS_MINUS,
	INPUT_KEY_KP_CLEAR,
	INPUT_KEY_KP_CLEAR_ENTRY,
	INPUT_KEY_KP_BIN,
	INPUT_KEY_KP_OCT,
	INPUT_KEY_KP_DEC,
	INPUT_KEY_KP_HEX,

	/* 0x00DE, 0x00DF RESERVED */

	INPUT_KEY_LEFT_CTRL = 0x00E0,
	INPUT_KEY_LEFT_SHIFT,
	INPUT_KEY_LEFT_ALT,
	INPUT_KEY_LEFT_GUI,
	INPUT_KEY_RIGHT_CTRL,
	INPUT_KEY_RIGHT_SHIFT,
	INPUT_KEY_RIGHT_ALT,
	INPUT_KEY_RIGHT_GUI

	/* 0x00E8 -- 0xFFFF RESERVED */
};

enum {
	INPUT_LED_NUMLOCK = 0x0001,
	INPUT_LED_CAPSLOCK,
	INPUT_LED_SCROLLLOCK
};

enum {
	INPUT_BUTTON_1 = 0x0001,
};

enum {
	INPUT_CONS_SCAN_NEXT_TRACK = 0x00B5,
	INPUT_CONS_SCAN_PREVIOUS_TRACK,
	INPUT_CONS_STOP,

	INPUT_CONS_PLAY_PAUSE = 0x00CD,

	INPUT_CONS_MUTE = 0x00E2,

	INPUT_CONS_VOLUME_UP = 0x00E9,
	INPUT_CONS_VOLUME_DOWN,

	INPUT_CONS_AL_MEDIA_SELECT = 0x0183,

	INPUT_CONS_AL_EMAIL_READER = 0x018A,

	INPUT_CONS_AL_CALCULATOR = 0x0192,

	INPUT_CONS_AL_LOCAL_BROWSER = 0x0194,

	INPUT_CONS_AC_SEARCH = 0x0221,
	INPUT_CONS_AC_GO_TO,
	INPUT_CONS_AC_HOME,
	INPUT_CONS_AC_BACK,
	INPUT_CONS_AC_FORWARD,
	INPUT_CONS_AC_STOP,
	INPUT_CONS_AC_REFRESH,

	INPUT_CONS_AC_BOOKMARKS = 0x022A
};

#endif
