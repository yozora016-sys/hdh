GPIO_DRIVER_VERSION = 1.0
GPIO_DRIVER_SITE = $(GPIO_DRIVER_PKGDIR)/src
GPIO_DRIVER_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
