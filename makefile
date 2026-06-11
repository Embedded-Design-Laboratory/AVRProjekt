SERIAL  = COM7
ARDUINO = -F -V -c arduino -P $(SERIAL) -b 115200
USBASP  = avrdude -c usbasp
DEF     = -DF_CPU=16000000UL -D__AVR_ATmega328P__
INC     = -I.

SRC_LIB = \
screen.c \
input.c

all:
	$(MAKE) burn_target NAME=main SRC="main.c $(SRC_LIB)"

demo:
	$(MAKE) burn_target NAME=demo SRC="demo.c $(SRC_LIB)"
	
demo2:
	$(MAKE) burn_target NAME=demo2 SRC="demo2.c $(SRC_LIB)"

burn_target:
	@echo "Building $(NAME)..."
	avr-gcc -Os $(DEF) -mmcu=atmega328p $(SRC) $(INC) -o $(NAME).elf
	avr-objcopy -j .text -j .data -O ihex $(NAME).elf $(NAME).hex
	@echo "Burning $(NAME) to Arduino..."
	avrdude $(ARDUINO) -p ATMEGA328P -U flash:w:$(NAME).hex:i
