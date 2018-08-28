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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <strings.h>
#include <libcli.h>
#include <inttypes.h>

#define CLITEST_PORT                8000
#define MODE_CONFIG_INT             10
#define BAR_NUM                     6
#define MAX_MAP_SIZE                1024

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif
int regular_count = 0;
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


typedef struct port_ctx {
	int slot;
	int bay;
	int fd;
	int bar;
	void *addr[6];
}port_ctx_t;
/*
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

*/

int idle_timeout(struct cli_def *cli)
{
    cli_print(cli, "Custom idle timeout");
    return CLI_QUIT;
}

int regular_callback(struct cli_def *cli)
{
    regular_count++;
    return CLI_OK;
}

int check_enable(const char *password)
{
    return !strcasecmp(password, "topsecret");
}

int cmd_show_port(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (myctx->fd > 0 && myctx->slot > 0) {
		cli_print(cli, "slot = %d, bay = %d", myctx->slot, myctx->bay);
		return CLI_OK;
	}
	cli_print(cli, "slot info is not set");
	return CLI_ERROR;
}

int cmd_show_valid(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int i = 0;
	for (i = 0; i < sizeof(map_ents)/sizeof(pci_port_t); i++)
		cli_print(cli, "%d: slot = %d, bay = %d", i, (int)(map_ents[i].slot), (int)(map_ents[i].bay));
	return CLI_OK;
}

int cmd_show_addr(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int i;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (myctx->fd > 0) {
		for (i = 0; i < BAR_NUM; i++)
			cli_print(cli, "BAR%d: %p", i, myctx->addr[i]);
		return CLI_OK;
	}
	cli_print(cli, "not init");
	return CLI_ERROR;
}

int cmd_set_mmap(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int bar;
	void *buff;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (argc < 1 || strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "mmap bar_id (0 - %d)", BAR_NUM -1);
		return CLI_OK;
	}
	if (myctx->fd < 0) {
		cli_print(cli, "please init first");
		return CLI_ERROR;
	}
	if (myctx->slot < 0 || myctx->bay < 0) {
		cli_print(cli, "please set port first");
		return CLI_ERROR;
	}
	sscanf(argv[0], "%d", &bar);
	if (bar < 0 || bar >= BAR_NUM) {
		cli_print(cli, "invalid bar");
		return CLI_ERROR;
	}
	myctx->bar = 0;
	//TBD, set bar (currently not implemented in the kernel)
	if (myctx->addr[myctx->bar]) {
		cli_print(cli, "this bar has already mmaped");
		return CLI_ERROR;
	}
	buff = mmap(NULL, MAX_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, myctx->fd, 0);
	if ((void *)buff == MAP_FAILED) {
		cli_print(cli, "mmap failed");
		return CLI_ERROR;
	}
	myctx->addr[myctx->bar] = buff;
	return CLI_OK;
}

int cmd_set_port(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int i, slot, bay, ret;
	switch_port_t port;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (argc < 2 || strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "set port slot bay");
		return CLI_OK;
	}
	if (myctx->fd < 0) {
		cli_print(cli, "please init first");
		return CLI_ERROR;
	}
	if (myctx->slot >= 0 ){
		cli_print(cli, "port already set");
		return CLI_ERROR;
	}
	sscanf(argv[0], "%d", &slot);
	sscanf(argv[1], "%d", &bay);
	for (i = 0; i < sizeof(map_ents)/sizeof(pci_port_t); i++) {
		if (slot == map_ents[i].slot && bay == map_ents[i].bay) {
			port.slot = slot;
			port.bay = bay;
			ret = ioctl(myctx->fd, MISC_TEST_SET_SLOT, &port);
			if (ret) {
				cli_print(cli, "set port failed");
				return CLI_ERROR;
			}
			myctx->slot = slot;
			myctx->bay = bay;
			break;
		}
	}
	if (myctx->slot < 0) {
		cli_print(cli, "this port is not supported");
		return CLI_ERROR;
	}
	return CLI_OK;
}

int cmd_init(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int fd, ret;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	fd = open("/dev/misc_test", O_RDWR);
	if (fd < 0) {
		cli_print(cli, "failed to open /dev/misc_test");
		return CLI_ERROR;
	}
	pci_array.ents =(unsigned long) &map_ents[0];
	ret = ioctl(fd, MISC_TEST_SET_PCIE_MAPPING, &pci_array);
	if (ret && errno != EPERM) {
		cli_print(cli, "set pcie mapping failed");
		return CLI_ERROR;
	}
	myctx->fd = fd;
	return CLI_OK;
}

int cmd_read(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int bar, offset;
	char *buff;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (argc < 2 || strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "read bar offset");
		return CLI_OK;
	}
	sscanf(argv[0], "%d", &bar);
	if (bar < 0 || bar >= BAR_NUM) {
		cli_print(cli, "invalid bar id");
		return CLI_ERROR;
	}
	sscanf(argv[1], "%d", &offset);
	if (offset < 0 || offset >= MAX_MAP_SIZE) {
		cli_print(cli, "invalid offset, offset must be in (0,%d)", MAX_MAP_SIZE);
		return CLI_ERROR;
	}
	buff = myctx->addr[bar];
	if (!buff) {
		cli_print(cli, "this bar is not available, do mmap if it exists");
		return CLI_ERROR;
	}
	cli_print(cli, "%p: 0x%x", buff + offset, *(buff + offset));
	return CLI_OK;
}


int cmd_write(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
	int bar, offset;
	uint8_t value, *buff;
	port_ctx_t *myctx = (port_ctx_t *)cli_get_context(cli);
	if (argc < 3 || strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "write bar offset value(hex, e.g. 0x8a)");
		return CLI_OK;
	}
	sscanf(argv[0], "%d", &bar);
	if (bar < 0 || bar >= BAR_NUM) {
		cli_print(cli, "invalid bar id");
		return CLI_ERROR;
	}
	sscanf(argv[1], "%d", &offset);
	if (offset < 0 || offset >= MAX_MAP_SIZE) {
		cli_print(cli, "invalid offset, offset must be in (0,%d)", MAX_MAP_SIZE);
		return CLI_ERROR;
	}
	sscanf(argv[2], "%" SCNx8, &value);
	buff = myctx->addr[bar];
	if (!buff) {
		cli_print(cli, "this bar is not available, do mmap if it exists");
		return CLI_ERROR;
	}
	*(buff+offset) = value;
	return CLI_OK;
}

int main()
{
	struct cli_command *c;
	struct cli_def *cli;
	int s, x;
	struct sockaddr_in addr;
	int on = 1;
	signal(SIGCHLD, SIG_IGN);

	port_ctx_t ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.slot = -1;
	ctx.bay = -1;
	ctx.fd = -1;

	cli = cli_init();
	cli_set_banner(cli, "misc test cli");
	cli_set_hostname(cli, "misc");
	cli_telnet_protocol(cli, 1);
	cli_regular(cli, regular_callback);
	cli_regular_interval(cli, 5); // Defa0);ults to 1 second
	cli_set_idle_timeout_callback(cli, 600, idle_timeout); // 600 second idle timeout
	cli_register_command(cli, NULL, "init", cmd_init, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	cli_register_command(cli, NULL, "read", cmd_read, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	cli_register_command(cli, NULL, "write", cmd_write, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	c = cli_register_command(cli, NULL, "set", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	cli_register_command(cli, c, "port", cmd_set_port, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "set port info(bar,slot)");
	cli_register_command(cli, c, "mmap", cmd_set_mmap, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "mmap bar addresses");

	c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	cli_register_command(cli, c, "port", cmd_show_port, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show port info(bar,slot)");
	cli_register_command(cli, c, "addr", cmd_show_addr, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show bar mmap addr");
	cli_register_command(cli, c, "valid_port", cmd_show_valid, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show valid port");


	// Set user context and its command
	cli_set_context(cli, (void*)&ctx);
	//cli_set_auth_callback(cli, check_auth);
	cli_set_enable_callback(cli, check_enable);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(CLITEST_PORT);
	if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	{
		perror("bind");
		return 1;
	}

	if (listen(s, 50) < 0)
	{
		perror("listen");
		return 1;
	}

	printf("Listening on port %d\n", CLITEST_PORT);
	while ((x = accept(s, NULL, 0)))
	{
		int pid = fork();
		if (pid < 0)
		{
			perror("fork");
			return 1;
		}

		/* parent */
		if (pid > 0)
		{
			socklen_t len = sizeof(addr);
			if (getpeername(x, (struct sockaddr *) &addr, &len) >= 0)
				printf(" * accepted connection from %s\n", inet_ntoa(addr.sin_addr));

			close(x);
			continue;
		}

		/* child */
		close(s);
		cli_loop(cli, x);
		exit(0);
	}

	cli_done(cli);
	return 0;
}
