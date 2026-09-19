$(BUILD)/fitgen: tools/fitgen.c
	@mkdir -p $(@D)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror $< -o $@
$(BUILD)/kernel.its: $(BUILD)/kernel.bin $(BUILD)/fitgen
	$(BUILD)/fitgen $(LOAD) > $@
$(BUILD)/kernel.itb: $(BUILD)/kernel.its
	cd $(BUILD) && dtc -I dts -O dtb -o kernel.itb kernel.its
.PHONY: image
image: $(BUILD)/kernel.itb
