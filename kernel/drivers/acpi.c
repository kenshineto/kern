#include <lib.h>
#include <comus/drivers/acpi.h>
#include <comus/asm.h>
#include <comus/memory.h>

struct acpi_header {
	uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oem_id[6];
	uint8_t oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed));

// root system descriptor pointer
// ACPI 1.0
struct rsdp {
	uint8_t signature[8];
	uint8_t checksum;
	uint8_t oemid[6];
	uint8_t revision;
	uint32_t rsdt_addr;
} __attribute__((packed));

// eXtended system descriptor pointer
// ACPI 2.0
struct xsdp {
	char signature[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t revision;
	uint32_t rsdt_addr;
	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t extendeid_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

// root system descriptor table
// ACPI 1.0
struct rsdt {
	struct acpi_header h;
	uint32_t sdt_pointers[];
} __attribute__((packed));

// eXtended system descriptor table
// ACPI 2.0
struct xsdt {
	struct acpi_header h;
	uint64_t sdt_pointers[];
} __attribute__((packed));


// generic address structure
struct gas {
	uint8_t address_space;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
};

// differentiated system description table
struct dsdt {
	struct acpi_header h;
	char s5_addr[];
} __attribute__((packed));

struct apic {
	struct acpi_header h;
	// todo
} __attribute__((packed));

struct hept {
	struct acpi_header h;
	// todo
} __attribute__((packed));

struct waet {
	struct acpi_header h;
	// todo
} __attribute__((packed));

// fixed acpi description table
struct fadt {
	struct acpi_header h;
	uint32_t firmware_ctrl;
	uint32_t dsdt;

	// field used in ACPI 1.0; no longer in use, for compatibility only
	uint8_t  reserved;

	uint8_t  preferred_power_management_profile;
	uint16_t sci_interrupt;
	uint32_t smi_command_port;
	uint8_t  acpi_enable;
	uint8_t  acpi_disable;
	uint8_t  s4bios_req;
	uint8_t  pstate_control;
	uint32_t pm1_a_event_block;
	uint32_t pm1_b_event_block;
	uint32_t pm1_a_control_block;
	uint32_t pm1_b_control_block;
	uint32_t pm2_control_block;
	uint32_t pm_timer_block;
	uint32_t gpe0_block;
	uint32_t gpe1_block;
	uint8_t  pm1_event_length;
	uint8_t  pm1_control_length;
	uint8_t  pm2_control_length;
	uint8_t  pm_timer_length;
	uint8_t  gpe0_length;
	uint8_t  gpe1_length;
	uint8_t  gpe1_base;
	uint8_t  cstate_control;
	uint16_t worst_c2_latency;
	uint16_t worst_c3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t  duty_offset;
	uint8_t  duty_width;
	uint8_t  day_alarm;
	uint8_t  month_alarm;
	uint8_t  century;

	// reserved in ACPI 1.0; used since ACPI 2.0+
	uint16_t boot_architecture_flags;

	uint8_t  reserved_2;
	uint32_t flags;

	// 12 byte structure; see below for details
	struct gas reset_reg;

	uint8_t  reset_value;
	uint8_t  reserved_3[3];

	// 64bit pointers - Available on ACPI 2.0+
	uint64_t x_firmware_control;
	uint64_t x_dsdt;

	struct gas x_pm1_a_event_block;
	struct gas x_pm1_b_event_block;
	struct gas x_pm1_a_control_block;
	struct gas x_pm1_b_control_block;
	struct gas x_pm2_control_block;
	struct gas x_pm_timer_block;
	struct gas x_gpe0_block;
	struct gas x_gpe1_block;
} __attribute__((packed));

struct acpi_state {
	union {
		struct xsdt *xsdt;
		struct rsdt *rsdt;
	} sdt;
	struct fadt *fadt;
	struct dsdt *dsdt;
	struct apic *apic;
	struct hept *hept;
	struct waet *waet;
	uint8_t version;

	uint16_t SLP_TYPa;
	uint16_t SLP_TYPb;
	uint16_t SLP_EN;
	uint16_t SCI_EN;
};

/* global state, idk a better way rn */
static struct acpi_state state;

static bool checksum(uint8_t *data, size_t len) {
	unsigned char sum = 0;
	for (size_t i = 0; i < len; i++)
		sum += data[i];
	return sum == 0;
}

static int read_s5_addr(struct dsdt *dsdt) {
	char *s5_addr = dsdt->s5_addr;
	int dsdt_len = dsdt->h.length - sizeof(struct acpi_header);

	while (0 < dsdt_len--) {
		if (memcmp(s5_addr, "_S5_", 4) == 0)
			break;
		s5_addr++;
	}

	if (dsdt_len > 0) {
		// check for valid AML structure
		if ( ( *(s5_addr-1) == 0x08 || ( *(s5_addr-2) == 0x08 && *(s5_addr-1) == '\\') ) && *(s5_addr+4) == 0x12 ) {
			s5_addr += 5;
			s5_addr += ((*s5_addr &0xC0)>>6) +2;   // calculate PkgLength size

			if (*s5_addr == 0x0A)
				s5_addr++;   // skip byteprefix
			state.SLP_TYPa = *(s5_addr)<<10;
			s5_addr++;

			if (*s5_addr == 0x0A)
				s5_addr++;   // skip byteprefix
			state.SLP_TYPb = *(s5_addr)<<10;

			state.SLP_EN = 1<<13;
			state.SCI_EN = 1;

		} else {
			return -1;
		}
	} else {
		return -1;
	}

	return -1;
}

static void acpi_load_table(uint64_t addr);

static void acpi_load_rsdt_tables(struct rsdt *rsdt) {
	int entries = (rsdt->h.length - sizeof(rsdt->h)) / 4;
	for (int i = 0; i < entries; i++) {
		uint32_t addr = rsdt->sdt_pointers[i];
		acpi_load_table(addr);
	}
}

static void acpi_load_xsdt_tables(struct xsdt *xsdt) {
	int entries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
	for (int i = 0; i < entries; i++) {
		uint64_t addr = xsdt->sdt_pointers[i];
		acpi_load_table(addr);
	}
}

#define SIG_RSDT    0x54445352
#define SIG_XSDT    0x54445358
#define SIG_FACP    0x50434146
#define SIG_DSDT    0x54445344
#define SIG_APIC    0x43495041
#define SIG_HEPT    0x54455048
#define SIG_WAET    0x54454157

static void acpi_handle_table(struct acpi_header *header) {
	switch (header->signature) {
		case SIG_RSDT:
			state.sdt.rsdt = (struct rsdt *) header;
			acpi_load_rsdt_tables(state.sdt.rsdt);
			break;
		case SIG_XSDT:
			state.sdt.xsdt = (struct xsdt *) header;
			acpi_load_xsdt_tables(state.sdt.xsdt);
			break;
		case SIG_FACP:
			state.fadt = (struct fadt *) header;
			acpi_load_table(state.fadt->dsdt);
			break;
		case SIG_DSDT:
			state.dsdt = (struct dsdt *) header;
			read_s5_addr(state.dsdt);
			break;
		case SIG_APIC:
			state.apic = (struct apic *) header;
			break;
		case SIG_HEPT:
			state.hept = (struct hept *) header;
			break;
		case SIG_WAET:
			state.waet = (struct waet *) header;
			break;
		default:
			break;
	}
}

static void acpi_load_table(uint64_t addr) {
	struct acpi_header *temp, *mapped;
	uint32_t length;
	temp = (struct acpi_header * ) (uintptr_t) addr;
	mapped = kmapaddr(temp, sizeof(struct acpi_header));
	length = mapped->length;
	kunmapaddr(mapped);
	mapped = kmapaddr(temp, length);
	if (!checksum((uint8_t *) mapped, mapped->length)) {
		kunmapaddr(mapped);
		return;
	}
	kprintf("%.*s: %#016lx\n", 4, (char*)&mapped->signature, (size_t)temp);
	acpi_handle_table(mapped);
}

void acpi_init(void *rootsdp) {
	memset(&state, 0, sizeof(struct acpi_state));
	struct rsdp *rsdp = (struct rsdp *) rootsdp;
	if (!checksum((uint8_t *)rsdp, sizeof(struct rsdp))) {
		panic("invalid acpi rsdp checksum");
	}
	if (memcmp(rsdp->signature, "RSD PTR ", 8) != 0) {
		panic("invalid acpi rsdp signature: %.*s\n", 8, rsdp->signature);
	}
	if (rsdp->revision == 0) {
		state.version = 0;
		kprintf("ACPI 1.0\n");
		acpi_load_table(rsdp->rsdt_addr);
	} else if (rsdp->revision == 2) {
		state.version = 2;
		struct xsdp *xsdp = (struct xsdp *) rsdp;
		kprintf("ACPI 2.0\n");
		acpi_load_table(xsdp->xsdt_addr);
	} else {
		panic("invalid acpi rev: %d\n", rsdp->revision);
	}
	kprintf("\n");
	outb(state.fadt->smi_command_port, state.fadt->acpi_enable);
}

void acpi_shutdown(void) {
	outw((unsigned int) state.fadt->pm1_a_control_block, state.SLP_TYPb | state.SLP_EN);
	panic("ACPI shutdown failed");
}
