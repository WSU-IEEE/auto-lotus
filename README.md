# Auto-Lotus

Embedded code for the WSU IEEE Auto-Lotus project: a dispenser that mixes a Lotus Energy drink for you! Built for a custom ESP32-WROOM-32E-N4 with an SH1106 128x64 OLED display, and peristaltic pumps. Utilizes the [U8glib library](https://github.com/olikraus/u8g2/).

### Components

- Kamoer KPHM400-SW3B25 Perisaltic Pump
- 1.3" SH1106 I2C Monochrome OLED Display
- Bespoke PCB with Motor Drivers
- Bespoke 4-Button Interface

### Flags and Debug

In `main.cpp` there is a un/commented `#define` called `SIMULATE_INPUT`, defining this macro will enable the simulated input source instead of physical buttons. You can change the scheduled inputs using the provided array of `InputEvent`s. The first argument represents the global time (ms) at which the input event is fired and the second argument is an array of inputs to be simulated. The helper function `std::array<input::Type, N> input::button_pressed<size_t N>)(size_t index)` allows you to quickly simulate a press for a single button, by index where the leftmost button is 0 and the rightmost is 3;
```c++
constexpr std::array simulated_buttons{
    input::Event<4>{10000, input::button_pressed<4>(0)},
    input::Event<4>{15000, {input::Type::Neutral, input::Type::Neutral, input::Type::Pressed, input::Type::Pressed}},
};
```

Defined in `/include/util.hpp` are some flags and debug options.

#### Flags

- `DISABLE_POWER_SAVING` will remove the sleep timeout that turns of the screen when sitting in the Home state for an extended period of time.

#### Debug

- `DEBUG_DISPENSE_TIME` when set to an unsigned long representing time (ms) will override the calculated dispene duration.

### JSON Schema

There are three sections in the schema: `assets`, `scenes`, `toasts`.
The `assets` section is made up of key-value pairs for the name or ID of the asset and either an ordered list of path strings or a singular path string to a monochrome bitmap. The `scenes` section is defined with a scene ID and list of elements.

There are 3 basic element Types (`asset`, `text`, and `rectangle`) and the `custom` element type. Here is an example scene with all 4 types and a static and animated asset.

```json
"dispensing_scene": [
  {
    "type": "custom",
    "name": "DISPENSE"
  },
  {
    "type": "rectangle",
    "x": 22,
    "y": 48,
    "width": 84,
    "height": 14,
    "draw_color": 1
    "filled": false,
  },
  {
    "type": "asset",
    "name": "ieee_logo",
    "x": 0,
    "y": 0,
    "draw_color": 1,
    "bitmap_mode": 0
  },
  {
    "type": "text",
    "x": 16,
    "y": 2,    
    "draw_color": 1,
    "font": "u8g2_font_t0_11b_tr",
    "text": "WSU IEEE",
    "font_mode": 0
  },  
  {
    "type": "asset",
    "name": "bubble_cup",
    "x": 94,
    "y": 16,
    "draw_color": 1,
    "time": 600,
    "time": 300,
    "offset": 0
  }
]
```
This example exposes all options for each type, explanations of what the color/mode options can be found on the [U8G2 Reference](https://github.com/olikraus/u8g2/wiki/u8g2reference#setfontmode).

Custom elements do not expose options outside of C++, the name corresponds to the `#define` of a constructor in the `/include/custom.hpp` header.

The `toasts` section defines notification toasts that may be displayed to the user. It's definition has two fields: `icon` and `label`. The `icon` field matches the options of the `asset` type and `label` matches the `text` type.

### Tools

- `/tools/generate_graphics.py` will read a JSON file in the above schema and generate a C++ header file with assets represented as bytes and scenes constructed as specified.
- `/tools/to_txt.py` will convert a bitmap file into a comma seperated list of bytes, padded so that the width is aligned with the byte size (8 bits). The first two bytes in this list correspond to the width and height of the image.
- `/tools/to_bmp.py` will convert a file of raw bytes (where the first two bytes represent the width and height of the image) into a bitmap image.
- `/tools/convert.sh` will take a bitmap (or bitmaps), convert them to text and back into a bitmap to ensure they are in the proper format and will be read correctly.
- `/tools/upload.sh` is a helper that will generate the `graphics.hpp` header from the provided JSON file path then build and upload to the microcontroller using `pio`.

### Nix/NixOS

Ensure you add the udev rules to your system configuration.
```nix
services.udev.packages = with pkgs; [
    platformio-core.udev
];
```
Using `nix develop` or `direnv allow` to enter the development environment and python venv, the `pio` command corresponds to a FHS environment that will run `pio` from the `platformio-core` package. To enter the dev shell ensure you use `nix develop`, this adds the `up` alias and `upload` function.

- `upload` will execute the `/tools/upload.sh` script and accepts a single argument pointing to the filename (without extension) of the JSON file to use. Append the flag `-m` to upload and then monitor.
- `up` will call `upload` with the argument `complex` generating assets from `/resources/complex.json`, building, and uploading to the ESP32. Append the flag `-m` to upload and then monitor. 

### IDE

PlatformIO, on the `espressif32` platform uses g++ to compile the source code. The LSP in my IDE did not appreciate the way libraries were organized and was not very usable. On Zed, I changed the project settings to specify the compiler to use though you have to manually replace `$PWD` with the absolute path to the project root as Zed settings do not parse environment variables.
```json
{
  "lsp": {
    "clangd": {
      "binary": {
        "arguments": [
          "--query-driver=$PWD/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-*"
        ]
      }
    }
  }
}
```
If you run into a similar issue, you may have to find the equivalent option in your IDE. This also could be caused by a stale/missing `compile_commands.json`. Generate them with PlatformIO using the command `pio run -t compiledb`.
