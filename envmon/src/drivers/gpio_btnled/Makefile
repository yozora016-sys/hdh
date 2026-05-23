# drivers/gpio_btnled/Makefile
obj-m += gpio_btnled.o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
