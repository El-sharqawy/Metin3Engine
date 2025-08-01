#include "Stdafx.h"
#include "Utils.h"
#include "../../LibMath/source/grid.h"
#include <filesystem>
#include <ctime>
#include <chrono>

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        sys_err("OpenGL Error: %s | File: %s (line: %d)", error.c_str(), file, line);
    }
    return errorCode;
}

bool IsGLVersionHigher(int MajorVer, int MinorVer)
{
    static int glMajorVersion = 0;
    static int glMinorVersion = 0;

    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);

    return ((glMajorVersion >= MajorVer) && (glMinorVersion >= MinorVer));
}

std::string GetDirFromFilename(const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex;

#ifdef _WIN64
    SlashIndex = Filename.find_last_of("\\");

    if (SlashIndex == -1)
    {
        SlashIndex = Filename.find_last_of("/");
    }
#else
    SlashIndex = Filename.find_last_of("/");
#endif

    std::string Dir;

    if (SlashIndex == std::string::npos)
    {
        Dir = ".";
    }
    else if (SlashIndex == 0)
    {
        Dir = "/";
    }
    else
    {
        Dir = Filename.substr(0, SlashIndex);
    }

    return Dir;

}

char* ReadBinaryFile(const char* pFilename, int& size)
{
    FILE* f = NULL;

    errno_t err = fopen_s(&f, pFilename, "rb");

    if (!f)
    {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        sys_err("Error opening '%s': %s\n", pFilename, buf);
        exit(0);
    }

    struct stat stat_buf;
    int error = stat(pFilename, &stat_buf);

    if (error)
    {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        sys_err("Error getting file stats: %s\n", buf);
        return NULL;
    }

    size = stat_buf.st_size;

    char* p = (char*)malloc(size);
    assert(p);

    size_t bytes_read = fread(p, 1, size, f);

    if (bytes_read != size)
    {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        sys_err("Read file error file: %s\n", buf);
        exit(0);
    }

    sys_log("File %s Have Been Readed Successfully with size %d", pFilename, size);
    fclose(f);

    return p;
}

void WriteBinaryFile(const char* pFilename, const void* pData, int size)
{
    FILE* f = NULL;

    errno_t err = fopen_s(&f, pFilename, "wb");

    if (!f)
    {
        sys_err("Error opening '%s'\n", pFilename);
        exit(0);
    }

    size_t bytes_written = fwrite(pData, 1, size, f);

    if (bytes_written != size)
    {
        sys_err("Error write file\n");
        exit(0);
    }

    fclose(f);
}

std::string GetFullPath(const std::string& Dir, const aiString& Path)
{
    std::string p(Path.data);

    if (p == "C:\\\\")
    {
        p = "";
    }
    else if (p.substr(0, 2) == ".\\")
    {
        p = p.substr(2, p.size() - 2);
    }

    std::string FullPath = Dir + "/" + p;

    return FullPath;
}

void create_directory_if_missing(const std::string& path)
{
    namespace fs = std::filesystem;
    if (!fs::exists(path))
    {
        fs::create_directories(path);  // Creates parent directories too
    }
}

std::string GetCurrentTimestamp()
{
    using namespace std::chrono;

    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t now_c = system_clock::to_time_t(now);
    std::tm local_tm;
#ifdef _WIN64
    localtime_s(&local_tm, &now_c);
#else
    localtime_r(&now_c, &local_tm);
#endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << local_tm.tm_mon + 1
        << std::setw(2) << local_tm.tm_mday << " "
        << std::setw(2) << local_tm.tm_hour << ":"
        << std::setw(2) << local_tm.tm_min << ":"
        << std::setw(2) << local_tm.tm_sec << std::setw(3) << ms.count();

    return oss.str();
}

void _log_to_file(const char* filename, const char* fmt, va_list args)
{
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::ofstream file(filename, std::ios::app);
    if (file.is_open())
    {
        file << GetCurrentTimestamp() << " :: " << buffer << "\n";
        file.close();
    }
}

// Error log
void _sys_err(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log_to_file("syserr.txt", fmt, args);
    va_end(args);
}

// Info log
void _sys_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log_to_file("syslog.txt", fmt, args);
    va_end(args);
}
