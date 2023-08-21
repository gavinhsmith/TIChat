# ----------------------------
# Makefile Options
# ----------------------------

NAME = TALKTOPC
ICON = icon.png
DESCRIPTION = "Serial Relay"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
