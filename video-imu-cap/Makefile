CC ?= arm-none-linux-gnueabi-gcc
LINUX_DIR ?= ../../linux-sunxi
CFLAGS := -I$(LINUX_DIR)/include -static 
TARGET := vicap
EXTRA_LIBS += -lpthread
SRC = main.c imu.c video.c

.PHONY: all clean

all: $(TARGET)

vicap: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(EXTRA_LIBS)

clean:
	rm -rf $(TARGET)
