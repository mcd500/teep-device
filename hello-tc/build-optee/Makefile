ifeq ($(TAREF_DIR),)
$(error "Error: Environment variable TAREF_DIR must be set to ta-ref directory")
endif


TUTORIAL_TA_UUID ?= 8d82573a-926d-4754-9353-32dc29997f74
export TUTORIAL_TA_UUID

CROSS_COMPILE = aarch64-linux-gnu-
export CROSS_COMPILE

.PHONY: build app enclave image run

build: app enclave

app:
	make -f app.mk

enclave:
	make -f enclave.mk

prefix ?= $(OPTEE_DIR)/out-br/images

install_qemu:
	test -n "$(prefix)"
	sudo rm -rf $(prefix)/rootfs
	mkdir -p $(prefix)/rootfs
	cd $(prefix)/rootfs; \
	gzip -dc ../rootfs.cpio.gz | sudo cpio -i
	sudo install $(TUTORIAL_TA_UUID).ta $(prefix)/rootfs/lib/optee_armtz
	sudo install App-optee $(prefix)/rootfs/root
	cd $(prefix)/rootfs; \
	sudo find . | sudo cpio -o -H newc 2> /dev/null | gzip -c9 > ../rootfs.cpio.gz

run-qemu:
	ln -sf $(prefix)/rootfs.cpio.gz $(OPTEE_DIR)/out/bin
	cd $(OPTEE_DIR)/out/bin && \
		$(OPTEE_DIR)/qemu/aarch64-softmmu/qemu-system-aarch64 \
			-nographic \
			-serial mon:stdio -serial file:serial1.log \
			-smp 2 \
			-machine virt,secure=on -cpu cortex-a57 \
			-d unimp -semihosting-config enable,target=native \
			-m 1057 \
			-bios bl1.bin \
			-initrd rootfs.cpio.gz \
			-kernel Image -no-acpi \
			-append 'console=ttyAMA0,38400 keep_bootcon root=/dev/vda2' \
			$(QEMU_EXTRA_ARGS)

clean:
	rm -f *.dmp *.elf *.map *.ta *.o ta.lds App-optee
