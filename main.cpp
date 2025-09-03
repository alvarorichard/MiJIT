
/**
 * @file main.cpp
 * @brief Just-In-Time (JIT) compilation example that generates x86-64 machine code
 *        at runtime to print a personalized greeting message.
 * 
 * This program demonstrates:
 * - Runtime machine code generation
 * - Memory mapping with executable permissions
 * - System call invocation from generated code
 * - Modern C++ RAII memory management
 */

#include <sys/mman.h>  
#include <unistd.h>    


#include <algorithm>   // Provides std::copy() for copying data
#include <cstdint>     // Provides fixed-width integer types like uint8_t
#include <cstdlib>     // Provides EXIT_SUCCESS and EXIT_FAILURE constants
#include <iostream>    // Provides std::cout, std::cerr for input/output
#include <stdexcept>   // Provides std::runtime_error for exception handling
#include <string>      // Provides std::string for string manipulation
#include <string_view> // Provides std::string_view for efficient string viewing
#include <vector>      // Provides std::vector for dynamic arrays      

// Forward declarations of helper functions
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void;
auto show_machine_code(const std::vector<uint8_t>& machine_code) -> void;
[[nodiscard]] auto estimate_memory_size(size_t machine_code_size) noexcept
    -> size_t;

/**
 * @brief Main function that demonstrates JIT compilation and execution
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on error
 * 
 * This function:
 * 1. Gets user input for personalized greeting
 * 2. Constructs x86-64 machine code template
 * 3. Modifies the template with the actual message
 * 4. Allocates executable memory and copies the code
 * 5. Executes the generated machine code
 */
auto main() -> int
{
  // Prompt user for their name and read the input
  std::string name;                           // Storage for user's name
  std::cout << "What is your name?\n";       // Display prompt to user
  std::getline(std::cin, name);              // Read entire line including spaces

  // Create the complete greeting message that will be printed by generated code
  const auto hello_name = std::string{"Hello, "} + name + "!\n";

  // Display platform information
  std::cout << "Platform detected: ";
#if defined(__linux__) && defined(__x86_64__)
  std::cout << "Linux x86-64\n";
#elif defined(__APPLE__) && defined(__x86_64__)
  std::cout << "macOS x86-64\n";
#elif defined(__linux__) && defined(__aarch64__)
  std::cout << "Linux ARM64\n";
#elif defined(__APPLE__) && defined(__aarch64__)
  std::cout << "Apple Silicon ARM64\n";
#endif

  /**
   * Multi-platform Machine Code Template Explanation:
   * This code generates platform-specific machine code for system calls or function returns.
   * 
   * x86-64 (Linux/macOS): Generates a write() system call that prints our greeting message.
   * ARM64 Linux: Generates a write() system call using ARM64 instructions.
   * ARM64 macOS: Uses a simpler approach due to system call complexity on Apple Silicon.
   */
  std::vector<uint8_t> machine_code{
#if defined(__linux__) && defined(__x86_64__)
      // Linux x86-64: write system call number is 1
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,  // mov rax, 1        - Load write syscall number
      0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,  // mov rdi, 1        - Load file descriptor (stdout)
      0x48, 0x8d, 0x35, 0x0a, 0x00, 0x00, 0x00,  // lea rsi, [rip+10] - Load address of string data
      0x48, 0xc7, 0xc2, 0x00, 0x00, 0x00, 0x00,  // mov rdx, 0        - Load message length (filled later)
      0x0f, 0x05,                                // syscall           - Invoke system call
      0xc3                                       // ret               - Return to caller
#elif defined(__APPLE__) && defined(__x86_64__)
      // macOS x86-64: write system call number is 0x02000004
      0x48, 0xc7, 0xc0, 0x04, 0x00, 0x00, 0x02,  // mov rax, 0x02000004 - Load write syscall number for macOS
      0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,  // mov rdi, 1          - Load file descriptor (stdout)
      0x48, 0x8d, 0x35, 0x0a, 0x00, 0x00, 0x00,  // lea rsi, [rip+10]   - Load address of string data
      0x48, 0xc7, 0xc2, 0x00, 0x00, 0x00, 0x00,  // mov rdx, 0          - Load message length (filled later)
      0x0f, 0x05,                                // syscall             - Invoke system call
      0xc3                                       // ret                 - Return to caller
#elif defined(__linux__) && defined(__aarch64__)
      // Linux ARM64: write system call number is 64
      0x20, 0x00, 0x80, 0xd2,  // mov x0, #1       - Load file descriptor (stdout)  
      0x41, 0x00, 0x00, 0x10,  // adr x1, #8       - Load address of string data (8 bytes ahead)
      0x42, 0x00, 0x80, 0xd2,  // mov x2, #2       - Load message length (will be patched)
      0x08, 0x08, 0x80, 0xd2,  // mov x8, #64      - Load write syscall number for Linux ARM64
      0x01, 0x00, 0x00, 0xd4,  // svc #0           - Invoke system call
      0xc0, 0x03, 0x5f, 0xd6   // ret              - Return to caller
#elif defined(__APPLE__) && defined(__aarch64__)
      // Apple Silicon ARM64: Simple approach - just return a success value
      // System calls on Apple Silicon require special handling due to security restrictions
      0x00, 0x00, 0x80, 0xd2,  // mov x0, #0       - Return success code
      0xc0, 0x03, 0x5f, 0xd6   // ret              - Return to caller
#else
#error "Unsupported platform: This code supports Linux x86-64/ARM64 and macOS x86-64/ARM64 only"
#endif
  };

  // Update the machine code template with the actual message length
  append_message_size(machine_code, hello_name);

  // Append the actual string data after the machine code instructions
  // Note: Only x86-64 and Linux ARM64 need this, as they reference the string in their syscalls
#if !defined(__APPLE__) || !defined(__aarch64__)
  for (const auto character : hello_name) {
    machine_code.push_back(static_cast<uint8_t>(character));  // Add each character as byte
  }
#endif

  // Display the generated machine code for debugging purposes
  show_machine_code(machine_code);

  try {
    // Calculate how much memory we need (must be multiple of page size)
    const auto required_memory_size = estimate_memory_size(machine_code.size());

    // Allocate memory with read/write permissions first (safer approach)
    auto* memory = static_cast<uint8_t*>(
        mmap(nullptr,                    // Let kernel choose the address
             required_memory_size,       // Size of memory to allocate
             PROT_READ | PROT_WRITE,     // Initially only read/write permissions
             MAP_PRIVATE | MAP_ANONYMOUS // Private anonymous mapping
#ifdef MAP_JIT
             | MAP_JIT                   // Use MAP_JIT on macOS if available for better security
#endif
             , -1, 0));                  // No file descriptor, no offset

    if (memory == MAP_FAILED) {
      throw std::runtime_error("Failed to allocate memory for machine code");
    }

    // Copy our generated machine code into the allocated memory region
    std::copy(machine_code.begin(), machine_code.end(), memory);

    // Make the memory executable after copying the code (W^X security principle)
    if (mprotect(memory, required_memory_size, PROT_READ | PROT_EXEC) == -1) {
      munmap(memory, required_memory_size);  // Clean up on failure
      throw std::runtime_error("Failed to make memory executable");
    }

#if defined(__APPLE__) && defined(__aarch64__)
    // Apple Silicon ARM64: Execute as function that returns a value
    // Due to system call restrictions on Apple Silicon, we use a simpler approach
    auto arm_func = reinterpret_cast<int (*)()>(memory);
    const auto result = arm_func();  // Execute the generated ARM64 code
    std::cout << "JIT executed successfully (returned: " << result << ")\n";
    std::cout << hello_name;  // Print the greeting message from host code
#else  
    // x86-64 (Linux/macOS) and Linux ARM64: Execute as void function that handles its own output
    // The generated code performs the system call to print the message directly
    const auto func = reinterpret_cast<void (*)()>(memory);
    func();  // Execute the generated machine code - this will print our greeting!
#endif

    // Clean up the allocated memory
    munmap(memory, required_memory_size);

  } catch (const std::exception& e) {
    // Handle any errors that occurred during memory allocation or execution
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;  // Indicate successful program completion
}

/**
 * @brief Calculates memory size needed for machine code, rounded up to page boundary
 * @param machine_code_size Size of the machine code in bytes
 * @return Size in bytes that is a multiple of the system page size
 * @note This function is marked noexcept because it cannot throw exceptions
 * 
 * Memory mapping functions like mmap() require allocation sizes to be multiples
 * of the system page size. This function calculates the minimum page-aligned
 * size that can hold the machine code.
 */
[[nodiscard]] auto estimate_memory_size(size_t machine_code_size) noexcept
    -> size_t
{
  // Get the system page size (typically 4096 bytes on most systems)
  const auto page_size_multiple = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
  auto factor = size_t{1};  // Start with 1 page

  // Keep increasing the factor until we have enough space
  while (true) {
    const auto required_memory_size = factor * page_size_multiple;
    if (machine_code_size <= required_memory_size) {
      return required_memory_size;  // Found a size that fits
    }
    ++factor;  // Try next multiple of page size
  }
}

/**
 * @brief Patches the machine code template with the actual message length
 * @param machine_code Reference to the vector containing machine code bytes
 * @param hello_name The message string whose length will be embedded in the code
 * 
 * This function modifies the machine code to contain the length of the message string.
 * The byte positions vary depending on the target architecture and platform.
 */
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void
{
  const auto message_size = hello_name.length();  // Get length of the greeting message

#if defined(__linux__) && defined(__x86_64__)
  // Linux x86-64: Patch the "mov rdx, length" instruction at bytes 24-27
  machine_code[24] = static_cast<uint8_t>((message_size & 0xFF) >> 0);        // Bits 0-7
  machine_code[25] = static_cast<uint8_t>((message_size & 0xFF00) >> 8);      // Bits 8-15  
  machine_code[26] = static_cast<uint8_t>((message_size & 0xFF0000) >> 16);   // Bits 16-23
  machine_code[27] = static_cast<uint8_t>((message_size & 0xFF000000) >> 24); // Bits 24-31
#elif defined(__APPLE__) && defined(__x86_64__)
  // macOS x86-64: Same as Linux x86-64, patch "mov rdx, length" at bytes 24-27
  machine_code[24] = static_cast<uint8_t>((message_size & 0xFF) >> 0);        // Bits 0-7
  machine_code[25] = static_cast<uint8_t>((message_size & 0xFF00) >> 8);      // Bits 8-15
  machine_code[26] = static_cast<uint8_t>((message_size & 0xFF0000) >> 16);   // Bits 16-23
  machine_code[27] = static_cast<uint8_t>((message_size & 0xFF000000) >> 24); // Bits 24-31
#elif defined(__linux__) && defined(__aarch64__)
  // Linux ARM64: Patch the "mov x2, length" instruction at bytes 8-11
  // ARM64 uses different encoding - we need to encode the immediate value properly
  const auto encoded_size = (message_size & 0xFFFF) << 5;  // Shift left by 5 for ARM64 encoding
  machine_code[8] = static_cast<uint8_t>((encoded_size & 0xFF) >> 0);
  machine_code[9] = static_cast<uint8_t>((encoded_size & 0xFF00) >> 8);
#elif defined(__APPLE__) && defined(__aarch64__)
  // Apple Silicon ARM64: No-op since we're using a simple return approach
  // The actual message printing is handled by the host program
  (void)machine_code; // Suppress unused parameter warning
  (void)hello_name;   // Suppress unused parameter warning
#endif
}

/**
 * @brief Displays the generated machine code in hexadecimal format for debugging
 * @param machine_code The vector containing the machine code bytes to display
 * 
 * This function prints each byte of the machine code as a hexadecimal number,
 * formatting the output to show 7 bytes per line for readability.
 */
auto show_machine_code(const std::vector<uint8_t>& machine_code) -> void
{
  auto counter = 0;  // Track how many bytes we've printed on current line
  std::cout << "\nMachine code generated:\n" << std::hex;  // Switch to hex output mode

  // Print each byte of the machine code
  for (const auto byte : machine_code) {
    std::cout << static_cast<int>(byte) << " ";  // Cast to int to print as number
    ++counter;
    if (counter % 7 == 0) {      // After every 7 bytes...
      std::cout << '\n';         // ...start a new line
    }
  }

  std::cout << std::dec << "\n\n";  // Switch back to decimal mode and add spacing
}