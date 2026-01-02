#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <optional>
#include <span>

namespace Scanner {

    // Simple pattern format: "48 8B 05 ? ? ? ? 48 85 C0"
    // '?' or '??' matches any byte
    inline std::optional<uintptr_t> Scan(const std::string& pattern, const std::string& moduleName = "RobloxPlayerBeta.exe") {
        HMODULE hModule = GetModuleHandleA(moduleName.c_str());
        if (!hModule) hModule = GetModuleHandleA(NULL); // Fallback to current process
        if (!hModule) return std::nullopt;

        MODULEINFO modInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO)))
            return std::nullopt;

        uint8_t* start = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
        size_t size = modInfo.SizeOfImage;

        // Parse pattern
        std::vector<std::optional<uint8_t>> bytes;
        for (size_t i = 0; i < pattern.length(); ++i) {
            if (pattern[i] == ' ') continue;
            if (pattern[i] == '?') {
                bytes.push_back(std::nullopt);
                if (i + 1 < pattern.length() && pattern[i + 1] == '?') i++;
            } else {
                std::string byteString = pattern.substr(i, 2);
                bytes.push_back(static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16)));
                i++;
            }
        }

        // Brute force scan (fast enough for start-up)
        for (size_t i = 0; i < size - bytes.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < bytes.size(); ++j) {
                if (bytes[j].has_value() && start[i + j] != bytes[j].value()) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return reinterpret_cast<uintptr_t>(start + i);
            }
        }
        return std::nullopt;
    }

    // Helper to resolve relative RIP-addressed instructions (common in x64)
    // Instruction: 48 8B 05 [OFFSET] -> Result: RIP + OFFSET + 7 (instruction size)
    inline uintptr_t ResolveRip(uintptr_t address, int32_t offset, int32_t instructionSize) {
        if (!address) return 0;
        int32_t relative = *reinterpret_cast<int32_t*>(address + offset);
        return address + relative + instructionSize;
    }
}
