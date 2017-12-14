
TARGETS=disable_interrupt

BUILDROOT_OUTPUT=$(BUILDROOT)/output
CC := $(TARGET_CROSS)gcc
AS := $(TARGET_CROSS)as
DESTDIR := $(BUILDROOT_OUTPUT)/target/usr/bin/test_apps
STAGING_DIR ?= $(BUILDROOT_OUTPUT)/staging

override CFLAGS += -Wall -Werror -g

all: build install

build: $(TARGETS)

clean:
	rm -rf $(TARGETS)

install:
	mkdir -p $(DESTDIR)
	cp --no-dereference $(TARGETS) $(DESTDIR)/
