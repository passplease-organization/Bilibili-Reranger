#include "socialAPI.h"

#include <map>
#include "../config.h"
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

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

thread_local socialAPI* handler;

socialAPI* socialAPI::instance(const std::string &platform,std::shared_ptr<const std::atomic<bool>>& stop) {
    if (handler != nullptr)
        return handler;
    if (socials.contains(platform))
        return nullptr;
    handler = socials[platform](stop);
    return handler;
}

socialAPI* socialAPI::instance() {
    return handler;
}

string socialAPI::allPlatform() {
    Json json = nlohmann::json::array();
    for (const auto & pair : socials) {
        json.push_back(pair.first);
    }
    return to_string(json);
}

string socialAPI::decrypt(const string &content) {
    return rsa.decrypt(content);
}

const string& socialAPI::encrypt(const string& content) {
    rsa = SimpleRSA();
    return rsa.publicKey();
}

SimpleRSA::SimpleRSA() : pKey(nullptr, EVP_PKEY_free) {
    EVP_PKEY* k = EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", 2048);
    if (!k) {
        throw std::runtime_error("Key generation failed");
    }
    pKey.reset(k);
}

void SimpleRSA::operator=(const SimpleRSA & other) {
    pKey.reset(other.pKey.get());
}

string SimpleRSA::publicKey() {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) return "";

    // 使用通用的 PEM 写函数
    // 这会自动以 X.509 (-----BEGIN PUBLIC KEY-----) 格式写入
    if (PEM_write_bio_PUBKEY(bio, pKey.get()) != 1) {
        BIO_free(bio);
        return "";
    }

    char* ptr = nullptr;
    long len = BIO_get_mem_data(bio, &ptr);
    std::string key(ptr, len);
    BIO_free(bio);
    return key;
}

string SimpleRSA::decrypt(const string &content) {
    BIO *bio, *b64;
    std::vector<unsigned char> buffer(content.size());
    bio = BIO_new_mem_buf(content.data(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int inLen = BIO_read(bio, buffer.data(), content.size());
    BIO_free_all(bio);

    if (inLen <= 0) return "";

    // --- B. OpenSSL 3.0 EVP 解密流程 ---

    // 1. 创建上下文
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
        EVP_PKEY_CTX_new(pKey.get(), nullptr),
        EVP_PKEY_CTX_free
    );
    if (!ctx) return "";

    // 2. 初始化解密操作
    if (EVP_PKEY_decrypt_init(ctx.get()) <= 0) return "";

    // 3. 设置 Padding (jsencrypt 默认使用 PKCS1)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) return "";

    // 4. 确定解密后的缓冲区大小 (第一次调用，out 设为 nullptr)
    size_t outLen = 0;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr, &outLen, buffer.data(), inLen) <= 0) {
        return ""; // 获取长度失败
    }

    // 5. 执行解密 (第二次调用)
    std::vector<unsigned char> outBuf(outLen);
    if (EVP_PKEY_decrypt(ctx.get(), outBuf.data(), &outLen, buffer.data(), inLen) <= 0) {
        // 这里可以添加 ERR_print_errors_fp(stderr) 查看具体错误
        return "";
    }

    return std::string((char*)outBuf.data(), outLen);
}
