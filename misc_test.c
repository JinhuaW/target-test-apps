#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/misc_test_ioctl.h>
#include <sys/mman.h>

pci_port_t map_ents[3] = {
	{
		1,
		1,
		0,
		3
	},
	{
		0,
		0,
		1,
		2
	},
	{
		3,
		3,
		3,
		1
	}
};

pci_array_t pci_array = {3, 0};




int main(int argc, char **argv)
{
	int fd, ret, slot, bay;
	switch_port_t port;
	char *buff;
	if (argc < 4) {
		printf("usage: %s slot bay\n", argv[0]);
		return -1;
	}
	sscanf(argv[1], "%d", &slot);
	sscanf(argv[2], "%d", &bay);
	printf("slot = %d, bay = %d\n", slot, bay);
	port.slot = slot;
	port.bay = bay;
	fd = open("/dev/misc_test", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	printf("open /dev/misc_test ok!\n");
	pci_array.ents =(unsigned long) &map_ents[0];
	ret = ioctl(fd, MISC_TEST_SET_PCIE_MAPPING, &pci_array);
	if (ret && errno != EPERM) {
		perror("set pcie mapping");
		return -1;
	}
	printf("set pcie mapping ok\n");
	ret = ioctl(fd, MISC_TEST_SET_SLOT, &port);
	if (ret) {
		perror("set slot");
		return -1;
	}
	printf("set slot ok\n");
	buff = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ((void *)buff == MAP_FAILED) {
		perror("mapping failed");
		return -1;
	}
	printf("mmap ok\n");
	if (strcmp(argv[3], "read") == 0) {
		printf("%s\n", buff);
	} else if (argv[4]) {
		strncpy(buff,argv[4], 1024);
	}
	munmap(buff, 1024);
	close(fd);
	return 0;
}
