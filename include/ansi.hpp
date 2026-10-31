#pragma once

#define ESC "\x1B"
#define CSI ESC "["

// foreground
#define FG_BLACK   CSI "30m"
#define FG_RED     CSI "31m"
#define FG_GREEN   CSI "32m"
#define FG_YELLOW  CSI "33m"
#define FG_BLUE    CSI "34m"
#define FG_MAGENTA CSI "35m"
#define FG_CYAN    CSI "36m"
#define FG_WHITE   CSI "37m"

// background
#define BG_BLACK   CSI "40m"
#define BG_RED     CSI "41m"
#define BG_GREEN   CSI "42m"
#define BG_YELLOW  CSI "43m"
#define BG_BLUE    CSI "44m"
#define BG_MAGENTA CSI "45m"
#define BG_CYAN    CSI "46m"
#define BG_WHITE   CSI "47m"

// formatting
#define ANSI_RESET  CSI "0m"
#define ANSI_INVERT CSI "7m"
#define ANSI_NORMAL CSI "27m"
