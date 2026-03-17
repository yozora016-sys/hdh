BLINK_APP_VERSION = 1.0
BLINK_APP_SITE = $(BLINK_APP_PKGDIR)/src
BLINK_APP_SITE_METHOD = local

define BLINK_APP_BUILD_CMDS
    $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define BLINK_APP_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/blink $(TARGET_DIR)/usr/bin/blink
endef

$(eval $(generic-package))
