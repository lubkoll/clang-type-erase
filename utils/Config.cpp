#include "Config.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <cctype>
#include <locale>
#include <stdexcept>
#include <string>

namespace clang
{
    const std::string ConfigFileName = ".clang-type-erase.config";

    namespace type_erasure
    {
        namespace
        {
            boost::filesystem::path getConfigFile()
            {
                auto Path = boost::filesystem::current_path();
                auto File = Path;
                File /= ConfigFileName;

                boost::system::error_code ErrorCode;
                while(!boost::filesystem::exists(File, ErrorCode) &&
                      File != "/" + ConfigFileName)
                {
                    Path = Path.parent_path();
                    File = Path;
                    File /= ConfigFileName;
                }

                if(boost::filesystem::exists(File, ErrorCode))
                    return File;

                throw std::runtime_error("Could not find '" + ConfigFileName + "'.");
            }

            // trim from start
            std::string lTrim(std::string&& Str) {
                Str.erase(Str.begin(), std::find_if(Str.begin(), Str.end(),
                        std::not1(std::ptr_fun<int, int>(std::isspace))));
                return Str;
            }

            // trim from end
            std::string rTrim(std::string&& Str) {
                Str.erase(std::find_if(Str.rbegin(), Str.rend(),
                        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), Str.end());
                return Str;
            }

            // trim from both ends
            std::string trim(std::string Str) {
                return lTrim(rTrim(std::move(Str)));
            }

            template <class T,
                      std::enable_if_t<std::is_integral<T>::value>* = nullptr>
            void readValue(std::ifstream& ConfigFile, T& Value)
            {
                std::string Buffer;
                getline(ConfigFile, Buffer);
                Buffer = trim(Buffer);
                Value = stoi(Buffer);
            }

            void readValue(std::ifstream& ConfigFile, bool& Value)
            {
                std::string Buffer;
                getline(ConfigFile, Buffer);
                Buffer = trim(Buffer);
                Value = Buffer == "true" ? true : false;
            }

            void readValue(std::ifstream& ConfigFile, std::string& Value)
            {
                getline(ConfigFile, Value);
                Value = trim(Value);
            }

            void read(boost::filesystem::path Path, Config& Configuration)
            {
                auto FileName = Path.c_str();
                std::ifstream ConfigFile(FileName);
                if(ConfigFile.is_open())
                {
                    std::string Buffer;
                    while(!ConfigFile.eof())
                    {
                        getline(ConfigFile, Buffer, ':');
                        Buffer = trim(Buffer);

                        if(Buffer == "copy-on-write")
                            readValue(ConfigFile, Configuration.CopyOnWrite);
                        else if(Buffer == "small-buffer-optimization")
                            readValue(ConfigFile, Configuration.SmallBufferOptimization);
                        else if(Buffer == "non-copyable")
                            readValue(ConfigFile, Configuration.NonCopyable);
                        else if(Buffer == "header-only")
                            readValue(ConfigFile, Configuration.HeaderOnly);
                        else if(Buffer == "no-rtti")
                            readValue(ConfigFile, Configuration.NoRTTI);
                        else if(Buffer == "buffer-size")
                            readValue(ConfigFile, Configuration.BufferSize);
                        else //if(buffer == "cpp-standard")
                            readValue(ConfigFile, Configuration.CppStandard);
                    }
                }
            }
        }


        Config::Config()
        {
            try {
                read(getConfigFile(),
                     *this);
            } catch (...)
            {
                std::cerr << "Could not find .clang-type-erase.config\n";
            }
        }

        std::ostream& operator<<(std::ostream& OS, const Config& Configuration)
        {
            OS << std::boolalpha;
            OS << "Config\n"
               << "copy-on-write: " << Configuration.CopyOnWrite << '\n'
               << "small-buffer-optimization: " << Configuration.SmallBufferOptimization << '\n'
               << "non-copyable: " << Configuration.NonCopyable << '\n'
               << "header-only: " << Configuration.HeaderOnly << '\n'
               << "no-rtti: " << Configuration.NoRTTI << '\n'
               << "buffer-size: " << Configuration.BufferSize << '\n'
               << "cpp-standard: " << Configuration.CppStandard << '\n'
               << "interface type: " << Configuration.InterfaceType << '\n'
               << "interface var: " << Configuration.InterfaceType << '\n'
               << "impl var: " << Configuration.StorageObject << '\n'
               << "function table type: " << Configuration.FunctionTableType << '\n'
               << "function table var: " << Configuration.FunctionTableObject << '\n'
               << "cast name: " << Configuration.CastName << '\n'
               << "detail dir: " << Configuration.DetailDir << '\n'
               << "util include: " << Configuration.UtilInclude << '\n'
               << "util dir: " << Configuration.UtilDir << '\n';
            return OS;
        }
    }
}
