# MicroMetro
MicroMetro is a small infastructure simulation.  The current scope is a metro train network, buty may be extended to include other types of transportation.

## Controls
| Action | Input |
| :--- | :--- |
| Place Station | `Right Mouse` |
| Draw Rail line | `Left Mouse` |
| Switch Rail | `Num 1-4` |

## Dependencies
- **CMake** (3.23 or newer)
- **[vcpkg](https://github.com/libsdl-org/SDL)**
- **C++ Compiler** with C++23 support


## Installation

1. **Clone repository**
```bash
git clone https://github.com/Matthew2025-byte/MicroMetro.git
cd MicroMetro
```

2. **Configure vcpkg**
Create `CMakeUserPresets.json` file in the project directory and update `VCPKG_ROOT` to match your vcpkg installation
```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "default",
            "inherits": "vcpkg",
            "environment": {
                "VCPKG_ROOT": "C:/vcpkg"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "Release",
            "configurePreset": "default",
            "configuration": "Release"
        }
    ]
}
```

3. **Configure and build**
```bash
cmake --preset default
cmake --build --preset Release
```