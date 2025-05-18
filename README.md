# toml-config

header-only library that loads a toml file and turns it into a easy to use struct to store values instead of using `#define` a bunch, plus you wont have to recompile for updates.
each table is used as a different section of constants, and lets the user use the same name for different values (see `config_section_get(<section>, <key>)`)

designed to reload the config every run, so code would prefferably run:
```c
// <includes>
#define CONFIG_IMPLEMENTATION
#include <config.h>

int main(void){
    Config config = config_load("./path/to/config.toml");
    // rest of code ...
}
```

### more info

the library uses a special `Value` type to get data back, which contains a union for the data and an enum for the datatype

there is an efficiency loss as to get anything it uses a linear search which is rather slow, but i will maybe fix that later

### example

```toml
# config.toml
[screen]
width = 400
height = 400
title = "Window Title"

[box]
width = 10
height = 10
gravity = 9.8
name = "Box"
```
```c
// main.c
#include <stdio.h>
#define CONFIG_IMPLEMENTATION
#include "config.h"

int main(void){
    Config config = config_load("config.toml");

    printf("Screen Title: %s\n", config_section_get(config, "screen", "title"));
    printf("Screen Height: %d\n", config_section_get(config, "screen", "height").as.integer);
    Value value = config_section_get(config, "box", "gravity");
    if(value.type == VALUE_DOUBLE){
        printf("Box uses %f for gravity\n", value.as.decimal);
    }
    return 0;
}
```
