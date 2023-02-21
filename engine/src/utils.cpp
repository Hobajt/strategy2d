#include "engine/utils/utils.h"
#include "engine/utils/setup.h"

#include <fstream>
#include <sstream>

namespace eng {

    std::string ReadFile(const char* filepath) {
        std::string text;
        if (TryReadFile(filepath, text))
            return text;
        else
            throw std::exception();
    }

    bool TryReadFile(const char* filepath, std::string& out_text) {
        using namespace std;

        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            std::stringstream ss;

            file.open(filepath);
            ss << file.rdbuf();
            file.close();

            out_text = ss.str();
        }
        catch (std::ifstream::failure&) {
            ENG_LOG_DEBUG("ReadFile - Failed to read from '{}'.", filepath);
            return false;
        }

        ENG_LOG_TRACE("[R] ReadFile - Loaded file '{}' ({})", filepath, (int)out_text.size());
        return true;
    }

    void WriteFile(const char* filepath, const std::string& text) {
        if(!TryWriteFile(filepath, text))
            throw std::runtime_error("WriteFile() failed.");
    }

    bool TryWriteFile(const char* filepath, const std::string& text) {
        using namespace std;

        ofstream file;
        file.exceptions(ofstream::failbit | ofstream::badbit);

        try {
            file.open(filepath, ios_base::trunc);
            file << text;
            file.close();
        }
        catch(ofstream::failure&) {
            ENG_LOG_DEBUG("WriteFile - Failed to write to '{}'.", filepath);
            return false;
        }

        ENG_LOG_TRACE("[R] WriteFile - Written to '{}' ({})\n", filepath, (int)text.size());
        return true;
    }

    std::string GetFilename(const std::string& path, bool stripExtension) {
        std::string name;

        size_t pos = path.find_last_of("/\\");
        if(pos != std::string::npos)
            name = path.substr(pos+1);
        else
            name = path;
        
        if(stripExtension) {
            pos = name.find_last_of(".");
            if(pos != std::string::npos)
                name = name.substr(0, pos);
        }
            
        return name;
    }

}//namespace eng
