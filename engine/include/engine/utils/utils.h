#pragma once

//==== Utility functions ====

#include <string>

namespace eng {

    //Reads a textfile, returns it as a string (throws exception on failure).
    std::string ReadFile(const char* filepath);

    //Same as ReadFile, but returns true/false as a status, file contents are returned through the out_text arugment.
    bool TryReadFile(const char* filepath, std::string& out_text);

    //Writes provided text into a textfile (throws exception on failure). Always overrides the entire file.
    void WriteFile(const char* filepath, const std::string& text);

    //Sames as WriteFile, but returns true/false as a status.
    bool TryWriteFile(const char* filepath, const std::string& text);

    //Extract filename from a path.
    std::string GetFilename(const std::string& path, bool stripExtension = false);

}//namespace eng
