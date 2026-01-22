#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <windows.h>

namespace fs = std::filesystem;

class ProgressTracker {
private:
    std::chrono::steady_clock::time_point start_time;
    std::atomic<size_t> completed{0};
    std::atomic<size_t> total{0};
    std::atomic<bool> running{false};
    std::mutex console_mutex;
    
public:
    void start(size_t total_tasks) {
        start_time = std::chrono::steady_clock::now();
        completed = 0;
        total = total_tasks;
        running = true;
    }
    
    void update(size_t increment = 1) {
        completed += increment;
    }
    
    void finish() {
        running = false;
        printProgress(); // Final update
        std::cout << std::endl;
    }
    
    void printProgress() {
        std::lock_guard<std::mutex> lock(console_mutex);
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        
        double progress = (total > 0) ? static_cast<double>(completed) / total : 0.0;
        
        // Calculate ETA
        auto estimated_total = (progress > 0) ? 
            std::chrono::seconds(static_cast<long long>(elapsed.count() / progress)) : 
            std::chrono::seconds(0);
        auto remaining = estimated_total - elapsed;
        if (remaining.count() < 0) remaining = std::chrono::seconds(0);
        
        // Format time strings
        auto formatTime = [](std::chrono::seconds seconds) {
            auto hours = seconds.count() / 3600;
            auto minutes = (seconds.count() % 3600) / 60;
            auto secs = seconds.count() % 60;
            
            std::stringstream ss;
            ss << std::setw(2) << std::setfill('0') << hours << ":"
               << std::setw(2) << std::setfill('0') << minutes << ":"
               << std::setw(2) << std::setfill('0') << secs;
            return ss.str();
        };
        
        // Create progress bar
        const int bar_width = 50;
        int pos = static_cast<int>(bar_width * progress);
        
        std::cout << "\r[";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] ";
        
        // Print percentage and times
        std::cout << std::fixed << std::setprecision(1) << (progress * 100.0) << "% ";
        std::cout << "(" << formatTime(elapsed) << " / " << formatTime(estimated_total) << ")";
        
        std::cout.flush();
    }
    
    bool isRunning() const {
        return running;
    }
};

class SimpleCompressor {
private:
    // 简单的XOR加密
    std::string xorEncryptDecrypt(const std::string& data, const std::string& key) {
        std::string result = data;
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = data[i] ^ key[i % key.size()];
        }
        return result;
    }

    // Base64编码
    std::string base64Encode(const std::string& input) {
        static const std::string base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string encoded;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (size_t n = 0; n < input.size(); n++) {
            char_array_3[i++] = input[n];
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    encoded += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < i + 1; j++)
                encoded += base64_chars[char_array_4[j]];

            while (i++ < 3)
                encoded += '=';
        }

        return encoded;
    }

    // Base64解码
    std::string base64Decode(const std::string& encoded_string) {
        static const std::string base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        auto is_base64 = [](unsigned char c) {
            return (isalnum(c) || (c == '+') || (c == '/'));
        };

        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string decoded;

        size_t in_len = encoded_string.size();
        while (in_len-- && encoded_string[in_] != '=' && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++)
                    decoded += char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; j < i - 1; j++)
                decoded += char_array_3[j];
        }

        return decoded;
    }

    // 简单的LZ77压缩算法
    struct LZ77Match {
        uint32_t offset;      // 4字节，支持最多4GB的offset
        uint32_t length;      // 4字节，支持最多4GB的匹配长度
        char next_char;
        
        LZ77Match(uint32_t o = 0, uint32_t l = 0, char c = 0) : offset(o), length(l), next_char(c) {}
    };

    LZ77Match findBestMatch(const std::string& data, size_t current_pos, size_t search_buffer_size, size_t lookahead_buffer_size) {
        LZ77Match best_match;
        size_t start_search = (current_pos > search_buffer_size) ? current_pos - search_buffer_size : 0;
        size_t end_pos = std::min(current_pos + lookahead_buffer_size, data.size());
        
        for (size_t i = start_search; i < current_pos; ++i) {
            size_t match_len = 0;
            while (match_len < (end_pos - current_pos) && 
                   data[i + match_len] == data[current_pos + match_len]) {
                match_len++;
            }
            
            if (match_len > best_match.length) {
                best_match.offset = current_pos - i;
                best_match.length = match_len;
                if (current_pos + match_len < data.size()) {
                    best_match.next_char = data[current_pos + match_len];
                } else {
                    best_match.next_char = 0;
                }
            }
        }
        
        return best_match;
    }

    std::string lz77Compress(const std::string& data, size_t search_buffer_size = 65536, size_t lookahead_buffer_size = 65536) {
        std::string compressed;
        size_t pos = 0;
        
        while (pos < data.size()) {
            LZ77Match match = findBestMatch(data, pos, search_buffer_size, lookahead_buffer_size);
            
            if (match.length > 3) {
                // 编码匹配：0xFD + offset(4字节) + length(4字节) + next_char(1字节)
                compressed.push_back(0xFD);
                compressed.push_back((match.offset >> 24) & 0xFF);
                compressed.push_back((match.offset >> 16) & 0xFF);
                compressed.push_back((match.offset >> 8) & 0xFF);
                compressed.push_back(match.offset & 0xFF);
                uint32_t len32 = match.length;
                compressed.push_back((len32 >> 24) & 0xFF);
                compressed.push_back((len32 >> 16) & 0xFF);
                compressed.push_back((len32 >> 8) & 0xFF);
                compressed.push_back(len32 & 0xFF);
                compressed.push_back(match.next_char);
                pos += match.length + 1;
            } else {
                // 编码字面量：0xFE + length(4字节) + chars...
                // 收集连续的短序列
                size_t literal_start = pos;
                while (pos < data.size() && (pos - literal_start) < 65536) {
                    LZ77Match next_match = findBestMatch(data, pos, search_buffer_size, lookahead_buffer_size);
                    if (next_match.length > 3) {
                        break;
                    }
                    pos++;
                }
                
                uint32_t literal_len = pos - literal_start;
                compressed.push_back(0xFE);
                compressed.push_back((literal_len >> 24) & 0xFF);
                compressed.push_back((literal_len >> 16) & 0xFF);
                compressed.push_back((literal_len >> 8) & 0xFF);
                compressed.push_back(literal_len & 0xFF);
                compressed.append(data.substr(literal_start, literal_len));
            }
        }
        
        return compressed;
    }

    std::string lz77Decompress(const std::string& compressed) {
        std::string decompressed;
        size_t pos = 0;
        
        while (pos < compressed.size()) {
            uint8_t marker = (uint8_t)compressed[pos];
            
            if (marker == 0xFE) {
                // 字面量块：0xFE + length(4字节) + 数据
                if (pos + 4 >= compressed.size()) {
                    std::cerr << "Warning: Incomplete literal block header" << std::endl;
                    break;
                }
                
                uint32_t literal_len = ((uint32_t)(uint8_t)compressed[pos + 1] << 24) |
                                      ((uint32_t)(uint8_t)compressed[pos + 2] << 16) |
                                      ((uint32_t)(uint8_t)compressed[pos + 3] << 8) |
                                      ((uint32_t)(uint8_t)compressed[pos + 4]);
                
                if (pos + 5 + literal_len > compressed.size()) {
                    std::cerr << "Warning: Incomplete literal data" << std::endl;
                    break;
                }
                
                decompressed.append(compressed.substr(pos + 5, literal_len));
                pos += 5 + literal_len;
                
            } else if (marker == 0xFD) {
                // 匹配块：0xFD + offset(4字节) + length(4字节) + next_char(1字节)
                if (pos + 9 >= compressed.size()) {
                    std::cerr << "Warning: Incomplete match block header" << std::endl;
                    break;
                }
                
                uint32_t offset = ((uint32_t)(uint8_t)compressed[pos + 1] << 24) |
                                 ((uint32_t)(uint8_t)compressed[pos + 2] << 16) |
                                 ((uint32_t)(uint8_t)compressed[pos + 3] << 8) |
                                 ((uint32_t)(uint8_t)compressed[pos + 4]);
                                 
                uint32_t length = ((uint32_t)(uint8_t)compressed[pos + 5] << 24) |
                                 ((uint32_t)(uint8_t)compressed[pos + 6] << 16) |
                                 ((uint32_t)(uint8_t)compressed[pos + 7] << 8) |
                                 ((uint32_t)(uint8_t)compressed[pos + 8]);
                                 
                char next_char = compressed[pos + 9];
                
                // 检查offset有效性，防止整数下溢
                if (offset > decompressed.size()) {
                    std::cerr << "Warning: Invalid offset " << offset << " at decompressed size " << decompressed.size() << std::endl;
                    break;  // 数据损坏，停止解压
                }
                
                size_t copy_pos = decompressed.size() - offset;
                for (uint32_t i = 0; i < length; ++i) {
                    if (copy_pos + i < decompressed.size()) {
                        decompressed.push_back(decompressed[copy_pos + i]);
                    }
                }
                if (next_char != 0) {
                    decompressed.push_back(next_char);
                }
                
                pos += 10;  // 1(marker) + 4(offset) + 4(length) + 1(next_char)
            } else {
                // 未知标记，跳过
                std::cerr << "Warning: Unknown marker 0x" << std::hex << (int)marker << std::dec << " at position " << pos << std::endl;
                pos++;
            }
        }
        
        return decompressed;
    }

    // 读取文件内容
    std::string readFile(const fs::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filepath.string());
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    // 写入文件内容
    void writeFile(const fs::path& filepath, const std::string& content) {
        // 创建目录
        fs::create_directories(filepath.parent_path());

        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file: " + filepath.string());
        }

        file.write(content.c_str(), content.size());
    }

    // 分割字符串为多个卷
    std::vector<std::string> splitIntoVolumes(const std::string& data, size_t max_chars_per_volume) {
        std::vector<std::string> volumes;
        size_t total_size = data.size();
        
        for (size_t i = 0; i < total_size; i += max_chars_per_volume) {
            size_t chunk_size = std::min(max_chars_per_volume, total_size - i);
            volumes.push_back(data.substr(i, chunk_size));
        }
        
        return volumes;
    }

    // 并行处理文件的任务结构
    struct FileTask {
        fs::path filePath;
        fs::path relativePath;
        std::string content;
        std::string compressed;
        bool processed = false;
    };

    // 并行压缩文件
    void parallelCompressFiles(std::vector<FileTask>& tasks, size_t num_threads, ProgressTracker& progress) {
        std::vector<std::thread> threads;
        std::atomic<size_t> next_task{0};
        std::mutex task_mutex;
        
        auto worker = [&]() {
            while (true) {
                size_t task_index = next_task.fetch_add(1);
                if (task_index >= tasks.size()) break;
                
                FileTask& task = tasks[task_index];
                try {
                    task.content = readFile(task.filePath);
                    task.compressed = lz77Compress(task.content);
                    task.processed = true;
                    
                    progress.update();
                    
                    std::lock_guard<std::mutex> lock(task_mutex);
                    std::cout << "\nCompressed: " << task.relativePath.string() 
                              << " (" << task.content.size() << " -> " << task.compressed.size() << " bytes)" << std::endl;
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(task_mutex);
                    std::cerr << "\nError compressing " << task.filePath.string() << ": " << e.what() << std::endl;
                }
            }
        };
        
        // 启动工作线程
        size_t actual_threads = std::min(num_threads, tasks.size());
        for (size_t i = 0; i < actual_threads; ++i) {
            threads.emplace_back(worker);
        }
        
        // 进度显示线程
        std::thread progress_thread([&]() {
            while (progress.isRunning()) {
                progress.printProgress();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        
        // 等待所有工作线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 停止进度显示
        progress.finish();
        if (progress_thread.joinable()) {
            progress_thread.join();
        }
    }

    // 并行解压文件
    void parallelDecompressFiles(const std::string& outputFolder, 
                                const std::vector<std::pair<std::string, std::string>>& files, 
                                size_t num_threads, ProgressTracker& progress) {
        std::vector<std::thread> threads;
        std::atomic<size_t> next_file{0};
        std::mutex file_mutex;
        
        auto worker = [&]() {
            while (true) {
                size_t file_index = next_file.fetch_add(1);
                if (file_index >= files.size()) break;
                
                const auto& file = files[file_index];
                try {
                    std::string decompressed = lz77Decompress(file.second);
                    fs::path outputPath = fs::path(outputFolder) / file.first;
                    writeFile(outputPath, decompressed);
                    
                    progress.update();
                    
                    std::lock_guard<std::mutex> lock(file_mutex);
                    std::cout << "\nExtracted: " << file.first 
                              << " (" << file.second.size() << " -> " << decompressed.size() << " bytes)" << std::endl;
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(file_mutex);
                    std::cerr << "\nError extracting " << file.first << ": " << e.what() << std::endl;
                }
            }
        };
        
        // 启动工作线程
        size_t actual_threads = std::min(num_threads, files.size());
        for (size_t i = 0; i < actual_threads; ++i) {
            threads.emplace_back(worker);
        }
        
        // 进度显示线程
        std::thread progress_thread([&]() {
            while (progress.isRunning()) {
                progress.printProgress();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        
        // 等待所有工作线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 停止进度显示
        progress.finish();
        if (progress_thread.joinable()) {
            progress_thread.join();
        }
    }

public:
    // 压缩并加密文件夹（分卷版本）
    bool compressFolderVolume(const std::string& folderPath, const std::string& outputBaseName, 
                             const std::string& password, size_t max_chars_per_volume, 
                             size_t num_threads = std::thread::hardware_concurrency()) {
        try {
            // 收集所有文件任务
            std::vector<FileTask> tasks;
            for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
                if (entry.is_regular_file()) {
                    FileTask task;
                    task.filePath = entry.path();
                    task.relativePath = fs::relative(task.filePath, folderPath);
                    tasks.push_back(std::move(task));
                }
            }
            
            if (tasks.empty()) {
                std::cout << "No files found in folder: " << folderPath << std::endl;
                return false;
            }
            
            std::cout << "Found " << tasks.size() << " files. Compressing with " << num_threads << " threads..." << std::endl;
            
            // 初始化进度跟踪器
            ProgressTracker progress;
            progress.start(tasks.size());
            
            // 并行压缩所有文件
            parallelCompressFiles(tasks, num_threads, progress);
            
            std::cout << "Building archive..." << std::endl;
            
            // 构建归档数据
            std::stringstream archive;
            for (const auto& task : tasks) {
                if (!task.processed) {
                    std::cerr << "Skipping unprocessed file: " << task.relativePath.string() << std::endl;
                    continue;
                }
                
                // 写入文件信息：路径长度 + 路径 + 内容长度 + 内容
                std::string utf8Path = task.relativePath.u8string();
                uint32_t pathLen = utf8Path.length();
                uint32_t contentLen = task.compressed.length();
                
                archive.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
                archive.write(utf8Path.c_str(), pathLen);
                archive.write(reinterpret_cast<const char*>(&contentLen), sizeof(contentLen));
                archive.write(task.compressed.c_str(), contentLen);
            }
            
            std::string archiveData = archive.str();
            
            std::cout << "Encrypting and encoding..." << std::endl;
            
            // 加密数据
            std::string encrypted = xorEncryptDecrypt(archiveData, password);
            
            // Base64编码
            std::string base64Data = base64Encode(encrypted);
            
            // 添加UTF-8 BOM到每个卷
            std::string volumePrefix = "\xEF\xBB\xBF";
            
            // 分卷
            auto volumes = splitIntoVolumes(base64Data, max_chars_per_volume);
            
            std::cout << "Creating volume files..." << std::endl;
            
            // 写入分卷文件
            for (size_t i = 0; i < volumes.size(); ++i) {
                std::string volumeFilename = outputBaseName + "_part" + std::to_string(i + 1) + ".txt";
                std::ofstream outFile(volumeFilename, std::ios::binary);
                if (!outFile) {
                    std::cerr << "Cannot create volume file: " << volumeFilename << std::endl;
                    return false;
                }
                
                // 写入卷头信息（卷号/总卷数）
                outFile << volumePrefix;
                outFile << "VOLUME:" << (i + 1) << "/" << volumes.size() << "\n";
                outFile << volumes[i];
                
                std::cout << "Created volume: " << volumeFilename << " (" << volumes[i].size() << " characters)" << std::endl;
            }
            
            std::cout << "Compression completed: " << volumes.size() << " volume(s) created" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Compression error: " << e.what() << std::endl;
            return false;
        }
    }

    // 解密并解压文件夹（分卷版本）
    bool decompressFolderVolume(const std::string& inputBaseName, const std::string& outputFolder, 
                               const std::string& password, size_t num_threads = std::thread::hardware_concurrency()) {
        try {
            // 查找所有分卷文件
            std::vector<std::string> volumeFiles;
            int volumeCount = 0;
            
            // 首先尝试确定总卷数
            for (int i = 1; ; ++i) {
                std::string volumeFilename = inputBaseName + "_part" + std::to_string(i) + ".txt";
                if (!fs::exists(volumeFilename)) {
                    volumeCount = i - 1;
                    break;
                }
                volumeFiles.push_back(volumeFilename);
            }
            
            if (volumeFiles.empty()) {
                std::cerr << "No volume files found with base name: " << inputBaseName << std::endl;
                return false;
            }
            
            std::cout << "Found " << volumeCount << " volume files" << std::endl;
            std::cout << "Reading volume files..." << std::endl;
            
            // 读取所有分卷数据
            std::string base64Data;
            for (const auto& volumeFile : volumeFiles) {
                std::ifstream inFile(volumeFile, std::ios::binary);
                if (!inFile) {
                    std::cerr << "Cannot open volume file: " << volumeFile << std::endl;
                    return false;
                }
                
                std::string volumeContent((std::istreambuf_iterator<char>(inFile)),
                                        std::istreambuf_iterator<char>());
                
                // 跳过UTF-8 BOM和卷头信息
                size_t dataStart = volumeContent.find('\n');
                if (dataStart != std::string::npos) {
                    volumeContent = volumeContent.substr(dataStart + 1);
                } else {
                    // 如果没有找到换行符，尝试跳过BOM
                    if (volumeContent.size() >= 3 && 
                        static_cast<unsigned char>(volumeContent[0]) == 0xEF &&
                        static_cast<unsigned char>(volumeContent[1]) == 0xBB &&
                        static_cast<unsigned char>(volumeContent[2]) == 0xBF) {
                        volumeContent = volumeContent.substr(3);
                    }
                }
                
                base64Data += volumeContent;
                std::cout << "Read volume: " << volumeFile << " (" << volumeContent.size() << " characters)" << std::endl;
            }
            
            std::cout << "Decoding and decrypting..." << std::endl;
            
            // Base64解码
            std::string encrypted = base64Decode(base64Data);
            
            // 解密数据
            std::string archiveData = xorEncryptDecrypt(encrypted, password);
            
            // 解析归档数据并收集文件信息
            std::vector<std::pair<std::string, std::string>> files;
            std::stringstream archiveStream(archiveData);
            
            while (archiveStream) {
                // 读取路径长度
                uint32_t pathLen;
                // 修改前：reinterpret_cast<const char*>(&pathLen)
                // 修改后：reinterpret_cast<char*>(&pathLen)
                if (!archiveStream.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) {
                    break;
                }
                
                // 读取路径
                std::vector<char> pathBuffer(pathLen);
                if (!archiveStream.read(pathBuffer.data(), pathLen)) {
                    throw std::runtime_error("Failed to read file path");
                }
                std::string relativePath(pathBuffer.data(), pathLen);
                
                // 读取内容长度
                uint32_t contentLen;
                // 修改前：reinterpret_cast<const char*>(&contentLen)
                // 修改后：reinterpret_cast<char*>(&contentLen)
                if (!archiveStream.read(reinterpret_cast<char*>(&contentLen), sizeof(contentLen))) {
                    throw std::runtime_error("Failed to read content length");
                }
                
                // 读取内容
                std::vector<char> contentBuffer(contentLen);
                if (!archiveStream.read(contentBuffer.data(), contentLen)) {
                    throw std::runtime_error("Failed to read file content");
                }
                
                files.emplace_back(relativePath, std::string(contentBuffer.data(), contentLen));
            }
            
            std::cout << "Found " << files.size() << " files. Decompressing with " << num_threads << " threads..." << std::endl;
            
            // 初始化进度跟踪器
            ProgressTracker progress;
            progress.start(files.size());
            
            // 并行解压所有文件
            parallelDecompressFiles(outputFolder, files, num_threads, progress);
            
            std::cout << "Decompression completed: " << outputFolder << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Decompression error: " << e.what() << std::endl;
            return false;
        }
    }

    // 单文件版本（不分卷）
    bool compressFolder(const std::string& folderPath, const std::string& outputFile, 
                       const std::string& password, size_t num_threads = std::thread::hardware_concurrency()) {
        return compressFolderVolume(folderPath, outputFile, password, SIZE_MAX, num_threads);
    }

    bool decompressFolder(const std::string& inputFile, const std::string& outputFolder, 
                         const std::string& password, size_t num_threads = std::thread::hardware_concurrency()) {
        return decompressFolderVolume(inputFile, outputFolder, password, num_threads);
    }
};

int main() {
    SimpleCompressor compressor;
    
    std::cout << "Simple File Compressor with Encryption, Volume Support and Multi-core Acceleration" << std::endl;
    std::cout << "==================================================================================" << std::endl;
    
    // 获取系统信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::cout << "Available processors: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    while (true) {
        std::cout << "\nOptions:" << std::endl;
        std::cout << "1. Compress folder (single file)" << std::endl;
        std::cout << "2. Compress folder with volumes" << std::endl;
        std::cout << "3. Decompress file/folder" << std::endl;
        std::cout << "4. Exit" << std::endl;
        std::cout << "Choose option: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore(); // 清除换行符
        
        if (choice == 1) {
            std::string folderPath, outputFile, password;
            size_t num_threads;
            
            std::cout << "Enter folder path to compress: ";
            std::getline(std::cin, folderPath);
            
            std::cout << "Enter output file name: ";
            std::getline(std::cin, outputFile);
            
            std::cout << "Enter password: ";
            std::getline(std::cin, password);
            
            std::cout << "Enter number of threads (0 for auto): ";
            std::cin >> num_threads;
            std::cin.ignore();
            
            if (num_threads == 0) {
                num_threads = sysInfo.dwNumberOfProcessors;
            }
            
            if (!compressor.compressFolder(folderPath, outputFile, password, num_threads)) {
                std::cout << "Compression failed!" << std::endl;
            }
            
        } else if (choice == 2) {
            std::string folderPath, outputBaseName, password;
            size_t max_chars, num_threads;
            
            std::cout << "Enter folder path to compress: ";
            std::getline(std::cin, folderPath);
            
            std::cout << "Enter output base name: ";
            std::getline(std::cin, outputBaseName);
            
            std::cout << "Enter password: ";
            std::getline(std::cin, password);
            
            std::cout << "Enter maximum characters per volume: ";
            std::cin >> max_chars;
            std::cin.ignore();
            
            std::cout << "Enter number of threads (0 for auto): ";
            std::cin >> num_threads;
            std::cin.ignore();
            
            if (num_threads == 0) {
                num_threads = sysInfo.dwNumberOfProcessors;
            }
            
            if (!compressor.compressFolderVolume(folderPath, outputBaseName, password, max_chars, num_threads)) {
                std::cout << "Compression failed!" << std::endl;
            }
            
        } else if (choice == 3) {
            std::string inputFile, outputFolder, password;
            size_t num_threads;
            
            std::cout << "Enter input file/folder base name: ";
            std::getline(std::cin, inputFile);
            
            std::cout << "Enter output folder: ";
            std::getline(std::cin, outputFolder);
            
            std::cout << "Enter password: ";
            std::getline(std::cin, password);
            
            std::cout << "Enter number of threads (0 for auto): ";
            std::cin >> num_threads;
            std::cin.ignore();
            
            if (num_threads == 0) {
                num_threads = sysInfo.dwNumberOfProcessors;
            }
            
            if (!compressor.decompressFolderVolume(inputFile, outputFolder, password, num_threads)) {
                std::cout << "Decompression failed!" << std::endl;
            }
            
        } else if (choice == 4) {
            break;
        } else {
            std::cout << "Invalid option!" << std::endl;
        }
    }
    
    return 0;
}