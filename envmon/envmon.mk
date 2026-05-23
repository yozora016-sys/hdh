ENVMON_VERSION = 1.0
ENVMON_SITE    = $(TOPDIR)/package/envmon/src
ENVMON_SITE_METHOD = local

ENVMON_DEPENDENCIES = linux

# Build kernel modules
define ENVMON_BUILD_CMDS
    # Build GPIO driver
    $(MAKE) -C $(LINUX_DIR) M=$(@D)/drivers/gpio_btnled \
        ARCH=$(KERNEL_ARCH) \
        CROSS_COMPILE=$(TARGET_CROSS) modules

    # Build DHT11 driver  
    #$(MAKE) -C $(LINUX_DIR) M=$(@D)/drivers/dht11_custom \
     #   ARCH=$(KERNEL_ARCH) \
      #  CROSS_COMPILE=$(TARGET_CROSS) modules

    # Build OLED driver
    $(MAKE) -C $(LINUX_DIR) M=$(@D)/drivers/oled_ssd1306 \
        ARCH=$(KERNEL_ARCH) \
        CROSS_COMPILE=$(TARGET_CROSS) modules

    # Build userspace app
    $(TARGET_CC) -o $(@D)/envmon_app $(@D)/app/main.c
endef

define ENVMON_INSTALL_TARGET_CMDS
    # Cài kernel modules
    $(INSTALL) -D -m 0644 \
        $(@D)/drivers/gpio_btnled/gpio_btnled.ko \
        $(TARGET_DIR)/lib/modules/gpio_btnled.ko
    #$(INSTALL) -D -m 0644 \
        #$(@D)/drivers/dht11_custom/dht11_drv.ko \
       # $(TARGET_DIR)/lib/modules/dht11_drv.ko
    $(INSTALL) -D -m 0644 \
        $(@D)/drivers/oled_ssd1306/oled_drv.ko \
        $(TARGET_DIR)/lib/modules/oled_drv.ko
    # Cài app
    $(INSTALL) -D -m 0755 \
        $(@D)/envmon_app \
        $(TARGET_DIR)/usr/bin/envmon
endef

$(eval $(generic-package))
