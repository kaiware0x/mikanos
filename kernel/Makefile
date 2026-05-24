TARGET = kernel.elf
OBJS = main.o

CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
			-fno-exceptions -fno-rtti -std=c++17
LDFLAGS	 += --entry KernelMain -z norelro --image-base 0x100000 --static

# `make` 単体実行のときは一番上のターゲットが実行される
# .PHONY = ダミーターゲット
.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

# `$<` =c 必須項目の先頭1つ
%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<
