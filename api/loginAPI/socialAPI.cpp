#include "socialAPI.h"

#include <map>
#include "../config.h"
#include "../Util.h"

using namespace webAPI;

socialAPI::socialAPI(std::shared_ptr<const std::atomic<bool>>& stop) : stop(std::ref(stop)) {}

auto socials = std::map<const std::string,creator>();

bool socialAPI::supportPlatform(const std::string& platform,creator function) {
    if (socials.contains(platform)) {
        return false;
    }else {
        socials[platform] = function;
        return true;
    }
}

void socialAPI::init() {}

socialAPI* socialAPI::instance(socialAPI** handler,const std::string &platform,std::shared_ptr<const std::atomic<bool>>& stop) {
    if (handler == nullptr)
        return nullptr;
    if (*handler != nullptr)
        return *handler;
    if (socials.contains(platform))
        return nullptr;
    *handler = socials[platform](stop);
    (*handler) -> init();
    return *handler;
}

string socialAPI::allPlatform() {
    Json json = nlohmann::json::array();
    for (const auto & pair : socials) {
        json.push_back(pair.first);
    }
    return to_string(json);
}

auto rsa = SimpleRSA();

const SimpleRSA &webAPI::getRSA() {
    return rsa;
}

SimpleRSA::SimpleRSA() {
    if (sodium_init() < 0) {
        throwError("Libsodium initialization failed");
    }
    // 2. 生成密钥对 (应用启动时生成一次即可)
    crypto_box_keypair(publickey, secretkey);
}

std::string SimpleRSA::publicKey() const {
    // 分配足够的空间 (Base64长度约为原始长度的1.33倍 + 换行/结束符)
    const size_t b64_maxlen = sodium_base64_ENCODED_LEN(crypto_box_PUBLICKEYBYTES, sodium_base64_VARIANT_ORIGINAL);
    char* b64 = new char[b64_maxlen];

    sodium_bin2base64(b64, b64_maxlen, publickey, crypto_box_PUBLICKEYBYTES, sodium_base64_VARIANT_ORIGINAL);

    std::string result(b64);
    delete[] b64;
    return result;
}

// By Gemini
std::string SimpleRSA::decrypt(const std::string &content) const {
    // 1. Base64 解码
    size_t cipher_len = content.length();
    // 估算二进制长度
    std::vector<unsigned char> cipher_bin(cipher_len);
    size_t bin_len = 0;

    // 这里有个坑：你需要忽略换行符等，这里简化处理
    if (sodium_base642bin(cipher_bin.data(), cipher_len,
                          content.c_str(), cipher_len,
                          NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        throw std::runtime_error("Base64 decoding failed");
                          }

    // 2. 密封盒解密 (Sealed Box Open)
    // 密文长度必须包含了 overhead 长度，否则就是非法数据
    if (bin_len < crypto_box_SEALBYTES) return "";

    std::vector<unsigned char> decrypted(bin_len - crypto_box_SEALBYTES);

    if (crypto_box_seal_open(decrypted.data(), cipher_bin.data(), bin_len, publickey, secretkey) != 0) {
        // 解密失败（密钥不对或数据被篡改）
        return "";
    }

    // 转为字符串返回
    return string(decrypted.begin(), decrypted.end());
}

SimpleESA::SimpleESA(const string& key) {
    if (key.length() != crypto_secretbox_KEYBYTES) {
        throwError("Session Key length invalid! Must be 32 bytes.");
    }

    std::memcpy(this -> key, key.data(), crypto_secretbox_KEYBYTES);
}

std::string SimpleESA::decrypt(const std::string &content) const {
    // 1. Base64 解码
    size_t b64len = content.length();
    // 估算最大长度
    std::vector<unsigned char> cipherBin(b64len);
    size_t binLen = 0;

    if (sodium_base642bin(cipherBin.data(), b64len, content.c_str(), b64len,
                          NULL, &binLen, NULL, sodium_base64_VARIANT_ORIGINAL) != 0) {
        throw std::runtime_error("Invalid Base64 string");
                          }

    // 2. 长度校验
    // 必须要比 Nonce(24) + MAC(16) 长，否则肯定是个坏包
    if (binLen < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
        return "";
    }

    // 3. 提取 Nonce
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    std::memcpy(nonce, cipherBin.data(), crypto_secretbox_NONCEBYTES);

    // 4. 提取真正的密文部分
    const unsigned char* cipherText = cipherBin.data() + crypto_secretbox_NONCEBYTES;
    size_t cipherTextLen = binLen - crypto_secretbox_NONCEBYTES;

    // 5. 解密
    std::vector<unsigned char> plain(cipherTextLen - crypto_secretbox_MACBYTES);

    // 如果返回 -1 表示解密失败（可能是遭到篡改，或者 Key 不对）
    if (crypto_secretbox_open_easy(plain.data(), cipherText, cipherTextLen, nonce, key) != 0) {
        // 这里可以记录日志：有人尝试攻击接口
        return "";
    }

    return string(plain.begin(), plain.end());
}

std::string SimpleESA::encrypt(const std::string &content) const {
    // 1. 生成随机 Nonce (24字节)
    // Libsodium 强制要求每次加密必须用不同的 Nonce，否则不安全
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    // 2. 加密
    // 密文长度 = 明文长度 + MAC (16字节)
    size_t cipherLen = crypto_secretbox_MACBYTES + content.length();
    std::vector<unsigned char> cipher(cipherLen);

    crypto_secretbox_easy(cipher.data(),
                          (const unsigned char*)content.c_str(), content.length(),
                          nonce, key);

    // 3. 拼接 [Nonce] + [Cipher]
    // 前端需要拿到 Nonce 才能解密，所以要一起发过去
    std::vector<unsigned char> finalData;
    finalData.insert(finalData.end(), nonce, nonce + sizeof nonce);
    finalData.insert(finalData.end(), cipher.begin(), cipher.end());

    // 4. Base64 编码
    size_t b64Len = sodium_base64_ENCODED_LEN(finalData.size(), sodium_base64_VARIANT_ORIGINAL);
    std::vector<char> b64(b64Len);
    sodium_bin2base64(b64.data(), b64Len, finalData.data(), finalData.size(), sodium_base64_VARIANT_ORIGINAL);

    return std::string(b64.data());
}

Client::Client(const string& key) : esa(key) {
    char nonce[32];
    randombytes_buf(nonce, sizeof nonce);
    ID = nonce;
    _handler = nullptr;
}

const string &Client::getID() const {
    return ID;
}