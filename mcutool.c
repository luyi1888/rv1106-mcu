#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define CORE_SGRF_HPMCU_BOOT_ADDR 0xff076044
#define CORECRU_CORESOFTRST_CON01 0xff3b8a04
#define CORE_GRF_CACHE_PERI_ADDR_START  0xff040024
#define CORE_GRF_CACHE_PERI_ADDR_END    0xff040028
#define CORE_GRF_MCU_CACHE_MISC         0xff04002c
#define RESET_MCU 0x1e001e
#define RELEASE_MCU 0x1e0000

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

//#define DEBUG

/*
mcu initialization sequence copy from sdk uboot code
u-boot/arch/arm/mach-rockchip/rv1106/rv1106.c
*/

unsigned long set_reg(unsigned long addr, unsigned long value) {
	int devmem;
	void *mapping, *virt_addr;	
	unsigned long read_result;
	
	devmem = open("/dev/mem", O_RDWR | O_SYNC);
	if (devmem == -1) {
		perror("Could not open /dev/mem");
		return -1;
	}

	mapping = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem, addr & ~MAP_MASK);
	if(mapping == (void *) -1) {
		return -1;
    		 
    	}
    	virt_addr = mapping + (addr & MAP_MASK);
	*((unsigned long *) virt_addr) = value;
    	//printf("Memory mapped at address %p.\n", mapping);
	read_result = *((unsigned long *) virt_addr);
#ifdef DEBUG	
	printf("Value at address 0x%08lx (%p): 0x%08lx\n", addr, virt_addr, read_result);
#endif
	
	munmap(mapping, MAP_SIZE);
	close(devmem);
	
	return read_result;

}

unsigned long parse_int(char *str) {
	long long result;
	char *endptr; 

	result = strtoll(str, &endptr, 0);
	if (str[0] == '\0' || *endptr != '\0') {
		fprintf(stderr, "\"%s\" is not a valid number\n", str);
		exit(EXIT_FAILURE);
	}

	return (unsigned long)result;
}

void mcu_reset() {
	set_reg(CORECRU_CORESOFTRST_CON01, RESET_MCU);
	printf("MCU reset.\n");
}

void mcu_release() {
	set_reg(CORECRU_CORESOFTRST_CON01, RELEASE_MCU);
	printf("MCU release.\n");
}

void mcu_load_fw(unsigned long addr, char* filename) {
	unsigned long length;

	int devmem;
	void *mapping;

	off_t map_base, extra_bytes;
	
	struct stat sb;
	FILE *fd;
#ifdef DEBUG	
	unsigned long read_result;
#endif	
	unsigned char *mcufw;
	
	fd = fopen(filename, "rb");
	if(fd == NULL)
	{
		fprintf(stderr, "Could not open %s", filename);
		goto open_fail;
	}
	
	stat(filename, &sb);
	length = sb.st_size;
	mcufw = malloc(length);
	fread(mcufw, length, 1, fd);
	fclose(fd);
	
	set_reg(CORE_GRF_CACHE_PERI_ADDR_START, 0xff000);
	set_reg(CORE_GRF_CACHE_PERI_ADDR_END, 0xffc00);
	//set_reg(CORE_GRF_MCU_CACHE_MISC, 0x00080008);
	
	mcu_reset();
	
	devmem = open("/dev/mem", O_RDWR | O_SYNC);
	if (devmem == -1) {
		perror("Could not open /dev/mem");
		goto open_fail;
	}
	map_base = addr & ~MAP_MASK;
	extra_bytes = addr - map_base;

	mapping = mmap(NULL, length + extra_bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
	               devmem, map_base);
	if (mapping == MAP_FAILED) {
		perror("Could not map memory");
		goto map_fail;
	}
	
	memcpy((void *)(mapping + extra_bytes), (void *)mcufw, length);
	printf("MCU FW load to 0x%08lx.\n", addr);
	
#ifdef DEBUG	
	read_result = *((unsigned long *) (mapping + extra_bytes));
	printf("Value at address 0x%08lx (%p): 0x%08lx\n", addr, (mapping + extra_bytes), read_result);
#endif	
	set_reg(CORE_SGRF_HPMCU_BOOT_ADDR, addr);
	printf("MCU boot addr set to 0x%08lx.\n", addr);
	
map_fail:
	close(devmem);

open_fail:
	return;
}

int main(int argc, char *argv[]) {

	char opt;
	
	if (argc < 2) {
		fprintf(stderr, "This tool built for Rockchip RV1106 MCU.\n");
		fprintf(stderr, "Usage: execute ADDR filename\n");
		fprintf(stderr, "              load fw and run\n");
		fprintf(stderr, "       load ADDR filename\n");
		fprintf(stderr, "              load fw only\n");
		fprintf(stderr, "       run\n");
		fprintf(stderr, "              release mcu\n");
		fprintf(stderr, "       stop\n");
		fprintf(stderr, "              reset mcu\n");
		exit(EXIT_FAILURE);
	}
	
	opt = argv[1][0];
	
	switch(opt)
	{
		case 'e':
			mcu_load_fw(parse_int(argv[2]), argv[3]);
			mcu_release();
			break;
		case 'l':
			mcu_load_fw(parse_int(argv[2]), argv[3]);
			break;
		case 'r':
			mcu_release();
			break;
		case 's':
			mcu_reset();
			break;
		default:
			break;
	}
	
	return EXIT_SUCCESS;


}

