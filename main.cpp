
/**
 * @file main.cpp
 * @brief JIT (Just-In-Time) compiler that creates machine code at runtime
 *
 * HOW IT WORKS:
 * 1. Ask user for their name
 * 2. Create machine code that will print "Hello, [name]!"
 * 3. Put that machine code in executable memory
 * 4. Run the machine code like a function
 *
 * WHAT IT DEMONSTRATES:
 * - Creating machine code while the program is running
 * - Making memory executable so we can run our generated code
 * - Cross-platform support (Linux, macOS, x86-64, ARM64)
 */

#include <sys/mman.h>
#include <unistd.h>

// Standard C++ headers
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Tell compiler these functions exist (defined later in file)
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void;
auto show_machine_code(const std::vector<uint8_t>& machine_code) -> void;
[[nodiscard]] auto estimate_memory_size(size_t machine_code_size) noexcept
    -> size_t;

/**
 * MAIN FUNCTION - This is where the program starts
 *
 * STEP BY STEP PROCESS:
 * 1. Get user's name
 * 2. Create machine code template
 * 3. Fill in the template with actual message
 * 4. Allocate executable memory
 * 5. Copy machine code to that memory
 * 6. Execute the machine code
 * 7. Clean up
 */
auto main() -> int
{
  // STEP 1: Get user input
  std::string name;                    // Create variable to store user's name
  std::cout << "What is your name?\n"; // Ask the user for their name
  std::getline(std::cin, name); // Read their entire input (including spaces)

  // STEP 2: Create the message we want the generated code to print
  const auto hello_name =
      std::string{"Hello, "} + name + "!\n"; // Build greeting message

  // STEP 3: Show what platform we're running on
  std::cout << "Platform detected: "; // Print platform info header
#if defined(__linux__) && defined(__x86_64__)
  std::cout << "Linux x86-64\n"; // We're on Linux with Intel/AMD processor
#elif defined(__APPLE__) && defined(__x86_64__)
  std::cout << "macOS x86-64\n"; // We're on macOS with Intel processor
#elif defined(__linux__) && defined(__aarch64__)
  std::cout << "Linux ARM64\n"; // We're on Linux with ARM processor
#elif defined(__APPLE__) && defined(__aarch64__)
  std::cout << "Apple Silicon ARM64\n"; // We're on macOS with Apple Silicon
  std::cout
      << "Note: Using simplified JIT approach due to system call "
         "security restrictions on Apple Silicon.\n"; // Explain why Apple
                                                      // Silicon is different
#endif /**                                                                    \
        * STEP 4: Create machine code for different processors                \
        *                                                                     \
        * WHAT IS MACHINE CODE?                                               \
        * - Machine code is the raw binary instructions that processors       \
        * understand                                                          \
        * - Different processors (x86-64 vs ARM64) use different instruction  \
        * sets                                                                \
        * - We're creating a small program that calls the "write" system call \
        * to print text                                                       \
        *                                                                     \
        * THE GENERATED PROGRAM DOES THIS:                                    \
        * - Set up registers with: file descriptor=1 (stdout), message        \
        * address, message length                                             \
        * - Call the operating system to write the message                    \
        * - Return to our main program                                        \
        */
  std::vector<uint8_t> machine_code{
      // Vector to hold our machine code bytes
#if defined(__linux__) && defined(__x86_64__)
      // LINUX x86-64 MACHINE CODE:
      // This creates assembly: mov rax,1; mov rdi,1; lea rsi,[rip+10]; mov
      // rdx,len; syscall; ret
      0x48, 0xc7, 0xc0, 0x01,
      0x00, 0x00, 0x00, // mov rax, 1    - Put system call number in rax
                        // register
      0x48, 0xc7, 0xc7, 0x01,
      0x00, 0x00, 0x00, // mov rdi, 1    - Put file descriptor (stdout) in rdi
                        // register
      0x48, 0x8d, 0x35, 0x0a,
      0x00, 0x00, 0x00, // lea rsi, [rip+10] - Put address of our text in rsi
                        // register
      0x48, 0xc7, 0xc2, 0x00,
      0x00, 0x00, 0x00, // mov rdx, 0    - Put message length in rdx (filled in
                        // later)
      0x0f, 0x05, // syscall       - Ask operating system to write the text
      0xc3        // ret           - Return to our main program
#elif defined(__APPLE__) && defined(__x86_64__)
      // MACOS x86-64 MACHINE CODE:
      // Same as Linux but different system call number (macOS uses different
      // numbers)
      0x48, 0xc7, 0xc0, 0x04,
      0x00, 0x00, 0x02, // mov rax, 0x02000004 - macOS write system call number
      0x48, 0xc7, 0xc7, 0x01,
      0x00, 0x00, 0x00, // mov rdi, 1          - File descriptor (stdout)
      0x48, 0x8d, 0x35, 0x0a,
      0x00, 0x00, 0x00, // lea rsi, [rip+10]   - Address of our text
      0x48, 0xc7, 0xc2, 0x00,
      0x00, 0x00, 0x00, // mov rdx, 0          - Message length (filled in
                        // later)
      0x0f, 0x05,       // syscall             - Ask macOS to write the text
      0xc3              // ret                 - Return to main program
#elif defined(__linux__) && defined(__aarch64__)
      // LINUX ARM64 MACHINE CODE:
      // ARM64 uses different instruction format but same concept
      0x20, 0x00,
      0x80, 0xd2, // mov x0, #1       - Put file descriptor in x0 register
      0x41, 0x00,
      0x00, 0x10, // adr x1, #8       - Put address of text in x1 register
      0x42, 0x00,
      0x80, 0xd2, // mov x2, #2       - Put message length in x2 (patched later)
      0x08, 0x08,
      0x80, 0xd2, // mov x8, #64      - Put write system call number in x8
      0x01, 0x00,
      0x00, 0xd4, // svc #0           - Ask Linux to write the text
      0xc0, 0x03,
      0x5f, 0xd6 // ret              - Return to main program
#elif defined(__APPLE__) && defined(__aarch64__)
      // APPLE SILICON MACHINE CODE:
      // Apple Silicon has strict security, so we just return a success code
      0x00, 0x00,
      0x80, 0xd2, // mov x0, #0       - Put success code (0) in x0 register
      0xc0, 0x03,
      0x5f, 0xd6 // ret              - Return to main program
#else
#error \
    "Unsupported platform: This code only works on Linux/macOS with x86-64/ARM64 processors"
#endif
  };

  // STEP 5: Update machine code with actual message length
  append_message_size(
      machine_code,
      hello_name); // Fill in the message length in our machine code

  // STEP 6: Add the actual text to the end of our machine code
  // (Only needed for platforms where the generated code directly accesses the
  // text)
#if !defined(__APPLE__) || !defined(__aarch64__) // Skip this for Apple Silicon
  for (const auto character :
       hello_name) { // Loop through each character in message
    machine_code.push_back(static_cast<uint8_t>(
        character)); // Add character to end of machine code
  }
#endif

  // STEP 7: Show the machine code we generated (for debugging)
  show_machine_code(machine_code); // Print out all the bytes in hexadecimal

  try { // Use try-catch to handle any errors that might happen

    // STEP 8: Calculate how much memory we need
    const auto required_memory_size =
        estimate_memory_size(machine_code.size()); // Get page-aligned size

    // STEP 9: Allocate memory that we can eventually execute code from
    // We start with read/write permissions for security (W^X principle)
    auto* memory = static_cast<uint8_t*>(mmap(
        nullptr,                // Let the system choose where to put the memory
        required_memory_size,   // How much memory we need
        PROT_READ | PROT_WRITE, // Start with read and write permissions only
        MAP_PRIVATE | MAP_ANONYMOUS // Private memory not backed by any file
#ifdef MAP_JIT
            | MAP_JIT // Use special JIT flag on macOS if available
#endif
        ,
        -1, 0)); // No file descriptor, no offset

    // STEP 10: Check if memory allocation worked
    if (memory == MAP_FAILED) { // MAP_FAILED means allocation failed
      throw std::runtime_error(
          "Failed to allocate memory for machine code"); // Throw error
    }

    // STEP 11: Copy our machine code into the allocated memory
    std::copy(machine_code.begin(), machine_code.end(),
              memory); // Copy all bytes

    // STEP 12: Make the memory executable (W^X security principle)
    // We change from read/write to read/execute permissions
    if (mprotect(memory, required_memory_size, PROT_READ | PROT_EXEC) ==
        -1) {                               // Try to make executable
      munmap(memory, required_memory_size); // Clean up memory if this fails
      throw std::runtime_error(
          "Failed to make memory executable"); // Throw error
    }

    // STEP 13: Execute our generated machine code!
#if defined(__APPLE__) && defined(__aarch64__)
    // APPLE SILICON: Execute as function that returns an integer
    auto arm_func =
        reinterpret_cast<int (*)()>(memory); // Treat memory as function pointer
    const auto result = arm_func();          // Call our generated function
    std::cout << "JIT executed successfully (returned: " << result
              << ")\n";      // Show return value
    std::cout << hello_name; // Print message from main program
#else
    // ALL OTHER PLATFORMS: Execute as function that prints directly
    const auto func = reinterpret_cast<void (*)()>(
        memory); // Treat memory as function pointer
    func(); // Call our generated function - it will print the message itself
#endif

    // STEP 14: Clean up the memory we allocated
    munmap(memory, required_memory_size); // Free the memory

  } catch (const std::exception& e) { // Catch any errors that happened
    std::cerr << "Error: " << e.what() << '\n'; // Print the error message
    return EXIT_FAILURE;                        // Return failure code
  }

  return EXIT_SUCCESS; // Return success code - program worked!
}

/**
 * HELPER FUNCTION: Calculate memory size needed
 *
 * WHY WE NEED THIS:
 * - Operating system memory allocation works in "pages" (usually 4096 bytes)
 * - We must allocate memory in multiples of page size
 * - This function finds the smallest page-multiple that fits our machine code
 */
[[nodiscard]] auto estimate_memory_size(size_t machine_code_size) noexcept
    -> size_t
{
  // Get the system page size (how big each memory "page" is)
  const auto page_size_multiple = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
  auto factor = size_t{1}; // Start with 1 page

  // Keep trying bigger multiples until we find one big enough
  while (true) {
    const auto required_memory_size =
        factor * page_size_multiple;                 // Calculate total size
    if (machine_code_size <= required_memory_size) { // Is it big enough?
      return required_memory_size;                   // Yes! Return this size
    }
    ++factor; // No, try next multiple (2 pages, 3 pages, etc.)
  }
}

/**
 * HELPER FUNCTION: Fill in message length in machine code
 *
 * WHY WE NEED THIS:
 * - Our machine code template has placeholder bytes for message length
 * - We need to replace those placeholders with the actual length
 * - Different processors store the length in different locations
 */
auto append_message_size(std::vector<uint8_t>& machine_code,
                         std::string_view hello_name) -> void
{
#if defined(__APPLE__) && defined(__aarch64__)
  // APPLE SILICON: We don't need message length since we're not calling system
  // calls
  (void)machine_code; // Tell compiler we're not using this parameter
  (void)hello_name;   // Tell compiler we're not using this parameter
#else
  const auto message_size =
      hello_name.length(); // Get how many characters in our message

#if defined(__linux__) && defined(__x86_64__)
  // LINUX x86-64: Put message length in bytes 24-27 of machine code
  // This overwrites the zeros in the "mov rdx, 0" instruction
  machine_code[24] =
      static_cast<uint8_t>((message_size & 0xFF) >> 0); // Lowest 8 bits
  machine_code[25] =
      static_cast<uint8_t>((message_size & 0xFF00) >> 8); // Next 8 bits
  machine_code[26] =
      static_cast<uint8_t>((message_size & 0xFF0000) >> 16); // Next 8 bits
  machine_code[27] =
      static_cast<uint8_t>((message_size & 0xFF000000) >> 24); // Highest 8 bits
#elif defined(__APPLE__) && defined(__x86_64__)
  // MACOS x86-64: Same as Linux x86-64 (put length in bytes 24-27)
  machine_code[24] = static_cast<uint8_t>((message_size & 0xFF) >> 0);
  machine_code[25] = static_cast<uint8_t>((message_size & 0xFF00) >> 8);
  machine_code[26] = static_cast<uint8_t>((message_size & 0xFF0000) >> 16);
  machine_code[27] = static_cast<uint8_t>((message_size & 0xFF000000) >> 24);
#elif defined(__linux__) && defined(__aarch64__)
  // LINUX ARM64: Put message length in bytes 8-9 of machine code
  // ARM64 encoding is different - we need to shift the value for proper
  // encoding
  const auto encoded_size = (message_size & 0xFFFF)
                            << 5; // Shift for ARM64 instruction encoding
  machine_code[8] =
      static_cast<uint8_t>((encoded_size & 0xFF) >> 0); // Lower byte
  machine_code[9] =
      static_cast<uint8_t>((encoded_size & 0xFF00) >> 8); // Upper byte
#endif
#endif
}

/**
 * HELPER FUNCTION: Display machine code for debugging
 *
 * WHAT THIS DOES:
 * - Takes our machine code bytes and prints them in hexadecimal
 * - Shows 7 bytes per line for easy reading
 * - Helps us see exactly what machine code we generated
 */
auto show_machine_code(const std::vector<uint8_t>& machine_code) -> void
{
  auto counter =
      0; // Keep track of how many bytes we've printed on current line
  std::cout << "\nMachine code generated:\n"
            << std::hex; // Print header and switch to hex mode

  // Loop through each byte in our machine code
  for (const auto byte : machine_code) {
    std::cout << static_cast<int>(byte)
              << " ";       // Print byte as hexadecimal number
    ++counter;              // Count this byte
    if (counter % 7 == 0) { // After every 7 bytes...
      std::cout << '\n';    // ...start a new line for readability
    }
  }

  std::cout << std::dec
            << "\n\n"; // Switch back to decimal mode and add blank line
}