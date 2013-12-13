#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "spidev.h"

#define LCD_WIDTH	128
#define LCD_HEIGHT	128

#define ST7735_NOP      (0x0)
#define ST7735_SWRESET  (0x01)
#define ST7735_SLPIN    (0x10)
#define ST7735_SLPOUT   (0x11)
#define ST7735_PTLON    (0x12)
#define ST7735_NORON    (0x13)
#define ST7735_INVOFF   (0x20)
#define ST7735_INVON    (0x21)
#define ST7735_DISPON   (0x29)
#define ST7735_CASET    (0x2A)
#define ST7735_RASET    (0x2B)
#define ST7735_RAMWR    (0x2C)
#define ST7735_COLMOD   (0x3A)
#define ST7735_MADCTL   (0x36)
#define ST7735_FRMCTR1  (0xB1)
#define ST7735_INVCTR   (0xB4)
#define ST7735_DISSET5  (0xB6)
#define ST7735_PWCTR1   (0xC0)
#define ST7735_PWCTR2   (0xC1)
#define ST7735_PWCTR3   (0xC2)
#define ST7735_VMCTR1   (0xC5)
#define ST7735_PWCTR6   (0xFC)
#define ST7735_GMCTRP1  (0xE0)
#define ST7735_GMCTRN1  (0xE1)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

struct st7735_ctrl{
	int spidev_fd;
	int rs_fd;
};

static struct st7735_ctrl lcdctrl;

static int init_lcdctrl(char *spidev, char *gpio)
{
	int fd;
	uint8_t data;
	uint32_t speed=20000000;

	fd = open(spidev, O_RDWR);
	if (fd < 0){
		printf("can't open spi device %s\n", spidev);
		return -1;
	}

	data = SPI_MODE_0;
	if (ioctl(fd, SPI_IOC_WR_MODE, &data) == -1){
		printf("can't set spi mode\n");
		return -1;
	}

	data = 8;
	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &data) == -1){
		printf("can't set bits per word\n");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1){
		printf("can't set max speed\n");
		return -1;
	}

	lcdctrl.spidev_fd = fd;
	fd = open(gpio, O_WRONLY);
	if (fd < 0){
		printf("can't open gpio device %s\n", gpio);
		close(lcdctrl.spidev_fd);
		lcdctrl.spidev_fd=0;
		return -1;
	}

	lcdctrl.rs_fd = fd;

	return 0;
}

static void st7735WriteByte(int iscmd, uint8_t data)
{
	int ret;
	uint8_t d = iscmd?'0':'1', d1;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)&data,
		.rx_buf = 0,
		.len = 1,
	};

	read(lcdctrl.rs_fd, &d1, 1);
	if(d1 != d){
		write(lcdctrl.rs_fd, &d, 1);
	}

	ret = ioctl(lcdctrl.spidev_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		printf("can't send spi message\n");
}

static void st7735WriteDataSerial(void* buf, int size)
{
	int ret;
	uint8_t d;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)buf,
		.rx_buf = 0,
		.len = size,
	};

	read(lcdctrl.rs_fd, &d, 1);
	if(d != '1'){
		d = '1';
		write(lcdctrl.rs_fd, &d, 1);
	}

	ret = ioctl(lcdctrl.spidev_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		printf("can't send spi message ret=%d\n", ret);
}


static void st7735WriteCmd(uint8_t command)
{
	st7735WriteByte(1, command);
}

static void st7735WriteData(uint8_t data)
{
	st7735WriteByte(0, data);
}

static void st7735InitDisplay(void)
{
	st7735WriteCmd(ST7735_SWRESET); // software reset
	usleep(50);

	st7735WriteCmd(ST7735_SLPOUT);  // out of sleep mode
	usleep(500);

	st7735WriteCmd(ST7735_COLMOD);  // set color mode
	st7735WriteData(0x05);          // 16-bit color
	usleep(10);

/*	st7735WriteCmd(ST7735_FRMCTR1); // frame rate control
	st7735WriteData(0x00);          // fastest refresh
	st7735WriteData(0x06);          // 6 lines front porch
	st7735WriteData(0x03);          // 3 lines backporch
	usleep(10);

	st7735WriteCmd(ST7735_MADCTL);  // memory access control (directions)
	st7735WriteData(0xC8);          // row address/col address, bottom to top refresh

	st7735WriteCmd(ST7735_DISSET5); // display settings #5
	st7735WriteData(0x15);          // 1 clock cycle nonoverlap, 2 cycle gate rise, 3 cycle oscil. equalize
	st7735WriteData(0x02);          // fix on VTL

	st7735WriteCmd(ST7735_INVCTR);  // display inversion control
	st7735WriteData(0x0);           // line inversion

	st7735WriteCmd(ST7735_PWCTR1);  // power control
	st7735WriteData(0x02);          // GVDD = 4.7V 
	st7735WriteData(0x70);          // 1.0uA
	usleep(10);
	st7735WriteCmd(ST7735_PWCTR2);  // power control
	st7735WriteData(0x05);          // VGH = 14.7V, VGL = -7.35V 
	st7735WriteCmd(ST7735_PWCTR3);  // power control
	st7735WriteData(0x01);          // Opamp current small 
	st7735WriteData(0x02);          // Boost frequency


	st7735WriteCmd(ST7735_VMCTR1);  // power control
	st7735WriteData(0x3C);          // VCOMH = 4V
	st7735WriteData(0x38);          // VCOML = -1.1V
	usleep(10);

	st7735WriteCmd(ST7735_PWCTR6);  // power control
	st7735WriteData(0x11); 
	st7735WriteData(0x15);

	st7735WriteCmd(ST7735_GMCTRP1);
	st7735WriteData(0x09);
	st7735WriteData(0x16);
	st7735WriteData(0x09);
	st7735WriteData(0x20);
	st7735WriteData(0x21);
	st7735WriteData(0x1B);
	st7735WriteData(0x13);
	st7735WriteData(0x19);
	st7735WriteData(0x17);
	st7735WriteData(0x15);
	st7735WriteData(0x1E);
	st7735WriteData(0x2B);
	st7735WriteData(0x04);
	st7735WriteData(0x05);
	st7735WriteData(0x02);
	st7735WriteData(0x0E);
	st7735WriteCmd(ST7735_GMCTRN1);
	st7735WriteData(0x0B); 
	st7735WriteData(0x14); 
	st7735WriteData(0x08); 
	st7735WriteData(0x1E); 
	st7735WriteData(0x22); 
	st7735WriteData(0x1D); 
	st7735WriteData(0x18); 
	st7735WriteData(0x1E); 
	st7735WriteData(0x1B); 
	st7735WriteData(0x1A); 
	st7735WriteData(0x24); 
	st7735WriteData(0x2B); 
	st7735WriteData(0x06); 
	st7735WriteData(0x06); 
	st7735WriteData(0x02); 
	st7735WriteData(0x0F); 
	usleep(10);
*/
	st7735WriteCmd(ST7735_CASET);   // column addr set
	st7735WriteData(0x00);
	st7735WriteData(0x00);          // XSTART = 0
	st7735WriteData(0x00);
	st7735WriteData(127);          // XEND = 127

	st7735WriteCmd(ST7735_RASET);   // row addr set
	st7735WriteData(0x00);
	st7735WriteData(0x00);          // XSTART = 0
	st7735WriteData(0x00);
	st7735WriteData(127);          // XEND = 127

	st7735WriteCmd(ST7735_NORON);   // normal display on
	usleep(10);

	st7735WriteCmd(ST7735_DISPON);
	usleep(500);
}

void st7735SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	st7735WriteCmd(ST7735_CASET);   // column addr set
	st7735WriteData(0x00);
	st7735WriteData(x0+2);          // XSTART 
	st7735WriteData(0x00);
	st7735WriteData(x1+2);          // XEND

	st7735WriteCmd(ST7735_RASET);   // row addr set
	st7735WriteData(0x00);
	st7735WriteData(y0+1);          // YSTART
	st7735WriteData(0x00);
	st7735WriteData(y1+1);          // YEND
}

static uint16_t frame[LCD_WIDTH*LCD_HEIGHT];

static void lcd_update(void)
{
	st7735SetAddrWindow(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);
	st7735WriteCmd(ST7735_RAMWR);  // write to RAM

	st7735WriteDataSerial(frame, sizeof(frame));
	st7735WriteCmd(ST7735_NOP);
}

void lcdFillRGB(uint16_t color)
{
	int i;

	for(i=0;i<ARRAY_SIZE(frame);i++)
		frame[i]=(color&0xff)<<8|(color>>8);

	lcd_update();
}

int main(int argc, char **argv)
{
	if(init_lcdctrl("/dev/spidev0.0", "/sys/class/gpio/gpio2_pi13/value")<0)
		return -1;

	st7735InitDisplay();

	lcdFillRGB(1<<10);
	
	return 0;
}

