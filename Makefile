
#TARGETS=disable_interrupt epoll poll select misc_test
TARGETS=misc_test misc_test_cli link_bin_to_capp

BUILDROOT_OUTPUT=$(BUILDROOT)/output
CC := $(TARGET_CROSS)gcc
AS := $(TARGET_CROSS)as
DESTDIR := $(BUILDROOT_OUTPUT)/target/usr/bin
STAGING_DIR ?= $(BUILDROOT_OUTPUT)/staging

override CFLAGS += -Wall -Werror -g


misc_test_cli: override LDFLAGS += -lcli

all: build install

build: $(TARGETS)

clean:
	rm -rf $(TARGETS)

install:
	mkdir -p $(DESTDIR)
	cp --no-dereference $(TARGETS) $(DESTDIR)/
