CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra
LIB_NAME = libFilmMaster2000.a
EXEC_NAME = runme
LIB_SOURCES = FilmMaster2000.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
MAIN_SOURCES = main.c
MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)

all: $(LIB_NAME) $(EXEC_NAME)

$(LIB_NAME): $(LIB_OBJECTS)
	@echo "Archiving $@ ..."
	ar rcs $@ $(LIB_OBJECTS)
	@echo "Done creating $@"

$(EXEC_NAME): $(MAIN_OBJECTS) $(LIB_NAME)
	@echo "Linking $@ ..."
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJECTS) $(LIB_NAME)
	@echo "Done creating $@"

%.o: %.c
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -rf *.o
	rm -rf *.a
	rm -rf $(EXEC_NAME)
	rm -rf output*.bin
	# -del /f /q *.o 2>nul
	# -del /f /q *.a 2>nul
	# -del /f /q $(EXEC_NAME).exe 2>nul
	# -del /f /q output*.bin 2>nul
	@echo "Cleanup complete."

test: all

	@echo "Running tests..."

	@echo "Running reverse test..."
	./$(EXEC_NAME) test.bin output_reverse.bin reverse
	./$(EXEC_NAME) test.bin output_reverse_s.bin -S reverse
	./$(EXEC_NAME) test.bin output_reverse_m.bin -M reverse
	cmp -s output_reverse.bin output_reverse_s.bin || echo "Failed -S"
	cmp -s output_reverse.bin output_reverse_m.bin || echo "Failed -M"
	@echo "Reverse test completed."

	@echo "Running swap channel test..."
	./$(EXEC_NAME) test.bin output_swap.bin swap_channel 0,1
	./$(EXEC_NAME) test.bin output_swap_s.bin -S swap_channel 0,1
	./$(EXEC_NAME) test.bin output_swap_m.bin -M swap_channel 0,1
	cmp -s output_swap.bin output_swap_s.bin || echo "Failed -S"
	cmp -s output_swap.bin output_swap_m.bin || echo "Failed -M"
	@echo "Swap channel test completed."

	@echo "Running clip channel test..."
	./$(EXEC_NAME) test.bin output_clip.bin clip_channel 0 50,200
	./$(EXEC_NAME) test.bin output_clip_s.bin -S clip_channel 0 50,200
	./$(EXEC_NAME) test.bin output_clip_m.bin -M clip_channel 0 50,200
	cmp -s output_clip.bin output_clip_s.bin || echo "Failed -S"
	cmp -s output_clip.bin output_clip_m.bin || echo "Failed -M"
	@echo "Clip channel test completed."

	@echo "Running scale channel test..."
	./$(EXEC_NAME) test.bin output_scale.bin    scale_channel 1 1.5
	./$(EXEC_NAME) test.bin output_scale_s.bin -S scale_channel 1 1.5
	./$(EXEC_NAME) test.bin output_scale_m.bin -M scale_channel 1 1.5
	cmp -s output_scale.bin output_scale_s.bin || echo "Failed -S"
	cmp -s output_scale.bin output_scale_m.bin || echo "Failed -M"
	@echo "Scale channel test completed."

	@echo "All tests completed successfully."
.PHONY: all clean test
