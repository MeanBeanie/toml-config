#ifndef CONFIG_H
#define CONFIG_H
#include <stddef.h>

enum ValueType {
	VALUE_INT,
	VALUE_DOUBLE,
	VALUE_STRING,
};

typedef struct {
	enum ValueType type;
	union {
		int integer;
		double decimal;
		char* string;
	} as;
	char name[64];
	char section[64];
} Value;


typedef struct {
	Value* values;
	size_t size;
	size_t capacity;
	int error;
} Config;

extern Config __config;

void config_load(const char* path);
Value config_section_get(const char* section, const char* name);
void config_close();

#endif // CONFIG_H

#ifdef CONFIG_IMPLEMENTATION
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

Config __config;

void config_load(const char* path){
	FILE* file = fopen(path, "r");
	__config = (Config){.values = NULL, .size = 0, .capacity = 0, .error = 0};
	if(file == NULL){
		perror("CONFIG ERROR: config_load: failed to open file");
		__config.error = 1;
		return;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char buffer[size];
	fread(buffer, sizeof(char), size, file);
	fclose(file);

	Config res = {
		.size = 0,
		.capacity = 8,
		.values = malloc(8*sizeof(Value))
	};

	bool inWord = false;
	int wordSize = 0;
	char currentSection[64] = {0};
	char currentName[64] = {0};
	int stage = 0;
	for(size_t i = 0; i < size; i++){
		char c = buffer[i];
		if(inWord){
			if(c == ']'){
				memset(currentSection, 0, 64);
				strncpy(currentSection, buffer+i-wordSize, wordSize);
				inWord = false;
				continue;
			}
			else if(stage == 0 && c == ' '){
				memset(currentName, 0, 64);
				strncpy(currentName, buffer+i-wordSize-1, wordSize+1);
				inWord = false;
				stage++;
				continue;
			}
			else if(c == '\n'){
				Value value = {0};
				strncpy(value.name, currentName, 64);
				strncpy(value.section, currentSection, 64);
				
				char starting = buffer[i-wordSize+1];
				if(starting == '"'){
					value.type = VALUE_STRING;
					value.as.string = malloc(sizeof(char)*(wordSize+2));
					strncpy(value.as.string, buffer+i-wordSize+2, wordSize-3);
					value.as.string[wordSize+2] = '\0';
				}
				else if(starting >= '0' && starting <= '9'){
					value.type = VALUE_INT;
					for(int j = 0; j < wordSize+1; j++){
						if(buffer[i-wordSize-1+j] == '.'){
							value.type = VALUE_DOUBLE;
						}
					}

					if(value.type == VALUE_INT){
						int integer = 0;
						for(int j = 0; j < wordSize-1; j++){
							int p10 = wordSize-j-2;
							int shift = 1;
							for(int k = 0; k < p10; k++){
								shift *= 10;
							}
							integer += ((buffer[i-wordSize+1+j] - '0') * shift);
						}
						value.as.integer = integer;
					}
					else{
						int lhs = 0;
						int rhs = 0;
						int index = 0;
						for(int j = 0; j < wordSize-1; j++){
							if(buffer[i-wordSize+1+j] == '.'){
								index = j;
							}
						}
						int integer = 0;
						for(int j = 0; j < index; j++){
							int p10 = index-j-1;
							int shift = 1;
							for(int k = 0; k < p10; k++){
								shift *= 10;
							}
							integer += ((buffer[i-wordSize+1+j] - '0') * shift);
						}
						int decimal = 0;
						for(int j = index+1; j < wordSize-1; j++){
							int p10 = wordSize-j-2;
							int shift = 1;
							for(int k = 0; k < p10; k++){
								shift *= 10;
							}
							decimal += ((buffer[i-wordSize+1+j] - '0') * shift);
						}
						int shift = 1;
						for(int j = 0; j < (wordSize-2-index); j++){
							shift *= 10;
						}
						value.as.decimal = integer + (double)decimal/shift;
					}
				}

				if(res.size >= res.capacity){
					res.capacity *= 2;
					res.values = realloc(res.values, res.capacity*sizeof(Value));
					if(res.values == NULL){
						fprintf(stderr, "Ran out of memory for the config values, there are too many\n");
						__config.error = 1;
						return;
					}
				}
				res.values[res.size] = value;
				res.size++;

				inWord = false;
				stage = 0;
				continue;
			}
			wordSize++;
			continue;
		}
		if(c >= 0x21 && c <= 0x7e){
			wordSize = 0;
			inWord = true;
		}
	}

	__config = res;
}

Value config_section_get(const char* section, const char* name){
	Value res = {0};
	if(__config.error != 0){
		fprintf(stderr, "CONFIG ERROR: config_section_get: Cannot get value from errored config\n");
		return res;
	}

	for(int i = 0; i < __config.size; i++){
		if(strncmp(__config.values[i].section, section, strlen(section)) == 0
		&& strncmp(__config.values[i].name, name, strlen(name)) == 0){
			res = __config.values[i];
		}
	}

	return res;
}

void config_close(){
	if(__config.error != 0){
		fprintf(stderr, "CONFIG ERROR: config_close: Closing an errored config may lead to errors, be warned\n");
	}
	for(int i = 0; i < (int)__config.size; i++){
		if(__config.values[i].type == VALUE_STRING){
			free(__config.values[i].as.string);
		}
	}
	free(__config.values);
	__config.values = NULL;
}

#endif // CONFIG_IMPLEMENTATION
