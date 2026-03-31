obj-m := basic_driver.o

# Khai bao cung cac bien moi truong
KDIR := $(HOME)/embedded-linux/buildroot/buildroot/output/build/linux-6.6.121
ARCH := arm
CROSS_COMPILE := arm-linux-

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean
