# drivers/gpio_btnled/Makefile
obj-m += oled_drv.o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
