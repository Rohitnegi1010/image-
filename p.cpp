#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <bitset>
#include <string>

// Node structure for the Huffman tree
struct Node {
    unsigned char pixel;
    int freq;
    Node *left, *right;

    Node(unsigned char p, int f) : pixel(p), freq(f), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue to order nodes by frequency
struct Compare {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

// Function to build the Huffman tree from pixel frequencies
Node* buildHuffmanTree(const std::unordered_map<unsigned char, int>& freq) {
    std::priority_queue<Node*, std::vector<Node*>, Compare> minHeap;

    for (const auto& pair : freq) {
        minHeap.push(new Node(pair.first, pair.second));
    }

    while (minHeap.size() > 1) {
        Node* left = minHeap.top(); minHeap.pop();
        Node* right = minHeap.top(); minHeap.pop();

        Node* sum = new Node(0, left->freq + right->freq);
        sum->left = left;
        sum->right = right;
        minHeap.push(sum);
    }

    return minHeap.top();
}

// Function to generate Huffman codes for each pixel value
void generateHuffmanCodes(Node* root, const std::string& str, std::unordered_map<unsigned char, std::string>& huffmanCode) {
    if (!root) return;

    if (root->pixel != 0) {
        huffmanCode[root->pixel] = str;
    }

    generateHuffmanCodes(root->left, str + "0", huffmanCode);
    generateHuffmanCodes(root->right, str + "1", huffmanCode);
}

// Function to read a PGM file
bool readPGM(const std::string& filename, std::vector<unsigned char>& image, int& width, int& height) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) return false;

    std::string format;
    infile >> format;
    if (format != "P5") return false;

    infile >> width >> height;
    int maxval;
    infile >> maxval;
    infile.ignore();

    image.resize(width * height);
    infile.read(reinterpret_cast<char*>(image.data()), image.size());
    return true;
}

// Function to write compressed data to a file
void writeCompressed(const std::string& filename, const std::string& compressedData, int width, int height) {
    std::ofstream outfile(filename, std::ios::binary);
    int bitsCount = compressedData.size();
    outfile.write(reinterpret_cast<const char*>(&bitsCount), sizeof(bitsCount));
    outfile.write(reinterpret_cast<const char*>(&width), sizeof(width));
    outfile.write(reinterpret_cast<const char*>(&height), sizeof(height));

    for (size_t i = 0; i < compressedData.size(); i += 8) {
        std::bitset<8> byte(compressedData.substr(i, 8));
        outfile.put(byte.to_ulong());
    }
}

// Function to read compressed data from a file
std::string readCompressed(const std::string& filename, int& width, int& height, int& bitsCount) {
    std::ifstream infile(filename, std::ios::binary);

    infile.read(reinterpret_cast<char*>(&bitsCount), sizeof(bitsCount));
    infile.read(reinterpret_cast<char*>(&width), sizeof(width));
    infile.read(reinterpret_cast<char*>(&height), sizeof(height));

    std::string compressedData(bitsCount, '0');
    for (int i = 0; i < bitsCount; i += 8) {
        char byte;
        infile.get(byte);
        std::bitset<8> bits(byte);
        compressedData.replace(i, 8, bits.to_string());
    }

    return compressedData;
}

// Function to decompress the image using Huffman tree
std::vector<unsigned char> decompressImage(const std::string& compressedData, Node* root, int width, int height) {
    std::vector<unsigned char> decompressedImage;
    Node* currentNode = root;

    for (char bit : compressedData) {
        if (bit == '0') {
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }

        if (currentNode->pixel != 0) {
            decompressedImage.push_back(currentNode->pixel);
            currentNode = root;
        }
    }

    return decompressedImage;
}

// Function to write a PGM file
void writePGM(const std::string& filename, const std::vector<unsigned char>& image, int width, int height) {
    std::ofstream outfile(filename, std::ios::binary);
    outfile << "P5\n" << width << " " << height << "\n255\n";
    outfile.write(reinterpret_cast<const char*>(image.data()), image.size());
}

int main() {
    std::string inputFilename = "/Users/rohitnegi/code/img0002.pgm";
    std::string outputFilename = "output.huff";
    std::string decompressedFilename = "decompressed.pgm";

    int width, height;
    std::vector<unsigned char> image;

    if (!readPGM(inputFilename, image, width, height)) {
        std::cerr << "Error reading PGM file" << std::endl;
        return 1;
    }

    std::unordered_map<unsigned char, int> freq;
    for (unsigned char pixel : image) {
        freq[pixel]++;
    }

    Node* root = buildHuffmanTree(freq);

    std::unordered_map<unsigned char, std::string> huffmanCode;
    generateHuffmanCodes(root, "", huffmanCode);

    std::string compressedData;
    for (unsigned char pixel : image) {
        compressedData += huffmanCode[pixel];
    }

    writeCompressed(outputFilename, compressedData, width, height);

    int bitsCount;
    std::string readCompressedData = readCompressed(outputFilename, width, height, bitsCount);
    std::vector<unsigned char> decompressedImage = decompressImage(readCompressedData, root, width, height);
    writePGM(decompressedFilename, decompressedImage, width, height);

    std::ifstream originalFile(inputFilename, std::ios::binary | std::ios::ate);
    std::ifstream compressedFile(outputFilename, std::ios::binary | std::ios::ate);
    std::streamsize originalSize = originalFile.tellg();
    std::streamsize compressedSize = compressedFile.tellg();

    std::cout << "Original size: " << originalSize << " bytes\n";
    std::cout << "Compressed size: " << compressedSize << " bytes\n";
    std::cout << "Compression ratio: " << (double)compressedSize / originalSize << std::endl;

    return 0;
}
