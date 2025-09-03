# MiJIT - A Simple Just-In-Time Compiler

**MiJIT** is a cross-platform Just-In-Time (JIT) compiler that demonstrates how to generate and execute machine code at runtime. It creates a personalized greeting program on the fly!

## What is MiJIT?

MiJIT is an educational JIT compiler that:
- **Generates machine code while running** - Creates assembly instructions in memory
- **Executes generated code** - Runs the machine code like a regular function
- **Works on multiple platforms** - Supports Linux, macOS, Intel, and ARM processors
- **Teaches JIT concepts** - Shows how runtime code generation works

## Supported Platforms

Linux x86-64 (Intel/AMD processors)  
macOS x86-64 (Intel Mac)  
Linux ARM64 (ARM processors like Raspberry Pi)  
Apple Silicon (M1/M2/M3 Macs)

## How to Build and Run

### Quick Start (Using xmake)

1. **Clone the repository**:
```bash
git clone https://github.com/alvarorichard/MiJIT.git
cd MiJIT
```

2. **Build the project**:
```bash
xmake build
```

3. **Run the program**:
```bash
xmake run MiJIT
```

### Alternative Build (Using g++ directly)

```bash
# Build with g++
g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o mijit

# Run the program
./mijit
```


### Platform Differences

- **Linux/macOS x86-64**: Generates full system call to write() function
- **Linux ARM64**: Generates ARM64 system call instructions  
- **Apple Silicon**: Uses simplified approach due to security restrictions



The machine code output shows the actual processor instructions that MiJIT created!


## License

This project is open source. Check the LICENSE file for details.

## Contributing

Contributions welcome! Feel free to:
- Add support for more platforms
- Improve error handling
- Add more machine code examples
- Enhance documentation

---

**MiJIT** - Making JIT compilation simple and accessible!
