
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


#include <algorithm>   
#include <cstdint>     
#include <cstdlib>     
#include <iostream>    
#include <memory>      
#include <stdexcept>   
#include <string>      
#include <string_view> 
#include <vector>      

// Forward declarations of helper functions
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void;
auto show_machine_code(const std::vector<uint8_t>& machine_code) -> void;
[[nodiscard]] auto estimate_memory_size(size_t machine_code_size) noexcept
    -> size_t;

/**
 * @struct MmapDeleter
 * @brief Custom deleter for memory-mapped regions used with std::unique_ptr
 * 
 * This struct implements the deleter pattern for RAII memory management
 * of memory-mapped regions allocated with mmap().
 */
struct MmapDeleter {
  size_t size;  // Size of the memory region to unmap
  
  /**
   * @brief Operator that performs the actual cleanup of memory-mapped region
   * @param ptr Pointer to the memory-mapped region to be unmapped
   */
  auto operator()(uint8_t* ptr) const noexcept -> void
  {
    // Check if pointer is valid and not the special MAP_FAILED value
    if (ptr && ptr != MAP_FAILED) {
      munmap(ptr, size);  // Unmap the memory region from virtual address space
    }
  }
};

/**
 * @typedef ExecutableMemoryPtr
 * @brief Type alias for a smart pointer that automatically manages executable memory
 * 
 * This creates a unique_ptr that uses our custom MmapDeleter to automatically
 * clean up memory-mapped executable regions when they go out of scope.
 */
using ExecutableMemoryPtr = std::unique_ptr<uint8_t, MmapDeleter>;

/**
 * @brief Factory function that allocates executable memory using mmap()
 * @param size Number of bytes to allocate for executable memory
 * @return Smart pointer to the allocated executable memory region
 * @throws std::runtime_error if memory allocation fails
 * 
 * This function creates a memory region that has read, write, and execute
 * permissions, which is necessary for storing and running generated machine code.
 */
[[nodiscard]] auto create_executable_memory(size_t size) -> ExecutableMemoryPtr
{
  // Allocate memory with read, write, and execute permissions
  auto* memory = static_cast<uint8_t*>(
      mmap(nullptr,                    // Let kernel choose the address
           size,                       // Size of memory to allocate
           PROT_READ |                 // Allow reading from this memory
           PROT_WRITE |                // Allow writing to this memory  
           PROT_EXEC,                  // Allow executing code from this memory
           MAP_PRIVATE |               // Create private copy-on-write mapping
           MAP_ANONYMOUS,              // Not backed by any file
           -1,                         // No file descriptor (ignored for anonymous)
           0));                        // No offset (ignored for anonymous)

  // Check if memory allocation failed
  if (memory == MAP_FAILED) {
    throw std::runtime_error("Failed to allocate executable memory");
  }

  // Return smart pointer with custom deleter that will automatically cleanup
  return ExecutableMemoryPtr{memory, MmapDeleter{size}};
}

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

  /**
   * x86-64 Machine Code Template Explanation:
   * This code generates a system call to write() that prints our greeting message.
   * The machine code performs the equivalent of: write(1, hello_name, strlen(hello_name))
   */
  std::vector<uint8_t> machine_code{
#ifdef __linux__
      // Linux write system call number is 1
      0x48, 0xc7, 0xc0, 0x01,          // mov rax, 1        - Load write syscall number
      0x00, 0x00, 0x00,                // (padding bytes for 64-bit immediate)
#elif __APPLE__
      // macOS write system call number is 0x02000004  
      0x48, 0xc7, 0xc0, 0x04,          // mov rax, 0x02000004 - Load write syscall number
      0x00, 0x00, 0x02,                // (macOS syscall numbers are different)
#endif
      0x48, 0xc7, 0xc7, 0x01,          // mov rdi, 1        - Load file descriptor (stdout)
      0x00, 0x00, 0x00,                // (padding bytes)
      0x48, 0x8d, 0x35, 0x0a,          // lea rsi, [rip+10] - Load address of string data
      0x00, 0x00, 0x00,                // (relative offset to string, 10 bytes ahead)
      0x48, 0xc7, 0xc2, 0x00,          // mov rdx, 0        - Load message length (filled later)
      0x00, 0x00, 0x00,                // (will be replaced with actual message size)
      0x0f, 0x05,                      // syscall           - Invoke system call
      0xc3                             // ret               - Return to caller
  };

  // Update the machine code template with the actual message length
  append_message_size(machine_code, hello_name);

  // Append the actual string data after the machine code instructions
  // The LEA instruction above calculates the address of this string data
  for (const auto character : hello_name) {
    machine_code.push_back(static_cast<uint8_t>(character));  // Add each character as byte
  }

  // Display the generated machine code for debugging purposes
  show_machine_code(machine_code);

  try {
    // Calculate how much memory we need (must be multiple of page size)
    const auto required_memory_size = estimate_memory_size(machine_code.size());

    // Allocate executable memory using RAII smart pointer
    auto executable_memory = create_executable_memory(required_memory_size);

    // Copy our generated machine code into the executable memory region
    std::copy(machine_code.begin(),           // Source start
              machine_code.end(),             // Source end  
              executable_memory.get());       // Destination

    // Cast the memory address to a function pointer and execute the generated code
    const auto func = reinterpret_cast<void (*)()>(executable_memory.get());
    func();  // Execute the generated machine code - this will print our greeting!

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
 * This function modifies bytes 24-27 of the machine code to contain the length
 * of the message string. These bytes correspond to the immediate operand of the
 * "mov rdx, length" instruction that sets up the third parameter for the write() syscall.
 */
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void
{
  const auto message_size = hello_name.length();  // Get length of the greeting message

  // Extract each byte of the 32-bit length value and store in machine code
  // x86-64 uses little-endian byte order (least significant byte first)
  machine_code[24] = static_cast<uint8_t>((message_size & 0xFF) >> 0);        // Bits 0-7
  machine_code[25] = static_cast<uint8_t>((message_size & 0xFF00) >> 8);      // Bits 8-15  
  machine_code[26] = static_cast<uint8_t>((message_size & 0xFF0000) >> 16);   // Bits 16-23
  machine_code[27] = static_cast<uint8_t>((message_size & 0xFF000000) >> 24); // Bits 24-31
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