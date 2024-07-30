#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// guid 파싱 함수, uint64_t가 8바이트인 관계로 2개로 나눠서 처리
void read_guid(std::ifstream& file, long long offset, uint64_t& part1, uint64_t& part2) {
    file.seekg(offset, std::ios::beg);
    
    part1 = 0;
    part2 = 0;

    // Read the first 8 bytes into part1
    for (int i = 0; i < 8; i++) {
        unsigned char byte;
        file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
        part1 = (part1 << 8) | byte;
    }

    // Read the next 8 bytes into part2
    for (int i = 0; i < 8; i++) {
        unsigned char byte;
        file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
        part2 = (part2 << 8) | byte;
    }
}

// 리틀엔디안 바이트 값 처리 함수
uint64_t read_file_bytes(ifstream& file, long long offset, int bytes) {
    file.seekg(offset, ios::beg);
    uint64_t result = 0;
    for (int i = 0; i < bytes; ++i) {
        unsigned char byte;
        file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
        result |= static_cast<uint64_t>(byte) << (i * 8);
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <image_path>" << endl;
        return -1;
    }

    string imagePath = argv[1];
    ifstream file(imagePath, ios::binary);
    if (!file.is_open()) {
        cerr << "Can't open file" << endl;
        return -1;
    }
    
    // 헤더 정보 확인
    uint64_t signature = read_file_bytes(file, 512, 8);

    if(signature != 0x5452415020494645){
        cerr << "I support only GPT file system." << endl;
        return -1;
    }

    uint64_t LBAstart = read_file_bytes(file, 584, 8);
    uint64_t LBAend = read_file_bytes(file, 592, 4);
    uint32_t entrySize = static_cast<uint32_t>(read_file_bytes(file, 596, 4));
    int i = 0;

    // 파티션 테이블 처리
   while(true){
        i++;
        long offset = LBAstart * 512 + i * entrySize;
        if(offset >= LBAend * 512)
            break;

        uint64_t part1, part2;
        read_guid(file, offset, part1, part2);
        if(part1 == 0 && part2 == 0)
            continue;
        
        // 파티션 정보 확인
        uint64_t partitionLBA_start = read_file_bytes(file, offset + 32, 8);
        uint64_t partitionLBA_end = read_file_bytes(file, offset + 40, 8);
        uint64_t partSize = (partitionLBA_end - partitionLBA_start + 1) * 512;
        uint64_t jumpcode = read_file_bytes(file, partitionLBA_start*512, 3);
        string sysType = "Unknown";

        // 점프코드로 파티션 종류 확인
        if(jumpcode == 0x9052EB)
            sysType = "NTFS";
        else if(jumpcode == 0x9058EB)
            sysType = "FAT32";
        
        // 출력
        cout << hex << uppercase << part1 << part2 << ' ' << sysType << ' ' << dec << partitionLBA_start << ' ' << partSize << endl;
    }

    return 0;
}
