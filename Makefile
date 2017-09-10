SOURCE_DIR=./src
OUTPUT_DIR=./bin

all:
	mkdir -p $(OUTPUT_DIR)
	gcc $(SOURCE_DIR)/ilkorsh.c -o $(OUTPUT_DIR)/ilkorsh

clean:
	rm -r $(OUTPUT_DIR)/*
