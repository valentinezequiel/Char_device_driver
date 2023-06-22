#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/ktime.h>



/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Johannes 4 GNU/Linux and Ensambladores");
MODULE_DESCRIPTION("Gpio driver, for 'Sist de computacion' exercise");

/* Variables for device and device class */
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "sensor_Driver"
#define DRIVER_CLASS "MyModuleClass"
#define DT_ROTARY 4
#define CLK_ROTARY 17
#define WATERSENS1 24
#define WATERSENS0 23

/* Por defecto */
int sensor_Selection = 0;
/* Variables para el control del rotary */
int estadoActual, estadoAntes;
int aux = 0;

/**
 * @brief Leer los datos de un buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	/* Variable que se aloja lo que se pasa al espacio de usuario */
	char tmp = '0';

	/* Obtengo el tamaño de lo que tengo que copiar */
	to_copy = min(count, sizeof(tmp));

	/* Lee el valor del puerto */
	//printk("Valor del puerto %d: %d\n", port_Selection, gpio_get_value(port_Selection));
	//aux = 504;
	//sprintf(tmp, "%d\n", aux);
	//tmp[0] = 1 + '0';
	/*Señal del rotary*/
	/* Envia un 0 si el rotary no se esta moviendo, un 1 a la derecha, un 2 a la izquierda */
	if(sensor_Selection == 0){
		estadoActual = gpio_get_value(CLK_ROTARY);
		if(estadoActual != estadoAntes && estadoActual == 1){
			if(gpio_get_value(DT_ROTARY) != estadoActual){
				tmp = 1 + '0';
				printk("Moviendo rotary derecha\n");
			}
			else{
				tmp = 2 + '0';
				printk("Moviendo rotary izquierda\n");
			}
		}
		estadoAntes = estadoActual;
	}
	/*Sensor de agua*/
	else if(sensor_Selection == 1){
		aux = 0;
		if(gpio_get_value(WATERSENS0)){
			aux = 1;
		}

		if(gpio_get_value(WATERSENS1)){
			aux += 2;
		}
		tmp = aux + '0';
    }

	/* Devolver al usuario el valor del puerto */
	not_copied = copy_to_user(user_buffer, &tmp, 1);

	delta = to_copy - not_copied;
	return delta;
}

/**
 * @brief Escribir los datos en un buffer
 */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	char value;

	/* Obtengo el tamaño de lo que tengo que copiar */
	to_copy = min(count, sizeof(value));

	/* Copy data to user */
	not_copied = copy_from_user(&value, user_buffer, to_copy);

	/* Configurar el sensor a utilizar */
	switch(value) {
		case '0':
			sensor_Selection = 0;
			printk("Sensor %d seleccionado, Rotary\n",sensor_Selection);
			break;
		case '1':
			sensor_Selection = 1;
			printk("Sensor %d seleccionado, Water sensor\n",sensor_Selection);
			break;
		default:
			break;
	}

	delta = to_copy - not_copied;
	return delta;
}

/**
 * @brief Esta funcion entra cuendo el device file es abierto
 */
static int driver_open(struct inode *device_file, struct file *instance) {
	printk("dev_nr - abriendo puertos!\n");
	return 0;
}

/**
 * @brief Esta funcion entra cuendo el device file es cerrado
 */
static int driver_close(struct inode *device_file, struct file *instance) {
	printk("dev_nr - cerrando puertos!\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
	.write = driver_write
};

/**
 * @brief Esta funcion es llamada, cuando el modulo es cargado en el kernel
 */
static int __init ModuleInit(void) {
	printk("Iniciando driver ... \n");

	/* Allocate a device nr */
	if( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
		printk("Device Nr. could not be allocated!\n");
		return -1;
	}
	printk("Trabajo Practico N5. Device: %d, Minor: %d regisrado!\n", my_device_nr >> 20, my_device_nr && 0xfffff);

	/* Create device class */
	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("Device class can not be created!\n");
		goto ClassError;
	}

	/* create device file */
	if(device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
		printk("Can not create device file!\n");
		goto FileError;
	}

	/* Initialize device file */
	cdev_init(&my_device, &fops);

	/* Regisering device to kernel */
	if(cdev_add(&my_device, my_device_nr, 1) == -1) {
		printk("Registering of device to kernel failed!\n");
		goto AddError;
	}

	/* GPIO 4 init */
	if(gpio_request(4, "rpi-gpio-4")) {
		printk("Can not allocate GPIO 4\n");
		goto AddError;
	}

	/* Set GPIO 4 direction */
	if(gpio_direction_input(4)) {
		printk("Can not set GPIO 4 to input!\n");
		goto Gpio4Error;
	}

	/* GPIO 17 init */
	if(gpio_request(17, "rpi-gpio-17")) {
		printk("Can not allocate GPIO 17\n");
		goto Gpio4Error;
	}

	/* Set GPIO 17 direction */
	if(gpio_direction_input(17)) {
		printk("Can not set GPIO 17 to input!\n");
		goto Gpio17Error;
	}

	/* GPIO 24 init */
	if(gpio_request(24, "rpi-gpio-24")) {
		printk("Can not allocate GPIO 24\n");
		goto AddError;
	}

	/* Set GPIO 24 direction */
	if(gpio_direction_input(24)) {
		printk("Can not set GPIO 24 to output!\n");
		goto Gpio24Error;
	}

	/* GPIO 23 init */
	if(gpio_request(23, "rpi-gpio-23")) {
		printk("Can not allocate GPIO 23\n");
		goto AddError;
	}

	/* Set GPIO 23 direction */
	if(gpio_direction_input(23)) {
		printk("Can not set GPIO 23 to input!\n");
		goto Gpio23Error;
	}


	return 0;
Gpio17Error:
	gpio_free(17);
Gpio4Error:
	gpio_free(4);
Gpio24Error:
	gpio_free(24);
Gpio23Error:
	gpio_free(23);
AddError:
	device_destroy(my_class, my_device_nr);
FileError:
	class_destroy(my_class);
ClassError:
	unregister_chrdev_region(my_device_nr, 1);
	return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
	gpio_free(17);
	gpio_free(4);
	gpio_free(24);
	gpio_free(23);
	cdev_del(&my_device);
	device_destroy(my_class, my_device_nr);
	class_destroy(my_class);
	unregister_chrdev_region(my_device_nr, 1);
	printk("Cerrando el Driver ...\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);

