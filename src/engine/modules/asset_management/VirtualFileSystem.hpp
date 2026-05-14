#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <memory>
#include <physfs.h>
#include <string>
#include <vector>

class VirtualFileSystem {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VirtualFileSystem");

public:
    explicit VirtualFileSystem(const std::vector<std::filesystem::path>& mountPaths = {},
                               const std::filesystem::path& writeDir = {}) {
        if (!PHYSFS_init(nullptr)) {
            LOG4CXX_ERROR(LOGGER,
                          "Failed to initialize PhysicsFS: " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            throw std::runtime_error("PhysicsFS initialization failed");
        }
        for (const auto& path: mountPaths) {
            mount(path);
        }
        if (!writeDir.empty()) {
            setWriteDir(writeDir);
        }
    }

    ~VirtualFileSystem() { PHYSFS_deinit(); }

    void mount(const std::filesystem::path& path) {
        if (!PHYSFS_mount(path.string().c_str(), nullptr, 1)) {
            LOG4CXX_ERROR(LOGGER,
                          "Failed to mount: " << path << " - " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            throw std::runtime_error("Failed to mount directory: " + path.string());
        }
        LOG4CXX_INFO(LOGGER, "Mounted: " << path);
    }

    void setWriteDir(const std::filesystem::path& path) {
        if (!PHYSFS_setWriteDir(path.string().c_str())) {
            LOG4CXX_ERROR(LOGGER,
                          "Failed to set write directory: " << path << " - "
                                                            << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            throw std::runtime_error("Failed to set write directory: " + path.string());
        }
        LOG4CXX_INFO(LOGGER, "Set write directory: " << path);
    }

    bool exists(const std::string& path) const { return PHYSFS_exists(path.c_str()); }

    std::vector<unsigned char> read(const std::string& path) const {
        auto file = PHYSFS_openRead(path.c_str());
        if (!file) {
            LOG4CXX_ERROR(LOGGER, "Failed to open file for reading: " << path);
            return {};
        }

        PHYSFS_sint64 fileSize = PHYSFS_fileLength(file);
        if (fileSize < 0) {
            LOG4CXX_ERROR(LOGGER, "Failed to get file size: " << path);
            PHYSFS_close(file);
            return {};
        }

        std::vector<unsigned char> data(fileSize);
        PHYSFS_sint64 bytesRead = PHYSFS_readBytes(file, data.data(), fileSize);
        PHYSFS_close(file);

        if (bytesRead != fileSize) {
            LOG4CXX_ERROR(LOGGER, "Failed to read complete file: " << path);
            return {};
        }

        return data;
    }

    bool write(const std::string& path, const std::vector<unsigned char>& data) {
        auto file = PHYSFS_openWrite(path.c_str());
        if (!file) {
            LOG4CXX_ERROR(LOGGER, "Failed to open file for writing: " << path);
            return false;
        }

        PHYSFS_sint64 bytesWritten = PHYSFS_writeBytes(file, data.data(), data.size());
        PHYSFS_close(file);

        if (bytesWritten != static_cast<PHYSFS_sint64>(data.size())) {
            LOG4CXX_ERROR(LOGGER, "Failed to write complete file: " << path);
            return false;
        }

        return true;
    }

    std::vector<std::string> listFiles(const std::string& dir) const {
        char** files = PHYSFS_enumerateFiles(dir.c_str());
        if (!files) return {};

        std::vector<std::string> result;
        for (char** i = files; *i != nullptr; ++i) {
            result.emplace_back(*i);
        }
        PHYSFS_freeList(files);
        return result;
    }

    std::vector<std::string> listFilesRecursive(const std::string& dir = "") const {
        std::vector<std::string> result;
        listFilesRecursiveHelper(dir, result);
        return result;
    }

    bool remove(const std::string& path) {
        if (!PHYSFS_delete(path.c_str())) {
            LOG4CXX_ERROR(LOGGER,
                          "Failed to delete: " << path << " - " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return false;
        }
        return true;
    }

    int64_t getModifiedTime(const std::string& path) const {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(path.c_str(), &stat)) {
            return stat.modtime;
        }
        return -1;
    }

private:
    void listFilesRecursiveHelper(const std::string& dir, std::vector<std::string>& result) const {
        char** files = PHYSFS_enumerateFiles(dir.empty() ? "/" : dir.c_str());
        if (!files) return;

        for (char** i = files; *i != nullptr; ++i) {
            std::string filename = *i;
            std::string fullPath = dir.empty() ? filename : dir + "/" + filename;

            PHYSFS_Stat stat;
            if (PHYSFS_stat(fullPath.c_str(), &stat) && stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
                listFilesRecursiveHelper(fullPath, result);
            } else {
                result.emplace_back(fullPath);
            }
        }
        PHYSFS_freeList(files);
    }
};
