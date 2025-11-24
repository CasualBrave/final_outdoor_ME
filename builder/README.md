# CG 2025 Template

To build, execute the following command:
```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To run, execute the following command:
```bash
# Mac/Linux
./build/CG2025Template

# Windows
./build/Debug/CG2025Template.exe
```
Make sure you are running from the project root directory